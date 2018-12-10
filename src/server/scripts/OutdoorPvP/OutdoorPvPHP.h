

#ifndef OUTDOOR_PVP_HP_
#define OUTDOOR_PVP_HP_

#include "OutdoorPvP.h"

enum DefenseMessages
{
    TEXT_OVERLOOK_TAKEN_ALLIANCE        = 14841, // '|cffffff00The Overlook has been taken by the Alliance!|r'
    TEXT_OVERLOOK_TAKEN_HORDE           = 14842, // '|cffffff00The Overlook has been taken by the Horde!|r'
    TEXT_STADIUM_TAKEN_ALLIANCE         = 14843, // '|cffffff00The Stadium has been taken by the Alliance!|r'
    TEXT_STADIUM_TAKEN_HORDE            = 14844, // '|cffffff00The Stadium has been taken by the Horde!|r'
    TEXT_BROKEN_HILL_TAKEN_ALLIANCE     = 14845, // '|cffffff00Broken Hill has been taken by the Alliance!|r'
    TEXT_BROKEN_HILL_TAKEN_HORDE        = 14846, // '|cffffff00Broken Hill has been taken by the Horde!|r'
};

#define OutdoorPvPHPBuffZonesNum 6
                                                         //  HP, citadel, ramparts, blood furnace, shattered halls, mag's lair
const uint32 OutdoorPvPHPBuffZones[OutdoorPvPHPBuffZonesNum] = { 3483, 3563, 3562, 3713, 3714, 3836 };

const uint32 AllianceBuff = 32071;

const uint32 HordeBuff = 32049;

const uint32 AlliancePlayerKillReward = 32155;

const uint32 HordePlayerKillReward = 32158;

enum OutdoorPvPHPTowerType{
    HP_TOWER_BROKEN_HILL = 0,
    HP_TOWER_OVERLOOK = 1,
    HP_TOWER_STADIUM = 2,
    HP_TOWER_NUM = 3
};

const uint32 HP_CREDITMARKER[HP_TOWER_NUM] = {19032,19028,19029};

const uint32 HP_CapturePointEvent_Enter[HP_TOWER_NUM] = {11404,11396,11388};

const uint32 HP_CapturePointEvent_Leave[HP_TOWER_NUM] = {11403,11395,11387};

enum OutdoorPvPHPWorldStates
{
    HP_UI_TOWER_DISPLAY_A       = 2490,
    HP_UI_TOWER_DISPLAY_H       = 2489,

    HP_UI_TOWER_COUNT_H         = 2478,
    HP_UI_TOWER_COUNT_A         = 2476,
};

// Broken hill, overlook, stadium
const uint32 HP_MAP_N[HP_TOWER_NUM] = { 2485,2482,2472 };

const uint32 HP_MAP_A[HP_TOWER_NUM] = { 2483,2480,2471 };

const uint32 HP_MAP_H[HP_TOWER_NUM] = { 2484,2481,2470 };

const uint32 HP_TowerArtKit_A[HP_TOWER_NUM] = {65,62,67};

const uint32 HP_TowerArtKit_H[HP_TOWER_NUM] = {64,61,68};

const uint32 HP_TowerArtKit_N[HP_TOWER_NUM] = {66,63,69};

const go_type HPCapturePoints[HP_TOWER_NUM] = 
{
    {182175,530,-471.462f,3451.09f,34.6432f,0.1745330f,0.0f,0.0f,0.087156f,0.9961950f},      // 0 - Broken Hill
    {182174,530,-184.889f,3476.93f,38.2050f,-0.017453f,0.0f,0.0f,0.008727f,-0.999962f},     // 1 - Overlook
    {182173,530,-290.016f,3702.42f,56.6729f,0.0349070f,0.0f,0.0f,0.017452f,0.9998480f}     // 2 - Stadium
};

const go_type HPTowerFlags[HP_TOWER_NUM] = 
{
    {183514,530,-467.078f,3528.17f,64.7121f,3.14159f,0.0f,0.0f,1.0f,0.0f},  // 0 broken hill
    {182525,530,-187.887f,3459.38f,60.0403f,-3.12414f,0.0f,0.0f,0.999962f,-0.008727f}, // 1 overlook
    {183515,530,-289.610f,3696.83f,75.9447f,3.12414f,0.0f,0.0f,0.999962f,0.008727f} // 2 stadium
};

class OPvPCapturePointHP : public OPvPCapturePoint
{
public:
    OPvPCapturePointHP(OutdoorPvP * pvp, OutdoorPvPHPTowerType type);
    //bool Update(uint32 diff);
    void ChangeState() override;
    void FillInitialWorldStates(WorldPacket & data) override;
private:
    OutdoorPvPHPTowerType m_TowerType;
};

class OutdoorPvPHP : public OutdoorPvP
{
friend class OPvPCapturePointHP;
public:
    OutdoorPvPHP();
    bool SetupOutdoorPvP();
    void HandlePlayerEnterZone(Player *plr, uint32 zone);
    void HandlePlayerLeaveZone(Player *plr, uint32 zone);
    bool Update(uint32 diff);
    void FillInitialWorldStates(WorldPacket &data);
    void SendRemoveWorldStates(Player * plr);
    void HandleKillImpl(Player * plr, Unit * killed);
    void BuffTeam(uint32 team);

	uint32 GetAllianceTowersControlled() const;
	void SetAllianceTowersControlled(uint32 count);

	uint32 GetHordeTowersControlled() const;
	void SetHordeTowersControlled(uint32 count);

private:
    // how many towers are controlled
    uint32 m_AllianceTowersControlled;
    uint32 m_HordeTowersControlled;
};

#endif

