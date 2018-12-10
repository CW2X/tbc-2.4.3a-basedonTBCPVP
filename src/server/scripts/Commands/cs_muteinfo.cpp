#include "Chat.h"
#include "Language.h"
#include "CharacterCache.h"
#include "AccountMgr.h"
class muteinfo_commandscript : public CommandScript
{
public:
    muteinfo_commandscript() : CommandScript("muteinfo_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> muteinfoCommandTable =
        {
            { "account",        SEC_GAMEMASTER3,  true, &HandleMuteInfoAccountCommand,      "" },
            { "character",      SEC_GAMEMASTER3,  true, &HandleMuteInfoCharacterCommand,    "" },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "muteinfo",        SEC_GAMEMASTER3, false,nullptr,                            "", muteinfoCommandTable },
        };
        return commandTable;
    }

    static bool MuteInfoForAccount(ChatHandler& chatHandler, uint32 accountid)
    {
        QueryResult result = LoginDatabase.PQuery("SELECT mutetime FROM account WHERE id = %u", accountid);
        //mutetime here is actually timestamp before player can speak again
        if (!result) {
            chatHandler.PSendSysMessage("No active sanction on this account.");
            return true;
        }
        else {
            Field* fields = result->Fetch();
            uint64 unmuteTime = fields[0].GetUInt64();

            chatHandler.PSendSysMessage("Active sanction on this account. Unban time: %s", TimeToTimestampStr(unmuteTime));
        }

        chatHandler.PSendSysMessage("Detailed mute list :");
        //PrepareStatement(LOGS_SEL_SANCTION_MUTE_ACCOUNT, "SELECT author_account, author_guid, target_account, duration, time, reason, IP FROM gm_sanction WHERE target_account = ? AND type = 5", CONNECTION_SYNCH); //5 is SANCTION_MUTE_ACCOUNT
        PreparedStatement* stmt = LogsDatabase.GetPreparedStatement(LOGS_SEL_SANCTION_MUTE_ACCOUNT);
        stmt->setUInt32(0, accountid);
        PreparedQueryResult result2 = LogsDatabase.Query(stmt);

        if (!result2) {
            chatHandler.PSendSysMessage("No sanction logged on this account.");
            return true;
        }

        do {
            Field* fields = result2->Fetch();

            uint32 authorAccount = fields[0].GetUInt32();
            ObjectGuid::LowType authorGUID = fields[1].GetUInt32();
    //        uint32 targetAccount = fields[2].GetUInt32();
            uint32 duration = fields[3].GetUInt32();
            uint64 muteTime = fields[4].GetUInt32();
            std::string reason = fields[5].GetString();
            std::string authorIP = fields[6].GetString();
            uint64 unbantimestamp = muteTime + (duration * MINUTE);
            std::string authorname;
            std::string displayName;
            bool nameResult = sCharacterCache->GetCharacterNameByGuid(ObjectGuid(HighGuid::Player, authorGUID), displayName);
            if (!nameResult)
                authorname = "<Unknown>";

            chatHandler.PSendSysMessage("Account %u: Mute %s for %s by %s (account %u) at %s (%s).", accountid, secsToTimeString(duration).c_str(), reason.c_str(), authorname.c_str(), authorAccount, TimeToTimestampStr(muteTime), (unbantimestamp > uint64(time(nullptr))) ? " (actif)" : "");
        } while (result2->NextRow());

        return true;
    }

    static bool HandleMuteInfoAccountCommand(ChatHandler* handler, char const* args)
    {
        ARGS_CHECK
        
        char* cname = strtok((char*)args, "");
        if(!cname)
            return false;

        std::string account_name = cname;
        if(!AccountMgr::normalizeString(account_name))
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST,account_name.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 accountid = sAccountMgr->GetId(account_name);
        if(!accountid)
        {
            handler->PSendSysMessage(LANG_ACCOUNT_NOT_EXIST,account_name.c_str());
            return true;
        }
    
        return MuteInfoForAccount(*handler, accountid);
    }

    static bool HandleMuteInfoCharacterCommand(ChatHandler* handler, char const* args)
    {
        char* cname = strtok ((char*)args, "");
        if(!cname)
            return false;

        std::string name = cname;
        if(!normalizePlayerName(name))
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        uint32 accountid = sCharacterCache->GetCharacterAccountIdByName(name);
        if(!accountid)
        {
            handler->SendSysMessage(LANG_PLAYER_NOT_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }
    
        return MuteInfoForAccount(*handler, accountid);
    }
};

void AddSC_muteinfo_commandscript()
{
    new muteinfo_commandscript();
}
