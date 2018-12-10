
#include "playerbot.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotFactory.h"
#include "DatabaseEnv.h"
#include "PlayerbotAI.h"
#include "AiFactory.h"
#include "MapManager.h"
#include "GuildTaskMgr.h"
#include "CharacterCache.h"
#include "RandomPlayerbotFactory.h"
#ifdef TESTS
#include "TestPlayer.h"
#endif

RandomPlayerbotMgr::RandomPlayerbotMgr() : PlayerbotHolder(), processTicks(0)
{
}

RandomPlayerbotMgr::~RandomPlayerbotMgr()
{
}

void RandomPlayerbotMgr::UpdateAIInternal(uint32 elapsed)
{
    SetNextCheckDelay(sPlayerbotAIConfig.randomBotUpdateInterval * 1000);

    if (!sPlayerbotAIConfig.randomBotAutologin || !sPlayerbotAIConfig.enabled)
        return;

    sLog->outMessage("playerbot", LOG_LEVEL_INFO, "Processing random bots...");

    int maxAllowedBotCount = GetEventValue(0, "bot_count");
    if (!maxAllowedBotCount)
    {
        maxAllowedBotCount = urand(sPlayerbotAIConfig.minRandomBots, sPlayerbotAIConfig.maxRandomBots);
        SetEventValue(0, "bot_count", maxAllowedBotCount,
                urand(sPlayerbotAIConfig.randomBotCountChangeMinInterval, sPlayerbotAIConfig.randomBotCountChangeMaxInterval));
    }

    list<uint32> bots = GetBots();
    int botCount = bots.size();
    int randomBotsPerInterval = (int)urand(sPlayerbotAIConfig.minRandomBotsPerInterval, sPlayerbotAIConfig.maxRandomBotsPerInterval);
    if (!processTicks)
    {
        if (sPlayerbotAIConfig.randomBotLoginAtStartup)
            randomBotsPerInterval = bots.size();
    }

    while (botCount++ < maxAllowedBotCount)
    {
        bool alliance = botCount % 2;
        uint32 bot = AddRandomBot(alliance);
        if (bot) 
            bots.push_back(bot);
        else 
            break;
    }

    int botProcessed = 0;
    for (list<uint32>::iterator i = bots.begin(); i != bots.end(); ++i)
    {
        uint32 bot = *i;
        if (ProcessBot(bot))
            botProcessed++;

        if (botProcessed >= randomBotsPerInterval)
            break;
    }

    sLog->outMessage("playerbot", LOG_LEVEL_INFO, "%d bots processed. Next check in %d seconds",
        botProcessed, sPlayerbotAIConfig.randomBotUpdateInterval);

    if (processTicks++ == 1)
        PrintStats();
}

uint32 RandomPlayerbotMgr::AddRandomBot(bool alliance)
{
    vector<uint32> bots = GetFreeBots(alliance);
    if (bots.size() == 0)
    {
        sLog->outMessage("playerbot", LOG_LEVEL_ERROR, "Failed to add random bot, no free bot found");
        return 0;
    }

    int index = urand(0, bots.size() - 1);
    uint32 bot = bots[index];
    SetEventValue(bot, "add", 1, urand(sPlayerbotAIConfig.minRandomBotInWorldTime, sPlayerbotAIConfig.maxRandomBotInWorldTime));
    uint32 randomTime = 30 + urand(sPlayerbotAIConfig.randomBotUpdateInterval, sPlayerbotAIConfig.randomBotUpdateInterval * 3);
    ScheduleRandomize(bot, randomTime);
    sLog->outMessage("playerbot", LOG_LEVEL_DEBUG, "Random bot %d added", bot);
    return bot;
}

void RandomPlayerbotMgr::ScheduleRandomize(uint32 bot, uint32 time)
{
    SetEventValue(bot, "randomize", 1, time);
    SetEventValue(bot, "logout", 1, time + 30 + urand(sPlayerbotAIConfig.randomBotUpdateInterval, sPlayerbotAIConfig.randomBotUpdateInterval * 3));
}

void RandomPlayerbotMgr::ScheduleTeleport(uint32 bot)
{
    SetEventValue(bot, "teleport", 1, 60 + urand(sPlayerbotAIConfig.randomBotUpdateInterval, sPlayerbotAIConfig.randomBotUpdateInterval * 3));
}

bool RandomPlayerbotMgr::ProcessBot(uint32 bot)
{
    uint32 isValid = GetEventValue(bot, "add");
    ObjectGuid guid = ObjectGuid(HighGuid::Player, bot);
    if (!isValid)
    {
        Player* player = GetPlayerBot(guid);
        if (!player || !player->GetGroup())
        {
            sLog->outMessage("playerbot", LOG_LEVEL_INFO, "Bot %d expired", bot);
            SetEventValue(bot, "add", 0, 0);
        }
        return true;
    }

    if (!GetPlayerBot(guid))
    {
        if(AddPlayerBot(guid, 0) != nullptr)
            sLog->outMessage("playerbot", LOG_LEVEL_INFO, "Bot %d logged in", bot);
        else
            sLog->outMessage("playerbot", LOG_LEVEL_INFO, "Bot %d tried to log in but failed", bot);

        if (!GetEventValue(bot, "online"))
        {
            SetEventValue(bot, "online", 1, sPlayerbotAIConfig.minRandomBotInWorldTime);
        }
        return true;
    }

    Player* player = GetPlayerBot(guid);
    if (!player)
        return false;

    PlayerbotAI* ai = player->GetPlayerbotAI();
    if (!ai)
        return false;

    if (player->GetGroup())
    {
        sLog->outMessage("playerbot", LOG_LEVEL_INFO, "Skipping bot %d as it is in group", bot);
        return false;
    }

    if (player->IsDead())
    {
        if (!GetEventValue(bot, "dead"))
        {
            sLog->outMessage("playerbot", LOG_LEVEL_INFO, "Setting dead flag for bot %d", bot);
            uint32 randomTime = urand(sPlayerbotAIConfig.minRandomBotReviveTime, sPlayerbotAIConfig.maxRandomBotReviveTime);
            SetEventValue(bot, "dead", 1, randomTime);
            SetEventValue(bot, "revive", 1, randomTime - 60);
            return false;
        }

        if (!GetEventValue(bot, "revive"))
        {
            sLog->outMessage("playerbot", LOG_LEVEL_INFO, "Reviving dead bot %d", bot);
            SetEventValue(bot, "dead", 0, 0);
            SetEventValue(bot, "revive", 0, 0);
            RandomTeleport(player, player->GetMapId(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ());
            return true;
        }

        return false;
    }

    if (player->GetGuild() && player->GetGuild()->GetLeaderGUID() == player->GetGUID())
    {
        for (vector<Player*>::iterator i = players.begin(); i != players.end(); ++i)
            sGuildTaskMgr.Update(*i, player);
    }

    uint32 randomize = GetEventValue(bot, "randomize");
    if (!randomize)
    {
        sLog->outMessage("playerbot", LOG_LEVEL_INFO, "Randomizing bot %d", bot);
        Randomize(player);
        uint32 randomTime = urand(sPlayerbotAIConfig.minRandomBotRandomizeTime, sPlayerbotAIConfig.maxRandomBotRandomizeTime);
        ScheduleRandomize(bot, randomTime);
        return true;
    }

    uint32 logout = GetEventValue(bot, "logout");
    if (!logout)
    {
        sLog->outMessage("playerbot", LOG_LEVEL_INFO, "Logging out bot %d", bot);
        LogoutPlayerBot(guid);
        SetEventValue(bot, "logout", 1, sPlayerbotAIConfig.maxRandomBotInWorldTime);
        return true;
    }

    uint32 teleport = GetEventValue(bot, "teleport");
    if (!teleport)
    {
        sLog->outMessage("playerbot", LOG_LEVEL_INFO, "Random teleporting bot %d", bot);
        RandomTeleportForLevel(ai->GetBot());
        SetEventValue(bot, "teleport", 1, sPlayerbotAIConfig.maxRandomBotInWorldTime);
        return true;
    }

    return false;
}

void RandomPlayerbotMgr::RandomTeleport(Player* bot, vector<WorldLocation> &locs)
{
    if (bot->IsBeingTeleported())
        return;

    if (locs.empty())
    {
        sLog->outMessage("playerbot", LOG_LEVEL_ERROR, "Cannot teleport bot %s - no locations available", bot->GetName().c_str());
        return;
    }

    for (int attemtps = 0; attemtps < 10; ++attemtps)
    {
        int index = urand(0, locs.size() - 1);
        WorldLocation loc = locs[index];
        float x = loc.m_positionX + urand(0, sPlayerbotAIConfig.grindDistance) - sPlayerbotAIConfig.grindDistance / 2;
        float y = loc.m_positionY + urand(0, sPlayerbotAIConfig.grindDistance) - sPlayerbotAIConfig.grindDistance / 2;
        float z = loc.m_positionZ;

        Map* map = sMapMgr->FindMap(loc.GetMapId(), 0);
        if (!map)
            continue;

        PositionFullTerrainStatus data;
        map->GetFullTerrainStatusForPosition(x, y, z, data, MAP_ALL_LIQUIDS);
        
        if (!data.outdoors ||
            data.liquidStatus != LIQUID_MAP_NO_WATER)
            continue;

        uint32 areaId = map->GetAreaId(x, y, z);
        if (!areaId)
            continue;

        AreaTableEntry const* area = sAreaTableStore.LookupEntry(areaId);
        if (!area)
            continue;

        float ground = map->GetHeight(x, y, z + 0.5f);
        if (ground <= INVALID_HEIGHT)
            continue;

        z = 0.05f + ground;
        sLog->outMessage("playerbot", LOG_LEVEL_INFO, "Random teleporting bot %s to %s %f,%f,%f", bot->GetName().c_str(), area->area_name[0], x, y, z);

        bot->GetMotionMaster()->Clear();
        bot->TeleportTo(loc.GetMapId(), x, y, z, 0);
        return;
    }

    sLog->outMessage("playerbot", LOG_LEVEL_ERROR, "Cannot teleport bot %s - no locations available", bot->GetName().c_str());
}

void RandomPlayerbotMgr::RandomTeleportForLevel(Player* bot)
{
    vector<WorldLocation> locs;
    // Select positions from creatures in bot level range AND in specified maps (sPlayerbotAIConfig.randomBotMapsAsString) AND 
    QueryResult results = WorldDatabase.PQuery(
        "SELECT map, position_x, position_y, position_z "
        "FROM "
        "("
            //                                                                                 bot->GetLevel()
            "SELECT map, position_x, position_y, position_z, avg(t.maxlevel), avg(t.minlevel), (%u - (avg(t.maxlevel) + avg(t.minlevel)) / 2) AS delta "
            "FROM creature c "
            "JOIN creature_entry ce ON ce.spawnID = c.spawnID "
            "INNER JOIN creature_template t ON ce.entry = t.entry "
            "GROUP BY t.entry "
        ") q "
        "WHERE delta >= 0 "
        "AND delta <= %u " //randomBotTeleLevel
        "AND map IN (%s) " //randomBotMapsAsString
        "AND NOT EXISTS "
        "( "
            "SELECT map, position_x, position_y, position_z "
            "FROM "
            "("
                //                                                                               bot->GetLevel()
               "SELECT map, c.position_x, c.position_y, c.position_z, avg(t.maxlevel), avg(t.minlevel), (%u - (avg(t.maxlevel) + avg(t.minlevel)) / 2) AS delta "
                "FROM creature c "
                "JOIN creature_entry ce ON ce.spawnID = c.spawnID "
                "INNER JOIN creature_template t on ce.entry = t.entry "
                "GROUP BY t.entry "
            ") q1 "
            //     randomBotTeleLevel
            "WHERE delta > %u and q1.map = q.map "
            "AND sqrt("
                "(q1.position_x - q.position_x)*(q1.position_x - q.position_x) +"
                "(q1.position_y - q.position_y)*(q1.position_y - q.position_y) +"
                "(q1.position_z - q.position_z)*(q1.position_z - q.position_z)"
                // sightDistance
                ") < %f"
        ")",
        bot->GetLevel(),
        sPlayerbotAIConfig.randomBotTeleLevel,
        sPlayerbotAIConfig.randomBotMapsAsString.c_str(),
        bot->GetLevel(),
        sPlayerbotAIConfig.randomBotTeleLevel,
        sPlayerbotAIConfig.sightDistance
        );
    if (results)
    {
        do
        {
            Field* fields = results->Fetch();
            uint16 mapId = fields[0].GetUInt16();
            float x = fields[1].GetFloat();
            float y = fields[2].GetFloat();
            float z = fields[3].GetFloat();
            WorldLocation loc(mapId, x, y, z, 0);
            locs.push_back(loc);
        } while (results->NextRow());
    }

    RandomTeleport(bot, locs);
}

void RandomPlayerbotMgr::RandomTeleport(Player* bot, uint16 mapId, float teleX, float teleY, float teleZ)
{
    vector<WorldLocation> locs;
    QueryResult results = WorldDatabase.PQuery("select position_x, position_y, position_z from creature where map = '%u' and abs(position_x - '%f') < '%u' and abs(position_y - '%f') < '%u'",
            mapId, teleX, sPlayerbotAIConfig.randomBotTeleportDistance / 2, teleY, sPlayerbotAIConfig.randomBotTeleportDistance / 2);
    if (results)
    {
        do
        {
            Field* fields = results->Fetch();
            float x = fields[0].GetFloat();
            float y = fields[1].GetFloat();
            float z = fields[2].GetFloat();
            WorldLocation loc(mapId, x, y, z, 0);
            locs.push_back(loc);
        } while (results->NextRow());
    }

    RandomTeleport(bot, locs);
    Refresh(bot);
}

void RandomPlayerbotMgr::Randomize(Player* bot)
{
    if (bot->GetLevel() == 1)
        RandomizeFirst(bot);
    else
        IncreaseLevel(bot);
}

void RandomPlayerbotMgr::IncreaseLevel(Player* bot)
{
    uint32 maxLevel = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);
    uint32 level = min((uint32)(bot->GetLevel() + 1), maxLevel);
    PlayerbotFactory factory(bot, level);
    if (bot->GetGuildId())
        factory.Refresh();
    else
        factory.Randomize();
    RandomTeleportForLevel(bot);
}

void RandomPlayerbotMgr::RandomizeFirst(Player* bot)
{
    uint32 maxLevel = sPlayerbotAIConfig.randomBotMaxLevel;
    if (maxLevel > sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
        maxLevel = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);

    for (int attempt = 0; attempt < 100; ++attempt)
    {
        int index = urand(0, sPlayerbotAIConfig.randomBotMaps.size() - 1);
        uint16 mapId = sPlayerbotAIConfig.randomBotMaps[index];

        vector<GameTele const*> locs;
        GameTeleContainer const & teleMap = sObjectMgr->GetGameTeleMap();
        for(GameTeleContainer::const_iterator itr = teleMap.begin(); itr != teleMap.end(); ++itr)
        {
            GameTele const* tele = &itr->second;
            if (tele->mapId == mapId)
                locs.push_back(tele);
        }

        index = urand(0, locs.size() - 1);
        GameTele const* tele = locs[index];
        uint32 level = GetZoneLevel(tele->mapId, tele->position_x, tele->position_y, tele->position_z);
        if (level > maxLevel + 5)
            continue;

        level = min(level, maxLevel);
        if (!level) level = 1;

        if (urand(0, 100) < 100 * sPlayerbotAIConfig.randomBotMaxLevelChance)
            level = maxLevel;

        if (level < sPlayerbotAIConfig.randomBotMinLevel)
            continue;

        PlayerbotFactory factory(bot, level);
        factory.CleanRandomize();
        RandomTeleport(bot, tele->mapId, tele->position_x, tele->position_y, tele->position_z);
        break;
    }
}

uint32 RandomPlayerbotMgr::GetZoneLevel(uint16 mapId, float teleX, float teleY, float teleZ)
{
    uint32 maxLevel = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);

    uint32 level;
    QueryResult results = WorldDatabase.PQuery("select avg(t.minlevel) minlevel, avg(t.maxlevel) maxlevel from creature c "
            "inner join creature_template t on c.id = t.entry "
            "where map = '%u' and minlevel > 1 and abs(position_x - '%f') < '%u' and abs(position_y - '%f') < '%u'",
            mapId, teleX, sPlayerbotAIConfig.randomBotTeleportDistance / 2, teleY, sPlayerbotAIConfig.randomBotTeleportDistance / 2);

    if (results)
    {
        Field* fields = results->Fetch();
        uint8 zoneMinLevel = uint8(fields[0].GetDouble());
        uint8 zoneMaxLevel = uint8(fields[1].GetDouble());
        level = urand(zoneMinLevel, zoneMaxLevel);
        if (level > zoneMaxLevel)
            level = zoneMaxLevel;
    }
    else
    {
        level = urand(1, maxLevel);
    }

    return level;
}

void RandomPlayerbotMgr::Refresh(Player* bot)
{
    if (bot->IsDead())
    {
        bot->ResurrectPlayer(1.0f);
        bot->SpawnCorpseBones();
        bot->SaveToDB();
        bot->GetPlayerbotAI()->ResetStrategies();
    }

    bot->GetPlayerbotAI()->Reset();

    bot->GetCombatManager().EndAllCombat();

    bot->RemoveAllAttackers();
    bot->ClearInCombat();

    bot->DurabilityRepairAll(false, 1.0f, false);
    bot->SetFullHealth();
    bot->SetPvP(true);

    if (bot->GetMaxPower(POWER_MANA) > 0)
        bot->SetPower(POWER_MANA, bot->GetMaxPower(POWER_MANA));

    if (bot->GetMaxPower(POWER_ENERGY) > 0)
        bot->SetPower(POWER_ENERGY, bot->GetMaxPower(POWER_ENERGY));
}


bool RandomPlayerbotMgr::IsRandomBot(Player* bot)
{
    return IsRandomBot(bot->GetGUID());
}

bool RandomPlayerbotMgr::IsRandomBot(uint32 bot)
{
    return GetEventValue(bot, "add");
}

std::list<uint32> RandomPlayerbotMgr::GetBots()
{
    list<uint32> bots;

    QueryResult results = CharacterDatabase.Query(
            "select bot from ai_playerbot_random_bots where owner = 0 and event = 'add'");

    if (results)
    {
        do
        {
            Field* fields = results->Fetch();
            uint32 bot = uint32(fields[0].GetUInt64());
            bots.push_back(bot);
        } while (results->NextRow());
    }

    //add data to player global data if not existing yet
    for (auto itr : bots)
    {
        auto data = sCharacterCache->GetCharacterCacheByGuid(itr);
        if (!data)
        {
            QueryResult results2 = CharacterDatabase.PQuery("select account, name, gender, race, class, level FROM characters where guid = %u", itr);
            if (results2)
            {
                Field* fields = results2->Fetch();
                uint32 account = fields[0].GetUInt32();
                std::string name = fields[1].GetString();
                uint8 gender = fields[2].GetUInt8();
                uint8 race = fields[3].GetUInt8();
                uint8 _class = fields[4].GetUInt8();
                uint8 level = fields[5].GetUInt8();
                sCharacterCache->AddCharacterCacheEntry(itr, account, name, gender, race, _class, level, 0);
            }
        }
    }

    return bots;
}

vector<uint32> RandomPlayerbotMgr::GetFreeBots(bool alliance)
{
    set<uint32> bots;

    QueryResult results = CharacterDatabase.PQuery(
            "select `bot` from ai_playerbot_random_bots where event = 'add'");

    if (results)
    {
        do
        {
            Field* fields = results->Fetch();
            uint32 bot = uint64(fields[0].GetUInt64());
            bots.insert(bot);
        } while (results->NextRow());
    }

    vector<uint32> guids;
    for (list<uint32>::iterator i = sPlayerbotAIConfig.randomBotAccounts.begin(); i != sPlayerbotAIConfig.randomBotAccounts.end(); i++)
    {
        uint32 accountId = *i;
        if (!sAccountMgr->GetCharactersCount(accountId))
            continue;

        QueryResult result = CharacterDatabase.PQuery("SELECT guid, race FROM characters WHERE account = '%u'", accountId);
        if (!result)
            continue;

        do
        {
            Field* fields = result->Fetch();
            uint32 guid = fields[0].GetUInt32();
            uint8 race = fields[1].GetUInt8();
            if (bots.find(guid) == bots.end() &&
                    (alliance == IsAlliance(race))
               )
                guids.push_back(guid);
        } while (result->NextRow());
    }


    return guids;
}

uint32 RandomPlayerbotMgr::GetEventValue(uint32 bot, std::string event)
{
    uint64 value = 0;

    QueryResult results = CharacterDatabase.PQuery(
            "select `value`, `time`, validIn from ai_playerbot_random_bots where owner = 0 and bot = '%u' and event = '%s'",
            bot, event.c_str());

    if (results)
    {
        Field* fields = results->Fetch();
        value = fields[0].GetUInt64();
        uint64 lastChangeTime = fields[1].GetUInt64();
        uint64 validIn = fields[2].GetUInt64();
        if ((time(0) - lastChangeTime) >= validIn)
            value = 0;
    }

    return value;
}

uint32 RandomPlayerbotMgr::SetEventValue(uint32 bot, std::string event, uint32 value, uint32 validIn)
{
    CharacterDatabase.PExecute("delete from ai_playerbot_random_bots where owner = 0 and bot = '%u' and event = '%s'",
            bot, event.c_str());
    if (value)
    {
        CharacterDatabase.PExecute(
                "insert into ai_playerbot_random_bots (owner, bot, `time`, validIn, event, `value`) values ('%u', '%u', '%u', '%u', '%s', '%u')",
                0, bot, (uint32)time(0), validIn, event.c_str(), value);
    }

    return value;
}

bool RandomPlayerbotMgr::HandlePlayerbotConsoleCommand(ChatHandler* handler, char const* args)
{
    if (!sPlayerbotAIConfig.enabled)
    {
        sLog->outMessage("playerbot", LOG_LEVEL_ERROR, "Playerbot system is currently disabled!");
        return false;
    }

    if (!args || !*args)
    {
        sLog->outMessage("playerbot", LOG_LEVEL_ERROR, "Usage: rndbot stats/update/reset/init/refresh/add/remove");
        return false;
    }

    std::string cmd = args;

    if (cmd == "reset")
    {
        CharacterDatabase.PExecute("delete from ai_playerbot_random_bots");
        sLog->outMessage("playerbot", LOG_LEVEL_INFO, "Random bots were reset for all players. Please restart the Server.");
        return true;
    }
    else if (cmd == "stats")
    {
        sRandomPlayerbotMgr.PrintStats();
        return true;
    }
    else if (cmd == "update")
    {
        sRandomPlayerbotMgr.UpdateAIInternal(0);
        return true;
    }
    else if (cmd == "init" || cmd == "refresh" || cmd == "teleport")
    {
        sLog->outMessage("playerbot", LOG_LEVEL_INFO, "Randomizing bots for %d accounts", sPlayerbotAIConfig.randomBotAccounts.size());
        for (list<uint32>::iterator i = sPlayerbotAIConfig.randomBotAccounts.begin(); i != sPlayerbotAIConfig.randomBotAccounts.end(); ++i)
        {
            uint32 account = *i;
            if (QueryResult results = CharacterDatabase.PQuery("SELECT guid FROM characters where account = '%u'", account))
            {
                do
                {
                    Field* fields = results->Fetch();
                    ObjectGuid guid = ObjectGuid(HighGuid::Player, fields[0].GetUInt32());
                    Player* bot = ObjectAccessor::FindPlayer(guid);
                    if (!bot)
                        continue;

                    if (cmd == "init")
                    {
                        sLog->outMessage("playerbot", LOG_LEVEL_INFO, "Randomizing bot %s for account %u", bot->GetName().c_str(), account);
                        sRandomPlayerbotMgr.RandomizeFirst(bot);
                    }
                    else if (cmd == "teleport")
                    {
                        sLog->outMessage("playerbot", LOG_LEVEL_INFO, "Random teleporting bot %s for account %u", bot->GetName().c_str(), account);
                        sRandomPlayerbotMgr.RandomTeleportForLevel(bot);
                    }
                    else
                    {
                        sLog->outMessage("playerbot", LOG_LEVEL_INFO, "Refreshing bot %s for account %u", bot->GetName().c_str(), account);
                        bot->SetLevel(bot->GetLevel() - 1);
                        sRandomPlayerbotMgr.IncreaseLevel(bot);
                    }
                    uint32 randomTime = urand(sPlayerbotAIConfig.minRandomBotRandomizeTime, sPlayerbotAIConfig.maxRandomBotRandomizeTime);
                    CharacterDatabase.PExecute("update ai_playerbot_random_bots set validIn = '%u' where event = 'randomize' and bot = '%u'",
                            randomTime, bot->GetGUID().GetCounter());
                    CharacterDatabase.PExecute("update ai_playerbot_random_bots set validIn = '%u' where event = 'logout' and bot = '%u'",
                            sPlayerbotAIConfig.maxRandomBotInWorldTime, bot->GetGUID().GetCounter());
                } while (results->NextRow());
            }
        }
        return true;
    }
    else
    {
        list<std::string> messages = sRandomPlayerbotMgr.HandlePlayerbotCommand(args, NULL);
        for (list<std::string>::iterator i = messages.begin(); i != messages.end(); ++i)
        {
            sLog->outMessage("playerbot", LOG_LEVEL_INFO, i->c_str());
        }
        return true;
    }

    return false;
}

void RandomPlayerbotMgr::HandleCommand(uint32 type, const std::string& text, Player& fromPlayer)
{
    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        bot->GetPlayerbotAI()->HandleCommand(type, text, fromPlayer);
    }
}

void RandomPlayerbotMgr::OnPlayerLogout(Player* player)
{
    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        PlayerbotAI* ai = bot->GetPlayerbotAI();
        if (player == ai->GetMaster())
        {
            ai->SetMaster(NULL);
            ai->ResetStrategies();
        }
    }

    if (!player->GetPlayerbotAI())
    {
        vector<Player*>::iterator i = find(players.begin(), players.end(), player);
        if (i != players.end())
            players.erase(i);
    }
}

void RandomPlayerbotMgr::OnPlayerLogin(Player* player)
{
    for (PlayerBotMap::const_iterator it = GetPlayerBotsBegin(); it != GetPlayerBotsEnd(); ++it)
    {
        Player* const bot = it->second;
        if (player == bot || player->GetPlayerbotAI())
            continue;

        Group* group = bot->GetGroup();
        if (!group)
            continue;

        for (GroupReference *gref = group->GetFirstMember(); gref; gref = gref->next())
        {
            Player* member = gref->GetSource();
            PlayerbotAI* ai = bot->GetPlayerbotAI();
            if (member == player && (!ai->GetMaster() || ai->GetMaster()->GetPlayerbotAI()))
            {
                ai->SetMaster(player);
                ai->ResetStrategies();
                ai->TellMaster("Hello");
                break;
            }
        }
    }

    if (player->GetPlayerbotAI())
        return;

    players.push_back(player);
}

Player* RandomPlayerbotMgr::GetRandomPlayer()
{
    if (players.empty())
        return NULL;

    uint32 index = urand(0, players.size() - 1);
    return players[index];
}

void RandomPlayerbotMgr::PrintStats()
{
    sLog->outMessage("playerbot", LOG_LEVEL_INFO, "%d Random Bots online", playerBots.size());

    map<uint32, int> alliance, horde;
    for (uint32 i = 0; i < 10; ++i)
    {
        alliance[i] = 0;
        horde[i] = 0;
    }

    map<uint8, int> perRace, perClass;
    for (uint8 race = RACE_HUMAN; race < MAX_RACES; ++race)
    {
        perRace[race] = 0;
    }
    for (uint8 cls = CLASS_WARRIOR; cls < MAX_CLASSES; ++cls)
    {
        perClass[cls] = 0;
    }

    int dps = 0, heal = 0, tank = 0;
    for (PlayerBotMap::iterator i = playerBots.begin(); i != playerBots.end(); ++i)
    {
        Player* bot = i->second;
        if (IsAlliance(bot->GetRace()))
            alliance[bot->GetLevel() / 10]++;
        else
            horde[bot->GetLevel() / 10]++;

        perRace[bot->GetRace()]++;
        perClass[bot->GetClass()]++;

        int spec = AiFactory::GetPlayerSpecTab(bot);
        switch (bot->GetClass())
        {
        case CLASS_DRUID:
            if (spec == 2)
                heal++;
            else
                dps++;
            break;
        case CLASS_PALADIN:
            if (spec == 1)
                tank++;
            else if (spec == 0)
                heal++;
            else
                dps++;
            break;
        case CLASS_PRIEST:
            if (spec != 2)
                heal++;
            else
                dps++;
            break;
        case CLASS_SHAMAN:
            if (spec == 2)
                heal++;
            else
                dps++;
            break;
        case CLASS_WARRIOR:
            if (spec == 2)
                tank++;
            else
                dps++;
            break;
        default:
            dps++;
            break;
        }
    }

    sLog->outMessage("playerbot", LOG_LEVEL_INFO, "Per level:");
    uint32 maxLevel = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);
    for (uint32 i = 0; i < 10; ++i)
    {
        if (!alliance[i] && !horde[i])
            continue;

        uint32 from = i*10;
        uint32 to = min(from + 9, maxLevel);
        if (!from) from = 1;
        sLog->outMessage("playerbot", LOG_LEVEL_INFO, "    %d..%d: %d alliance, %d horde", from, to, alliance[i], horde[i]);
    }
    sLog->outMessage("playerbot", LOG_LEVEL_INFO, "Per race:");
    for (uint8 race = RACE_HUMAN; race < MAX_RACES; ++race)
    {
        if (perRace[race])
            sLog->outMessage("playerbot", LOG_LEVEL_INFO, "    %s: %d", ChatHelper::formatRace(race).c_str(), perRace[race]);
    }
    sLog->outMessage("playerbot", LOG_LEVEL_INFO, "Per class:");
    for (uint8 cls = CLASS_WARRIOR; cls < MAX_CLASSES; ++cls)
    {
        if (perClass[cls])
            sLog->outMessage("playerbot", LOG_LEVEL_INFO, "    %s: %d", ChatHelper::formatClass(cls).c_str(), perClass[cls]);
    }
    sLog->outMessage("playerbot", LOG_LEVEL_INFO, "Per role:");
    sLog->outMessage("playerbot", LOG_LEVEL_INFO, "    tank: %d", tank);
    sLog->outMessage("playerbot", LOG_LEVEL_INFO, "    heal: %d", heal);
    sLog->outMessage("playerbot", LOG_LEVEL_INFO, "    dps: %d", dps);
}

double RandomPlayerbotMgr::GetBuyMultiplier(Player* bot)
{
    uint32 id = bot->GetGUID();
    uint32 value = GetEventValue(id, "buymultiplier");
    if (!value)
    {
        value = urand(1, 120);
        uint32 validIn = urand(sPlayerbotAIConfig.minRandomBotsPriceChangeInterval, sPlayerbotAIConfig.maxRandomBotsPriceChangeInterval);
        SetEventValue(id, "buymultiplier", value, validIn);
    }

    return (double)value / 100.0;
}

double RandomPlayerbotMgr::GetSellMultiplier(Player* bot)
{
    uint32 id = bot->GetGUID();
    uint32 value = GetEventValue(id, "sellmultiplier");
    if (!value)
    {
        value = urand(80, 250);
        uint32 validIn = urand(sPlayerbotAIConfig.minRandomBotsPriceChangeInterval, sPlayerbotAIConfig.maxRandomBotsPriceChangeInterval);
        SetEventValue(id, "sellmultiplier", value, validIn);
    }

    return (double)value / 100.0;
}

uint32 RandomPlayerbotMgr::GetLootAmount(Player* bot)
{
    uint32 id = bot->GetGUID();
    return GetEventValue(id, "lootamount");
}

void RandomPlayerbotMgr::SetLootAmount(Player* bot, uint32 value)
{
    uint32 id = bot->GetGUID();
    SetEventValue(id, "lootamount", value, 24 * 3600);
}

uint32 RandomPlayerbotMgr::GetTradeDiscount(Player* bot)
{
    Group* group = bot->GetGroup();
    return GetLootAmount(bot) / (group ? group->GetMembersCount() : 10);
}

string RandomPlayerbotMgr::HandleRemoteCommand(std::string request)
{
    std::string::iterator pos = find(request.begin(), request.end(), ',');
    if (pos == request.end())
    {
        std::ostringstream out; out << "invalid request: " << request;
        return out.str();
    }

    std::string command = std::string(request.begin(), pos);
    ObjectGuid::LowType guid = atoi(string(pos + 1, request.end()).c_str());
    Player* bot = GetPlayerBot(ObjectGuid(HighGuid::Player, guid));
    if (!bot)
        return "invalid guid";

    PlayerbotAI *ai = bot->GetPlayerbotAI();
    if (!ai)
        return "invalid guid";

    return ai->HandleRemoteCommand(command);
}
