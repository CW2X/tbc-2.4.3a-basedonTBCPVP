
#include "Common.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Log.h"
#include "Opcodes.h"
#include "ByteBuffer.h"
#include <openssl/md5.h>
#include <openssl/sha.h>
#include "World.h"
#include "Player.h"
#include "Util.h"
#include "WardenBase.h"
#include "WardenWin.h"
#include "Cryptography/ARC4.h"
#include "GameTime.h"

WardenBase::WardenBase() : 
    iCrypto(16), oCrypto(16), _wardenCheckTimer(10000/*10 sec*/), _wardenKickTimer(0), 
    _wardenDataSent(false), m_initialized(false), _wardenTimer(0)
{
}

WardenBase::~WardenBase()
{
    delete[] Module->CompressedData;
    delete Module;
    Module = nullptr;
    m_initialized = false;
}

void WardenBase::Init(WorldSession *pClient, BigNumber *K)
{
    ABORT();
}

ClientWardenModule *WardenBase::GetModuleForClient(WorldSession *session)
{
    ABORT();
    // return NULL;
}

void WardenBase::InitializeModule()
{
    ABORT();
}

void WardenBase::RequestHash()
{
    ABORT();
}

void WardenBase::HandleHashResult(ByteBuffer &buff)
{
    ABORT();
}

void WardenBase::RequestData()
{
    ABORT();
}

void WardenBase::HandleData(ByteBuffer &buff)
{
    ABORT();
}

void WardenBase::SendModuleToClient()
{
    // Create packet structure
    WardenModuleTransfer pkt;
    uint32 size_left = Module->CompressedSize;
    uint32 pos = 0;
    uint16 burst_size;
    while (size_left > 0)
    {
        burst_size = size_left < 500 ? size_left : 500;
        pkt.Command = WARDEN_SMSG_MODULE_CACHE;
        pkt.DataSize = burst_size;
        memcpy(pkt.Data, &Module->CompressedData[pos], burst_size);
        size_left -= burst_size;
        pos += burst_size;
        EncryptData((uint8*)&pkt, burst_size + 3);
        WorldPacket pkt1(SMSG_WARDEN_DATA, burst_size + 3);
        pkt1.append((uint8*)&pkt, burst_size + 3);
        Client->SendPacket(&pkt1);
    }
}

void WardenBase::RequestModule()
{
    // Create packet structure
    WardenModuleUse Request;
    Request.Command = WARDEN_SMSG_MODULE_USE;
    memcpy(Request.Module_Id, Module->ID, 16);
    memcpy(Request.Module_Key, Module->Key, 16);
    Request.Size = Module->CompressedSize;
    // Encrypt with warden RC4 key.
    EncryptData((uint8*)&Request, sizeof(WardenModuleUse));
    WorldPacket pkt(SMSG_WARDEN_DATA, sizeof(WardenModuleUse));
    pkt.append((uint8*)&Request, sizeof(WardenModuleUse));
    Client->SendPacket(&pkt);
}

void WardenBase::Update()
{
    if (m_initialized)
    {
        uint32 ticks = WorldGameTime::GetGameTimeMS();
        uint32 diff = ticks - _wardenTimer;
        _wardenTimer = ticks;
        if (_wardenDataSent)
        {
            // 1.5 minutes after send packet
            uint32 maxClientResponseDelay = sWorld->getConfig(CONFIG_WARDEN_CLIENT_RESPONSE_DELAY);
            if ((_wardenKickTimer > maxClientResponseDelay * IN_MILLISECONDS)) {
                if (sWorld->getConfig(CONFIG_WARDEN_KICK))
                    Client->KickPlayer();
                
                //if (sWorld->getConfig(CONFIG_WARDEN_DB_LOG))
                    //LogsDatabase.PQuery("INSERT INTO warden_fails (guid, account, check_id, comment, time) VALUES (%u, %u, 0, 'Response timeout', %u)", Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetAccountId(), time(NULL));
            }
            else
                _wardenKickTimer += diff;
        }
        else if (_wardenCheckTimer > 0)
        {
            if (diff >= _wardenCheckTimer)
            {
                RequestData();
                _wardenCheckTimer = sWorld->getConfig(CONFIG_WARDEN_CLIENT_CHECK_HOLDOFF) * IN_MILLISECONDS;
            }
            else
                _wardenCheckTimer -= diff;
        }
    }
}

void WardenBase::DecryptData(uint8 *Buffer, uint32 Len)
{
    iCrypto.UpdateData(Len, Buffer);
}

void WardenBase::EncryptData(uint8 *Buffer, uint32 Len)
{
    oCrypto.UpdateData(Len, Buffer);
}

bool WardenBase::IsValidCheckSum(uint32 checksum, const uint8 *Data, const uint16 Length)
{
    uint32 newchecksum = BuildChecksum(Data, Length);
    if (checksum != newchecksum) {
        TC_LOG_TRACE("warden","CHECKSUM IS NOT VALID");
        return false;
    }

    return true;
}

uint32 WardenBase::BuildChecksum(const uint8* data, uint32 dataLen)
{
    uint8 hash[20];
    SHA1(data, dataLen, hash);
    uint32 checkSum = 0;
    for (uint8 i = 0; i < 5; ++i)
        checkSum = checkSum ^ *(uint32*)(&hash[0] + i * 4);
    return checkSum;
}

void WorldSession::HandleWardenDataOpcode(WorldPacket & recvData)
{
    _warden->DecryptData(const_cast<uint8*>(recvData.contents()), recvData.size());
    uint8 Opcode;
    recvData >> Opcode;

    switch(Opcode)
    {
        case WARDEN_CMSG_MODULE_MISSING:
            _warden->SendModuleToClient();
            break;
        case WARDEN_CMSG_MODULE_OK:
            _warden->RequestHash();
            break;
        case WARDEN_CMSG_CHEAT_CHECKS_RESULT:
            _warden->HandleData(recvData);
            break;
        case WARDEN_CMSG_MEM_CHECKS_RESULT:
            break;
        case WARDEN_CMSG_HASH_RESULT:
            _warden->HandleHashResult(recvData);
            _warden->InitializeModule();
            break;
        case WARDEN_CMSG_MODULE_FAILED:
            break;
        default:
            TC_LOG_ERROR("network","Got unknown warden opcode %02X of size %u.", Opcode, uint32(recvData.size() - 1));
            break;
    }
}
