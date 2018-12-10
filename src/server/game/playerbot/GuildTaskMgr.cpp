
#include "playerbot.h"
#include "PlayerbotAIConfig.h"
#include "GuildTaskMgr.h"
#include "GuildMgr.h"

//#include "../../plugins/ahbot/AhBot.h"
//#include "GuildMgr.h"
#include "DatabaseEnv.h"
#include "Mail.h"
#include "PlayerbotAI.h"
#include "CharacterCache.h"

//#include "../../plugins/ahbot/AhBotConfig.h"
#include "RandomItemMgr.h"

char * strstri (const char* str1, const char* str2);

enum GuildTaskType
{
    GUILD_TASK_TYPE_NONE = 0,
    GUILD_TASK_TYPE_ITEM = 1,
    GUILD_TASK_TYPE_KILL = 2
};

GuildTaskMgr::GuildTaskMgr()
{
}

GuildTaskMgr::~GuildTaskMgr()
{
}

void GuildTaskMgr::Update(Player* player, Player* guildMaster)
{
    if (!sPlayerbotAIConfig.guildTaskEnabled)
        return;

    uint32 guildId = guildMaster->GetGuildId();
    if (!guildId || !guildMaster->GetPlayerbotAI() || !guildMaster->GetGuild())
        return;

    if (!player->IsFriendlyTo(guildMaster))
        return;

    DenyReason reason = PLAYERBOT_DENY_NONE;
    PlayerbotSecurityLevel secLevel = guildMaster->GetPlayerbotAI()->GetSecurity()->LevelFor(player, &reason);
    if (secLevel == PLAYERBOT_SECURITY_DENY_ALL || (secLevel == PLAYERBOT_SECURITY_TALK && reason != PLAYERBOT_DENY_FAR))
    {
        sLog->outMessage("gtask", LOG_LEVEL_DEBUG, "%s / %s: skipping guild task update - not enough security level, reason = %u",
                guildMaster->GetGuild()->GetName().c_str(), player->GetName().c_str(), reason);
        return;
    }

    uint32 owner = (uint32)player->GetGUID().GetCounter();

    uint32 activeTask = GetTaskValue(owner, guildId, "activeTask");
    if (!activeTask)
    {
        SetTaskValue(owner, guildId, "killTask", 0, 0);
        SetTaskValue(owner, guildId, "itemTask", 0, 0);
        SetTaskValue(owner, guildId, "itemCount", 0, 0);
        SetTaskValue(owner, guildId, "killTask", 0, 0);
        SetTaskValue(owner, guildId, "killCount", 0, 0);
        SetTaskValue(owner, guildId, "payment", 0, 0);
        SetTaskValue(owner, guildId, "thanks", 1, 2 * sPlayerbotAIConfig.maxGuildTaskChangeTime);
        SetTaskValue(owner, guildId, "reward", 1, 2 * sPlayerbotAIConfig.maxGuildTaskChangeTime);

        uint32 task = CreateTask(owner, guildId);

        if (task == GUILD_TASK_TYPE_NONE)
        {
            sLog->outMessage("gtask", LOG_LEVEL_ERROR, "%s / %s: error creating guild task",
                    guildMaster->GetGuild()->GetName().c_str(), player->GetName().c_str());
        }

        uint32 time = urand(sPlayerbotAIConfig.minGuildTaskChangeTime, sPlayerbotAIConfig.maxGuildTaskChangeTime);
        SetTaskValue(owner, guildId, "activeTask", task, time);
        SetTaskValue(owner, guildId, "advertisement", 1,
                urand(sPlayerbotAIConfig.minGuildTaskAdvertisementTime, sPlayerbotAIConfig.maxGuildTaskAdvertisementTime));

        sLog->outMessage("gtask", LOG_LEVEL_DEBUG, "%s / %s: guild task %u is set for %u secs",
                guildMaster->GetGuild()->GetName().c_str(), player->GetName().c_str(),
                task, time);
        return;
    }

    uint32 advertisement = GetTaskValue(owner, guildId, "advertisement");
    if (!advertisement)
    {
        sLog->outMessage("gtask", LOG_LEVEL_DEBUG, "%s / %s: sending advertisement",
                guildMaster->GetGuild()->GetName().c_str(), player->GetName().c_str());
        if (SendAdvertisement(owner, guildId))
        {
            SetTaskValue(owner, guildId, "advertisement", 1,
                    urand(sPlayerbotAIConfig.minGuildTaskAdvertisementTime, sPlayerbotAIConfig.maxGuildTaskAdvertisementTime));
        }
        else
        {
            sLog->outMessage("gtask", LOG_LEVEL_ERROR, "%s / %s: error sending advertisement",
                    guildMaster->GetGuild()->GetName().c_str(), player->GetName().c_str());
        }
    }

    uint32 thanks = GetTaskValue(owner, guildId, "thanks");
    if (!thanks)
    {
        sLog->outMessage("gtask", LOG_LEVEL_DEBUG, "%s / %s: sending thanks",
                guildMaster->GetGuild()->GetName().c_str(), player->GetName().c_str());
        if (SendThanks(owner, guildId))
        {
            SetTaskValue(owner, guildId, "thanks", 1, 2 * sPlayerbotAIConfig.maxGuildTaskChangeTime);
            SetTaskValue(owner, guildId, "payment", 0, 0);
        }
        else
        {
            sLog->outMessage("gtask", LOG_LEVEL_ERROR, "%s / %s: error sending thanks",
                    guildMaster->GetGuild()->GetName().c_str(), player->GetName().c_str());
        }
    }

    uint32 reward = GetTaskValue(owner, guildId, "reward");
    if (!reward)
    {
        sLog->outMessage("gtask", LOG_LEVEL_DEBUG, "%s / %s: sending reward",
                guildMaster->GetGuild()->GetName().c_str(), player->GetName().c_str());
        if (Reward(owner, guildId))
        {
            SetTaskValue(owner, guildId, "reward", 1, 2 * sPlayerbotAIConfig.maxGuildTaskChangeTime);
            SetTaskValue(owner, guildId, "payment", 0, 0);
        }
        else
        {
            sLog->outMessage("gtask", LOG_LEVEL_ERROR, "%s / %s: error sending reward",
                    guildMaster->GetGuild()->GetName().c_str(), player->GetName().c_str());
        }
    }
}

uint32 GuildTaskMgr::CreateTask(uint32 owner, uint32 guildId)
{
    switch (urand(0, 1))
    {
    case 0:
        CreateItemTask(owner, guildId);
        return GUILD_TASK_TYPE_ITEM;
    default:
        CreateKillTask(owner, guildId);
        return GUILD_TASK_TYPE_KILL;
    }
}

bool GuildTaskMgr::CreateItemTask(uint32 owner, uint32 guildId)
{
    Player* player = ObjectAccessor::FindPlayer(ObjectGuid(HighGuid::Player, owner));
    if (!player)
        return false;

    uint32 itemId = sRandomItemMgr.GetRandomItem(RANDOM_ITEM_GUILD_TASK);
    if (!itemId)
    {
        sLog->outMessage("gtask", LOG_LEVEL_ERROR, "%s / %s: no items avaible for item task",
                sGuildMgr->GetGuildById(guildId)->GetName().c_str(), player->GetName().c_str());
        return false;
    }

    uint32 count = GetMaxItemTaskCount(itemId);

    sLog->outMessage("gtask", LOG_LEVEL_DEBUG, "%s / %s: item task %u (x%d)",
            sGuildMgr->GetGuildById(guildId)->GetName().c_str(), player->GetName().c_str(),
            itemId, count);

    SetTaskValue(owner, guildId, "itemCount", count, sPlayerbotAIConfig.maxGuildTaskChangeTime);
    SetTaskValue(owner, guildId, "itemTask", itemId, sPlayerbotAIConfig.maxGuildTaskChangeTime);
    return true;
}

bool GuildTaskMgr::CreateKillTask(uint32 owner, uint32 guildId)
{
    Player* player = ObjectAccessor::FindPlayer(ObjectGuid(HighGuid::Player, (owner)));
    if (!player)
        return false;

    uint32 rank = !urand(0, 2) ? CREATURE_ELITE_RAREELITE : CREATURE_ELITE_RARE;
    vector<uint32> ids;
    CreatureTemplateContainer const& creatureTemplateContainer = sObjectMgr->GetCreatureTemplates();
    for (CreatureTemplateContainer::const_iterator i = creatureTemplateContainer.begin(); i != creatureTemplateContainer.end(); ++i)
    {
        CreatureTemplate const& co = i->second;
        if (co.rank != rank)
            continue;

        if (co.minlevel > player->GetLevel() + 4 || co.minlevel  < player->GetLevel() - 3)
            continue;

        if (co.Name.find("UNUSED") != string::npos)
            continue;

        ids.push_back(i->first);
    }

    if (ids.empty())
    {
        sLog->outMessage("gtask", LOG_LEVEL_ERROR, "%s / %s: no rare creatures available for kill task",
                sGuildMgr->GetGuildById(guildId)->GetName().c_str(), player->GetName().c_str());
        return false;
    }

    uint32 index = urand(0, ids.size() - 1);
    uint32 creatureId = ids[index];

    sLog->outMessage("gtask", LOG_LEVEL_DEBUG, "%s / %s: kill task %u",
            sGuildMgr->GetGuildById(guildId)->GetName().c_str(), player->GetName().c_str(),
            creatureId);

    SetTaskValue(owner, guildId, "killTask", creatureId, sPlayerbotAIConfig.maxGuildTaskChangeTime);
    return true;
}

bool GuildTaskMgr::SendAdvertisement(uint32 owner, uint32 guildId)
{
    Guild *guild = sGuildMgr->GetGuildById(guildId);
    if (!guild)
        return false;

    Player* player = ObjectAccessor::FindPlayer(ObjectGuid(HighGuid::Player, (owner)));
    if (!player)
        return false;

    Player* leader = ObjectAccessor::FindPlayer(guild->GetLeaderGUID());
    if (!leader)
        return false;

    uint32 itemTask = GetTaskValue(owner, guildId, "itemTask");
    if (itemTask)
        return SendItemAdvertisement(itemTask, owner, guildId);

    uint32 killTask = GetTaskValue(owner, guildId, "killTask");
    if (killTask)
        return SendKillAdvertisement(killTask, owner, guildId);

    return false;
}

bool GuildTaskMgr::SendItemAdvertisement(uint32 itemId, uint32 owner, uint32 guildId)
{
    Guild *guild = sGuildMgr->GetGuildById(guildId);
    Player* player = ObjectAccessor::FindPlayer(ObjectGuid(HighGuid::Player, (owner)));
    Player* leader = ObjectAccessor::FindPlayer(guild->GetLeaderGUID());

    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
    if (!proto)
        return false;

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    std::ostringstream body;
    body << "Hello, " << player->GetName() << ",\n";
    body << "\n";
    body << "We are in a great need of " << proto->Name1 << ". If you could sell us ";
    uint32 count = GetTaskValue(owner, guildId, "itemCount");
    if (count > 1)
        body << "at least " << count << " of them ";
    else
        body << "some ";
    body << "we'd really appreciate that and pay a high price.\n";
    body << "\n";
    body << "Best Regards,\n";
    body << guild->GetName() << "\n";
    body << leader->GetName() << "\n";
    MailDraft("Guild Task Advertisement", body.str()).SendMailTo(trans, MailReceiver(player), MailSender(leader));

    CharacterDatabase.CommitTransaction(trans);

    return true;
}


bool GuildTaskMgr::SendKillAdvertisement(uint32 creatureId, uint32 owner, uint32 guildId)
{
    Guild *guild = sGuildMgr->GetGuildById(guildId);
    Player* player = ObjectAccessor::FindPlayer(ObjectGuid(HighGuid::Player, owner));
    Player* leader = ObjectAccessor::FindPlayer(guild->GetLeaderGUID());

    CreatureTemplate const* proto = sObjectMgr->GetCreatureTemplate(creatureId);
    if (!proto)
        return false;

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    std::ostringstream body;
    body << "Hello, " << player->GetName() << ",\n";
    body << "\n";
    body << "As you probably know " << proto->Name << " is wanted dead for the crimes it did against our guild. If you should kill it ";
    body << "we'd really appreciate that.\n";
    body << "\n";
    body << "Best Regards,\n";
    body << guild->GetName() << "\n";
    body << leader->GetName() << "\n";
    MailDraft("Guild Task Advertisement", body.str()).SendMailTo(trans, MailReceiver(player), MailSender(leader));
    CharacterDatabase.CommitTransaction(trans);

    return true;
}

bool GuildTaskMgr::SendThanks(uint32 owner, uint32 guildId)
{
    Guild *guild = sGuildMgr->GetGuildById(guildId);
    if (!guild)
        return false;

    Player* player = ObjectAccessor::FindPlayer(ObjectGuid(HighGuid::Player, owner));
    if (!player)
        return false;

    Player* leader = ObjectAccessor::FindPlayer(guild->GetLeaderGUID());
    if (!leader)
        return false;

    uint32 itemTask = GetTaskValue(owner, guildId, "itemTask");
    if (itemTask)
    {
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemTask);
        if (!proto)
            return false;

        SQLTransaction trans = CharacterDatabase.BeginTransaction();
        std::ostringstream body;
        body << "Hello, " << player->GetName() << ",\n";
        body << "\n";
        body << "One of our guild members wishes to thank you for the " << proto->Name1 << "! If we have another ";
        uint32 count = GetTaskValue(owner, guildId, "itemCount");
        body << count << " of them that would help us tremendously.\n";
        body << "\n";
        body << "Thanks again,\n";
        body << guild->GetName() << "\n";
        body << leader->GetName() << "\n";

            MailDraft("Thank You", body.str()).
                AddMoney(GetTaskValue(owner, guildId, "payment")).
                SendMailTo(trans, MailReceiver(player), MailSender(leader));

        CharacterDatabase.CommitTransaction(trans);
        return true;
    }

    return false;
}

uint32 GuildTaskMgr::GetMaxItemTaskCount(uint32 itemId)
{
    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
    if (!proto)
        return 0;

    if (proto->Quality < ITEM_QUALITY_RARE && proto->Stackable && proto->GetMaxStackSize() > 1)
        return proto->GetMaxStackSize();

    return 1;
}

bool GuildTaskMgr::IsGuildTaskItem(uint32 itemId, uint32 guildId)
{
    uint32 value = 0;

    QueryResult results = CharacterDatabase.PQuery(
            "select `value`, `time`, validIn from ai_playerbot_guild_tasks where `value` = '%u' and guildid = '%u' and `type` = 'itemTask'",
            itemId, guildId);

    if (results)
    {
        Field* fields = results->Fetch();
        value = fields[0].GetUInt32();
        uint32 lastChangeTime = fields[1].GetUInt32();
        uint32 validIn = fields[2].GetUInt32();
        if ((time(0) - lastChangeTime) >= validIn)
            value = 0;
    }

    return value;
}

map<uint32,uint32> GuildTaskMgr::GetTaskValues(uint32 owner, std::string type, uint32 *validIn /* = NULL */)
{
    map<uint32,uint32> result;

    QueryResult results = CharacterDatabase.PQuery(
            "select `value`, `time`, validIn, guildid from ai_playerbot_guild_tasks where owner = '%u' and `type` = '%s'",
            owner, type.c_str());

    if (!results)
        return result;

    do
    {
        Field* fields = results->Fetch();
        uint32 value = fields[0].GetUInt32();
        uint32 lastChangeTime = fields[1].GetUInt32();
        uint32 secs = fields[2].GetUInt32();
        uint32 guildId = fields[3].GetUInt32();
        if ((time(0) - lastChangeTime) >= secs)
            value = 0;

        result[guildId] = value;

    } while (results->NextRow());

    return result;
}

uint32 GuildTaskMgr::GetTaskValue(uint32 owner, uint32 guildId, std::string type, uint32 *validIn /* = NULL */)
{
    uint32 value = 0;

    QueryResult results = CharacterDatabase.PQuery(
            "select `value`, `time`, validIn from ai_playerbot_guild_tasks where owner = '%u' and guildid = '%u' and `type` = '%s'",
            owner, guildId, type.c_str());

    if (results)
    {
        Field* fields = results->Fetch();
        value = fields[0].GetUInt32();
        uint32 lastChangeTime = fields[1].GetUInt32();
        uint32 secs = fields[2].GetUInt32();
        if ((time(0) - lastChangeTime) >= secs)
            value = 0;

        if (validIn) *validIn = secs;
    }

    return value;
}

uint32 GuildTaskMgr::SetTaskValue(uint32 owner, uint32 guildId, std::string type, uint32 value, uint32 validIn)
{
    CharacterDatabase.PExecute("delete from ai_playerbot_guild_tasks where owner = '%u' and guildid = '%u' and `type` = '%s'",
            owner, guildId, type.c_str());
    if (value)
    {
        CharacterDatabase.PExecute(
                "insert into ai_playerbot_guild_tasks (owner, guildid, `time`, validIn, `type`, `value`) values ('%u', '%u', '%u', '%u', '%s', '%u')",
                owner, guildId, (uint32)time(0), validIn, type.c_str(), value);
    }

    return value;
}

bool GuildTaskMgr::HandleConsoleCommand(ChatHandler* handler, char const* args)
{
    if (!sPlayerbotAIConfig.guildTaskEnabled)
    {
        sLog->outMessage("gtask", LOG_LEVEL_ERROR, "Guild task system is currently disabled!");
        return false;
    }

    if (!args || !*args)
    {
        sLog->outMessage("gtask", LOG_LEVEL_ERROR, "Usage: gtask stats/reset");
        return false;
    }

    std::string cmd = args;

    if (cmd == "reset")
    {
        CharacterDatabase.PExecute("delete from ai_playerbot_guild_tasks");
        sLog->outMessage("gtask", LOG_LEVEL_INFO, "Guild tasks were reset for all players");
        return true;
    }

    if (cmd == "stats")
    {
        sLog->outMessage("gtask", LOG_LEVEL_INFO, "Usage: gtask stats <player name>");
        return true;
    }

    if (cmd.find("stats ") != std::string::npos)
    {
        std::string charName = cmd.substr(cmd.find("stats ") + 6);
        ObjectGuid guid = sCharacterCache->GetCharacterGuidByName(charName);
        if (!guid)
        {
            sLog->outMessage("gtask", LOG_LEVEL_ERROR, "Player %s not found", charName.c_str());
            return false;
        }

        uint32 owner = guid.GetCounter();

        QueryResult result = CharacterDatabase.PQuery(
                "select `value`, `time`, validIn, guildid, `type` from ai_playerbot_guild_tasks where owner = '%u' order by guildid, `type`",
                owner);

        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                uint32 value = fields[0].GetUInt32();
                uint32 lastChangeTime = fields[1].GetUInt32();
                uint32 validIn = fields[2].GetUInt32();
                if ((time(0) - lastChangeTime) >= validIn)
                    value = 0;
                uint32 guildId = fields[3].GetUInt32();
                std::string type = fields[4].GetString();

                Guild *guild = sGuildMgr->GetGuildById(guildId);
                if (!guild)
                    continue;

                ostringstream name;
                name << value;
                if (type == "killTask")
                {
                    if (CreatureTemplate const* proto = sObjectMgr->GetCreatureTemplate(value))
                    {
                        string rank = proto->rank == CREATURE_ELITE_RARE ? "rare" : "elite";
                        name << " (" << proto->Name << "," << rank << ")";
                    }
                }
                else if (type == "itemTask")
                {
                    if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(value))
                    {
                        string rank = proto->Quality == ITEM_QUALITY_UNCOMMON ? "uncommon" : "rare";
                        name << " (" << proto->Name1 << "," << rank << ")";
                    }
                }

                sLog->outMessage("gtask", LOG_LEVEL_INFO, "Player '%s' Guild '%s' %s=%u (%u secs)",
                        charName.c_str(), guild->GetName().c_str(),
                        type.c_str(), name.str().c_str(), validIn);

            } while (result->NextRow());
        }

        return true;
    }

    if (cmd == "reward")
    {
        sLog->outMessage("gtask", LOG_LEVEL_INFO, "Usage: gtask reward <player name>");
        return true;
    }

    if (cmd.find("reward ") != std::string::npos)
    {
        std::string charName = cmd.substr(cmd.find("reward ") + 7);
        ObjectGuid guid = sCharacterCache->GetCharacterGuidByName(charName);
        if (!guid)
        {
            sLog->outMessage("gtask", LOG_LEVEL_ERROR, "Player %s not found", charName.c_str());
            return false;
        }

        uint32 owner = guid.GetCounter();
        QueryResult result = CharacterDatabase.PQuery(
                "select distinct guildid from ai_playerbot_guild_tasks where owner = '%u'",
                owner);

        if (result)
        {
            do
            {
                Field* fields = result->Fetch();
                uint32 guildId = fields[0].GetUInt32();
                Guild *guild = sGuildMgr->GetGuildById(guildId);
                if (!guild)
                    continue;

                sGuildTaskMgr.Reward(owner, guildId);
            } while (result->NextRow());

            return true;
        }
    }

    return false;
}

void GuildTaskMgr::CheckItemTask(uint32 itemId, uint32 obtained, Player* ownerPlayer, Player* bot, bool byMail)
{
    uint32 guildId = bot->GetGuildId();
    if (!guildId)
        return;

    uint32 owner = ownerPlayer->GetGUID().GetCounter();

    sLog->outMessage("gtask", LOG_LEVEL_DEBUG, "%s / %s: checking guild task",
            bot->GetGuild()->GetName().c_str(), ownerPlayer->GetName().c_str());

    uint32 itemTask = GetTaskValue(owner, guildId, "itemTask");
    if (itemTask != itemId)
    {
        sLog->outMessage("gtask", LOG_LEVEL_DEBUG, "%s / %s: item %u is not guild task item (%u)",
                bot->GetGuild()->GetName().c_str(), ownerPlayer->GetName().c_str(),
                itemId, itemTask);
        return;
    }

    if (byMail)
    {
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
        if (!proto)
            return;

        uint32 money = GetTaskValue(owner, guildId, "payment");
        SetTaskValue(owner, guildId, "payment", money + proto->BuyPrice /* TODO PLAYERBOT auctionbot.GetBuyPrice(proto) */  * obtained,
                sPlayerbotAIConfig.maxGuildTaskRewardTime);
    }

    uint32 count = GetTaskValue(owner, guildId, "itemCount");
    if (obtained >= count)
    {
        sLog->outMessage("gtask", LOG_LEVEL_DEBUG, "%s / %s: guild task complete",
                bot->GetGuild()->GetName().c_str(), ownerPlayer->GetName().c_str());
        SetTaskValue(owner, guildId, "reward", 1,
                urand(sPlayerbotAIConfig.minGuildTaskRewardTime, sPlayerbotAIConfig.maxGuildTaskRewardTime));
    }
    else
    {
        sLog->outMessage("gtask", LOG_LEVEL_DEBUG, "%s / %s: guild task progress",
                bot->GetGuild()->GetName().c_str(), ownerPlayer->GetName().c_str());
        SetTaskValue(owner, guildId, "itemCount", count - obtained, sPlayerbotAIConfig.maxGuildTaskChangeTime);
        SetTaskValue(owner, guildId, "thanks", 1,
                urand(sPlayerbotAIConfig.minGuildTaskRewardTime, sPlayerbotAIConfig.maxGuildTaskRewardTime));
    }
}

bool GuildTaskMgr::Reward(uint32 owner, uint32 guildId)
{
    Guild *guild = sGuildMgr->GetGuildById(guildId);
    if (!guild)
        return false;

    Player* player = ObjectAccessor::FindPlayer(ObjectGuid(HighGuid::Player, owner));
    if (!player)
        return false;

    Player* leader = ObjectAccessor::FindPlayer(guild->GetLeaderGUID());
    if (!leader)
        return false;

    uint32 itemTask = GetTaskValue(owner, guildId, "itemTask");
    uint32 killTask = GetTaskValue(owner, guildId, "killTask");
    if (!itemTask && !killTask)
        return false;

    std::ostringstream body;
    body << "Hello, " << player->GetName() << ",\n";
    body << "\n";

    if (itemTask)
    {
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemTask);
        if (!proto)
            return false;

        body << "We wish to thank you for the " << proto->Name1 << " you provided so kindly. We really appreciate this and may this small gift bring you our thanks!\n";
        body << "\n";
        body << "Many thanks,\n";
        body << guild->GetName() << "\n";
        body << leader->GetName() << "\n";
    }
    else if (killTask)
    {
        CreatureTemplate const* proto = sObjectMgr->GetCreatureTemplate(killTask);
        if (!proto)
            return false;

        body << "We wish to thank you for the " << proto->Name << " you've killed recently. We really appreciate this and may this small gift bring you our thanks!\n";
        body << "\n";
        body << "Many thanks,\n";
        body << guild->GetName() << "\n";
        body << leader->GetName() << "\n";
    }

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    MailDraft draft("Thank You", body.str());

    uint32 count = urand(1, 3);
    for (uint32 i = 0; i < count; ++i)
    {
        uint32 itemId = sRandomItemMgr.GetRandomItem(RANDOM_ITEM_GUILD_TASK_REWARD);
        if (itemId)
        {
            Item* item = Item::CreateItem(itemId, 1, leader);
            item->SaveToDB(trans);
            draft.AddItem(item);
        }
    }

    draft.AddMoney(GetTaskValue(owner, guildId, "payment")).SendMailTo(trans, MailReceiver(player), MailSender(leader));
    CharacterDatabase.CommitTransaction(trans);

    SetTaskValue(owner, guildId, "activeTask", 0, 0);
    return true;
}

void GuildTaskMgr::CheckKillTask(Player* player, Unit* victim)
{
    uint32 owner = player->GetGUID().GetCounter();
    Creature* creature = victim->ToCreature();
    if (!creature)
        return;

    map<uint32,uint32> tasks = GetTaskValues(owner, "killTask");
    for (map<uint32,uint32>::iterator i = tasks.begin(); i != tasks.end(); ++i)
    {
        uint32 guildId = i->first;
        uint32 value = i->second;
        Guild* guild = sGuildMgr->GetGuildById(guildId);

        if (value != creature->GetCreatureTemplate()->Entry)
            continue;

        sLog->outMessage("gtask", LOG_LEVEL_DEBUG, "%s / %s: guild task complete",
                guild->GetName().c_str(), player->GetName().c_str());
        SetTaskValue(owner, guildId, "reward", 1,
                urand(sPlayerbotAIConfig.minGuildTaskRewardTime, sPlayerbotAIConfig.maxGuildTaskRewardTime));
    }
}