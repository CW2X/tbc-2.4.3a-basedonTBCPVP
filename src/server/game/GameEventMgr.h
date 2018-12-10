

#ifndef TRINITY_GAMEEVENT_H
#define TRINITY_GAMEEVENT_H

#include "Define.h"
#include "Creature.h"
#include "GameObject.h"

#define max_ge_check_delay 86400                            // 1 day in seconds

enum GameEventState
{
    GAMEEVENT_NORMAL = 0,   // standard game events
    GAMEEVENT_WORLD_INACTIVE,   // not yet started
    GAMEEVENT_WORLD_CONDITIONS,  // condition matching phase
    GAMEEVENT_WORLD_NEXTPHASE,   // conditions are met, now 'lenght' timer to start next event
    GAMEEVENT_WORLD_FINISHED    // next events are started, unapply this one
};

//some events from DB for core usage
enum GameEventList
{
    GAME_EVENT_MIDSUMMER_FIRE_FESTIVAL = 1,
    GAME_EVENT_HALLOWS_END             = 12,
    GAME_EVENT_NIGHTS                  = 27,
    GAME_EVENT_WICKERMAN_FESTIVAL      = 50,
    GAME_EVENT_BETA                    = 62,
    GAME_EVENT_2_4                     = 67,
    GAME_EVENT_PIRATES_DAY             = 80,
#ifdef LICH_KING
    GAME_EVENT_DAY_OF_THE_DEAD         = 81,
#endif
};

struct GameEventFinishCondition
{
    float reqNum;  // required number // use float, since some events use percent
    float done;    // done number
    uint32 max_world_state;  // max resource count world state update id
    uint32 done_world_state; // done resource count world state update id
};

struct GameEventQuestToEventConditionNum
{
    uint32 event_id;
    uint32 condition;
    float num;
};

struct GameEventData
{
    GameEventData() : start(1),end(0),nextstart(0),occurence(0),length(0),state(GAMEEVENT_NORMAL) {}
    time_t start;   // occurs after this time
    time_t end;     // occurs before this time
    time_t nextstart; // after this time the follow-up events count this phase completed
    uint32 occurence;   // time between end and start
    uint32 length;  // length of the event (minutes) after finishing all conditions
    GameEventState state;   // state of the game event, these are saved into the game_event table on change!
    std::map<uint32 /*condition id*/, GameEventFinishCondition> conditions;  // conditions to finish
    std::set<uint16 /*gameevent id*/> prerequisite_events;  // events that must be completed before starting this event
    std::string description;

    bool isValid() const { return ((length > 0) || (state > GAMEEVENT_NORMAL)); }
};

struct ModelEquip
{
    uint32 modelid;
    int8 equipment_id;
    uint32 modelid_prev;
    uint32 equipement_id_prev;
};

struct NPCVendorEntry
{
    uint32 entry;                                           // creature entry
    ItemTemplate const* proto;                             // item id
    uint32 maxcount;                                        // 0 for infinite
    uint32 incrtime;                                        // time for restore items amount if maxcount != 0
    uint32 ExtendedCost;
};

class Player;

class TC_GAME_API GameEventMgr
{
   private:
        GameEventMgr();
        ~GameEventMgr() = default;;

    public:
        static GameEventMgr* instance()
        {
            static GameEventMgr instance;
            return &instance;
        }

        typedef std::set<uint16> ActiveEvents;
        typedef std::vector<GameEventData> GameEventDataMap;
        ActiveEvents const& GetActiveEventList() const { return m_ActiveEvents; }
        GameEventDataMap const& GetEventMap() const { return mGameEvent; }
        bool CheckOneGameEvent(uint16 entry) const;
        uint32 NextCheck(uint16 entry) const;
        void LoadFromDB();
        void LoadVendors();
        void LoadTrainers();
        uint32 Update();
        bool IsActiveEvent(uint16 event_id) { return ( m_ActiveEvents.find(event_id)!=m_ActiveEvents.end()); }
        uint32 Initialize();
        bool StartEvent(uint16 event_id, bool overwrite = false);
        void StopEvent(uint16 event_id, bool overwrite = false);
        void HandleQuestComplete(uint32 quest_id);  // called on world event type quest completions
        uint32 GetNPCFlag(Creature * cr);
        /* Add a creature of given guid to an event (both in DB + in live memory). Return success.
        Event can be negative to have the creature spawned when event is inactive.
		If map is given, this function will also add/remove the creature from the map if found
        */
        bool AddCreatureToEvent(uint32 spawnId, int16 event_id, Map* map = nullptr);
        /* Add a gobject of given guid to an event (both in DB + in live memory).  Return success.
        Event can be negative to have the gameobject spawned when event is inactive.
		If map is given, this function will also add/remove the gobject from the map if found
         */
        bool AddGameObjectToEvent(uint32 spawnId, int16 event_id, Map* map = nullptr);
        /* Remove a creature of given guid from all events (both in DB + in live memory). If map is given, this function will also add/remove the gobject from the map if found. Return success. */
        bool RemoveCreatureFromEvent(uint32 spawnId, Map* map = nullptr);
        /* Remove a gameobject of given guid from all events (both in DB + in live memory). If map is given, this function will also add/remove the gobject from the map if found. Return success. */
        bool RemoveGameObjectFromEvent(uint32 spawnId, Map* map = nullptr);
        /* Create a new game event, both in database and live memory, return success & the id of the created event in reference */
        bool CreateGameEvent(const char* name, int16& event_id);
        /* Set start time of a game event, both in database and live memory, return success */
        //bool SetEventStartTime(uint16 event_id,time_t startTime);
        /* Set end time of a game event, both in database and live memory, return success */
        //bool SetEventEndTime(uint16 event_id,time_t endTime);
        /* Return an event_id if a given creature is linked to game_event system, null otherwise. Can be negative (despawned when event is enabled and vice versa). */
        int16 GetCreatureEvent(uint32 spawnId);
        /* Return an event_id if a given gameobject is linked to game_event system, null otherwise. Can be negative (despawned when event is enabled and vice versa). */
        int16 GetGameObjectEvent(uint32 spawnId);

        /* just a helper function returning wheter object of this event should be spawned depending if event is active or not
        Negative events can be used here too.
        */
        bool ShouldHaveObjectsSpawned(int16 event_id);

        Optional<uint32> GetEquipmentOverride(uint32 spawnId);
    private:
        void SendWorldStateUpdate(Player * plr, uint16 event_id);
        void AddActiveEvent(uint16 event_id) { m_ActiveEvents.insert(event_id); }
        void RemoveActiveEvent(uint16 event_id) { m_ActiveEvents.erase(event_id); }
        void ApplyNewEvent(uint16 event_id);
        void UnApplyEvent(uint16 event_id);
        void SpawnCreature(uint32 spawnId);
        void SpawnGameObject(uint32 spawnId);
        void GameEventSpawn(int16 event_id);
        void DespawnCreature(uint32 spawnId, uint16 event_id); //despawn due to event_id
        void DespawnGameObject(uint32 guid);
        void GameEventUnspawn(int16 event_id);
        void ChangeEquipOrModel(int16 event_id, bool activate);
        void UpdateEventQuests(uint16 event_id, bool Activate);
        void UpdateEventNPCFlags(uint16 event_id);
        void UpdateEventNPCVendor(uint16 event_id, bool activate);
        void UpdateEventNPCTrainer(uint16 event_id, bool activate);
        void UpdateBattlegroundSettings();
        void WarnAIScripts(uint16 event_id, bool activate); //TC RunSmartAIScripts
        bool CheckOneGameEventConditions(uint16 event_id);
        void SaveWorldEventStateToDB(uint16 event_id);
        bool hasCreatureQuestActiveEventExcept(uint32 quest_id, uint16 event_id);
        bool hasGameObjectQuestActiveEventExcept(uint32 quest_id, uint16 event_id);
        bool hasCreatureActiveEventExcept(uint32 creature_guid, uint16 event_id);
        bool hasGameObjectActiveEventExcept(uint32 go_guid, uint16 event_id);
    protected:
        typedef std::vector<std::list<ObjectGuid::LowType>> GameEventGuidMap;
        typedef std::pair<uint32, ModelEquip> ModelEquipPair;
        typedef std::list<ModelEquipPair> ModelEquipList;
        typedef std::vector<ModelEquipList> GameEventModelEquipMap;
        typedef std::unordered_map<uint32 /*spawnId*/, uint32 /*override equipment*/> GameEventModelEquipOverride;
        typedef std::pair<uint32, uint32> QuestRelation;
        typedef std::list<QuestRelation> QuestRelList;
        typedef std::vector<QuestRelList> GameEventQuestMap;
        typedef std::list<NPCVendorEntry> NPCVendorList;
        typedef std::vector<NPCVendorList> GameEventNPCVendorMap;
        //arr this is complicated to use, consider changing the type
        typedef std::multimap<uint16 /*event*/, std::multimap<uint32 /* trainer */, TrainerSpell /* spell */>> GameEventNPCTrainerSpellsMap;
        typedef std::map<uint32 /*quest id*/, GameEventQuestToEventConditionNum> QuestIdToEventConditionMap;
        typedef std::pair<uint32 /*guid*/, uint32 /*npcflag*/> GuidNPCFlagPair;
        typedef std::list<GuidNPCFlagPair> NPCFlagList;
        typedef std::vector<NPCFlagList> GameEventNPCFlagMap;
        typedef std::pair<uint16 /*event id*/, uint32 /*menu id*/> EventNPCGossipIdPair;
        typedef std::map<ObjectGuid::LowType /*guid*/, EventNPCGossipIdPair> GuidEventNpcGossipIdMap;
        typedef std::vector<uint32> GameEventBitmask;
        GameEventQuestMap mGameEventCreatureQuests;
        GameEventQuestMap mGameEventGameObjectQuests; 
        GameEventNPCVendorMap mGameEventVendors;
        GameEventModelEquipMap mGameEventModelEquip;
        GameEventModelEquipOverride mGameEventEquipOverrides; // Equip overrides from currently enable events
        GameEventNPCTrainerSpellsMap mGameEventTrainers;
        GameEventGuidMap  mGameEventCreatureGuids;
        GameEventGuidMap  mGameEventGameobjectGuids;
        GameEventDataMap  mGameEvent;
        GameEventBitmask  mGameEventBattlegroundHolidays;
        QuestIdToEventConditionMap mQuestToEventConditions;
        GameEventNPCFlagMap mGameEventNPCFlags;
        ActiveEvents m_ActiveEvents;
        bool isSystemInit;
};

#define sGameEventMgr GameEventMgr::instance()

#endif

