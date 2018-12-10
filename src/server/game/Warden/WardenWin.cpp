

#include "Cryptography/HMACSHA1.h"
#include "WardenKeyGeneration.h"
#include "Common.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "Log.h"
#include "Opcodes.h"
#include "ByteBuffer.h"
#include <openssl/md5.h>
#include "DatabaseEnv.h"
#include "World.h"
#include "Player.h"
#include "Util.h"
#include "WardenWin.h"
#include "WardenModuleWin.h"
#include "WardenDataStorage.h"
#include "Chat.h"
#include "GameTime.h"
#include "PlayerAntiCheat.h"

CWardenDataStorage WardenDataStorage;

WardenWin::WardenWin()
{
}

WardenWin::~WardenWin()
{
}

void WardenWin::Init(WorldSession *pClient, BigNumber *K)
{
    Client = pClient;

    // Generate Warden Key
    SHA1Randx WK(K->AsByteArray().get(), K->GetNumBytes());
    WK.Generate(InputKey, 16);
    WK.Generate(OutputKey, 16);

    /*
    Seed: 4D808D2C77D905C41A6380EC08586AFE (0x05 packet)
    Hash: 568C054C781A972A6037A2290C22B52571A06F4E (0x04 packet)
    Module MD5: 79C0768D657977D697E10BAD956CCED1
    New Client Key: 7F 96 EE FD A5 B6 3D 20 A4 DF 8E 00 CB F4 83 04
    New Server Key: C2 B7 AD ED FC CC A9 C2 BF B3 F8 56 02 BA 80 9B
    */

    uint8 mod_seed[16] = { 0x4D, 0x80, 0x8D, 0x2C, 0x77, 0xD9, 0x05, 0xC4, 0x1A, 0x63, 0x80, 0xEC, 0x08, 0x58, 0x6A, 0xFE };
    memcpy(Seed, mod_seed, 16);

    iCrypto.Init(InputKey);
    oCrypto.Init(OutputKey);

    Module = GetModuleForClient(Client);

    RequestModule();
}

ClientWardenModule *WardenWin::GetModuleForClient(WorldSession *session)
{
    ClientWardenModule *mod = new ClientWardenModule;
    uint32 len = sizeof(Module_79C0768D657977D697E10BAD956CCED1_Data);

    // data assign
    mod->CompressedSize = len;
    mod->CompressedData = new uint8[len];
    memcpy(mod->CompressedData, Module_79C0768D657977D697E10BAD956CCED1_Data, len);
    memcpy(mod->Key, Module_79C0768D657977D697E10BAD956CCED1_Key, 16);

    // md5 hash
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, mod->CompressedData, len);
    MD5_Final((uint8*)&mod->ID, &ctx);

    return mod;
}

void WardenWin::InitializeModule()
{
    // Create packet structure
    WardenInitModuleRequest Request;
    Request.Command1 = WARDEN_SMSG_MODULE_INITIALIZE;
    Request.Size1 = 20;
    Request.CheckSumm1 = BuildChecksum(&Request.Unk1, 20);
    Request.Unk1 = 1;
    Request.Unk2 = 0;
    Request.Type = 1;
    Request.String_library1 = 0;
    Request.Function1[0] = 0x00024F80;                      // 0x00400000 + 0x00024F80 SFileOpenFile
    Request.Function1[1] = 0x000218C0;                      // 0x00400000 + 0x000218C0 SFileGetFileSize
    Request.Function1[2] = 0x00022530;                      // 0x00400000 + 0x00022530 SFileReadFile
    Request.Function1[3] = 0x00022910;                      // 0x00400000 + 0x00022910 SFileCloseFile
    Request.Command2 = WARDEN_SMSG_MODULE_INITIALIZE;
    Request.Size2 = 8;
    Request.CheckSumm2 = BuildChecksum(&Request.Unk2, 8);
    Request.Unk3 = 4;
    Request.Unk4 = 0;
    Request.String_library2 = 0;
    Request.Function2 = 0x00419D40;                         // 0x00400000 + 0x00419D40 FrameScript::GetText
    Request.Function2_set = 1;
    Request.Command3 = WARDEN_SMSG_MODULE_INITIALIZE;
    Request.Size3 = 8;
    Request.CheckSumm3 = BuildChecksum(&Request.Unk5, 8);
    Request.Unk5 = 1;
    Request.Unk6 = 1;
    Request.String_library3 = 0;
    Request.Function3 = 0x0046AE20;                         // 0x00400000 + 0x0046AE20 PerformanceCounter
    Request.Function3_set = 1;

    // Encrypt with warden RC4 key.
    EncryptData((uint8*)&Request, sizeof(WardenInitModuleRequest));
    WorldPacket pkt(SMSG_WARDEN_DATA, sizeof(WardenInitModuleRequest));
    pkt.append((uint8*)&Request, sizeof(WardenInitModuleRequest));

    Client->SendPacket(&pkt);
}

void WardenWin::RequestHash()
{
    // Create packet structure
    WardenHashRequest Request;
    Request.Command = WARDEN_SMSG_HASH_REQUEST;
    memcpy(Request.Seed, Seed, 16);

    // Encrypt with warden RC4 key.
    EncryptData((uint8*)&Request, sizeof(WardenHashRequest));
    WorldPacket pkt(SMSG_WARDEN_DATA, sizeof(WardenHashRequest));
    pkt.append((uint8*)&Request, sizeof(WardenHashRequest));
    Client->SendPacket(&pkt);
}

void WardenWin::HandleHashResult(ByteBuffer &buff)
{
    buff.rpos(buff.wpos());

    const uint8 validHash[20] = { 0x56, 0x8C, 0x05, 0x4C, 0x78, 0x1A, 0x97, 0x2A, 0x60, 0x37, 0xA2, 0x29, 0x0C, 0x22, 0xB5, 0x25, 0x71, 0xA0, 0x6F, 0x4E };

    // verify key not equal kick player
    if (memcmp(buff.contents() + 1, validHash, sizeof(validHash)) != 0)
    {
        TC_LOG_TRACE("warden","Request hash reply: failed");
        if (sWorld->getConfig(CONFIG_WARDEN_DB_LOG))
            LogsDatabase.PQuery("INSERT INTO warden_fails (guid, account, check_id, comment, time) VALUES (%u, %u, 0, 'Hash reply failed', %u)", Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetAccountId(), time(NULL));

        if (sWorld->getConfig(CONFIG_WARDEN_KICK))
            Client->KickPlayer();

        return;
    }

    // client 7F96EEFDA5B63D20A4DF8E00CBF48304
    const uint8 client_key[16] = { 0x7F, 0x96, 0xEE, 0xFD, 0xA5, 0xB6, 0x3D, 0x20, 0xA4, 0xDF, 0x8E, 0x00, 0xCB, 0xF4, 0x83, 0x04 };

    // server C2B7ADEDFCCCA9C2BFB3F85602BA809B
    const uint8 server_key[16] = { 0xC2, 0xB7, 0xAD, 0xED, 0xFC, 0xCC, 0xA9, 0xC2, 0xBF, 0xB3, 0xF8, 0x56, 0x02, 0xBA, 0x80, 0x9B };

    // change keys here
    memcpy(InputKey, client_key, 16);
    memcpy(OutputKey, server_key, 16);
    iCrypto.Init(InputKey);
    oCrypto.Init(OutputKey);
    m_initialized = true;
    _wardenTimer = WorldGameTime::GetGameTimeMS();
}

void WardenWin::RequestData()
{
    if (MemCheck.empty())
        MemCheck.assign(WardenDataStorage.MemCheckIds.begin(), WardenDataStorage.MemCheckIds.end());

    ServerTicks = WorldGameTime::GetGameTimeMS();
    //uint32 maxid = WardenDataStorage.InternalDataID;
    uint32 id;
    uint8 type;
    WardenData *wd;
    SendDataId.clear();

    for (int i = 0; i < sWorld->getConfig(CONFIG_WARDEN_NUM_CHECKS); ++i)                             // for now include 3 MEM_CHECK's
    {   
        if (MemCheck.empty())
            break;

        id = MemCheck.back();
        SendDataId.push_back(id);
        MemCheck.pop_back();
    }

    ByteBuffer buff;
    buff << uint8(WARDEN_SMSG_CHEAT_CHECKS_REQUEST);

    // This doesn't apply for 2.4.3
    //for (int i = 0; i < 5; ++i)                             // for now include 5 random checks
    //{
    //    id = irand(1, maxid - 1);
    //    wd = WardenDataStorage.GetWardenDataById(id);
    //    SendDataId.push_back(id);
    //    switch (wd->Type)
    //    {
    //        case MPQ_CHECK:
    //        case LUA_STR_CHECK:
    //        case DRIVER_CHECK:
    //            buff << uint8(wd->str.size());
    //            buff.append(wd->str.c_str(), wd->str.size());
    //            break;
    //        default:
    //            break;
    //    }
    //}

    uint8 xorByte = InputKey[0];
    buff << uint8(0x00);
    buff << uint8(TIMING_CHECK ^ xorByte);                  // check TIMING_CHECK
    uint8 index = 1;

    for (std::vector<uint32>::iterator itr = SendDataId.begin(); itr != SendDataId.end(); ++itr)
    {
        wd = WardenDataStorage.GetWardenDataById(*itr);
        type = wd->Type;
        buff << uint8(type ^ xorByte);

        switch (type)
        {
            case MEM_CHECK:
            {
                buff << uint8(0x00);
                buff << uint32(wd->Address);
                buff << uint8(wd->Length);
                break;
            }
            case PAGE_CHECK_A:
            case PAGE_CHECK_B:
            {
                buff.append(wd->i.AsByteArray(0, false).get(), wd->i.GetNumBytes());
                buff << uint32(wd->Address);
                buff << uint8(wd->Length);
                break;
            }
            case MPQ_CHECK:
            case LUA_STR_CHECK:
            {
                buff << uint8(index++);
                break;
            }
            case DRIVER_CHECK:
            {
                buff.append(wd->i.AsByteArray(0, false).get(), wd->i.GetNumBytes());
                buff << uint8(index++);
                break;
            }
            case MODULE_CHECK:
            {
                uint32 seed = static_cast<uint32>(rand32());
                buff << uint32(seed);
                HmacHash hmac(4, (uint8*)&seed);
                hmac.UpdateData(wd->str);
                hmac.Finalize();
                buff.append(hmac.GetDigest(), hmac.GetLength());
                break;
            }
            /*case PROC_CHECK:
            {
                buff.append(wd->i.AsByteArray(0, false), wd->i.GetNumBytes());
                buff << uint8(index++);
                buff << uint8(index++);
                buff << uint32(wd->Address);
                buff << uint8(wd->Length);
                break;
            }*/
            default:
                break;                                      // should never happens
        }
    }

    buff << uint8(xorByte);
    buff.hexlike();

    // Encrypt with warden RC4 key.
    EncryptData(const_cast<uint8*>(buff.contents()), buff.size());
    WorldPacket pkt(SMSG_WARDEN_DATA, buff.size());
    pkt.append(buff);
    Client->SendPacket(&pkt);
    _wardenDataSent = true;
}

void WardenWin::HandleData(ByteBuffer &buff)
{
    _wardenDataSent = false;
    _wardenKickTimer = 0;
    uint16 Length;
    buff >> Length;
    uint32 Checksum;
    buff >> Checksum;

    if (!IsValidCheckSum(Checksum, buff.contents() + buff.rpos(), Length))
    {
        buff.rpos(buff.wpos());
        
        if (sWorld->getConfig(CONFIG_WARDEN_DB_LOG))
            LogsDatabase.PQuery("INSERT INTO warden_fails (guid, account, check_id, comment, time) VALUES (%u, %u, 0, 'Invalid checksum', %u)", Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetAccountId(), time(NULL));
        
        if (sWorld->getConfig(CONFIG_WARDEN_KICK))
            Client->KickPlayer();

        return;
    }

    bool found = false;
    bool ban = false;
    std::ostringstream ban_reason;
    ban_reason << "Cheat ";
    bool kick = false;

    //TIMING_CHECK
    {
        uint8 result;
        buff >> result;
        // TODO: test it.
        if (result == 0x00)
        {
            //TC_LOG_DEBUG("warden","TIMING CHECK FAIL result 0x00");
            TC_LOG_DEBUG("warden","Warden: TIMING CHECK FAILED (result 0x00) for account %u, player %u (%s).", Client->GetAccountId(), Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetPlayer() ? Client->GetPlayer()->GetName().c_str() : "<Not connected>");
            found = true;
            if (sWorld->getConfig(CONFIG_WARDEN_DB_LOG))
                LogsDatabase.PQuery("INSERT INTO warden_fails (guid, account, check_id, comment, time) VALUES (%u, %u, 0, 'Timing check', %u)", Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetAccountId(), time(NULL));
        }

        uint32 newClientTicks;
        buff >> newClientTicks;

        /*
         uint32 ticksNow = WorldGameTime::GetGameTimeMS();
        uint32 ourTicks = newClientTicks + (ticksNow - _serverTicks);

        TC_LOG_DEBUG("warden", "ServerTicks %u", ticksNow);         // Now
        TC_LOG_DEBUG("warden", "RequestTicks %u", _serverTicks);    // At request
        TC_LOG_DEBUG("warden", "Ticks %u", newClientTicks);         // At response
        TC_LOG_DEBUG("warden", "Ticks diff %u", ourTicks - newClientTicks);
        */
    }

    WardenDataResult *rs;
    WardenData *rd;
    uint8 type;

    for (std::vector<uint32>::iterator itr = SendDataId.begin(); itr != SendDataId.end(); ++itr)
    {
        rd = WardenDataStorage.GetWardenDataById(*itr);
        rs = WardenDataStorage.GetWardenResultById(*itr);
        type = rd->Type;

        switch (type)
        {
            case MEM_CHECK:
            {
                uint8 Mem_Result;
                buff >> Mem_Result;

                if (Mem_Result != 0)
                {
                    //TC_LOG_DEBUG("warden","RESULT MEM_CHECK not 0x00, CheckId %u account Id %u", rd->id, Client->GetAccountId());
                    TC_LOG_DEBUG("warden","Warden: MEM CHECK not 0x00 at check %u (%s) for account %u, player %u (%s).", rd->id, rd->comment.c_str(), Client->GetAccountId(), Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetPlayer() ? Client->GetPlayer()->GetName().c_str() : "<Not connected>");
                    found = true;

                    if (rd->action & WA_ACT_LOG)
                        LogsDatabase.PQuery("INSERT INTO warden_fails (guid, account, check_id, comment, time) VALUES (%u, %u, %u, '%s', %u)", Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetAccountId(), rd->id, rd->comment.c_str(), time(NULL));
                    if (rd->action & WA_ACT_KICK)
                        kick = true;
                    if (rd->action & WA_ACT_BAN) {
                        ban = true;
                        ban_reason << "[" << rd->id << "] ";
                    }

                    continue;
                }

                if (memcmp(buff.contents() + buff.rpos(), rs->res.AsByteArray(0, false).get(), rd->Length) != 0)
                {
                    //TC_LOG_DEBUG("warden","RESULT MEM_CHECK fail CheckId %u account Id %u", rd->id, Client->GetAccountId());
                    TC_LOG_DEBUG("warden","Warden: MEM CHECK FAILED at check %u (%s) for account %u, player %u (%s).", rd->id, rd->comment.c_str(), Client->GetAccountId(), Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetPlayer() ? Client->GetPlayer()->GetName().c_str() : "<Not connected>");
                    
                    if (rd->action & WA_ACT_LOG)
                        LogsDatabase.PQuery("INSERT INTO warden_fails (guid, account, check_id, comment, time) VALUES (%u, %u, %u, '%s', %u)", Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetAccountId(), rd->id, rd->comment.c_str(), time(NULL));
                    if (rd->action & WA_ACT_KICK)
                        kick = true;
                    if (rd->action & WA_ACT_BAN) {
                        ban = true;
                        ban_reason << "[" << rd->id << "] ";
                    }
                    
                    found = true;
                    buff.rpos(buff.rpos() + rd->Length);
                    continue;
                }

                buff.rpos(buff.rpos() + rd->Length);
                TC_LOG_DEBUG("warden","Warden: MEM CHECK PASSED at check %u (%s) for account %u, player %u (%s).", rd->id, rd->comment.c_str(), Client->GetAccountId(), Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetPlayer() ? Client->GetPlayer()->GetName().c_str() : "<Not connected>");
                break;
            }
            case PAGE_CHECK_A:
            case PAGE_CHECK_B:
            case DRIVER_CHECK:
            case MODULE_CHECK:
            {
                const uint8 byte = 0xE9;

                if (memcmp(buff.contents() + buff.rpos(), &byte, sizeof(uint8)) != 0)
                {
                    if (type == PAGE_CHECK_A || type == PAGE_CHECK_B) {
                        //TC_LOG_DEBUG("warden","RESULT PAGE_CHECK fail, CheckId %u account Id %u", rd->id, Client->GetAccountId());
                        TC_LOG_DEBUG("warden","Warden: PAGE CHECK FAILED at check %u (%s) for account %u, player %u (%s).", rd->id, rd->comment.c_str(), Client->GetAccountId(), Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetPlayer() ? Client->GetPlayer()->GetName().c_str() : "<Not connected>");
                        if (rd->action & WA_ACT_LOG)
                            LogsDatabase.PQuery("INSERT INTO warden_fails (guid, account, check_id, comment, time) VALUES (%u, %u, %u, '%s', %u)", Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetAccountId(), rd->id, rd->comment.c_str(), time(NULL));
                        if (rd->action & WA_ACT_KICK)
                            kick = true;
                        if (rd->action & WA_ACT_BAN) {
                            ban = true;
                            ban_reason << "[" << rd->id << "] ";
                        }
                    }

                    if (type == MODULE_CHECK) {
                        //TC_LOG_DEBUG("warden","RESULT MODULE_CHECK fail, CheckId %u account Id %u", rd->id, Client->GetAccountId());
                        TC_LOG_DEBUG("warden","Warden: MODULE CHECK FAILED at check %u (%s) for account %u, player %u (%s).", rd->id, rd->comment.c_str(), Client->GetAccountId(), Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetPlayer() ? Client->GetPlayer()->GetName().c_str() : "<Not connected>");
                        if (rd->action & WA_ACT_LOG)
                            LogsDatabase.PQuery("INSERT INTO warden_fails (guid, account, check_id, comment, time) VALUES (%u, %u, %u, '%s', %u)", Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetAccountId(), rd->id, rd->comment.c_str(), time(NULL));
                        if (rd->action & WA_ACT_KICK)
                            kick = true;
                        if (rd->action & WA_ACT_BAN) {
                            ban = true;
                            ban_reason << "[" << rd->id << "] ";
                        }
                    }

                    if (type == DRIVER_CHECK) {
                        //TC_LOG_DEBUG("warden","RESULT DRIVER_CHECK fail, CheckId %u account Id %u", rd->id, Client->GetAccountId());
                        TC_LOG_DEBUG("warden","Warden: DRIVER_CHECK CHECK FAILED at check %u (%s) for account %u, player %u (%s).", rd->id, rd->comment.c_str(), Client->GetAccountId(), Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetPlayer() ? Client->GetPlayer()->GetName().c_str() : "<Not connected>");
                        if (rd->action & WA_ACT_LOG)
                            LogsDatabase.PQuery("INSERT INTO warden_fails (guid, account, check_id, comment, time) VALUES (%u, %u, %u, '%s', %u)", Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetAccountId(), rd->id, rd->comment.c_str(), time(NULL));
                        if (rd->action & WA_ACT_KICK)
                            kick = true;
                        if (rd->action & WA_ACT_BAN) {
                            ban = true;
                            ban_reason << "[" << rd->id << "] ";
                        }
                    }

                    found = true;
                    buff.rpos(buff.rpos() + 1);
                    continue;
                }

                buff.rpos(buff.rpos() + 1);
                if (type == PAGE_CHECK_A || type == PAGE_CHECK_B)
                    TC_LOG_DEBUG("warden","Warden: PAGE CHECK PASSED at check %u (%s) for account %u, player %u (%s).", rd->id, rd->comment.c_str(), Client->GetAccountId(), Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetPlayer() ? Client->GetPlayer()->GetName().c_str() : "<Not connected>");
                else if (type == MODULE_CHECK)
                    TC_LOG_DEBUG("warden","Warden: MODULE CHECK PASSED at check %u (%s) for account %u, player %u (%s).", rd->id, rd->comment.c_str(), Client->GetAccountId(), Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetPlayer() ? Client->GetPlayer()->GetName().c_str() : "<Not connected>");
                else if (type == DRIVER_CHECK)
                    TC_LOG_DEBUG("warden","Warden: DRIVER CHECK PASSED at check %u (%s) for account %u, player %u (%s).", rd->id, rd->comment.c_str(), Client->GetAccountId(), Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetPlayer() ? Client->GetPlayer()->GetName().c_str() : "<Not connected>");

                break;
            }
            case LUA_STR_CHECK:
            {
                uint8 Lua_Result;
                buff >> Lua_Result;

                if (Lua_Result != 0)
                {
                    //TC_LOG_DEBUG("warden","RESULT LUA_STR_CHECK fail, CheckId %u account Id %u", rd->id, Client->GetAccountId());
                    TC_LOG_DEBUG("warden","Warden: LUA STR CHECK FAILED at check %u (%s) for account %u, player %u (%s).", rd->id, rd->comment.c_str(), Client->GetAccountId(), Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetPlayer() ? Client->GetPlayer()->GetName().c_str() : "<Not connected>");
                    found = true;
                    if (rd->action & WA_ACT_LOG)
                        LogsDatabase.PQuery("INSERT INTO warden_fails (guid, account, check_id, comment, time) VALUES (%u, %u, %u, '%s', %u)", Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetAccountId(), rd->id, rd->comment.c_str(), time(NULL));
                    if (rd->action & WA_ACT_KICK)
                        kick = true;
                    if (rd->action & WA_ACT_BAN) {
                        ban = true;
                        ban_reason << "[" << rd->id << "] ";
                    }
                    continue;
                }

                uint8 luaStrLen;
                buff >> luaStrLen;

                if (luaStrLen != 0)
                {
                    char *str = new char[luaStrLen + 1];
                    memset(str, 0, luaStrLen + 1);
                    memcpy(str, buff.contents() + buff.rpos(), luaStrLen);
                    delete[] str;
                }

                buff.rpos(buff.rpos() + luaStrLen);         // skip string
                break;
            }
            case MPQ_CHECK:
            {
                uint8 Mpq_Result;
                buff >> Mpq_Result;

                if (Mpq_Result != 0)
                {
                    //TC_LOG_DEBUG("warden","RESULT MPQ_CHECK not 0x00 account id %u", Client->GetAccountId());
                    TC_LOG_DEBUG("warden","Warden: MPQ CHECK NOT 0x00 for account %u, player %u (%s).", Client->GetAccountId(), Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetPlayer() ? Client->GetPlayer()->GetName().c_str() : "<Not connected>");
                    found = true;
                    if (rd->action & WA_ACT_LOG)
                        LogsDatabase.PQuery("INSERT INTO warden_fails (guid, account, check_id, comment, time) VALUES (%u, %u, %u, '%s', %u)", Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetAccountId(), rd->id, rd->comment.c_str(), time(NULL));
                    if (rd->action & WA_ACT_KICK)
                        kick = true;
                    if (rd->action & WA_ACT_BAN) {
                        ban = true;
                        ban_reason << "[" << rd->id << "] ";
                    }
                    continue;
                }

                if (memcmp(buff.contents() + buff.rpos(), rs->res.AsByteArray(0, false).get(), 20) != 0) // SHA1
                {
                    //TC_LOG_DEBUG("warden","RESULT MPQ_CHECK fail, CheckId %u account Id %u", rd->id, Client->GetAccountId());
                    TC_LOG_DEBUG("warden","Warden: MPQ CHECK FAILED at check %u (%s) for account %u, player %u (%s).", rd->id, rd->comment.c_str(), Client->GetAccountId(), Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetPlayer() ? Client->GetPlayer()->GetName().c_str() : "<Not connected>");
                    found = true;
                    if (rd->action & WA_ACT_LOG)
                        LogsDatabase.PQuery("INSERT INTO warden_fails (guid, account, check_id, comment, time) VALUES (%u, %u, %u, '%s', %u)", Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetAccountId(), rd->id, rd->comment.c_str(), time(NULL));
                    if (rd->action & WA_ACT_KICK)
                        kick = true;
                    if (rd->action & WA_ACT_BAN) {
                        ban = true;
                        ban_reason << "[" << rd->id << "] ";
                    }
                    
                    buff.rpos(buff.rpos() + 20);            // 20 bytes SHA1
                    continue;
                }

                buff.rpos(buff.rpos() + 20);                // 20 bytes SHA1
                TC_LOG_DEBUG("warden","Warden: MPQ CHECK PASSED at check %u (%s) for account %u, player %u (%s).", rd->id, rd->comment.c_str(), Client->GetAccountId(), Client->GetPlayer() ? Client->GetPlayer()->GetGUID().GetCounter() : 0, Client->GetPlayer() ? Client->GetPlayer()->GetName().c_str() : "<Not connected>");
                break;
            }
            default:                                        // should never happens
                break;
        }
    }

    if (found) {
        if (ban) {
            std::string banuname; 
            QueryResult result = LoginDatabase.PQuery("SELECT username FROM account WHERE id = '%u'", Client->GetAccountId());
            if (result) {
                Field* fields = result->Fetch();
                banuname = fields[0].GetString();
                sWorld->BanAccount(SANCTION_BAN_ACCOUNT, banuname, sWorld->GetWardenBanTime(), ban_reason.str(), "Warden", nullptr);
            }
        }
        if (kick)
            Client->KickPlayer();
    }
}
