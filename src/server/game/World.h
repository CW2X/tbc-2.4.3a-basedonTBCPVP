
/// \addtogroup world The World
/// @{
/// \file

#ifndef __WORLD_H
#define __WORLD_H

#include "Common.h"
#include "Timer.h"
#include "SharedDefines.h"
#include "QueryResult.h"
#include "QueryCallbackProcessor.h"
#include "Realm/Realm.h"

#include <map>
#include <set>
#include <list>
#include <future>

class Object;
class WorldPacket;
class WorldSession;
class Player;
struct Gladiator;
class Weather;
struct ScriptAction;
struct ScriptInfo;
class SQLResultQueue;
class WorldSocket;
class ArenaTeam;
struct CharTitlesEntry;
class ObjectGuid;

typedef std::unordered_map<uint32, WorldSession*> SessionMap;

#define DEFAULT_PLAYER_LIMIT 100
#define DEFAULT_WORLDSERVER_PORT 8085


enum GlobalPlayerUpdateMask
{
    PLAYER_UPDATE_DATA_LEVEL = 0x01,
    PLAYER_UPDATE_DATA_RACE = 0x02,
    PLAYER_UPDATE_DATA_CLASS = 0x04,
    PLAYER_UPDATE_DATA_GENDER = 0x08,
    PLAYER_UPDATE_DATA_NAME = 0x10,
};

enum ShutdownMask : int
{
    SHUTDOWN_MASK_RESTART = 1,
    SHUTDOWN_MASK_IDLE    = 2,
};

enum ShutdownExitCode : int
{
    SHUTDOWN_EXIT_CODE = 0,
    ERROR_EXIT_CODE    = 1,
    RESTART_EXIT_CODE  = 2,
};

/// Timers for different object refresh rates
enum WorldTimers
{
    // WUPDATE_UNUSED        = 0,
	// WUPDATE_UNUSED        = 1,
    WUPDATE_AUCTIONS       = 2,
    WUPDATE_WEATHERS       = 3,
    WUPDATE_UPTIME         = 4,
    WUPDATE_CORPSES        = 5,
    WUPDATE_EVENTS         = 6,
    WUPDATE_ANNOUNCES      = 7,
    WUPDATE_ARENASEASONLOG = 8,
    WUPDATE_CHECK_FILECHANGES = 9,
    WUPDATE_WHO_LIST      = 10,
    WUPDATE_PINGDB        = 11,
    WUPDATE_COUNT         = 12,
};

/// Configuration elements
enum WorldConfigs
{
    CONFIG_COMPRESSION = 0,
    CONFIG_GRID_UNLOAD,
    CONFIG_INTERVAL_SAVE,
    CONFIG_INTERVAL_MAPUPDATE,
    CONFIG_INTERVAL_CHANGEWEATHER,
    CONFIG_INTERVAL_DISCONNECT_TOLERANCE,
    CONFIG_PORT_WORLD,
    CONFIG_SOCKET_TIMEOUTTIME,
    CONFIG_SOCKET_TIMEOUTTIME_ACTIVE,
    CONFIG_GROUP_XP_DISTANCE,
    CONFIG_SIGHT_MONSTER,
    CONFIG_GAME_TYPE,
    CONFIG_REALM_ZONE,
    CONFIG_ALLOW_TWO_SIDE_ACCOUNTS,
    CONFIG_ALLOW_TWO_SIDE_INTERACTION_CHAT,
    CONFIG_ALLOW_TWO_SIDE_INTERACTION_CHANNEL,
    CONFIG_ALLOW_TWO_SIDE_INTERACTION_GROUP,
    CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD,
    CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION,
    CONFIG_ALLOW_TWO_SIDE_INTERACTION_MAIL,
    CONFIG_ALLOW_TWO_SIDE_WHO_LIST,
    CONFIG_ALLOW_TWO_SIDE_ADD_FRIEND,
    CONFIG_ALLOW_TWO_SIDE_TRADE,
    CONFIG_STRICT_PLAYER_NAMES,
    CONFIG_STRICT_CHARTER_NAMES,
    CONFIG_STRICT_PET_NAMES,
    CONFIG_CHARACTERS_CREATING_DISABLED,
    CONFIG_CHARACTERS_PER_ACCOUNT,
    CONFIG_CHARACTERS_PER_REALM,
    CONFIG_SKIP_CINEMATICS,
    CONFIG_MAX_PLAYER_LEVEL,
    CONFIG_START_PLAYER_LEVEL,
    CONFIG_START_PLAYER_MONEY,
    CONFIG_MAX_HONOR_POINTS,
    CONFIG_START_HONOR_POINTS,
    CONFIG_MAX_ARENA_POINTS,
    CONFIG_START_ARENA_POINTS,
    CONFIG_INSTANCE_IGNORE_LEVEL,
    CONFIG_INSTANCE_IGNORE_RAID,
    CONFIG_BATTLEGROUND_CAST_DESERTER,
    CONFIG_BATTLEGROUND_QUEUE_ANNOUNCER_ENABLE,
    CONFIG_BATTLEGROUND_QUEUE_ANNOUNCER_PLAYERONLY,
    CONFIG_BATTLEGROUND_QUEUE_ANNOUNCER_WORLDONLY,
    CONFIG_BATTLEGROUND_ARENA_RATED_ENABLE,
    CONFIG_BATTLEGROUND_ARENA_CLOSE_AT_NIGHT_MASK,
    CONFIG_BATTLEGROUND_ARENA_ALTERNATE_RATING,
    CONFIG_BATTLEGROUND_ARENA_ANNOUNCE,
    CONFIG_INSTANCE_RESET_TIME_HOUR,
    CONFIG_INSTANCE_UNLOAD_DELAY,
    CONFIG_CAST_UNSTUCK,
    CONFIG_MAX_PRIMARY_TRADE_SKILL,
    CONFIG_MIN_PETITION_SIGNS,
    CONFIG_GM_LOGIN_STATE,
    CONFIG_GM_VISIBLE_STATE,
    CONFIG_GM_CHAT,
    CONFIG_GM_WISPERING_TO,
    CONFIG_GM_LEVEL_IN_GM_LIST,
    CONFIG_GM_LEVEL_IN_WHO_LIST,
    CONFIG_START_GM_LEVEL,
    CONFIG_ALLOW_GM_GROUP,
    CONFIG_ALLOW_GM_FRIEND,
    CONFIG_GM_DEFAULT_GUILD,
    CONFIG_GM_FORCE_GUILD,
    CONFIG_GROUP_VISIBILITY,
    CONFIG_MAIL_DELIVERY_DELAY,
    CONFIG_UPTIME_UPDATE,
    CONFIG_SKILL_CHANCE_ORANGE,
    CONFIG_SKILL_CHANCE_YELLOW,
    CONFIG_SKILL_CHANCE_GREEN,
    CONFIG_SKILL_CHANCE_GREY,
    CONFIG_SKILL_CHANCE_MINING_STEPS,
    CONFIG_SKILL_CHANCE_SKINNING_STEPS,
    CONFIG_SKILL_PROSPECTING,
    CONFIG_SKILL_GAIN_CRAFTING,
    CONFIG_SKILL_GAIN_DEFENSE,
    CONFIG_SKILL_GAIN_GATHERING,
    CONFIG_SKILL_GAIN_WEAPON,
    CONFIG_MAX_OVERSPEED_PINGS,
    CONFIG_ALWAYS_MAX_SKILL_FOR_LEVEL,
    CONFIG_TRANSPORT_DOCKING_SOUNDS,
    CONFIG_WEATHER,
    CONFIG_EXPANSION,
    CONFIG_CHATFLOOD_MESSAGE_COUNT,
    CONFIG_CHATFLOOD_MESSAGE_DELAY,
    CONFIG_CHATFLOOD_MUTE_TIME,
    CONFIG_EVENT_ANNOUNCE,
    CONFIG_CREATURE_FAMILY_ASSISTANCE_RADIUS,
    CONFIG_CREATURE_FAMILY_ASSISTANCE_DELAY,
    CONFIG_CREATURE_FAMILY_FLEE_DELAY,
    //creature unreachable for this time enter evades mode and return home
    CONFIG_CREATURE_UNREACHABLE_TARGET_EVADE_TIME,
    //creature unreachable for this time start evading all attacks
    CONFIG_CREATURE_UNREACHABLE_TARGET_EVADE_ATTACKS_TIME,
    CONFIG_CREATURE_STOP_FOR_PLAYER,
    CONFIG_QUEST_LOW_LEVEL_HIDE_DIFF,
    CONFIG_QUEST_HIGH_LEVEL_HIDE_DIFF,
    CONFIG_RESTRICTED_LFG_CHANNEL,
    CONFIG_SILENTLY_GM_JOIN_TO_CHANNEL,
    CONFIG_TALENTS_INSPECTING,

    CONFIG_RESPAWN_MINCHECKINTERVALMS,
    CONFIG_RESPAWN_DYNAMIC_ESCORTNPC,
    CONFIG_SAVE_RESPAWN_TIME_IMMEDIATELY,
    CONFIG_RESPAWN_DYNAMICMODE,
    CONFIG_RESPAWN_GUIDWARNLEVEL,
    CONFIG_RESPAWN_GUIDALERTLEVEL,
    CONFIG_RESPAWN_RESTARTQUIETTIME,
    CONFIG_RESPAWN_DYNAMICMINIMUM_CREATURE,
    CONFIG_RESPAWN_DYNAMICMINIMUM_GAMEOBJECT,
    CONFIG_RESPAWN_GUIDWARNING_FREQUENCY,
    CONFIG_RESPAWN_DYNAMICRATE_CREATURE,
    CONFIG_RESPAWN_DYNAMICRATE_GAMEOBJECT,

    CONFIG_DETECT_POS_COLLISION,
    CONFIG_CHAT_FAKE_MESSAGE_PREVENTING,
    CONFIG_CHAT_STRICT_LINK_CHECKING_SEVERITY,
    CONFIG_CHAT_STRICT_LINK_CHECKING_KICK,
    CONFIG_CORPSE_DECAY_NORMAL,
    CONFIG_CORPSE_DECAY_RARE,
    CONFIG_CORPSE_DECAY_ELITE,
    CONFIG_CORPSE_DECAY_RAREELITE,
    CONFIG_CORPSE_DECAY_WORLDBOSS,
    CONFIG_ADDON_CHANNEL,
    CONFIG_DEATH_SICKNESS_LEVEL,
    CONFIG_DEATH_CORPSE_RECLAIM_DELAY_PVP,
    CONFIG_DEATH_CORPSE_RECLAIM_DELAY_PVE,
    CONFIG_DEATH_BONES_WORLD,
    CONFIG_DEATH_BONES_BG_OR_ARENA,
    CONFIG_THREAT_RADIUS,
    CONFIG_INSTANT_LOGOUT,
    CONFIG_GROUPLEADER_RECONNECT_PERIOD,
    CONFIG_ALL_TAXI_PATHS,
    CONFIG_INSTANT_TAXI,
    CONFIG_DECLINED_NAMES_USED,
    CONFIG_LISTEN_RANGE_SAY,
    CONFIG_LISTEN_RANGE_TEXTEMOTE,
    CONFIG_LISTEN_RANGE_YELL,
    CONFIG_ARENA_MAX_RATING_DIFFERENCE,
    CONFIG_ARENA_RATING_DISCARD_TIMER,
    CONFIG_ARENA_RATED_UPDATE_TIMER,
    CONFIG_ARENA_AUTO_DISTRIBUTE_POINTS,
    CONFIG_ARENA_AUTO_DISTRIBUTE_INTERVAL_DAYS,
    CONFIG_BATTLEGROUND_PREMATURE_FINISH_TIMER,
    CONFIG_BATTLEGROUND_PREMADE_GROUP_WAIT_FOR_MATCH,
    CONFIG_BATTLEGROUND_INVITATION_TYPE,
    CONFIG_BATTLEGROUND_TIMELIMIT_WARSONG,
    CONFIG_BATTLEGROUND_TIMELIMIT_ARENA,
    CONFIG_CHARDELETE_KEEP_DAYS,

    CONFIG_START_ALL_EXPLORED,
    CONFIG_START_ALL_REP,
    CONFIG_ALWAYS_MAXSKILL,
    CONFIG_PVP_TOKEN_ENABLE,
    CONFIG_PVP_TOKEN_MAP_TYPE,
    CONFIG_PVP_TOKEN_ID,
    CONFIG_PVP_TOKEN_COUNT,
    CONFIG_PVP_TOKEN_ZONE_ID,
    CONFIG_NO_RESET_TALENT_COST,
    CONFIG_SHOW_KICK_IN_WORLD,
    CONFIG_ENABLE_SINFO_LOGIN,
    CONFIG_PREMATURE_BG_REWARD,
    CONFIG_NUMTHREADS,

    CONFIG_WORLDCHANNEL_MINLEVEL,

    //logs duration in days. -1 to keep forever. 0 to disable logging.
    CONFIG_LOG_BG_STATS,
    CONFIG_LOG_BOSS_DOWNS,
    CONFIG_LOG_CHAR_DELETE,
    CONFIG_GM_LOG_CHAR_DELETE,
    CONFIG_LOG_CHAR_CHAT,
    CONFIG_GM_LOG_CHAR_CHAT,
    CONFIG_LOG_CHAR_GUILD_MONEY,
    CONFIG_GM_LOG_CHAR_GUILD_MONEY,
    CONFIG_LOG_CHAR_ITEM_DELETE,
    CONFIG_GM_LOG_CHAR_ITEM_DELETE,
    CONFIG_LOG_CHAR_ITEM_GUILD_BANK,
    CONFIG_GM_LOG_CHAR_ITEM_GUILD_BANK,
    CONFIG_LOG_CHAR_ITEM_VENDOR,
    CONFIG_GM_LOG_CHAR_ITEM_VENDOR,
    CONFIG_LOG_CHAR_ITEM_ENCHANT,
    CONFIG_GM_LOG_CHAR_ITEM_ENCHANT,
    CONFIG_LOG_CHAR_ITEM_AUCTION,
    CONFIG_GM_LOG_CHAR_ITEM_AUCTION,
    CONFIG_LOG_CHAR_ITEM_TRADE,
    CONFIG_GM_LOG_CHAR_ITEM_TRADE,
    CONFIG_LOG_CHAR_MAIL,
    CONFIG_GM_LOG_CHAR_MAIL,
    CONFIG_LOG_CHAR_RENAME,
    CONFIG_GM_LOG_CHAR_RENAME,
    CONFIG_LOG_GM_COMMANDS,
    CONFIG_LOG_SANCTIONS,
    CONFIG_LOG_CONNECTION_IP,
    CONFIG_GM_LOG_CONNECTION_IP,

    CONFIG_MYSQL_BUNDLE_LOGINDB,
    CONFIG_MYSQL_BUNDLE_CHARDB,
    CONFIG_MYSQL_BUNDLE_WORLDDB,

    CONFIG_AUTOANNOUNCE_ENABLED,

    CONFIG_ANTICHEAT_MOVEMENT_ENABLE,
    CONFIG_ANTICHEAT_MOVEMENT_KICK,
    CONFIG_ANTICHEAT_MOVEMENT_BAN,
    CONFIG_ANTICHEAT_MOVEMENT_BAN_TIME,
    CONFIG_ANTICHEAT_MOVEMENT_GM,
    CONFIG_ANTICHEAT_MOVEMENT_KILL,
    CONFIG_ANTICHEAT_MOVEMENT_WARN_GM,
    CONFIG_ANTICHEAT_MOVEMENT_WARN_GM_COOLDOWN,
    CONFIG_PENDING_MOVE_CHANGES_TIMEOUT,

    CONFIG_WARDEN_ENABLED,
    CONFIG_WARDEN_KICK,
    CONFIG_WARDEN_NUM_CHECKS,
    CONFIG_WARDEN_CLIENT_CHECK_HOLDOFF,
    CONFIG_WARDEN_CLIENT_RESPONSE_DELAY,
    CONFIG_WARDEN_DB_LOG,
    CONFIG_WARDEN_BAN_TIME,

    CONFIG_WHISPER_MINLEVEL,

    CONFIG_ARENA_SPECTATOR_ENABLE,
    CONFIG_ARENA_SPECTATOR_MAX,
    CONFIG_ARENA_SPECTATOR_GHOST,
    CONFIG_ARENA_SPECTATOR_STEALTH,

    CONFIG_ARENA_SEASON,
    CONFIG_ARENA_NEW_TITLE_DISTRIB,
    CONFIG_ARENA_NEW_TITLE_DISTRIB_MIN_RATING,
    CONFIG_ARENA_DECAY_ENABLED,
    CONFIG_ARENA_DECAY_MINIMUM_RATING,
    CONFIG_ARENA_DECAY_VALUE,
    CONFIG_ARENA_DECAY_CONSECUTIVE_WEEKS,

    CONFIG_PACKET_SPOOF_POLICY,
    CONFIG_PACKET_SPOOF_BANMODE,
    CONFIG_PACKET_SPOOF_BANDURATION,

    CONFIG_SPAM_REPORT_THRESHOLD,
    CONFIG_SPAM_REPORT_PERIOD,
    CONFIG_SPAM_REPORT_COOLDOWN,

    CONFIG_FACTION_CHANGE_ENABLED,
    CONFIG_FACTION_CHANGE_A2H,
    CONFIG_FACTION_CHANGE_H2A,
    CONFIG_FACTION_CHANGE_A2H_COST,
    CONFIG_FACTION_CHANGE_H2A_COST,
    CONFIG_RACE_CHANGE_COST, //Not interfaction

    CONFIG_ARENASERVER_ENABLED,
    CONFIG_ARENASERVER_USE_CLOSESCHEDULE,
    CONFIG_ARENASERVER_PLAYER_REPARTITION_THRESHOLD,

    CONFIG_BETASERVER_ENABLED,
    CONFIG_BETASERVER_LVL70,

    CONFIG_TESTING_MAX_PARALLEL_TESTS,
    CONFIG_TESTING_MAX_UPDATE_TIME,
    CONFIG_TESTING_WARN_UPDATE_TIME_THRESHOLD,

    CONFIG_DEBUG_DISABLE_MAINHAND,
    CONFIG_DEBUG_DISABLE_ARMOR,
    CONFIG_DEBUG_LOG_LAST_PACKETS, //See WorldSocket::_lastPacketSent
    CONFIG_DEBUG_LOG_ALL_PACKETS,
    CONFIG_DEBUG_DISABLE_CREATURES_LOADING,
    CONFIG_DEBUG_DISABLE_GAMEOBJECTS_LOADING,
    CONFIG_DEBUG_DISABLE_TRANSPORTS,
    CONFIG_DEBUG_DISABLE_OUTDOOR_PVP,

    CONFIG_CLIENTCACHE_VERSION,

    CONFIG_GUILD_EVENT_LOG_COUNT,
    CONFIG_GUILD_BANK_EVENT_LOG_COUNT,

	CONFIG_MONITORING_ENABLED,
	CONFIG_MONITORING_GENERALINFOS_UPDATE,
	CONFIG_MONITORING_KEEP_DURATION,
	CONFIG_MONITORING_ABNORMAL_WORLD_UPDATE_DIFF,
	CONFIG_MONITORING_ABNORMAL_MAP_UPDATE_DIFF,
    CONFIG_MONITORING_ALERT_THRESHOLD_COUNT,  //trigger alert on at least avg <diff> on the last <count> update
	CONFIG_MONITORING_DYNAMIC_VIEWDIST,
	CONFIG_MONITORING_DYNAMIC_VIEWDIST_MINDIST,
    CONFIG_MONITORING_DYNAMIC_VIEWDIST_IDEAL_DIFF,
    CONFIG_MONITORING_DYNAMIC_VIEWDIST_TRIGGER_DIFF,
    CONFIG_MONITORING_DYNAMIC_VIEWDIST_CHECK_INTERVAL,
    CONFIG_MONITORING_DYNAMIC_VIEWDIST_AVERAGE_COUNT,

	CONFIG_MONITORING_LAG_AUTO_REBOOT_COUNT,

    CONFIG_HOTSWAP_ENABLED,
    CONFIG_HOTSWAP_RECOMPILER_ENABLED,
    CONFIG_HOTSWAP_EARLY_TERMINATION_ENABLED,
    CONFIG_HOTSWAP_BUILD_FILE_RECREATION_ENABLED,
    CONFIG_HOTSWAP_INSTALL_ENABLED,
    CONFIG_HOTSWAP_PREFIX_CORRECTION_ENABLED,

    CONFIG_DB_PING_INTERVAL,

    CONFIG_CACHE_DATA_QUERIES,

    CONFIG_VALUE_COUNT,
};

/// Server rates
enum Rates
{
    RATE_HEALTH=0,
    RATE_POWER_MANA,
    RATE_POWER_RAGE_INCOME,
    RATE_POWER_RAGE_LOSS,
    RATE_POWER_FOCUS,
    RATE_SKILL_DISCOVERY,
    RATE_DROP_ITEM_POOR,
    RATE_DROP_ITEM_NORMAL,
    RATE_DROP_ITEM_UNCOMMON,
    RATE_DROP_ITEM_RARE,
    RATE_DROP_ITEM_EPIC,
    RATE_DROP_ITEM_LEGENDARY,
    RATE_DROP_ITEM_ARTIFACT,
    RATE_DROP_ITEM_REFERENCED,
    RATE_DROP_MONEY,
    RATE_XP_KILL,
    RATE_XP_QUEST,
    RATE_XP_EXPLORE,
    RATE_XP_PAST_70,
    RATE_REPUTATION_GAIN,
    RATE_CREATURE_AGGRO,
    RATE_REST_INGAME,
    RATE_REST_OFFLINE_IN_TAVERN_OR_CITY,
    RATE_REST_OFFLINE_IN_WILDERNESS,
    RATE_DAMAGE_FALL,
    RATE_AUCTION_TIME,
    RATE_AUCTION_DEPOSIT,
    RATE_AUCTION_CUT,
    RATE_HONOR,
    RATE_MINING_AMOUNT,
    RATE_MINING_NEXT,
    RATE_TALENT,
    RATE_LOYALTY,
    RATE_CORPSE_DECAY_LOOTED,
    RATE_INSTANCE_RESET_TIME,
    RATE_DURABILITY_LOSS_DAMAGE,
    RATE_DURABILITY_LOSS_PARRY,
    RATE_DURABILITY_LOSS_ABSORB,
    RATE_DURABILITY_LOSS_BLOCK,
    MAX_RATES
};

enum RealmZone
{
    REALM_ZONE_UNKNOWN       = 0,                           // any language
    REALM_ZONE_DEVELOPMENT   = 1,                           // any language
    REALM_ZONE_UNITED_STATES = 2,                           // extended-Latin
    REALM_ZONE_OCEANIC       = 3,                           // extended-Latin
    REALM_ZONE_LATIN_AMERICA = 4,                           // extended-Latin
    REALM_ZONE_TOURNAMENT_5  = 5,                           // basic-Latin at create, any at login
    REALM_ZONE_KOREA         = 6,                           // East-Asian
    REALM_ZONE_TOURNAMENT_7  = 7,                           // basic-Latin at create, any at login
    REALM_ZONE_ENGLISH       = 8,                           // extended-Latin
    REALM_ZONE_GERMAN        = 9,                           // extended-Latin
    REALM_ZONE_FRENCH        = 10,                          // extended-Latin
    REALM_ZONE_SPANISH       = 11,                          // extended-Latin
    REALM_ZONE_RUSSIAN       = 12,                          // Cyrillic
    REALM_ZONE_TOURNAMENT_13 = 13,                          // basic-Latin at create, any at login
    REALM_ZONE_TAIWAN        = 14,                          // East-Asian
    REALM_ZONE_TOURNAMENT_15 = 15,                          // basic-Latin at create, any at login
    REALM_ZONE_CHINA         = 16,                          // East-Asian
    REALM_ZONE_CN1           = 17,                          // basic-Latin at create, any at login
    REALM_ZONE_CN2           = 18,                          // basic-Latin at create, any at login
    REALM_ZONE_CN3           = 19,                          // basic-Latin at create, any at login
    REALM_ZONE_CN4           = 20,                          // basic-Latin at create, any at login
    REALM_ZONE_CN5           = 21,                          // basic-Latin at create, any at login
    REALM_ZONE_CN6           = 22,                          // basic-Latin at create, any at login
    REALM_ZONE_CN7           = 23,                          // basic-Latin at create, any at login
    REALM_ZONE_CN8           = 24,                          // basic-Latin at create, any at login
    REALM_ZONE_TOURNAMENT_25 = 25,                          // basic-Latin at create, any at login
    REALM_ZONE_TEST_SERVER   = 26,                          // any language
    REALM_ZONE_TOURNAMENT_27 = 27,                          // basic-Latin at create, any at login
    REALM_ZONE_QA_SERVER     = 28,                          // any language
    REALM_ZONE_CN9           = 29                           // basic-Latin at create, any at login
};

enum WowPatch
{
    WOW_PATCH_200   = 0,
    WOW_PATCH_210   = 1,
    WOW_PATCH_220   = 2,
    WOW_PATCH_230   = 3,
    WOW_PATCH_240   = 4,
    WOW_PATCH_300   = 5,
    WOW_PATCH_310   = 6,
    WOW_PATCH_320   = 7,
    WOW_PATCH_322   = 8,
    WOW_PATCH_330   = 9,
    WOW_PATCH_335   = 10,
    WOW_PATCH_MIN   = WOW_PATCH_200,
    WOW_PATCH_MAX   = WOW_PATCH_335
};

struct AutoAnnounceMessage
{
    std::string message;
    uint64 nextAnnounce;
};

enum HonorKillPvPRank
{
    HKRANK00,
    HKRANK01,
    HKRANK02,
    HKRANK03,
    HKRANK04,
    HKRANK05,
    HKRANK06,
    HKRANK07,
    HKRANK08,
    HKRANK09,
    HKRANK10,
    HKRANK11,
    HKRANK12,
    HKRANK13,
    HKRANK14,
    HKRANKMAX
};

// DB scripting commands
enum ScriptCommands
{
    SCRIPT_COMMAND_TALK                  = 0,              // source/target = Creature, target = any, datalong = talk type (see ChatType enum), datalong2 & 1 = player talk (instead of creature), dataint = string_id
    SCRIPT_COMMAND_EMOTE =                 1,              // source = unit, datalong = anim_id
    SCRIPT_COMMAND_FIELD_SET =             2,              // source = any, datalong = field_id, datalog2 = value
    SCRIPT_COMMAND_MOVE_TO =               3,              // source = Creature, datalog2 = time, x/y/z
    SCRIPT_COMMAND_FLAG_SET =              4,              // source = any, datalong = field_id, datalog2 = bitmask
    SCRIPT_COMMAND_FLAG_REMOVE =           5,              // source = any, datalong = field_id, datalog2 = bitmask
    SCRIPT_COMMAND_TELEPORT_TO =           6,              // source or target with Player, datalong = map_id, x/y/z
    SCRIPT_COMMAND_QUEST_EXPLORED =        7,              // one from source or target must be Player, another GO/Creature, datalong=quest_id, datalong2=distance or0,
    SCRIPT_COMMAND_KILL_CREDIT =           8,              // source = player, target = player, datalong = kill credit entry
    SCRIPT_COMMAND_RESPAWN_GAMEOBJECT =    9,              // source = any (summoner), datalong=db_guid, datalong2=despawn_delay
    SCRIPT_COMMAND_TEMP_SUMMON_CREATURE = 10,              // source = any (summoner), datalong=creature entry, datalong2=despawn_delay
    SCRIPT_COMMAND_OPEN_DOOR =            11,              // source = unit, datalong=db_guid, datalong2=reset_delay
    SCRIPT_COMMAND_CLOSE_DOOR =           12,              // source = unit, datalong=db_guid, datalong2=reset_delay
    SCRIPT_COMMAND_ACTIVATE_OBJECT =      13,              // source = unit, target=GO
    SCRIPT_COMMAND_REMOVE_AURA =          14,              // source (datalong2!=0) or target (datalong==0) unit, datalong = spell_id
    SCRIPT_COMMAND_CAST_SPELL =           15,              // source (datalong2!=0) or target (datalong==0) unit, datalong = spell_id
    SCRIPT_COMMAND_PLAY_SOUND =            16,              // datalong soundid, datalong2 play only self

    SCRIPT_COMMAND_LOAD_PATH =            20,              // source = unit, path = datalong
    SCRIPT_COMMAND_CALLSCRIPT_TO_UNIT =   21,              // datalong scriptid, lowguid datalong2, dataint table
    SCRIPT_COMMAND_KILL =                 22,              // datalong removecorpse
    SCRIPT_COMMAND_SMART_SET_DATA =       23,              // source = unit, datalong=id, datalong2=value // triggers SMART_EVENT_DATA_SET for unit if using SmartAI
};

/// Storage class for commands issued for delayed execution
struct CliCommandHolder
{
    typedef void Print(void*, const char*);
    typedef void CommandFinished(void*, bool success);

    void* m_callbackArg;
    char *m_command;
    Print* m_print;

    CommandFinished* m_commandFinished;

    CliCommandHolder(void* callbackArg, const char *command, Print* zprint, CommandFinished* commandFinished)
        : m_callbackArg(callbackArg), m_command(strdup(command)), m_print(zprint), m_commandFinished(commandFinished)
    {
    }

    ~CliCommandHolder() { free(m_command); }

private:
    CliCommandHolder(CliCommandHolder const& right) = delete;
    CliCommandHolder& operator=(CliCommandHolder const& right) = delete;
};
/// The World
class TC_GAME_API World
{
    public:
        static std::atomic<uint32> m_worldLoopCounter;

        static World* instance()
        {
            static World instance;
            return &instance;
        }

        World();
        ~World();

        // Continuous integration testing. If set, all tests are started, then when they're done the world will shutdown.
        void SetCITesting();

        WorldSession* FindSession(uint32 id) const;
        void AddSession(WorldSession *s);
        bool RemoveSession(uint32 id);
        /// Get the number of current active sessions
        const SessionMap& GetAllSessions() const { return m_sessions; }
        void UpdateMaxSessionCounters();
        uint32 GetActiveAndQueuedSessionCount() const { return m_sessions.size(); }
        uint32 GetActiveSessionCount() const { return m_sessions.size() - m_QueuedPlayer.size(); }
        uint32 GetQueuedSessionCount() const { return m_QueuedPlayer.size(); }
        /// Get the maximum number of parallel sessions on the server since last reboot
        uint32 GetMaxQueuedSessionCount() const { return m_maxQueuedSessionCount; }
        uint32 GetMaxActiveSessionCount() const { return m_maxActiveSessionCount; }
        Player* FindPlayerInZone(uint32 zone);

        Weather* FindWeather(uint32 id) const;
        Weather* AddWeather(uint32 zone_id);
        void RemoveWeather(uint32 zone_id);

        bool IsZoneFFA(uint32) const;

        /// Get the active session server limit (or security level limitations)
        uint32 GetPlayerAmountLimit() const { return m_playerLimit >= 0 ? m_playerLimit : 0; }
        AccountTypes GetPlayerSecurityLimit() const { return m_allowedSecurityLevel; }

        /// Set the active session server limit (or security level limitation)
        void SetPlayerLimit(int32 limit);

        //player Queue
        typedef std::list<WorldSession*> Queue;
        void AddQueuedPlayer(WorldSession*);
        bool RemoveQueuedPlayer(WorldSession* session);
        int32 GetQueuePos(WorldSession*);
        bool HasRecentlyDisconnected(WorldSession*);
        uint32 GetQueueSize() const { return m_QueuedPlayer.size(); }

        /// Set a new Message of the Day
        void SetMotd(std::string motd) { m_motd = motd; }
        /// Get the current Message of the Day
        const char* GetMotd() const { return m_motd.c_str(); }
        /// Get the last Twitter

        /// Set the string for new characters (first login)
        void SetNewCharString(std::string str) { m_newCharString = str; }
        /// Get the string for new characters (first login)
        const std::string& GetNewCharString() const { return m_newCharString; }

        LocaleConstant GetDefaultDbcLocale() const { return m_defaultDbcLocale; }

        /// Get the path where data (dbc, maps) are stored on disk
        std::string GetDataPath() const { return m_dataPath; }

        /// Next daily quests and random bg reset time
        time_t GetNextDailyQuestsResetTime() const { return m_NextDailyQuestReset; }

        /// Get the maximum skill level a player can reach
        uint16 GetConfigMaxSkillValue() const
        {
            uint32 lvl = getConfig(CONFIG_MAX_PLAYER_LEVEL);
            return lvl > 60 ? 300 + ((lvl - 60) * 75) / 10 : lvl*5;
        }

        /// Get current server's WoW Patch
        uint8 GetWowPatch() const { return m_wowPatch; }
        std::string const GetPatchName() const;

        void SetInitialWorldSettings();
        void LoadConfigSettings(bool reload = false);
        void LoadMotdAndTwitter();

        void SendWorldText(int32 string_id, ...);
        void SendGlobalText(const char* text, WorldSession* self = nullptr);
        void SendGMText(int32 string_id, ...);
        void SendGlobalMessage(WorldPacket* packet, WorldSession* self = nullptr, uint32 team = 0);
        void SendGlobalGMMessage(WorldPacket* packet, WorldSession* self = nullptr, uint32 team = 0);
        void SendZoneMessage(uint32 zone, WorldPacket* packet, WorldSession *self = nullptr, uint32 team = 0);
        void SendZoneText(uint32 zone, const char* text, WorldSession* self = nullptr, uint32 team = 0);
        void SendServerMessage(uint32 type, const char* text = "", Player* player = nullptr);

        //Force all connected clients to clear specified player cache
        void InvalidatePlayerDataToAllClient(ObjectGuid guid);

        void SendZoneUnderAttack(uint32 zoneId, Team team);

        uint32 pvp_ranks[HKRANKMAX];
        uint32 confStaticLeaders[12];
        std::list<Gladiator> confGladiators;

        /// Are we in the middle of a shutdown?
        bool IsShuttingDown() const { return IsStopped() || m_ShutdownTimer > 0; }
        uint32 GetShutDownTimeLeft() const { return m_ShutdownTimer; }
        void ShutdownServ(uint32 time, uint32 options, uint8 exitcode, const std::string& reason = std::string());
        void ShutdownCancel();
        void ShutdownMsg(bool show = false, Player* player = nullptr, const std::string& reason = std::string());
        std::string GetShutdownReason() { return m_ShutdownReason; }
        static uint8 GetExitCode() { return m_ExitCode; }
        static void StopNow(uint8 exitcode) { m_stopEvent = true; m_ExitCode = exitcode; }
        static bool IsStopped() { return m_stopEvent; }

        void Update(time_t diff);

        void UpdateSessions(uint32 diff);
        /// Set a server rate (see #Rates)
        void setRate(Rates rate,float value) { rate_values[rate]=value; }
        /// Get a server rate (see #Rates)
        float GetRate(Rates rate) const { return rate_values[rate]; }

        /// Set a server configuration element (see #WorldConfigs)
        void setConfig(uint32 index,uint32 value)
        {
            if(index < CONFIG_VALUE_COUNT)
                m_configs[index]=value;
        }

        /// Get a server configuration element (see #WorldConfigs)
        uint32 getConfig(uint32 index) const
        {
            if(index<CONFIG_VALUE_COUNT)
                return m_configs[index];
            else
                return 0;
        }

//TC compatibility macros
#define getIntConfig(a) getConfig(a)
#define getBoolConfig(a) getConfig(a)
#define getFloatConfig(a) getConfig(a)

        /// Are we on a "Player versus Player" server?
        bool IsPvPRealm() { return (getConfig(CONFIG_GAME_TYPE) == REALM_TYPE_PVP || getConfig(CONFIG_GAME_TYPE) == REALM_TYPE_RPPVP || getConfig(CONFIG_GAME_TYPE) == REALM_TYPE_FFA_PVP); }
        bool IsFFAPvPRealm() { return getConfig(CONFIG_GAME_TYPE) == REALM_TYPE_FFA_PVP; }

        bool KickPlayer(const std::string& playerName);
        void KickAll();
        void KickAllLess(AccountTypes sec);
        //author_session can be null
        BanReturn BanAccount(SanctionType mode, std::string const& nameOrIP, std::string const& duration, std::string const& reason, std::string const& author, WorldSession const* author_session);
        //author_session can be null
        BanReturn BanAccount(SanctionType mode, std::string const& nameOrIP, uint32 duration_secs, std::string const& reason, std::string const& author, WorldSession const* author_session);
        // unbanAuthor can be null
        bool RemoveBanAccount(SanctionType mode, std::string nameOrIP, WorldSession const* unbanAuthor);


        bool IsAllowedMap(uint32 mapid) { return m_forbiddenMapIds.count(mapid) == 0 ;}

        // for max speed access
        static float GetMaxVisibleDistanceOnContinents()    { return m_MaxVisibleDistanceOnContinents; }
        static float GetMaxVisibleDistanceInInstances()     { return m_MaxVisibleDistanceInInstances;  }
        static float GetMaxVisibleDistanceInBGArenas()      { return m_MaxVisibleDistanceInBGArenas;   }
        static float GetMaxVisibleDistanceForObject()   { return m_MaxVisibleDistanceForObject;   }
        static float GetMaxVisibleDistanceInFlight()    { return m_MaxVisibleDistanceInFlight;    }

		static int32 GetVisibilityNotifyPeriodOnContinents() { return m_visibility_notify_periodOnContinents; }
		static int32 GetVisibilityNotifyPeriodInInstances() { return m_visibility_notify_periodInInstances; }
		static int32 GetVisibilityNotifyPeriodInBGArenas() { return m_visibility_notify_periodInBGArenas; }

        inline std::string GetWardenBanTime()          {return m_wardenBanTime;}


        void ProcessCliCommands();
        void QueueCliCommand(CliCommandHolder* commandHolder) { cliCmdQueue.add(commandHolder); }

        void ForceGameEventUpdate();

        void UpdateRealmCharCount(uint32 accid);

        void UpdateAllowedSecurity();

        /// Deny clients?
        bool IsClosed() const;

        /// Close world
        void SetClosed(bool val);

        LocaleConstant GetAvailableDbcLocale(LocaleConstant locale) const { if(m_availableDbcLocaleMask & (1 << locale)) return locale; else return m_defaultDbcLocale; }

        //used World DB version
        void LoadDBVersion();
        char const* GetDBVersion() { return m_DBVersion.c_str(); }

        void ResetEventSeasonalQuests(uint16 event_id);

        uint32 GetCurrentQuestForPool(uint32 poolId);
        bool IsQuestInAPool(uint32 questId);
        bool IsQuestCurrentOfAPool(uint32 questId);
        bool IsPhishing(std::string msg);
        void LogPhishing(uint32 src, uint32 dst, std::string msg);
        void ResetDailyQuests();
        void LoadAutoAnnounce();

        // Custom Dynamic Arena titles system
        std::vector<ArenaTeam*> getArenaLeaderTeams() { return firstArenaTeams; };
        void updateArenaLeaderTeams(uint8 maxcount, uint8 type = 2, uint32 minimalRating = 1800);
        void updateArenaLeadersTitles();
        //must be between 1 and 3
        CharTitlesEntry const* getArenaLeaderTitle(uint8 rank);
        CharTitlesEntry const* getGladiatorTitle(uint8 rank);
        // --

        void RemoveOldCorpses();

        void TriggerGuidWarning();
        void TriggerGuidAlert();
        bool IsGuidWarning() { return _guidWarn; }
        bool IsGuidAlert() { return _guidAlert; }

    protected:
        void _UpdateGameTime();
        // callback for UpdateRealmCharacters
        void _UpdateRealmCharCount(PreparedQueryResult resultCharCount);

        void InitDailyQuestResetTime(bool loading = true);
        void InitNewDataForQuestPools();
        void LoadQuestPoolsData();
        void UpdateMonitoring(uint32 diff);
    private:

        void UpdateArenaSeasonLogs();

        static std::atomic<bool> m_stopEvent;
        static uint8 m_ExitCode;
        uint32 m_ShutdownTimer;
        uint32 m_ShutdownMask;
        std::string m_ShutdownReason;

        bool m_isClosed;

        time_t m_startTime;
        IntervalTimer m_timers[WUPDATE_COUNT];
        uint32 mail_timer;
        uint32 mail_timer_expires;

        typedef std::unordered_map<uint32, Weather* > WeatherMap;
        WeatherMap m_weathers;
        SessionMap m_sessions;
        typedef std::unordered_map<uint32, time_t> DisconnectMap;
        DisconnectMap m_disconnects;
        uint32 m_maxActiveSessionCount;
        uint32 m_maxQueuedSessionCount;

        std::string m_newCharString;

        float rate_values[MAX_RATES];
        int32 m_configs[CONFIG_VALUE_COUNT];
        int32 m_playerLimit;
        uint8 m_wowPatch;
        AccountTypes m_allowedSecurityLevel;
        LocaleConstant m_defaultDbcLocale;                     // from config for one from loaded DBC locales
        uint32 m_availableDbcLocaleMask;                       // by loaded DBC
        void DetectDBCLang();
        std::string m_motd;
        std::string m_dataPath;
        std::set<uint32> m_forbiddenMapIds;

        void LoadCustomFFAZones();
        std::set<uint32> configFFAZones;

        void LoadFishingWords();
        std::list<std::string> fishingWords;

        // for max speed access
        static float m_MaxVisibleDistanceOnContinents;
        static float m_MaxVisibleDistanceInInstances;
        static float m_MaxVisibleDistanceInBGArenas;
        static float m_MaxVisibleDistanceForObject;
        static float m_MaxVisibleDistanceInFlight;

		static int32 m_visibility_notify_periodOnContinents;
		static int32 m_visibility_notify_periodInInstances;
		static int32 m_visibility_notify_periodInBGArenas;

        std::string m_wardenBanTime;

       // CLI command holder to be thread safe
        LockedQueue<CliCommandHolder*> cliCmdQueue;

        // next daily quests reset time
        time_t m_NextDailyQuestReset;

        //Player Queue
        Queue m_QueuedPlayer;

        //sessions that are added async
        void AddSession_(WorldSession* s);
        LockedQueue<WorldSession*> addSessQueue;

        //used versions
        std::string m_DBVersion;

        std::vector<uint32> m_questInPools;
        std::map<uint32, uint32> m_currentQuestInPools;

        std::map<uint32, AutoAnnounceMessage*> autoAnnounces;

        std::vector<ArenaTeam*> firstArenaTeams;

        void SendGuidWarning();
        void DoGuidWarningRestart();
        void DoGuidAlertRestart();

        void ProcessQueryCallbacks();
        QueryCallbackProcessor _queryProcessor;

        std::string _guidWarningMsg;
        std::string _alertRestartReason;

        std::mutex _guidAlertLock;

        bool _guidWarn;
        bool _guidAlert;
        uint32 _warnDiff;
        time_t _warnShutdownTime;

        bool _CITesting; //continuous integration testing
};

TC_GAME_API extern Realm realm;

#define sWorld World::instance()
#endif
/// @}
