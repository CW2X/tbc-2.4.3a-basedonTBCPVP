#include "Chat.h"
#include "Language.h"
#include "GameEventMgr.h"
#include "AccountMgr.h"
#include "ReputationMgr.h"

class lookup_commandscript : public CommandScript
{
public:
    lookup_commandscript() : CommandScript("lookup_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> lookupPlayerCommandTable =
        {
            { "ip",             SEC_GAMEMASTER2,  true, &HandleLookupPlayerIpCommand,       "" },
            { "account",        SEC_GAMEMASTER2,  true, &HandleLookupPlayerAccountCommand,  "" },
            { "email",          SEC_GAMEMASTER2,  true, &HandleLookupPlayerEmailCommand,    "" },
        };
        static std::vector<ChatCommand> lookupCommandTable =
        {
            { "area",           SEC_GAMEMASTER1,  true, &HandleLookupAreaCommand,          "" },
            { "creature",       SEC_GAMEMASTER3,  true, &HandleLookupCreatureCommand,      "" },
            { "event",          SEC_GAMEMASTER2,  true, &HandleLookupEventCommand,         "" },
            { "faction",        SEC_GAMEMASTER3,  true, &HandleLookupFactionCommand,       "" },
            { "item",           SEC_GAMEMASTER3,  true, &HandleLookupItemCommand,          "" },
            { "itemset",        SEC_GAMEMASTER3,  true, &HandleLookupItemSetCommand,       "" },
            { "object",         SEC_GAMEMASTER3,  true, &HandleLookupObjectCommand,        "" },
            { "quest",          SEC_GAMEMASTER3,  true, &HandleLookupQuestCommand,         "" },
            { "player",         SEC_GAMEMASTER2,  true, nullptr,                           "", lookupPlayerCommandTable },
            { "skill",          SEC_GAMEMASTER3,  true, &HandleLookupSkillCommand,         "" },
            { "spell",          SEC_GAMEMASTER3,  true, &HandleGetSpellInfoCommand,        "" },
            { "tele",           SEC_GAMEMASTER1,  true, &HandleLookupTeleCommand,          "" },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "lookup",         SEC_GAMEMASTER3,  true,  nullptr,                          "", lookupCommandTable },
        };
        return commandTable;
    }

    static bool HandleLookupAreaCommand(ChatHandler* handler, char const* args)
    {
        ARGS_CHECK

            std::string namepart = args;
        std::wstring wnamepart;

        if (!Utf8toWStr(namepart, wnamepart))
            return false;

        uint32 counter = 0;                                     // Counter for figure out that we found smth.

        // converting string that we try to find to lower case
        wstrToLower(wnamepart);

        // Search in AreaTable.dbc
        for (uint32 areaID = 0; areaID < sAreaTableStore.GetNumRows(); ++areaID)
        {
            AreaTableEntry const *areaEntry = sAreaTableStore.LookupEntry(areaID);
            if (areaEntry)
            {
                int loc = handler->GetSessionDbcLocale();
                std::string name = areaEntry->area_name[loc];
                if (name.empty())
                    continue;

                if (!Utf8FitTo(name, wnamepart))
                {
                    loc = 0;
                    for (; loc < TOTAL_LOCALES; ++loc)
                    {
                        if (loc == handler->GetSessionDbcLocale())
                            continue;

                        name = areaEntry->area_name[loc];
                        if (name.empty())
                            continue;

                        if (Utf8FitTo(name, wnamepart))
                            break;
                    }
                }

                if (loc < TOTAL_LOCALES)
                {
                    // send area in "id - [name]" format
                    std::ostringstream ss;
                    if (handler->GetSession())
                        ss << areaEntry->ID << " - |cffffffff|Harea:" << areaEntry->ID << "|h[" << name << " " << localeNames[loc] << "]|h|r";
                    else
                        ss << areaEntry->ID << " - " << name << " " << localeNames[loc];

                    handler->SendSysMessage(ss.str().c_str());

                    ++counter;
                }
            }
        }
        if (counter == 0)                                      // if counter == 0 then we found nth
            handler->SendSysMessage(LANG_COMMAND_NOAREAFOUND);
        return true;
    }

    //Find tele in game_tele order by name
    static bool HandleLookupTeleCommand(ChatHandler* handler, char const* args)
    {
        if (!*args)
        {
            handler->SendSysMessage(LANG_COMMAND_TELE_PARAMETER);
            handler->SetSentErrorMessage(true);
            return false;
        }

        char const* str = strtok((char*)args, " ");
        if (!str)
            return false;

        std::string namepart = str;
        std::wstring wnamepart;

        if (!Utf8toWStr(namepart, wnamepart))
            return false;

        // converting string that we try to find to lower case
        wstrToLower(wnamepart);

        std::ostringstream reply;

        GameTeleContainer const & teleMap = sObjectMgr->GetGameTeleMap();
        for (const auto & itr : teleMap)
        {
            GameTele const* tele = &itr.second;

            if (tele->wnameLow.find(wnamepart) == std::wstring::npos)
                continue;

            if (handler->GetSession())
                reply << "  |cffffffff|Htele:" << itr.first << "|h[" << tele->name << "]|h|r\n";
            else
                reply << "  " << itr.first << " " << tele->name << "\n";
        }

        if (reply.str().empty())
            handler->SendSysMessage(LANG_COMMAND_TELE_NOLOCATION);
        else
            handler->PSendSysMessage(LANG_COMMAND_TELE_LOCATION, reply.str().c_str());

        return true;
    }


    static bool HandleLookupCreatureCommand(ChatHandler* handler, char const* args)
    {
        ARGS_CHECK

        std::string namepart = args;
        std::wstring wnamepart;

        // converting string that we try to find to lower case
        if (!Utf8toWStr(namepart, wnamepart))
            return false;

        wstrToLower(wnamepart);

        uint32 counter = 0;

        CreatureTemplateContainer const& ctc = sObjectMgr->GetCreatureTemplates();
        for (const auto & itr : ctc)
        {
            uint32 id = itr.second.Entry;
            CreatureTemplate const* cInfo = &(itr.second);
            LocaleConstant loc_idx = handler->GetSessionDbcLocale();
            if (loc_idx != DEFAULT_LOCALE)
            {
                CreatureLocale const *cl = sObjectMgr->GetCreatureLocale(id);
                if (cl)
                {
                    if (cl->Name.size() > loc_idx && !cl->Name[loc_idx].empty())
                    {
                        std::string name = cl->Name[loc_idx];

                        if (Utf8FitTo(name, wnamepart))
                        {
                            if (handler->GetSession())
                                handler->PSendSysMessage(LANG_CREATURE_ENTRY_LIST_CHAT, id, id, name.c_str());
                            else
                                handler->PSendSysMessage(LANG_CREATURE_ENTRY_LIST_CONSOLE, id, name.c_str());
                            ++counter;
                            continue;
                        }
                    }
                }
            }

            std::string name = cInfo->Name;
            if (name.empty())
                continue;

            if (Utf8FitTo(name, wnamepart))
            {
                if (handler->GetSession())
                    handler->PSendSysMessage(LANG_CREATURE_ENTRY_LIST_CHAT, id, id, name.c_str());
                else
                    handler->PSendSysMessage(LANG_CREATURE_ENTRY_LIST_CONSOLE, id, name.c_str());
                ++counter;
            }
        }

        if (counter == 0)
            handler->SendSysMessage(LANG_COMMAND_NOCREATUREFOUND);

        return true;
    }

    static bool HandleLookupObjectCommand(ChatHandler* handler, char const* args)
    {
        ARGS_CHECK

        std::string namepart = args;
        std::wstring wnamepart;

        // converting string that we try to find to lower case
        if (!Utf8toWStr(namepart, wnamepart))
            return false;

        wstrToLower(wnamepart);

        uint32 counter = 0;


        GameObjectTemplateContainer const* gotc = sObjectMgr->GetGameObjectTemplateStore();
        for (const auto & itr : *gotc)
        {
            uint32 id = itr.first;
            GameObjectTemplate const* gInfo = &(itr.second);
            LocaleConstant loc_idx = handler->GetSessionDbcLocale();
            if (loc_idx != DEFAULT_LOCALE)
            {
                GameObjectLocale const *gl = sObjectMgr->GetGameObjectLocale(id);
                if (gl)
                {
                    if (gl->Name.size() > loc_idx && !gl->Name[loc_idx].empty())
                    {
                        std::string name = gl->Name[loc_idx];

                        if (Utf8FitTo(name, wnamepart))
                        {
                            if (handler->GetSession())
                                handler->PSendSysMessage(LANG_GO_ENTRY_LIST_CHAT, id, id, name.c_str());
                            else
                                handler->PSendSysMessage(LANG_GO_ENTRY_LIST_CONSOLE, id, name.c_str());
                            ++counter;
                            continue;
                        }
                    }
                }
            }

            std::string name = gInfo->name;
            if (name.empty())
                continue;

            if (Utf8FitTo(name, wnamepart))
            {
                if (handler->GetSession())
                    handler->PSendSysMessage(LANG_GO_ENTRY_LIST_CHAT, id, id, name.c_str());
                else
                    handler->PSendSysMessage(LANG_GO_ENTRY_LIST_CONSOLE, id, name.c_str());
                ++counter;
            }
        }

        if (counter == 0)
            handler->SendSysMessage(LANG_COMMAND_NOGAMEOBJECTFOUND);

        return true;
    }

    static bool HandleLookupFactionCommand(ChatHandler* handler, char const* args)
    {
        ARGS_CHECK

        // Can be NULL at console call
        Player* target = handler->GetSelectedPlayer();

        std::string namepart = args;
        std::wstring wnamepart;

        if (!Utf8toWStr(namepart, wnamepart))
            return false;

        // converting string that we try to find to lower case
        wstrToLower(wnamepart);

        uint32 counter = 0;                                     // Counter for figure out that we found smth.

        for (uint32 id = 0; id < sFactionStore.GetNumRows(); ++id)
        {
            FactionEntry const *factionEntry = sFactionStore.LookupEntry(id);
            if (factionEntry)
            {
                FactionState const* factionState = target ? target->GetReputationMgr().GetState(factionEntry) : nullptr;

                int loc = handler->GetSessionDbcLocale();
                std::string name = factionEntry->name[loc];
                if (name.empty())
                    continue;

                if (!Utf8FitTo(name, wnamepart))
                {
                    loc = 0;
                    for (; loc < TOTAL_LOCALES; ++loc)
                    {
                        if (handler->GetSessionDbcLocale())
                            continue;

                        name = factionEntry->name[loc];
                        if (name.empty())
                            continue;

                        if (Utf8FitTo(name, wnamepart))
                            break;
                    }
                }

                if (loc < TOTAL_LOCALES)
                {
                    // send faction in "id - [faction] rank reputation [visible] [at war] [own team] [unknown] [invisible] [inactive]" format
                    // or              "id - [faction] [no reputation]" format
                    std::ostringstream ss;
                    if (handler->GetSession())
                        ss << id << " - |cffffffff|Hfaction:" << id << "|h[" << name << " " << localeNames[loc] << "]|h|r";
                    else
                        ss << id << " - " << name << " " << localeNames[loc];

                    if (factionState)                               // and then target!=NULL also
                    {
                        uint32 index = target->GetReputationMgr().GetReputationRankStrIndex(factionEntry);
                        std::string rankName = handler->GetTrinityString(index);

                        ss << " " << rankName << "|h|r (" << target->GetReputationMgr().GetReputation(factionEntry) << ")";

                        if (factionState->Flags & FACTION_FLAG_VISIBLE)
                            ss << handler->GetTrinityString(LANG_FACTION_VISIBLE);
                        if (factionState->Flags & FACTION_FLAG_AT_WAR)
                            ss << handler->GetTrinityString(LANG_FACTION_ATWAR);
                        if (factionState->Flags & FACTION_FLAG_PEACE_FORCED)
                            ss << handler->GetTrinityString(LANG_FACTION_PEACE_FORCED);
                        if (factionState->Flags & FACTION_FLAG_HIDDEN)
                            ss << handler->GetTrinityString(LANG_FACTION_HIDDEN);
                        if (factionState->Flags & FACTION_FLAG_INVISIBLE_FORCED)
                            ss << handler->GetTrinityString(LANG_FACTION_INVISIBLE_FORCED);
                        if (factionState->Flags & FACTION_FLAG_INACTIVE)
                            ss << handler->GetTrinityString(LANG_FACTION_INACTIVE);
                    }
                    else
                        ss << handler->GetTrinityString(LANG_FACTION_NOREPUTATION);

                    handler->SendSysMessage(ss.str().c_str());
                    counter++;
                }
            }
        }

        if (counter == 0)                                       // if counter == 0 then we found nth
            handler->SendSysMessage(LANG_COMMAND_FACTION_NOTFOUND);
        return true;
    }

    static bool HandleLookupItemCommand(ChatHandler* handler, char const* args)
    {
        ARGS_CHECK

            std::string namepart = args;
        std::wstring wnamepart;

        // converting string that we try to find to lower case
        if (!Utf8toWStr(namepart, wnamepart))
            return false;

        wstrToLower(wnamepart);

        uint32 counter = 0;

        // Search in `item_template`
        ItemTemplateContainer const& its = sObjectMgr->GetItemTemplateStore();
        for (const auto & it : its)
        {
            uint32 id = it.first;
            ItemTemplate const *pProto = &(it.second);
            if (!pProto)
                continue;

            LocaleConstant loc_idx = handler->GetSessionDbcLocale();
            if (loc_idx != DEFAULT_LOCALE)
            {
                ItemLocale const *il = sObjectMgr->GetItemLocale(pProto->ItemId);
                if (il)
                {
                    if (il->Name.size() > loc_idx && !il->Name[loc_idx].empty())
                    {
                        std::string name = il->Name[loc_idx];

                        if (Utf8FitTo(name, wnamepart))
                        {
                            if (handler->GetSession())
                                handler->PSendSysMessage(LANG_ITEM_LIST_CHAT, id, id, name.c_str());
                            else
                                handler->PSendSysMessage(LANG_ITEM_LIST_CONSOLE, id, name.c_str());
                            ++counter;
                            continue;
                        }
                    }
                }
            }

            std::string name = pProto->Name1;
            if (name.empty())
                continue;

            if (Utf8FitTo(name, wnamepart))
            {
                if (handler->GetSession())
                    handler->PSendSysMessage(LANG_ITEM_LIST_CHAT, id, id, name.c_str());
                else
                    handler->PSendSysMessage(LANG_ITEM_LIST_CONSOLE, id, name.c_str());
                ++counter;
            }
        }

        if (counter == 0)
            handler->SendSysMessage(LANG_COMMAND_NOITEMFOUND);

        return true;
    }

    static bool HandleLookupItemSetCommand(ChatHandler* handler, char const* args)
    {
        ARGS_CHECK

            std::string namepart = args;
        std::wstring wnamepart;

        if (!Utf8toWStr(namepart, wnamepart))
            return false;

        // converting string that we try to find to lower case
        wstrToLower(wnamepart);

        uint32 counter = 0;                                     // Counter for figure out that we found smth.

        // Search in ItemSet.dbc
        for (uint32 id = 0; id < sItemSetStore.GetNumRows(); id++)
        {
            ItemSetEntry const *set = sItemSetStore.LookupEntry(id);
            if (set)
            {
                int loc = handler->GetSessionDbcLocale();
                std::string name = set->name[loc];
                if (name.empty())
                    continue;

                if (!Utf8FitTo(name, wnamepart))
                {
                    loc = 0;
                    for (; loc < TOTAL_LOCALES; ++loc)
                    {
                        if (handler->GetSessionDbcLocale())
                            continue;

                        name = set->name[loc];
                        if (name.empty())
                            continue;

                        if (Utf8FitTo(name, wnamepart))
                            break;
                    }
                }

                if (loc < TOTAL_LOCALES)
                {
                    // send item set in "id - [namedlink locale]" format
                    if (handler->GetSession())
                        handler->PSendSysMessage(LANG_ITEMSET_LIST_CHAT, id, id, name.c_str(), localeNames[loc]);
                    else
                        handler->PSendSysMessage(LANG_ITEMSET_LIST_CONSOLE, id, name.c_str(), localeNames[loc]);
                    ++counter;
                }
            }
        }
        if (counter == 0)                                       // if counter == 0 then we found nth
            handler->SendSysMessage(LANG_COMMAND_NOITEMSETFOUND);
        return true;
    }

    static bool HandleLookupSkillCommand(ChatHandler* handler, char const* args)
    {
        ARGS_CHECK

            // can be NULL in console call
            Player* target = handler->GetSelectedPlayerOrSelf();

        std::string namepart = args;
        std::wstring wnamepart;

        if (!Utf8toWStr(namepart, wnamepart))
            return false;

        // converting string that we try to find to lower case
        wstrToLower(wnamepart);

        uint32 counter = 0;                                     // Counter for figure out that we found smth.

        // Search in SkillLine.dbc
        for (uint32 id = 0; id < sSkillLineStore.GetNumRows(); id++)
        {
            SkillLineEntry const *skillInfo = sSkillLineStore.LookupEntry(id);
            if (skillInfo)
            {
                int loc = handler->GetSessionDbcLocale();
                std::string name = skillInfo->name[loc];
                if (name.empty())
                    continue;

                if (!Utf8FitTo(name, wnamepart))
                {
                    loc = 0;
                    for (; loc < TOTAL_LOCALES; ++loc)
                    {
                        if (handler->GetSessionDbcLocale())
                            continue;

                        name = skillInfo->name[loc];
                        if (name.empty())
                            continue;

                        if (Utf8FitTo(name, wnamepart))
                            break;
                    }
                }

                if (loc < TOTAL_LOCALES)
                {
                    char const* knownStr = "";
                    if (target && target->HasSkill(id))
                        knownStr = handler->GetTrinityString(LANG_KNOWN);

                    // send skill in "id - [namedlink locale]" format
                    if (handler->GetSession())
                        handler->PSendSysMessage(LANG_SKILL_LIST_CHAT, id, id, name.c_str(), localeNames[loc], knownStr);
                    else
                        handler->PSendSysMessage(LANG_SKILL_LIST_CONSOLE, id, name.c_str(), localeNames[loc], knownStr);

                    ++counter;
                }
            }
        }
        if (counter == 0)                                       // if counter == 0 then we found nth
            handler->SendSysMessage(LANG_COMMAND_NOSKILLFOUND);
        return true;
    }

    static bool HandleLookupQuestCommand(ChatHandler* handler, char const* args)
    {
        ARGS_CHECK

            // can be NULL at console call
            Player* target = handler->GetSelectedPlayerOrSelf();

        std::string namepart = args;
        std::wstring wnamepart;

        // converting string that we try to find to lower case
        if (!Utf8toWStr(namepart, wnamepart))
            return false;

        wstrToLower(wnamepart);

        uint32 counter = 0;

        ObjectMgr::QuestContainer const& qTemplates = sObjectMgr->GetQuestTemplates();
        for (const auto & qTemplate : qTemplates)
        {
            auto qinfo = &qTemplate.second;

            LocaleConstant loc_idx = handler->GetSessionDbcLocale();
            if (loc_idx != DEFAULT_LOCALE)
            {
                auto il = sObjectMgr->GetQuestLocale(qinfo->GetQuestId());
                if (il)
                {
                    if (il->Title.size() > loc_idx && !il->Title[loc_idx].empty())
                    {
                        std::string title = il->Title[loc_idx];

                        if (Utf8FitTo(title, wnamepart))
                        {
                            char const* statusStr = "";

                            if (target)
                            {
                                QuestStatus status = target->GetQuestStatus(qinfo->GetQuestId());

                                if (status == QUEST_STATUS_COMPLETE)
                                {
                                    if (target->GetQuestRewardStatus(qinfo->GetQuestId()))
                                        statusStr = handler->GetTrinityString(LANG_COMMAND_QUEST_REWARDED);
                                    else
                                        statusStr = handler->GetTrinityString(LANG_COMMAND_QUEST_COMPLETE);
                                }
                                else if (status == QUEST_STATUS_INCOMPLETE)
                                    statusStr = handler->GetTrinityString(LANG_COMMAND_QUEST_ACTIVE);
                            }

                            if (handler->GetSession())
                                handler->PSendSysMessage(LANG_QUEST_LIST_CHAT, qinfo->GetQuestId(), qinfo->GetQuestId(), title.c_str(), statusStr);
                            else
                                handler->PSendSysMessage(LANG_QUEST_LIST_CONSOLE, qinfo->GetQuestId(), title.c_str(), statusStr);

                            ++counter;
                            continue;
                        }
                    }
                }
            }

            std::string title = qinfo->GetTitle();
            if (title.empty())
                continue;

            if (Utf8FitTo(title, wnamepart))
            {
                char const* statusStr = "";

                if (target)
                {
                    QuestStatus status = target->GetQuestStatus(qinfo->GetQuestId());

                    if (status == QUEST_STATUS_COMPLETE)
                    {
                        if (target->GetQuestRewardStatus(qinfo->GetQuestId()))
                            statusStr = handler->GetTrinityString(LANG_COMMAND_QUEST_REWARDED);
                        else
                            statusStr = handler->GetTrinityString(LANG_COMMAND_QUEST_COMPLETE);
                    }
                    else if (status == QUEST_STATUS_INCOMPLETE)
                        statusStr = handler->GetTrinityString(LANG_COMMAND_QUEST_ACTIVE);
                }

                if (handler->GetSession())
                    handler->PSendSysMessage(LANG_QUEST_LIST_CHAT, qinfo->GetQuestId(), qinfo->GetQuestId(), title.c_str(), statusStr);
                else
                    handler->PSendSysMessage(LANG_QUEST_LIST_CONSOLE, qinfo->GetQuestId(), title.c_str(), statusStr);

                ++counter;
            }
        }

        if (counter == 0)
            handler->SendSysMessage(LANG_COMMAND_NOQUESTFOUND);

        return true;
    }

    static bool HandleGetSpellInfoCommand(ChatHandler* handler, char const* args)
    {
        ARGS_CHECK

            // can be NULL at console call
            Player* target = handler->GetSelectedPlayerOrSelf();

        std::string namepart = args;
        std::wstring wnamepart;

        if (!Utf8toWStr(namepart, wnamepart))
            return false;

        // converting string that we try to find to lower case
        wstrToLower(wnamepart);

        uint32 counter = 0;                                     // Counter for figure out that we found smth.

        for (uint32 id = 0; id < sObjectMgr->GetSpellStore().size(); id++)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(id);
            if (spellInfo)
            {
                int loc = handler->GetSessionDbcLocale();
                std::string name = spellInfo->SpellName[loc];
                if (name.empty())
                    continue;

                //if name is not valid, try every locales until we find one valid
                if (!Utf8FitTo(name, wnamepart))
                {
                    loc = 0;
                    for (; loc < TOTAL_LOCALES; ++loc)
                    {
                        name = spellInfo->SpellName[loc];
                        if (name.empty())
                            continue;

                        if (Utf8FitTo(name, wnamepart))
                            break;
                    }
                }

                if (loc < TOTAL_LOCALES)
                {
                    bool known = target && target->HasSpell(id);
                    bool learn = (spellInfo->Effects[0].Effect == SPELL_EFFECT_LEARN_SPELL);

                    uint32 talentCost = GetTalentSpellCost(id);

                    bool talent = (talentCost > 0);
                    bool passive = spellInfo->IsPassive();
                    bool active = target && target->HasAura(id);

                    // unit32 used to prevent interpreting uint8 as char at output
                    // find rank of learned spell for learning spell, or talent rank
                    uint32 rank = talentCost ? talentCost : sSpellMgr->GetSpellRank(learn ? spellInfo->Effects[0].TriggerSpell : id);

                    // send spell in "id - [name, rank N] [talent] [passive] [learn] [known]" format
                    std::ostringstream ss;
                    if (handler->GetSession())
                        ss << id << " - |cffffffff|Hspell:" << id << "|h[" << name;
                    else
                        ss << id << " - " << name;

                    // include rank in link name
                    if (rank)
                        ss << handler->GetTrinityString(LANG_SPELL_RANK) << rank;

                    if (handler->GetSession())
                        ss << " " << localeNames[loc] << "]|h|r";
                    else
                        ss << " " << localeNames[loc];

                    if (talent)
                        ss << handler->GetTrinityString(LANG_TALENT);
                    if (passive)
                        ss << handler->GetTrinityString(LANG_PASSIVE);
                    if (learn)
                        ss << handler->GetTrinityString(LANG_LEARN);
                    if (known)
                        ss << handler->GetTrinityString(LANG_KNOWN);
                    if (active)
                        ss << handler->GetTrinityString(LANG_ACTIVE);

                    handler->SendSysMessage(ss.str().c_str());

                    ++counter;
                }
            }
        }
        if (counter == 0)                                       // if counter == 0 then we found nth
            handler->SendSysMessage(LANG_COMMAND_NOSPELLFOUND);
        return true;
    }

    static bool HandleLookupPlayerIpCommand(ChatHandler* handler, char const* args)
    {
        ARGS_CHECK

            std::string ip = strtok((char*)args, " ");
        char* limit_str = strtok(nullptr, " ");
        int32 limit = limit_str ? atoi(limit_str) : -1;

        LoginDatabase.EscapeString(ip);

        QueryResult result = LoginDatabase.PQuery("SELECT id,username FROM account WHERE last_ip = '%s'", ip.c_str());

        return LookupPlayerSearchCommand(handler, result, limit);
    }

    static bool HandleLookupPlayerAccountCommand(ChatHandler* handler, char const* args)
    {
        ARGS_CHECK

            std::string account = strtok((char*)args, " ");
        char* limit_str = strtok(nullptr, " ");
        int32 limit = limit_str ? atoi(limit_str) : -1;

        if (!AccountMgr::normalizeString(account))
            return false;

        LoginDatabase.EscapeString(account);

        QueryResult result = LoginDatabase.PQuery("SELECT id,username FROM account WHERE username = '%s'", account.c_str());

        return LookupPlayerSearchCommand(handler, result, limit);
    }

    static bool HandleLookupPlayerEmailCommand(ChatHandler* handler, char const* args)
    {
        ARGS_CHECK

            std::string email = strtok((char*)args, " ");
        char* limit_str = strtok(nullptr, " ");
        int32 limit = limit_str ? atoi(limit_str) : -1;

        LoginDatabase.EscapeString(email);

        QueryResult result = LoginDatabase.PQuery("SELECT id,username FROM account WHERE email = '%s'", email.c_str());

        return LookupPlayerSearchCommand(handler, result, limit);
    }

    static bool HandleLookupEventCommand(ChatHandler* handler, char const* args)
    {
        ARGS_CHECK

            std::string namepart = args;
        std::wstring wnamepart;

        // converting string that we try to find to lower case
        if (!Utf8toWStr(namepart, wnamepart))
            return false;

        wstrToLower(wnamepart);

        uint32 counter = 0;

        GameEventMgr::GameEventDataMap const& events = sGameEventMgr->GetEventMap();
        GameEventMgr::ActiveEvents const& activeEvents = sGameEventMgr->GetActiveEventList();

        for (uint32 id = 0; id < events.size(); ++id)
        {
            GameEventData const& eventData = events[id];

            std::string descr = eventData.description;
            if (descr.empty())
                continue;

            if (Utf8FitTo(descr, wnamepart))
            {
                char const* active = activeEvents.find(id) != activeEvents.end() ? handler->GetTrinityString(LANG_ACTIVE) : "";

                if (handler->GetSession())
                    handler->PSendSysMessage(LANG_EVENT_ENTRY_LIST_CHAT, id, id, eventData.description.c_str(), active);
                else
                    handler->PSendSysMessage(LANG_EVENT_ENTRY_LIST_CONSOLE, id, eventData.description.c_str(), active);

                ++counter;
            }
        }

        if (counter == 0)
            handler->SendSysMessage(LANG_NOEVENTFOUND);

        return true;
    }

    static bool LookupPlayerSearchCommand(ChatHandler* handler, QueryResult result, int32 limit)
    {
        if (!result)
        {
            handler->PSendSysMessage(LANG_NO_PLAYERS_FOUND);
            handler->SetSentErrorMessage(true);
            return false;
        }

        int i = 0;
        do
        {
            Field* fields = result->Fetch();
            uint32 acc_id = fields[0].GetUInt32();
            std::string acc_name = fields[1].GetString();

            QueryResult chars = CharacterDatabase.PQuery("SELECT guid,name FROM characters WHERE account = '%u'", acc_id);
            if (chars)
            {
                handler->PSendSysMessage(LANG_LOOKUP_PLAYER_ACCOUNT, acc_name.c_str(), acc_id);

                ObjectGuid guid = ObjectGuid::Empty;
                std::string name;

                do
                {
                    Field* charfields = chars->Fetch();
                    guid = ObjectGuid(charfields[0].GetUInt64());
                    name = charfields[1].GetString();

                    handler->PSendSysMessage(LANG_LOOKUP_PLAYER_CHARACTER, name.c_str(), guid);
                    ++i;

                } while (chars->NextRow() && (limit == -1 || i < limit));
            }
        } while (result->NextRow());

        return true;
    }
};

void AddSC_lookup_commandscript()
{
    new lookup_commandscript();
}

