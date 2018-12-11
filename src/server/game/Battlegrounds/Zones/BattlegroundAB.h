
#ifndef __BATTLEGROUNDAB_H
#define __BATTLEGROUNDAB_H

#include "Battleground.h"

enum BG_AB_WorldStates
{
    BG_AB_OP_OCCUPIED_BASES_HORDE       = 1778,
    BG_AB_OP_OCCUPIED_BASES_ALLY        = 1779,
    BG_AB_OP_RESOURCES_ALLY             = 1776,
    BG_AB_OP_RESOURCES_HORDE            = 1777,
    BG_AB_OP_RESOURCES_MAX              = 1780,
    BG_AB_OP_RESOURCES_WARNING          = 1955,
/*
    BG_AB_OP_STABLE_ICON                = 1842,             //Stable map icon (NONE)
    BG_AB_OP_STABLE_STATE_ALIENCE       = 1767,             //Stable map state (ALIENCE)
    BG_AB_OP_STABLE_STATE_HORDE         = 1768,             //Stable map state (HORDE)
    BG_AB_OP_STABLE_STATE_CON_ALI       = 1769,             //Stable map state (CON ALIENCE)
    BG_AB_OP_STABLE_STATE_CON_HOR       = 1770,             //Stable map state (CON HORDE)
    BG_AB_OP_FARM_ICON                  = 1845,             //Farm map icon (NONE)
    BG_AB_OP_FARM_STATE_ALIENCE         = 1772,             //Farm state (ALIENCE)
    BG_AB_OP_FARM_STATE_HORDE           = 1773,             //Farm state (HORDE)
    BG_AB_OP_FARM_STATE_CON_ALI         = 1774,             //Farm state (CON ALIENCE)
    BG_AB_OP_FARM_STATE_CON_HOR         = 1775,             //Farm state (CON HORDE)

    BG_AB_OP_BLACKSMITH_ICON            = 1846,             //Blacksmith map icon (NONE)
    BG_AB_OP_BLACKSMITH_STATE_ALIENCE   = 1782,             //Blacksmith map state (ALIENCE)
    BG_AB_OP_BLACKSMITH_STATE_HORDE     = 1783,             //Blacksmith map state (HORDE)
    BG_AB_OP_BLACKSMITH_STATE_CON_ALI   = 1784,             //Blacksmith map state (CON ALIENCE)
    BG_AB_OP_BLACKSMITH_STATE_CON_HOR   = 1785,             //Blacksmith map state (CON HORDE)
    BG_AB_OP_LUMBERMILL_ICON            = 1844,             //Lumber Mill map icon (NONE)
    BG_AB_OP_LUMBERMILL_STATE_ALIENCE   = 1792,             //Lumber Mill map state (ALIENCE)
    BG_AB_OP_LUMBERMILL_STATE_HORDE     = 1793,             //Lumber Mill map state (HORDE)
    BG_AB_OP_LUMBERMILL_STATE_CON_ALI   = 1794,             //Lumber Mill map state (CON ALIENCE)
    BG_AB_OP_LUMBERMILL_STATE_CON_HOR   = 1795,             //Lumber Mill map state (CON HORDE)
    BG_AB_OP_GOLDMINE_ICON              = 1843,             //Gold Mine map icon (NONE)
    BG_AB_OP_GOLDMINE_STATE_ALIENCE     = 1787,             //Gold Mine map state (ALIENCE)
    BG_AB_OP_GOLDMINE_STATE_HORDE       = 1788,             //Gold Mine map state (HORDE)
    BG_AB_OP_GOLDMINE_STATE_CON_ALI     = 1789,             //Gold Mine map state (CON ALIENCE
    BG_AB_OP_GOLDMINE_STATE_CON_HOR     = 1790,             //Gold Mine map state (CON HORDE)
*/
};

const uint32 BG_AB_OP_NODESTATES[5] =    {1767, 1782, 1772, 1792, 1787};

const uint32 BG_AB_OP_NODEICONS[5]  =    {1842, 1846, 1845, 1844, 1843};

/* Note: code uses that these IDs follow each other */
/*enum BG_AB_NodeObjectId
{
    BG_AB_OBJECTID_NODE_BANNER_0    = 180087,       // Stables banner
    BG_AB_OBJECTID_NODE_BANNER_1    = 180088,       // Blacksmith banner
    BG_AB_OBJECTID_NODE_BANNER_2    = 180089,       // Farm banner
    BG_AB_OBJECTID_NODE_BANNER_3    = 180090,       // Lumber mill banner
    BG_AB_OBJECTID_NODE_BANNER_4    = 180091        // Gold mine banner
};*/

enum BG_AB_ObjectType
{
    // for all 5 node points 8*5=40 objects
    BG_AB_OBJECT_BANNER_NEUTRAL     = 0,
    BG_AB_OBJECT_BANNER_CONT_A      = 1,
    BG_AB_OBJECT_BANNER_CONT_H      = 2,
    BG_AB_OBJECT_BANNER_ALLY        = 3,
    BG_AB_OBJECT_BANNER_HORDE       = 4,
    BG_AB_OBJECT_AURA_ALLY          = 5,
    BG_AB_OBJECT_AURA_HORDE         = 6,
    BG_AB_OBJECT_AURA_CONTESTED     = 7,
    //gates
    BG_AB_OBJECT_GATE_A             = 40,
    BG_AB_OBJECT_GATE_H             = 41,
    //buffs
    BG_AB_OBJECT_SPEEDBUFF_STABLES       = 42,
    BG_AB_OBJECT_REGENBUFF_STABLES       = 43,
    BG_AB_OBJECT_BERSERKBUFF_STABLES     = 44,
    BG_AB_OBJECT_SPEEDBUFF_BLACKSMITH    = 45,
    BG_AB_OBJECT_REGENBUFF_BLACKSMITH    = 46,
    BG_AB_OBJECT_BERSERKBUFF_BLACKSMITH  = 47,
    BG_AB_OBJECT_SPEEDBUFF_FARM          = 48,
    BG_AB_OBJECT_REGENBUFF_FARM          = 49,
    BG_AB_OBJECT_BERSERKBUFF_FARM        = 50,
    BG_AB_OBJECT_SPEEDBUFF_LUMBER_MILL   = 51,
    BG_AB_OBJECT_REGENBUFF_LUMBER_MILL   = 52,
    BG_AB_OBJECT_BERSERKBUFF_LUMBER_MILL = 53,
    BG_AB_OBJECT_SPEEDBUFF_GOLD_MINE     = 54,
    BG_AB_OBJECT_REGENBUFF_GOLD_MINE     = 55,
    BG_AB_OBJECT_BERSERKBUFF_GOLD_MINE   = 56,
    BG_AB_OBJECT_MAX                     = 57,
};

/* Object id templates from DB */
/*enum BG_AB_ObjectTypes
{
    BG_AB_OBJECTID_BANNER_A             = 180058,
    BG_AB_OBJECTID_BANNER_CONT_A        = 180059,
    BG_AB_OBJECTID_BANNER_H             = 180060,
    BG_AB_OBJECTID_BANNER_CONT_H        = 180061,

    BG_AB_OBJECTID_AURA_A               = 180100,
    BG_AB_OBJECTID_AURA_H               = 180101,
    BG_AB_OBJECTID_AURA_C               = 180102,

    BG_AB_OBJECTID_GATE_A               = 180255,
    BG_AB_OBJECTID_GATE_H               = 180256
};*/

enum BG_AB_Timers
{
    BG_AB_FLAG_CAPTURING_TIME           = 60000,
};

enum BG_AB_Score
{
    BG_AB_MAX_TEAM_SCORE                = 2000,
    BG_AB_WARNING_SCORE                 = 1800
};

/* do NOT change the order, else wrong behaviour */
enum BG_AB_BattlegroundNodes
{
    BG_AB_NODE_STABLES          = 0,
    BG_AB_NODE_BLACKSMITH       = 1,
    BG_AB_NODE_FARM             = 2,
    BG_AB_NODE_LUMBER_MILL      = 3,
    BG_AB_NODE_GOLD_MINE        = 4,

    BG_AB_DYNAMIC_NODES_COUNT   = 5,                        // dynamic nodes that can be captured

    BG_AB_SPIRIT_ALIANCE        = 5,
    BG_AB_SPIRIT_HORDE          = 6,

    BG_AB_ALL_NODES_COUNT       = 7,                        // all nodes (dynamic and static)
};

enum BG_AB_NodeStatus
{
    BG_AB_NODE_TYPE_NEUTRAL             = 0,
    BG_AB_NODE_TYPE_CONTESTED           = 1,
    BG_AB_NODE_STATUS_ALLY_CONTESTED    = 1,
    BG_AB_NODE_STATUS_HORDE_CONTESTED   = 2,
    BG_AB_NODE_TYPE_OCCUPIED            = 3,
    BG_AB_NODE_STATUS_ALLY_OCCUPIED     = 3,
    BG_AB_NODE_STATUS_HORDE_OCCUPIED    = 4
};

enum BG_AB_Sounds
{
    SOUND_NODE_CLAIMED                  = 8192,
    SOUND_NODE_CAPTURED_ALLIANCE        = 8173,
    SOUND_NODE_CAPTURED_HORDE           = 8213,
    SOUND_NODE_ASSAULTED_ALLIANCE       = 8174,
    SOUND_NODE_ASSAULTED_HORDE          = 8212,
    SOUND_NEAR_VICTORY                  = 8456
};

// Tick intervals and given points: case 0,1,2,3,4,5 captured nodes
const uint32 BG_AB_TickIntervals[6] = {0, 12000, 9000, 6000, 3000, 1000};
const uint32 BG_AB_TickPoints[6] = {0, 10, 10, 10, 10, 30};

// WorldSafeLocs ids for 5 nodes, and for ally, and horde starting location
const uint32 BG_AB_GraveyardIds[BG_AB_ALL_NODES_COUNT] = {895, 894, 893, 897, 896, 898, 899};

// x, y, z, o
const float BG_AB_BuffPositions[BG_AB_DYNAMIC_NODES_COUNT][4] = {
    { 1185.566f, 1184.629f, -56.36329f, 2.303831f },         // stables
    { 990.1131f, 1008.73f,  -42.60328f, 0.820303f },         // blacksmith
    { 818.0089f, 842.3543f, -56.54062f, 3.176533f },         // farm
    { 808.8463f, 1185.417f,  11.92161f, 5.619962f },         // lumber mill
    { 1147.091f, 816.8362f, -98.39896f, 6.056293f }          // gold mine
};

struct BG_AB_BannerTimer
{
    uint32      timer;
    uint8       type;
    uint8       teamIndex;
};

class BattlegroundABScore : public BattlegroundScore
{
    public:
        BattlegroundABScore(): BasesAssaulted(0), BasesDefended(0) {};
        ~BattlegroundABScore() override {};
        uint32 BasesAssaulted;
        uint32 BasesDefended;
};

class BattlegroundAB : public Battleground
{
    friend class BattlegroundMgr;

    public:
        BattlegroundAB();
        ~BattlegroundAB() override;

        void Update(time_t diff) override;
        void AddPlayer(Player *plr) override;
        void RemovePlayer(Player *plr,ObjectGuid guid) override;
        void HandleAreaTrigger(Player *Source, uint32 Trigger) override;
        bool SetupBattleground() override;
        void Reset() override;
        WorldSafeLocsEntry const* GetClosestGraveYard(float x, float y, float z, uint32 team) override;
        void StartingEventCloseDoors() override;
        void StartingEventOpenDoors() override;

        /* Scorekeeping */
        void UpdatePlayerScore(Player *Source, uint32 type, uint32 value) override;

        void FillInitialWorldStates(WorldPacket& data) override;

        /* Nodes occupying */
        void EventPlayerClickedOnFlag(Player *source, GameObject* target_obj) override;

    private:
        /* Gameobject spawning/despawning */
        void _CreateBanner(uint8 node, uint8 type, uint8 teamIndex, Player* invoker = nullptr);
        void _DelBanner(uint8 node, uint8 type, uint8 teamIndex, Player* invoker = nullptr);
        void _SendNodeUpdate(uint8 node);

        void _NodeOccupied(uint8 node,Team team);
        void _NodeDeOccupied(uint8 node);

        int32 _GetNodeNameId(uint8 node);

        /* Nodes info:
            0: neutral
            1: ally contested
            2: horde contested
            3: ally occupied
            4: horde occupied     */
        uint8             m_Nodes[BG_AB_DYNAMIC_NODES_COUNT];
        uint8             m_prevNodes[BG_AB_DYNAMIC_NODES_COUNT];
        BG_AB_BannerTimer m_BannerTimers[BG_AB_DYNAMIC_NODES_COUNT];
        int32             m_NodeTimers[BG_AB_DYNAMIC_NODES_COUNT];
        uint32            m_TeamScores[2];
        uint32            m_lastTick[2];
        uint32            m_HonorScoreTics[2];
        uint32            m_ReputationScoreTics[2];
        bool              m_IsInformedNearVictory;
};
#endif

