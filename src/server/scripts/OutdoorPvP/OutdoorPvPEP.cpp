
#include "OutdoorPvPEP.h"
#include "WorldPacket.h"
#include "Player.h"
#include "GameObject.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "Creature.h"
#include "Language.h"
#include "World.h"
#include "GossipDef.h"
#include "ScriptMgr.h"
#include "MapManager.h"

OPvPCapturePointEP_EWT::OPvPCapturePointEP_EWT(OutdoorPvP *pvp)
: OPvPCapturePoint(pvp), m_TowerState(EP_TS_N), m_UnitsSummonedSide(0)
{
    SetCapturePointData(EPCapturePoints[EP_EWT].entry,EPCapturePoints[EP_EWT].map,EPCapturePoints[EP_EWT].x,EPCapturePoints[EP_EWT].y,EPCapturePoints[EP_EWT].z,EPCapturePoints[EP_EWT].o,EPCapturePoints[EP_EWT].rot0,EPCapturePoints[EP_EWT].rot1,EPCapturePoints[EP_EWT].rot2,EPCapturePoints[EP_EWT].rot3);
    AddObject(EP_EWT_FLAGS,EPTowerFlags[EP_EWT].entry,EPTowerFlags[EP_EWT].map,EPTowerFlags[EP_EWT].x,EPTowerFlags[EP_EWT].y,EPTowerFlags[EP_EWT].z,EPTowerFlags[EP_EWT].o,EPTowerFlags[EP_EWT].rot0,EPTowerFlags[EP_EWT].rot1,EPTowerFlags[EP_EWT].rot2,EPTowerFlags[EP_EWT].rot3);
}

void OPvPCapturePointEP_EWT::ChangeState()
{
    // if changing from controlling alliance to horde or vice versa
    if (m_OldState == OBJECTIVESTATE_ALLIANCE && m_OldState != m_State)
        ((OutdoorPvPEP*)m_PvP)->EP_Controls[EP_EWT] = 0;
    else if (m_OldState == OBJECTIVESTATE_HORDE && m_OldState != m_State)
        ((OutdoorPvPEP*)m_PvP)->EP_Controls[EP_EWT] = 0;

    uint32 artkit = 21;

    switch (m_State)
    {
    case OBJECTIVESTATE_ALLIANCE:
        if (m_value == m_maxValue)
            m_TowerState = EP_TS_A;
        else
            m_TowerState = EP_TS_A_P;
        artkit = 2;
        SummonSupportUnitAtNorthpassTower(ALLIANCE);
        ((OutdoorPvPEP*)m_PvP)->EP_Controls[EP_EWT] = ALLIANCE;
        if (m_OldState != m_State)
            m_PvP->SendDefenseMessage(EP_GraveyardZone, TEXT_EASTWALL_TOWER_TAKEN_ALLIANCE);
        break;
    case OBJECTIVESTATE_HORDE:
        if (m_value == -m_maxValue)
            m_TowerState = EP_TS_H;
        else
            m_TowerState = EP_TS_H_P;
        artkit = 1;
        SummonSupportUnitAtNorthpassTower(HORDE);
        ((OutdoorPvPEP*)m_PvP)->EP_Controls[EP_EWT] = HORDE;
        if (m_OldState != m_State)
            m_PvP->SendDefenseMessage(EP_GraveyardZone, TEXT_EASTWALL_TOWER_TAKEN_HORDE);
        break;
    case OBJECTIVESTATE_NEUTRAL:
        m_TowerState = EP_TS_N;
        break;
    case OBJECTIVESTATE_NEUTRAL_ALLIANCE_CHALLENGE:
    case OBJECTIVESTATE_HORDE_ALLIANCE_CHALLENGE:
        m_TowerState = EP_TS_N_A;
        break;
    case OBJECTIVESTATE_NEUTRAL_HORDE_CHALLENGE:
    case OBJECTIVESTATE_ALLIANCE_HORDE_CHALLENGE:
        m_TowerState = EP_TS_N_H;
        break;
    }

    Map* map = sMapMgr->FindMap(0, 0);
    auto bounds = map->GetGameObjectBySpawnIdStore().equal_range(m_capturePointSpawnId);
    for (auto itr = bounds.first; itr != bounds.second; ++itr)
        itr->second->SetGoArtKit(artkit);

    auto bounds2 = map->GetGameObjectBySpawnIdStore().equal_range(m_Objects[EP_EWT_FLAGS]);
    for (auto itr = bounds2.first; itr != bounds2.second; ++itr)
        itr->second->SetGoArtKit(artkit);

    GameObject* flag = m_capturePoint;
    GameObject* flag2 = m_PvP->GetMap()->GetGameObject(m_Objects[EP_EWT_FLAGS]);
    if (flag)
    {
        flag->SetGoArtKit(artkit);
    }
    if (flag2)
    {
        flag2->SetGoArtKit(artkit);
    }


    UpdateTowerState();

    // complete quest objective
    if (m_TowerState == EP_TS_H_P || m_TowerState == EP_TS_A_P)
        SendObjectiveComplete(EP_EWT_CM, ObjectGuid::Empty);
}

void OPvPCapturePointEP_EWT::FillInitialWorldStates(WorldPacket &data)
{
    data << EP_EWT_A << uint32(bool(m_TowerState & EP_TS_A));
    data << EP_EWT_H << uint32(bool(m_TowerState & EP_TS_H));
    data << EP_EWT_A_P << uint32(bool(m_TowerState & EP_TS_A_P));
    data << EP_EWT_H_P << uint32(bool(m_TowerState & EP_TS_H_P));
    data << EP_EWT_N_A << uint32(bool(m_TowerState & EP_TS_N_A));
    data << EP_EWT_N_H << uint32(bool(m_TowerState & EP_TS_N_H));
    data << EP_EWT_N << uint32(bool(m_TowerState & EP_TS_N));
}

void OPvPCapturePointEP_EWT::UpdateTowerState()
{
    m_PvP->SendUpdateWorldState(EP_EWT_A , bool(m_TowerState & EP_TS_A));
    m_PvP->SendUpdateWorldState(EP_EWT_H , bool(m_TowerState & EP_TS_H));
    m_PvP->SendUpdateWorldState(EP_EWT_A_P , bool(m_TowerState & EP_TS_A_P));
    m_PvP->SendUpdateWorldState(EP_EWT_H_P , bool(m_TowerState & EP_TS_H_P));
    m_PvP->SendUpdateWorldState(EP_EWT_N_A , bool(m_TowerState & EP_TS_N_A));
    m_PvP->SendUpdateWorldState(EP_EWT_N_H , bool(m_TowerState & EP_TS_N_H));
    m_PvP->SendUpdateWorldState(EP_EWT_N , bool(m_TowerState & EP_TS_N));
}

void OPvPCapturePointEP_EWT::SummonSupportUnitAtNorthpassTower(uint32 team)
{
    if(m_UnitsSummonedSide != team)
    {
        m_UnitsSummonedSide = team;
        const creature_type * ct = nullptr;
        if(team == ALLIANCE)
            ct=EP_EWT_Summons_A;
        else
            ct=EP_EWT_Summons_H;

        for(int i = 0; i < EP_EWT_NUM_CREATURES; ++i)
        {
            DelCreature(i);
            AddCreature(i,ct[i].entry,ct[i].map,ct[i].x,ct[i].y,ct[i].z,ct[i].o,1000000);
        }
    }
}

// NPT
OPvPCapturePointEP_NPT::OPvPCapturePointEP_NPT(OutdoorPvP *pvp)
: OPvPCapturePoint(pvp), m_TowerState(EP_TS_N), m_SummonedGOSide(0)
{
    SetCapturePointData(EPCapturePoints[EP_NPT].entry,EPCapturePoints[EP_NPT].map,EPCapturePoints[EP_NPT].x,EPCapturePoints[EP_NPT].y,EPCapturePoints[EP_NPT].z,EPCapturePoints[EP_NPT].o,EPCapturePoints[EP_NPT].rot0,EPCapturePoints[EP_NPT].rot1,EPCapturePoints[EP_NPT].rot2,EPCapturePoints[EP_NPT].rot3);
    AddObject(EP_NPT_FLAGS,EPTowerFlags[EP_NPT].entry,EPTowerFlags[EP_NPT].map,EPTowerFlags[EP_NPT].x,EPTowerFlags[EP_NPT].y,EPTowerFlags[EP_NPT].z,EPTowerFlags[EP_NPT].o,EPTowerFlags[EP_NPT].rot0,EPTowerFlags[EP_NPT].rot1,EPTowerFlags[EP_NPT].rot2,EPTowerFlags[EP_NPT].rot3);
}

void OPvPCapturePointEP_NPT::ChangeState()
{
    // if changing from controlling alliance to horde or vice versa
    if (m_OldState == OBJECTIVESTATE_ALLIANCE && m_OldState != m_State)
        ((OutdoorPvPEP*)m_PvP)->EP_Controls[EP_NPT] = 0;
    else if (m_OldState == OBJECTIVESTATE_HORDE && m_OldState != m_State)
        ((OutdoorPvPEP*)m_PvP)->EP_Controls[EP_NPT] = 0;

    uint32 artkit = 21;

    switch (m_State)
    {
    case OBJECTIVESTATE_ALLIANCE:
        if (m_value == m_maxValue)
            m_TowerState = EP_TS_A;
        else
            m_TowerState = EP_TS_A_P;
        artkit = 2;
        SummonGO(ALLIANCE);
        ((OutdoorPvPEP*)m_PvP)->EP_Controls[EP_NPT] = ALLIANCE;
        if (m_OldState != m_State) 
            m_PvP->SendDefenseMessage(EP_GraveyardZone, TEXT_NORTHPASS_TOWER_TAKEN_ALLIANCE);
        break;
    case OBJECTIVESTATE_HORDE:
        if (m_value == -m_maxValue)
            m_TowerState = EP_TS_H;
        else
            m_TowerState = EP_TS_H_P;
        artkit = 1;
        SummonGO(HORDE);
        ((OutdoorPvPEP*)m_PvP)->EP_Controls[EP_NPT] = HORDE;
        if (m_OldState != m_State) 
            m_PvP->SendDefenseMessage(EP_GraveyardZone, TEXT_NORTHPASS_TOWER_TAKEN_HORDE);
        break;
    case OBJECTIVESTATE_NEUTRAL:
        m_TowerState = EP_TS_N;
        m_SummonedGOSide = 0;
        DelObject(EP_NPT_BUFF);
        break;
    case OBJECTIVESTATE_NEUTRAL_ALLIANCE_CHALLENGE:
    case OBJECTIVESTATE_HORDE_ALLIANCE_CHALLENGE:
        m_TowerState = EP_TS_N_A;
        break;
    case OBJECTIVESTATE_NEUTRAL_HORDE_CHALLENGE:
    case OBJECTIVESTATE_ALLIANCE_HORDE_CHALLENGE:
        m_TowerState = EP_TS_N_H;
        break;
    }

    Map* map = sMapMgr->FindMap(0, 0);
    auto bounds = map->GetGameObjectBySpawnIdStore().equal_range(m_capturePointSpawnId);
    for (auto itr = bounds.first; itr != bounds.second; ++itr)
        itr->second->SetGoArtKit(artkit);

    bounds = map->GetGameObjectBySpawnIdStore().equal_range(m_Objects[EP_NPT_FLAGS]);
    for (auto itr = bounds.first; itr != bounds.second; ++itr)
        itr->second->SetGoArtKit(artkit);

    UpdateTowerState();

    // complete quest objective
    if (m_TowerState == EP_TS_H_P || m_TowerState == EP_TS_A_P)
        SendObjectiveComplete(EP_NPT_CM, ObjectGuid::Empty);
}

void OPvPCapturePointEP_NPT::FillInitialWorldStates(WorldPacket &data)
{
    data << EP_NPT_A << uint32(bool(m_TowerState & EP_TS_A));
    data << EP_NPT_H << uint32(bool(m_TowerState & EP_TS_H));
    data << EP_NPT_A_P << uint32(bool(m_TowerState & EP_TS_A_P));
    data << EP_NPT_H_P << uint32(bool(m_TowerState & EP_TS_H_P));
    data << EP_NPT_N_A << uint32(bool(m_TowerState & EP_TS_N_A));
    data << EP_NPT_N_H << uint32(bool(m_TowerState & EP_TS_N_H));
    data << EP_NPT_N << uint32(bool(m_TowerState & EP_TS_N));
}

void OPvPCapturePointEP_NPT::UpdateTowerState()
{
    m_PvP->SendUpdateWorldState(EP_NPT_A , bool(m_TowerState & EP_TS_A));
    m_PvP->SendUpdateWorldState(EP_NPT_H , bool(m_TowerState & EP_TS_H));
    m_PvP->SendUpdateWorldState(EP_NPT_A_P , bool(m_TowerState & EP_TS_A_P));
    m_PvP->SendUpdateWorldState(EP_NPT_H_P , bool(m_TowerState & EP_TS_H_P));
    m_PvP->SendUpdateWorldState(EP_NPT_N_A , bool(m_TowerState & EP_TS_N_A));
    m_PvP->SendUpdateWorldState(EP_NPT_N_H , bool(m_TowerState & EP_TS_N_H));
    m_PvP->SendUpdateWorldState(EP_NPT_N , bool(m_TowerState & EP_TS_N));
}

void OPvPCapturePointEP_NPT::SummonGO(uint32 team)
{
    if(m_SummonedGOSide != team)
    {
        m_SummonedGOSide = team;
        DelObject(EP_NPT_BUFF);
        AddObject(EP_NPT_BUFF,EP_NPT_LordaeronShrine.entry,EP_NPT_LordaeronShrine.map,EP_NPT_LordaeronShrine.x,EP_NPT_LordaeronShrine.y,EP_NPT_LordaeronShrine.z,EP_NPT_LordaeronShrine.o,EP_NPT_LordaeronShrine.rot0,EP_NPT_LordaeronShrine.rot1,EP_NPT_LordaeronShrine.rot2,EP_NPT_LordaeronShrine.rot3);
        GameObject * go = m_PvP->GetMap()->GetGameObject(m_Objects[EP_NPT_BUFF]);
        if(go)
            go->SetUInt32Value(GAMEOBJECT_FACTION,(team == ALLIANCE ? 84 : 83));
    }
}

// CGT
OPvPCapturePointEP_CGT::OPvPCapturePointEP_CGT(OutdoorPvP *pvp)
: OPvPCapturePoint(pvp), m_TowerState(EP_TS_N), m_GraveyardSide(0)
{
    SetCapturePointData(EPCapturePoints[EP_CGT].entry,EPCapturePoints[EP_CGT].map,EPCapturePoints[EP_CGT].x,EPCapturePoints[EP_CGT].y,EPCapturePoints[EP_CGT].z,EPCapturePoints[EP_CGT].o,EPCapturePoints[EP_CGT].rot0,EPCapturePoints[EP_CGT].rot1,EPCapturePoints[EP_CGT].rot2,EPCapturePoints[EP_CGT].rot3);
    AddObject(EP_CGT_FLAGS,EPTowerFlags[EP_CGT].entry,EPTowerFlags[EP_CGT].map,EPTowerFlags[EP_CGT].x,EPTowerFlags[EP_CGT].y,EPTowerFlags[EP_CGT].z,EPTowerFlags[EP_CGT].o,EPTowerFlags[EP_CGT].rot0,EPTowerFlags[EP_CGT].rot1,EPTowerFlags[EP_CGT].rot2,EPTowerFlags[EP_CGT].rot3);
}

void OPvPCapturePointEP_CGT::ChangeState()
{
    // if changing from controlling alliance to horde or vice versa
    if (m_OldState == OBJECTIVESTATE_ALLIANCE && m_OldState != m_State)
        ((OutdoorPvPEP*)m_PvP)->EP_Controls[EP_CGT] = 0;
    else if (m_OldState == OBJECTIVESTATE_HORDE && m_OldState != m_State)
        ((OutdoorPvPEP*)m_PvP)->EP_Controls[EP_CGT] = 0;

    uint32 artkit = 21;

    switch (m_State)
    {
    case OBJECTIVESTATE_ALLIANCE:
        if (m_value == m_maxValue)
            m_TowerState = EP_TS_A;
        else
            m_TowerState = EP_TS_A_P;
        artkit = 2;
        LinkGraveYard(ALLIANCE);
        ((OutdoorPvPEP*)m_PvP)->EP_Controls[EP_CGT] = ALLIANCE;
        m_PvP->SendDefenseMessage(EP_GraveyardZone, TEXT_CROWN_GUARD_TOWER_TAKEN_ALLIANCE);
        break;
    case OBJECTIVESTATE_HORDE:
        if (m_value == -m_maxValue)
            m_TowerState = EP_TS_H;
        else
            m_TowerState = EP_TS_H_P;
        artkit = 1;
        LinkGraveYard(HORDE);
        ((OutdoorPvPEP*)m_PvP)->EP_Controls[EP_CGT] = HORDE;
        m_PvP->SendDefenseMessage(EP_GraveyardZone, TEXT_CROWN_GUARD_TOWER_TAKEN_HORDE);
        break;
    case OBJECTIVESTATE_NEUTRAL:
        m_TowerState = EP_TS_N;
        break;
    case OBJECTIVESTATE_NEUTRAL_ALLIANCE_CHALLENGE:
    case OBJECTIVESTATE_HORDE_ALLIANCE_CHALLENGE:
        m_TowerState = EP_TS_N_A;
        break;
    case OBJECTIVESTATE_NEUTRAL_HORDE_CHALLENGE:
    case OBJECTIVESTATE_ALLIANCE_HORDE_CHALLENGE:
        m_TowerState = EP_TS_N_H;
        break;
    }

    Map* map = sMapMgr->FindMap(0, 0);
    auto bounds = map->GetGameObjectBySpawnIdStore().equal_range(m_capturePointSpawnId);
    for (auto itr = bounds.first; itr != bounds.second; ++itr)
        itr->second->SetGoArtKit(artkit);

    bounds = map->GetGameObjectBySpawnIdStore().equal_range(m_Objects[EP_CGT_FLAGS]);
    for (auto itr = bounds.first; itr != bounds.second; ++itr)
        itr->second->SetGoArtKit(artkit);

    UpdateTowerState();

    // complete quest objective
    if (m_TowerState == EP_TS_H_P || m_TowerState == EP_TS_A_P)
        SendObjectiveComplete(EP_CGT_CM, ObjectGuid::Empty);
}

void OPvPCapturePointEP_CGT::FillInitialWorldStates(WorldPacket &data)
{
    data << EP_CGT_A << uint32(bool(m_TowerState & EP_TS_A));
    data << EP_CGT_H << uint32(bool(m_TowerState & EP_TS_H));
    data << EP_CGT_A_P << uint32(bool(m_TowerState & EP_TS_A_P));
    data << EP_CGT_H_P << uint32(bool(m_TowerState & EP_TS_H_P));
    data << EP_CGT_N_A << uint32(bool(m_TowerState & EP_TS_N_A));
    data << EP_CGT_N_H << uint32(bool(m_TowerState & EP_TS_N_H));
    data << EP_CGT_N << uint32(bool(m_TowerState & EP_TS_N));
}

void OPvPCapturePointEP_CGT::UpdateTowerState()
{
    m_PvP->SendUpdateWorldState(EP_CGT_A , bool(m_TowerState & EP_TS_A));
    m_PvP->SendUpdateWorldState(EP_CGT_H , bool(m_TowerState & EP_TS_H));
    m_PvP->SendUpdateWorldState(EP_CGT_A_P , bool(m_TowerState & EP_TS_A_P));
    m_PvP->SendUpdateWorldState(EP_CGT_H_P , bool(m_TowerState & EP_TS_H_P));
    m_PvP->SendUpdateWorldState(EP_CGT_N_A , bool(m_TowerState & EP_TS_N_A));
    m_PvP->SendUpdateWorldState(EP_CGT_N_H , bool(m_TowerState & EP_TS_N_H));
    m_PvP->SendUpdateWorldState(EP_CGT_N , bool(m_TowerState & EP_TS_N));
}

void OPvPCapturePointEP_CGT::LinkGraveYard(uint32 team)
{
    if(m_GraveyardSide != team)
    {
        m_GraveyardSide = team;
        sObjectMgr->RemoveGraveYardLink(EP_GraveYardId,EP_GraveyardZone,team,false);
        sObjectMgr->AddGraveYardLink(EP_GraveYardId,EP_GraveyardZone,team,false);
    }
}

// PWT
OPvPCapturePointEP_PWT::OPvPCapturePointEP_PWT(OutdoorPvP *pvp)
: OPvPCapturePoint(pvp), m_TowerState(EP_TS_N), m_FlightMasterSpawned(0)
{
    SetCapturePointData(EPCapturePoints[EP_PWT].entry,EPCapturePoints[EP_PWT].map,EPCapturePoints[EP_PWT].x,EPCapturePoints[EP_PWT].y,EPCapturePoints[EP_PWT].z,EPCapturePoints[EP_PWT].o,EPCapturePoints[EP_PWT].rot0,EPCapturePoints[EP_PWT].rot1,EPCapturePoints[EP_PWT].rot2,EPCapturePoints[EP_PWT].rot3);
    AddObject(EP_PWT_FLAGS,EPTowerFlags[EP_PWT].entry,EPTowerFlags[EP_PWT].map,EPTowerFlags[EP_PWT].x,EPTowerFlags[EP_PWT].y,EPTowerFlags[EP_PWT].z,EPTowerFlags[EP_PWT].o,EPTowerFlags[EP_PWT].rot0,EPTowerFlags[EP_PWT].rot1,EPTowerFlags[EP_PWT].rot2,EPTowerFlags[EP_PWT].rot3);
}

void OPvPCapturePointEP_PWT::ChangeState()
{
    // if changing from controlling alliance to horde or vice versa
    if (m_OldState == OBJECTIVESTATE_ALLIANCE && m_OldState != m_State)
        ((OutdoorPvPEP*)m_PvP)->EP_Controls[EP_PWT] = 0;
    else if (m_OldState == OBJECTIVESTATE_HORDE && m_OldState != m_State)
        ((OutdoorPvPEP*)m_PvP)->EP_Controls[EP_PWT] = 0;

    uint32 artkit = 21;

    switch (m_State)
    {
    case OBJECTIVESTATE_ALLIANCE:
        if (m_value == m_maxValue)
            m_TowerState = EP_TS_A;
        else
            m_TowerState = EP_TS_A_P;
        SummonFlightMaster(ALLIANCE);
        artkit = 2;
        ((OutdoorPvPEP*)m_PvP)->EP_Controls[EP_PWT] = ALLIANCE;
        if (m_OldState != m_State)
            m_PvP->SendDefenseMessage(EP_GraveyardZone, TEXT_EASTWALL_TOWER_TAKEN_ALLIANCE);
        break;
    case OBJECTIVESTATE_HORDE:
        if (m_value == -m_maxValue)
            m_TowerState = EP_TS_H;
        else
            m_TowerState = EP_TS_H_P;
        SummonFlightMaster(HORDE);
        artkit = 1;
        ((OutdoorPvPEP*)m_PvP)->EP_Controls[EP_PWT] = HORDE;
        if (m_OldState != m_State)
            m_PvP->SendDefenseMessage(EP_GraveyardZone, TEXT_EASTWALL_TOWER_TAKEN_HORDE);
        break;
    case OBJECTIVESTATE_NEUTRAL:
        m_TowerState = EP_TS_N;
        DelCreature(EP_PWT_FLIGHTMASTER);
        m_FlightMasterSpawned = 0;
        break;
    case OBJECTIVESTATE_NEUTRAL_ALLIANCE_CHALLENGE:
    case OBJECTIVESTATE_HORDE_ALLIANCE_CHALLENGE:
        m_TowerState = EP_TS_N_A;
        break;
    case OBJECTIVESTATE_NEUTRAL_HORDE_CHALLENGE:
    case OBJECTIVESTATE_ALLIANCE_HORDE_CHALLENGE:
        m_TowerState = EP_TS_N_H;
        break;
    }


    Map* map = sMapMgr->FindMap(0, 0);
    auto bounds = map->GetGameObjectBySpawnIdStore().equal_range(m_capturePointSpawnId);
    for (auto itr = bounds.first; itr != bounds.second; ++itr)
        itr->second->SetGoArtKit(artkit);

    bounds = map->GetGameObjectBySpawnIdStore().equal_range(m_Objects[EP_PWT_FLAGS]);
    for (auto itr = bounds.first; itr != bounds.second; ++itr)
        itr->second->SetGoArtKit(artkit);

    UpdateTowerState();

    // complete quest objective
    if (m_TowerState == EP_TS_H_P || m_TowerState == EP_TS_A_P)
        SendObjectiveComplete(EP_PWT_CM, ObjectGuid::Empty);
}

void OPvPCapturePointEP_PWT::FillInitialWorldStates(WorldPacket &data)
{
    data << EP_PWT_A << uint32(bool(m_TowerState & EP_TS_A));
    data << EP_PWT_H << uint32(bool(m_TowerState & EP_TS_H));
    data << EP_PWT_A_P << uint32(bool(m_TowerState & EP_TS_A_P));
    data << EP_PWT_H_P << uint32(bool(m_TowerState & EP_TS_H_P));
    data << EP_PWT_N_A << uint32(bool(m_TowerState & EP_TS_N_A));
    data << EP_PWT_N_H << uint32(bool(m_TowerState & EP_TS_N_H));
    data << EP_PWT_N << uint32(bool(m_TowerState & EP_TS_N));
}

void OPvPCapturePointEP_PWT::UpdateTowerState()
{
    m_PvP->SendUpdateWorldState(EP_PWT_A , bool(m_TowerState & EP_TS_A));
    m_PvP->SendUpdateWorldState(EP_PWT_H , bool(m_TowerState & EP_TS_H));
    m_PvP->SendUpdateWorldState(EP_PWT_A_P , bool(m_TowerState & EP_TS_A_P));
    m_PvP->SendUpdateWorldState(EP_PWT_H_P , bool(m_TowerState & EP_TS_H_P));
    m_PvP->SendUpdateWorldState(EP_PWT_N_A , bool(m_TowerState & EP_TS_N_A));
    m_PvP->SendUpdateWorldState(EP_PWT_N_H , bool(m_TowerState & EP_TS_N_H));
    m_PvP->SendUpdateWorldState(EP_PWT_N , bool(m_TowerState & EP_TS_N));
}

void OPvPCapturePointEP_PWT::SummonFlightMaster(uint32 team)
{
    if(m_FlightMasterSpawned != team)
    {
        m_FlightMasterSpawned = team;
        DelCreature(EP_PWT_FLIGHTMASTER);
        AddCreature(EP_PWT_FLIGHTMASTER,EP_PWT_FlightMaster.entry,EP_PWT_FlightMaster.map,EP_PWT_FlightMaster.x,EP_PWT_FlightMaster.y,EP_PWT_FlightMaster.z,EP_PWT_FlightMaster.o);
        Creature * c = m_PvP->GetMap()->GetCreature(m_Creatures[EP_PWT_FLIGHTMASTER]);
        if(c)
        {
            GossipMenuItems gso;
            /* TODO GOSSIP
            gso.OptionType = GOSSIP_OPTION_OUTDOORPVP;
            gso.OptionText.assign(sObjectMgr->GetTrinityStringForDBCLocale(LANG_OPVP_EP_FLIGHT_NPT));
            gso.OptionIndex = 50;
            c->AddGossipOption(gso);

            gso.OptionType = GOSSIP_OPTION_OUTDOORPVP;
            gso.OptionText.assign(sObjectMgr->GetTrinityStringForDBCLocale(LANG_OPVP_EP_FLIGHT_EWT));
            gso.OptionIndex = 50;
            c->AddGossipOption(gso);

            gso.OptionType = GOSSIP_OPTION_OUTDOORPVP;
            gso.OptionText.assign(sObjectMgr->GetTrinityStringForDBCLocale(LANG_OPVP_EP_FLIGHT_CGT));
            gso.OptionIndex = 50;
            c->AddGossipOption(gso);
            */
        }
    }
}

bool OPvPCapturePointEP_PWT::CanTalkTo(Player * p, Creature * c, GossipMenuItems const& gso)
{
    if( p->GetTeam() == m_FlightMasterSpawned
        && c->GetGUID() == m_Creatures[EP_PWT_FLIGHTMASTER]
       /* TODO GOSSIP && gso.Id == 50 */)
        return true;
    return false;
}

bool OPvPCapturePointEP_PWT::HandleGossipOption(Player *plr, ObjectGuid guid, uint32 gossipid)
{
    auto itr = m_CreatureTypes.find(guid);
    if(itr != m_CreatureTypes.end())
    {
        Creature * cr = m_PvP->GetMap()->GetCreature(guid);
        if(!cr)
            return true;
        if(itr->second == EP_PWT_FLIGHTMASTER)
        {
            uint32 src = EP_PWT_Taxi;
            uint32 dst = 0;
            switch(gossipid)
            {
            case 0:
                dst = EP_NPT_Taxi;
                break;
            case 1:
                dst = EP_EWT_Taxi;
                break;
            default:
                dst = EP_CGT_Taxi;
                break;
            }

            std::vector<uint32> nodes;
            nodes.resize(2);
            nodes[0] = src;
            nodes[1] = dst;

            plr->PlayerTalkClass->SendCloseGossip();
            plr->ActivateTaxiPathTo(nodes, cr);
            // leave the opvp, seems like moveinlineofsight isn't called when entering a taxi
            HandlePlayerLeave(plr);
        }
        return true;
    }
    return false;
}

// ep
OutdoorPvPEP::OutdoorPvPEP()
{
    m_TypeId = OUTDOOR_PVP_EP;
    memset(EP_Controls,0,sizeof(EP_Controls));
    m_AllianceTowersControlled = 0;
    m_HordeTowersControlled = 0;
}

bool OutdoorPvPEP::SetupOutdoorPvP()
{
    for(uint32 EPBuffZone : EPBuffZones)
        RegisterZone(EPBuffZone);

    AddCapturePoint(new OPvPCapturePointEP_EWT(this));
    AddCapturePoint(new OPvPCapturePointEP_PWT(this));
    AddCapturePoint(new OPvPCapturePointEP_CGT(this));
    AddCapturePoint(new OPvPCapturePointEP_NPT(this));
    SetMapFromZone(EPBuffZones[0]);
    return true;
}

bool OutdoorPvPEP::Update(uint32 diff)
{
    if(OutdoorPvP::Update(diff))
    {
        m_AllianceTowersControlled = 0;
        m_HordeTowersControlled = 0;
        for(uint32 EP_Control : EP_Controls)
        {
            if(EP_Control == ALLIANCE)
                ++m_AllianceTowersControlled;
            else if(EP_Control == HORDE)
                ++m_HordeTowersControlled;
            SendUpdateWorldState(EP_UI_TOWER_COUNT_A,m_AllianceTowersControlled);
            SendUpdateWorldState(EP_UI_TOWER_COUNT_H,m_HordeTowersControlled);
            BuffTeams();
        }
        return true;
    }
    return false;
}

void OutdoorPvPEP::HandlePlayerEnterZone(Player * plr, uint32 zone)
{
    // add buffs
    if(plr->GetTeam() == ALLIANCE)
    {
        if(m_AllianceTowersControlled && m_AllianceTowersControlled < 5)
            plr->CastSpell(plr,EP_AllianceBuffs[m_AllianceTowersControlled-1], true);
    }
    else
    {
        if(m_HordeTowersControlled && m_HordeTowersControlled < 5)
            plr->CastSpell(plr,EP_HordeBuffs[m_HordeTowersControlled-1], true);
    }
    OutdoorPvP::HandlePlayerEnterZone(plr,zone);
}

void OutdoorPvPEP::HandlePlayerLeaveZone(Player * plr, uint32 zone)
{
    // remove buffs
    if(plr->GetTeam() == ALLIANCE)
    {
        for(uint32 EP_AllianceBuff : EP_AllianceBuffs)
            plr->RemoveAurasDueToSpell(EP_AllianceBuff);
    }
    else
    {
        for(uint32 EP_HordeBuff : EP_HordeBuffs)
            plr->RemoveAurasDueToSpell(EP_HordeBuff);
    }
    OutdoorPvP::HandlePlayerLeaveZone(plr, zone);
}

void OutdoorPvPEP::BuffTeams()
{
    for(ObjectGuid itr : m_players[0])
    {
        if(Player * plr = ObjectAccessor::FindPlayer(itr))
        {
            for(uint32 EP_AllianceBuff : EP_AllianceBuffs)
                if(plr->IsInWorld()) plr->RemoveAurasDueToSpell(EP_AllianceBuff);
            if(m_AllianceTowersControlled && m_AllianceTowersControlled < 5)
                if(plr->IsInWorld()) plr->CastSpell(plr,EP_AllianceBuffs[m_AllianceTowersControlled-1], true);
        }
    }
    for(ObjectGuid itr : m_players[1])
    {
        if(Player * plr = ObjectAccessor::FindPlayer(itr))
        {
            for(uint32 EP_HordeBuff : EP_HordeBuffs)
                if(plr->IsInWorld()) plr->RemoveAurasDueToSpell(EP_HordeBuff);
            if(m_HordeTowersControlled && m_HordeTowersControlled < 5)
                if(plr->IsInWorld()) plr->CastSpell(plr,EP_HordeBuffs[m_HordeTowersControlled-1], true);
        }
    }
}

void OutdoorPvPEP::FillInitialWorldStates(WorldPacket & data)
{
    data << EP_UI_TOWER_COUNT_A << m_AllianceTowersControlled;
    data << EP_UI_TOWER_COUNT_H << m_HordeTowersControlled;
    for(auto & m_OPvPCapturePoint : m_capturePoints)
        m_OPvPCapturePoint.second->FillInitialWorldStates(data);
}

void OutdoorPvPEP::SendRemoveWorldStates(Player *plr)
{
    plr->SendUpdateWorldState(EP_UI_TOWER_COUNT_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_UI_TOWER_COUNT_H,WORLD_STATE_REMOVE);

    plr->SendUpdateWorldState(EP_EWT_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_EWT_H,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_EWT_N,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_EWT_A_P,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_EWT_H_P,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_EWT_N_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_EWT_N_H,WORLD_STATE_REMOVE);

    plr->SendUpdateWorldState(EP_PWT_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_PWT_H,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_PWT_N,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_PWT_A_P,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_PWT_H_P,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_PWT_N_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_PWT_N_H,WORLD_STATE_REMOVE);

    plr->SendUpdateWorldState(EP_NPT_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_NPT_H,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_NPT_N,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_NPT_A_P,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_NPT_H_P,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_NPT_N_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_NPT_N_H,WORLD_STATE_REMOVE);

    plr->SendUpdateWorldState(EP_CGT_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_CGT_H,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_CGT_N,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_CGT_A_P,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_CGT_H_P,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_CGT_N_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(EP_CGT_N_H,WORLD_STATE_REMOVE);
}

class OutdoorPvP_eastern_plaguelands : public OutdoorPvPScript
{
    public:
        OutdoorPvP_eastern_plaguelands() : OutdoorPvPScript("outdoorpvp_ep") { }

        OutdoorPvP* GetOutdoorPvP() const override
        {
            return new OutdoorPvPEP();
        }
};

void AddSC_outdoorpvp_ep()
{
    new OutdoorPvP_eastern_plaguelands();
}
