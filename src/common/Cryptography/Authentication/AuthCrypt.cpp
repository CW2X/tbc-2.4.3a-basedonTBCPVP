/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "AuthCrypt.h"
#include "Cryptography/HMACSHA1.h"
#include "Cryptography/BigNumber.h"
#include <cstring>
#include "Log.h"
#include "Errors.h"
#include "Common.h"

AuthCrypt::AuthCrypt(ClientBuild build) :
    clientBuild(build),
    _initialized(false)
{ }

void AuthCrypt::Init(BigNumber* K)
{
    switch(clientBuild)
    {
    case BUILD_243:
    {
        _send_i = _send_j = _recv_i = _recv_j = 0;
        auto  key = new uint8[SHA_DIGEST_LENGTH];

        uint8 temp[SEED_KEY_SIZE] = { 0x38, 0xA7, 0x83, 0x15, 0xF8, 0x92, 0x25, 0x30, 0x71, 0x98, 0x67, 0xB1, 0x8C, 0x4, 0xE2, 0xAA };
        HmacHash hash(16,temp);
        hash.UpdateData(K->AsByteArray().get(), K->GetNumBytes());
        hash.Finalize();
        memcpy(key, hash.GetDigest(), SHA_DIGEST_LENGTH);

        _key.resize(SHA_DIGEST_LENGTH);
        std::copy(key, key + SHA_DIGEST_LENGTH, _key.begin());
        delete[] key;
        break;
    }
    case BUILD_335:
    {
        _serverEncrypt = new ARC4(SHA_DIGEST_LENGTH);
        _clientDecrypt = new ARC4(SHA_DIGEST_LENGTH);

        uint8 ServerEncryptionKey[SEED_KEY_SIZE] = { 0xCC, 0x98, 0xAE, 0x04, 0xE8, 0x97, 0xEA, 0xCA, 0x12, 0xDD, 0xC0, 0x93, 0x42, 0x91, 0x53, 0x57 };
        HmacHash serverEncryptHmac(SEED_KEY_SIZE, (uint8*)ServerEncryptionKey);
        uint8 *encryptHash = serverEncryptHmac.ComputeHash(K);

        uint8 ServerDecryptionKey[SEED_KEY_SIZE] = { 0xC2, 0xB3, 0x72, 0x3C, 0xC6, 0xAE, 0xD9, 0xB5, 0x34, 0x3C, 0x53, 0xEE, 0x2F, 0x43, 0x67, 0xCE };
        HmacHash clientDecryptHmac(SEED_KEY_SIZE, (uint8*)ServerDecryptionKey);
        uint8 *decryptHash = clientDecryptHmac.ComputeHash(K);

        //ARC4 _serverDecrypt(encryptHash);
        _clientDecrypt->Init(decryptHash);
        _serverEncrypt->Init(encryptHash);
        //ARC4 _clientEncrypt(decryptHash);

        // Drop first 1024 bytes, as WoW uses ARC4-drop1024.
        uint8 syncBuf[1024];
        memset(syncBuf, 0, 1024);

        _serverEncrypt->UpdateData(1024, syncBuf);
        //_clientEncrypt.UpdateData(1024, syncBuf);

        memset(syncBuf, 0, 1024);

        //_serverDecrypt.UpdateData(1024, syncBuf);
        _clientDecrypt->UpdateData(1024, syncBuf);

        _initialized = true;
        break;
    }
    default:
        TC_LOG_ERROR("network", "AuthCrypt::Init, wrong build %u given, cannot initialize", uint32(clientBuild));
        return;
    }
    _initialized = true;
}

void AuthCrypt::DecryptRecv(uint8 *data, size_t len)
{
    if (!_initialized) 
        return;

    switch(clientBuild)
    {
    case BUILD_243:
        if (len < CRYPTED_SEND_LEN) 
            return;

        for (size_t t = 0; t < CRYPTED_RECV_LEN; t++)
        {
            _recv_i %= _key.size();
            uint8 x = (data[t] - _recv_j) ^ _key[_recv_i];
            ++_recv_i;
            _recv_j = data[t];
            data[t] = x;
        }
        break;
    case BUILD_335:
        _clientDecrypt->UpdateData(len, data);
        break;
    default:
        //never supposed to happen, we check this at init and _initialized should stay false
        ASSERT(false); 
    }

}

void AuthCrypt::EncryptSend(uint8 *data, size_t len)
{
    if (!_initialized) 
        return;

    switch(clientBuild)
    {
    case BUILD_243:

        if (len < CRYPTED_SEND_LEN) 
            return;

        for (size_t t = 0; t < CRYPTED_SEND_LEN; t++)
        {
            _send_i %= _key.size();
            uint8 x = (data[t] ^ _key[_send_i]) + _send_j;
            ++_send_i;
            data[t] = _send_j = x;
        }
        break;
    case BUILD_335:
        _serverEncrypt->UpdateData(len, data);
        break;
    default:
        //never supposed to happen, we check this at init and _initialized should stay false
        ASSERT(false); 
    }
}
