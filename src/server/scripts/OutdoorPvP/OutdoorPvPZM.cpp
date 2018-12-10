
#include "OutdoorPvPZM.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Creature.h"
#include "ObjectAccessor.h"
#include "WorldPacket.h"
#include "GossipDef.h"
#include "World.h"
#include "ScriptMgr.h"

OPvPCapturePointZM_Beacon::OPvPCapturePointZM_Beacon(OutdoorPvP *pvp, ZM_BeaconType type)
: OPvPCapturePoint(pvp), m_TowerType(type), m_TowerState(ZM_TOWERSTATE_N)
{
    SetCapturePointData(ZMCapturePoints[type].entry,ZMCapturePoints[type].map,ZMCapturePoints[type].x,ZMCapturePoints[type].y,ZMCapturePoints[type].z,ZMCapturePoints[type].o,ZMCapturePoints[type].rot0,ZMCapturePoints[type].rot1,ZMCapturePoints[type].rot2,ZMCapturePoints[type].rot3);
}

void OPvPCapturePointZM_Beacon::FillInitialWorldStates(WorldPacket &data)
{
    data << uint32(ZMBeaconInfo[m_TowerType].ui_tower_n) << uint32(bool(m_TowerState & ZM_TOWERSTATE_N));
    data << uint32(ZMBeaconInfo[m_TowerType].map_tower_n) << uint32(bool(m_TowerState & ZM_TOWERSTATE_N));
    data << uint32(ZMBeaconInfo[m_TowerType].ui_tower_a) << uint32(bool(m_TowerState & ZM_TOWERSTATE_A));
    data << uint32(ZMBeaconInfo[m_TowerType].map_tower_a) << uint32(bool(m_TowerState & ZM_TOWERSTATE_A));
    data << uint32(ZMBeaconInfo[m_TowerType].ui_tower_h) << uint32(bool(m_TowerState & ZM_TOWERSTATE_H));
    data << uint32(ZMBeaconInfo[m_TowerType].map_tower_h) << uint32(bool(m_TowerState & ZM_TOWERSTATE_H));
}

void OPvPCapturePointZM_Beacon::UpdateTowerState()
{
    m_PvP->SendUpdateWorldState(uint32(ZMBeaconInfo[m_TowerType].ui_tower_n),uint32(bool(m_TowerState & ZM_TOWERSTATE_N)));
    m_PvP->SendUpdateWorldState(uint32(ZMBeaconInfo[m_TowerType].map_tower_n),uint32(bool(m_TowerState & ZM_TOWERSTATE_N)));
    m_PvP->SendUpdateWorldState(uint32(ZMBeaconInfo[m_TowerType].ui_tower_a),uint32(bool(m_TowerState & ZM_TOWERSTATE_A)));
    m_PvP->SendUpdateWorldState(uint32(ZMBeaconInfo[m_TowerType].map_tower_a),uint32(bool(m_TowerState & ZM_TOWERSTATE_A)));
    m_PvP->SendUpdateWorldState(uint32(ZMBeaconInfo[m_TowerType].ui_tower_h),uint32(bool(m_TowerState & ZM_TOWERSTATE_H)));
    m_PvP->SendUpdateWorldState(uint32(ZMBeaconInfo[m_TowerType].map_tower_h),uint32(bool(m_TowerState & ZM_TOWERSTATE_H)));
}

void OPvPCapturePointZM_Beacon::ChangeState()
{
    // if changing from controlling alliance to horde
    if (m_OldState == OBJECTIVESTATE_ALLIANCE)
    {
        if (((OutdoorPvPZM*)m_PvP)->m_AllianceTowersControlled)
            ((OutdoorPvPZM*)m_PvP)->m_AllianceTowersControlled--;
    }
    // if changing from controlling horde to alliance
    else if (m_OldState == OBJECTIVESTATE_HORDE)
    {
        if (((OutdoorPvPZM*)m_PvP)->m_HordeTowersControlled)
            ((OutdoorPvPZM*)m_PvP)->m_HordeTowersControlled--;
    }

    switch (m_State)
    {
    case OBJECTIVESTATE_ALLIANCE:
        m_TowerState = ZM_TOWERSTATE_A;
        if (((OutdoorPvPZM*)m_PvP)->m_AllianceTowersControlled<ZM_NUM_BEACONS)
            ((OutdoorPvPZM*)m_PvP)->m_AllianceTowersControlled++;
        m_PvP->SendDefenseMessage(ZM_GRAVEYARD_ZONE, ZMBeaconCaptureA[m_TowerType]);
        break;
    case OBJECTIVESTATE_HORDE:
        m_TowerState = ZM_TOWERSTATE_H;
        if (((OutdoorPvPZM*)m_PvP)->m_HordeTowersControlled<ZM_NUM_BEACONS)
            ((OutdoorPvPZM*)m_PvP)->m_HordeTowersControlled++;

        m_PvP->SendDefenseMessage(ZM_GRAVEYARD_ZONE, ZMBeaconCaptureH[m_TowerType]);
        break;
    case OBJECTIVESTATE_NEUTRAL:
    case OBJECTIVESTATE_NEUTRAL_ALLIANCE_CHALLENGE:
    case OBJECTIVESTATE_NEUTRAL_HORDE_CHALLENGE:
    case OBJECTIVESTATE_ALLIANCE_HORDE_CHALLENGE:
    case OBJECTIVESTATE_HORDE_ALLIANCE_CHALLENGE:
        m_TowerState = ZM_TOWERSTATE_N;
        break;
    }

    UpdateTowerState();
}

bool OutdoorPvPZM::Update(uint32 diff)
{
    bool changed = false;
    if((changed = OutdoorPvP::Update(diff)))
    {
        if(m_AllianceTowersControlled == ZM_NUM_BEACONS)
            m_GraveYard->SetBeaconState(ALLIANCE);
        else if(m_HordeTowersControlled == ZM_NUM_BEACONS)
            m_GraveYard->SetBeaconState(HORDE);
        else
            m_GraveYard->SetBeaconState(0);
    }
    return changed;
}

void OutdoorPvPZM::HandlePlayerEnterZone(Player * plr, uint32 zone)
{
    if(plr->GetTeam() == ALLIANCE)
    {
        if(m_GraveYard->m_GraveYardState & ZM_GRAVEYARD_A)
            plr->CastSpell(plr,ZM_CAPTURE_BUFF, TRIGGERED_FULL_MASK);
    }
    else
    {
        if(m_GraveYard->m_GraveYardState & ZM_GRAVEYARD_H)
            plr->CastSpell(plr,ZM_CAPTURE_BUFF, TRIGGERED_FULL_MASK);
    }
    OutdoorPvP::HandlePlayerEnterZone(plr,zone);
}

void OutdoorPvPZM::HandlePlayerLeaveZone(Player * plr, uint32 zone)
{
    // remove buffs
    plr->RemoveAurasDueToSpell(ZM_CAPTURE_BUFF);
    // remove flag
    plr->RemoveAurasDueToSpell(ZM_BATTLE_STANDARD_A);
    plr->RemoveAurasDueToSpell(ZM_BATTLE_STANDARD_H);
    OutdoorPvP::HandlePlayerLeaveZone(plr, zone);
}

OutdoorPvPZM::OutdoorPvPZM()
{
    m_TypeId = OUTDOOR_PVP_ZM;
    m_GraveYard = nullptr;
    m_AllianceTowersControlled = 0;
    m_HordeTowersControlled = 0;

}

bool OutdoorPvPZM::SetupOutdoorPvP()
{
    m_AllianceTowersControlled = 0;
    m_HordeTowersControlled = 0;

    SetMapFromZone(OutdoorPvPZMBuffZones[0]);

    // add the zones affected by the pvp buff
    for(uint32 OutdoorPvPZMBuffZone : OutdoorPvPZMBuffZones)
        RegisterZone(OutdoorPvPZMBuffZone);

    AddCapturePoint(new OPvPCapturePointZM_Beacon(this,ZM_BEACON_WEST));
    AddCapturePoint(new OPvPCapturePointZM_Beacon(this,ZM_BEACON_EAST));
    m_GraveYard = new OPvPCapturePointZM_GraveYard(this);
    AddCapturePoint(m_GraveYard); // though the update function isn't used, the handleusego is!

    return true;
}

void OutdoorPvPZM::HandleKillImpl(Player *plr, Unit * killed)
{
    if(killed->GetTypeId() != TYPEID_PLAYER)
        return;

    if(plr->GetTeam() == ALLIANCE && (killed->ToPlayer())->GetTeam() != ALLIANCE)
        plr->CastSpell(plr,ZM_AlliancePlayerKillReward, true);
    else if(plr->GetTeam() == HORDE && (killed->ToPlayer())->GetTeam() != HORDE)
        plr->CastSpell(plr,ZM_HordePlayerKillReward, true);
}

void OutdoorPvPZM::BuffTeam(uint32 team)
{
    if(team == ALLIANCE)
    {
        for(ObjectGuid itr : m_players[0])
        {
            if(Player * plr = ObjectAccessor::FindPlayer(itr))
                if(plr->IsInWorld()) plr->CastSpell(plr,ZM_CAPTURE_BUFF, true);
        }
        for(ObjectGuid itr : m_players[1])
        {
            if(Player * plr = ObjectAccessor::FindPlayer(itr))
                if(plr->IsInWorld()) plr->RemoveAurasDueToSpell(ZM_CAPTURE_BUFF);
        }
    }
    else if(team == HORDE)
    {
        for(ObjectGuid itr : m_players[1])
        {
            if(Player * plr = ObjectAccessor::FindPlayer(itr))
                if(plr->IsInWorld()) plr->CastSpell(plr,ZM_CAPTURE_BUFF, true);
        }
        for(ObjectGuid itr : m_players[0])
        {
            if(Player * plr = ObjectAccessor::FindPlayer(itr))
                if(plr->IsInWorld()) plr->RemoveAurasDueToSpell(ZM_CAPTURE_BUFF);
        }
    }
    else
    {
        for(ObjectGuid itr : m_players[0])
        {
            if(Player * plr = ObjectAccessor::FindPlayer(itr))
                if(plr->IsInWorld()) plr->RemoveAurasDueToSpell(ZM_CAPTURE_BUFF);
        }
        for(ObjectGuid itr : m_players[1])
        {
            if(Player * plr = ObjectAccessor::FindPlayer(itr))
                if(plr->IsInWorld()) plr->RemoveAurasDueToSpell(ZM_CAPTURE_BUFF);
        }
    }
}

bool OPvPCapturePointZM_GraveYard::Update(uint32 diff)
{
    bool retval = m_State != m_OldState;
    m_State = m_OldState;
    return retval;
}

int32 OPvPCapturePointZM_GraveYard::HandleOpenGo(Player *plr, ObjectGuid guid)
{
    int32 retval = OPvPCapturePoint::HandleOpenGo(plr, guid);
    if(retval>=0)
    {
        if(plr->HasAuraEffect(ZM_BATTLE_STANDARD_A,0) && m_GraveYardState != ZM_GRAVEYARD_A)
        {
            m_GraveYardState = ZM_GRAVEYARD_A;
            DelObject(0);   // only one gotype is used in the whole outdoor pvp, no need to call it a constant
            AddObject(0,ZM_Banner_A.entry,ZM_Banner_A.map,ZM_Banner_A.x,ZM_Banner_A.y,ZM_Banner_A.z,ZM_Banner_A.o,ZM_Banner_A.rot0,ZM_Banner_A.rot1,ZM_Banner_A.rot2,ZM_Banner_A.rot3);
            sObjectMgr->RemoveGraveYardLink(ZM_GRAVEYARD_ID, ZM_GRAVEYARD_ZONE, HORDE);          // rem gy
            sObjectMgr->AddGraveYardLink(ZM_GRAVEYARD_ID, ZM_GRAVEYARD_ZONE, ALLIANCE, false);   // add gy
            ((OutdoorPvPZM*)m_PvP)->BuffTeam(ALLIANCE);
            plr->RemoveAurasDueToSpell(ZM_BATTLE_STANDARD_A);
            m_PvP->SendDefenseMessage(ZM_GRAVEYARD_ZONE, TEXT_TWIN_SPIRE_RUINS_TAKEN_ALLIANCE);
        }
        else if(plr->HasAuraEffect(ZM_BATTLE_STANDARD_H,0) && m_GraveYardState != ZM_GRAVEYARD_H)
        {
            m_GraveYardState = ZM_GRAVEYARD_H;
            DelObject(0);   // only one gotype is used in the whole outdoor pvp, no need to call it a constant
            AddObject(0,ZM_Banner_H.entry,ZM_Banner_H.map,ZM_Banner_H.x,ZM_Banner_H.y,ZM_Banner_H.z,ZM_Banner_H.o,ZM_Banner_H.rot0,ZM_Banner_H.rot1,ZM_Banner_H.rot2,ZM_Banner_H.rot3);
            sObjectMgr->RemoveGraveYardLink(ZM_GRAVEYARD_ID, ZM_GRAVEYARD_ZONE, ALLIANCE);          // rem gy
            sObjectMgr->AddGraveYardLink(ZM_GRAVEYARD_ID, ZM_GRAVEYARD_ZONE, HORDE, false);   // add gy
            ((OutdoorPvPZM*)m_PvP)->BuffTeam(HORDE);
            plr->RemoveAurasDueToSpell(ZM_BATTLE_STANDARD_H);
            m_PvP->SendDefenseMessage(ZM_GRAVEYARD_ZONE, TEXT_TWIN_SPIRE_RUINS_TAKEN_HORDE);
        }
        UpdateTowerState();
    }
    return retval;
}

OPvPCapturePointZM_GraveYard::OPvPCapturePointZM_GraveYard(OutdoorPvP *pvp)
: OPvPCapturePoint(pvp)
{
    m_BothControllingFaction = 0;
    m_GraveYardState = ZM_GRAVEYARD_N;
    m_FlagCarrierGUID.Clear();
    // add field scouts here
    AddCreature(ZM_ALLIANCE_FIELD_SCOUT,ZM_AllianceFieldScout.entry,ZM_AllianceFieldScout.map,ZM_AllianceFieldScout.x,ZM_AllianceFieldScout.y,ZM_AllianceFieldScout.z,ZM_AllianceFieldScout.o);
    AddCreature(ZM_HORDE_FIELD_SCOUT,ZM_HordeFieldScout.entry,ZM_HordeFieldScout.map,ZM_HordeFieldScout.x,ZM_HordeFieldScout.y,ZM_HordeFieldScout.z,ZM_HordeFieldScout.o);
    // add neutral banner
    AddObject(0,ZM_Banner_N.entry,ZM_Banner_N.map,ZM_Banner_N.x,ZM_Banner_N.y,ZM_Banner_N.z,ZM_Banner_N.o,ZM_Banner_N.rot0,ZM_Banner_N.rot1,ZM_Banner_N.rot2,ZM_Banner_N.rot3);
}

void OPvPCapturePointZM_GraveYard::UpdateTowerState()
{
    m_PvP->SendUpdateWorldState(ZM_MAP_GRAVEYARD_N,uint32(bool(m_GraveYardState & ZM_GRAVEYARD_N)));
    m_PvP->SendUpdateWorldState(ZM_MAP_GRAVEYARD_H,uint32(bool(m_GraveYardState & ZM_GRAVEYARD_H)));
    m_PvP->SendUpdateWorldState(ZM_MAP_GRAVEYARD_A,uint32(bool(m_GraveYardState & ZM_GRAVEYARD_A)));

    m_PvP->SendUpdateWorldState(ZM_MAP_ALLIANCE_FLAG_READY,uint32(m_BothControllingFaction == ALLIANCE));
    m_PvP->SendUpdateWorldState(ZM_MAP_ALLIANCE_FLAG_NOT_READY,uint32(m_BothControllingFaction != ALLIANCE));
    m_PvP->SendUpdateWorldState(ZM_MAP_HORDE_FLAG_READY,uint32(m_BothControllingFaction == HORDE));
    m_PvP->SendUpdateWorldState(ZM_MAP_HORDE_FLAG_NOT_READY,uint32(m_BothControllingFaction != HORDE));
}

void OPvPCapturePointZM_GraveYard::FillInitialWorldStates(WorldPacket &data)
{
    data << ZM_MAP_GRAVEYARD_N  << uint32(bool(m_GraveYardState & ZM_GRAVEYARD_N));
    data << ZM_MAP_GRAVEYARD_H  << uint32(bool(m_GraveYardState & ZM_GRAVEYARD_H));
    data << ZM_MAP_GRAVEYARD_A  << uint32(bool(m_GraveYardState & ZM_GRAVEYARD_A));

    data << ZM_MAP_ALLIANCE_FLAG_READY  << uint32(m_BothControllingFaction == ALLIANCE);
    data << ZM_MAP_ALLIANCE_FLAG_NOT_READY  << uint32(m_BothControllingFaction != ALLIANCE);
    data << ZM_MAP_HORDE_FLAG_READY  << uint32(m_BothControllingFaction == HORDE);
    data << ZM_MAP_HORDE_FLAG_NOT_READY  << uint32(m_BothControllingFaction != HORDE);
}

void OPvPCapturePointZM_GraveYard::SetBeaconState(uint32 controlling_faction)
{
    // nothing to do here
    if(m_BothControllingFaction == controlling_faction)
        return;
    m_BothControllingFaction = controlling_faction;

    switch(controlling_faction)
    {
    case ALLIANCE:
        // if ally already controls the gy and taken back both beacons, return, nothing to do for us
        if(m_GraveYardState & ZM_GRAVEYARD_A)
            return;
        // ally doesn't control the gy, but controls the side beacons -> add gossip option, add neutral banner
        break;
    case HORDE:
        // if horde already controls the gy and taken back both beacons, return, nothing to do for us
        if(m_GraveYardState & ZM_GRAVEYARD_H)
            return;
        // horde doesn't control the gy, but controls the side beacons -> add gossip option, add neutral banner
        break;
    default:
        // if the graveyard is not neutral, then leave it that way
        // if the graveyard is neutral, then we have to dispel the buff from the flag carrier
        if(m_GraveYardState & ZM_GRAVEYARD_N)
        {
            // gy was neutral, thus neutral banner was spawned, it is possible that someone was taking the flag to the gy
            if(m_FlagCarrierGUID)
            {
                // remove flag from carrier, reset flag carrier guid
                Player * p = ObjectAccessor::FindPlayer(m_FlagCarrierGUID);
                if(p)
                {
                   p->RemoveAurasDueToSpell(ZM_BATTLE_STANDARD_A);
                   p->RemoveAurasDueToSpell(ZM_BATTLE_STANDARD_H);
                }
                m_FlagCarrierGUID.Clear();
            }
        }
        break;
    }
    // send worldstateupdate
    UpdateTowerState();
}

bool OPvPCapturePointZM_GraveYard::CanTalkTo(Player* plr, Creature* c, GossipMenuItems const& gso)
{
    ObjectGuid guid = c->GetGUID();
    auto itr = m_CreatureTypes.find(guid);
    if(itr != m_CreatureTypes.end())
    {
        if(itr->second == ZM_ALLIANCE_FIELD_SCOUT && plr->GetTeam() == ALLIANCE && m_BothControllingFaction == ALLIANCE && !m_FlagCarrierGUID && m_GraveYardState != ZM_GRAVEYARD_A)
            return true;
        else if(itr->second == ZM_HORDE_FIELD_SCOUT && plr->GetTeam() == HORDE && m_BothControllingFaction == HORDE && !m_FlagCarrierGUID && m_GraveYardState != ZM_GRAVEYARD_H)
            return true;
    }
    return false;
}

bool OPvPCapturePointZM_GraveYard::HandleGossipOption(Player *plr, ObjectGuid guid, uint32 gossipid)
{
    auto itr = m_CreatureTypes.find(guid);
    if(itr != m_CreatureTypes.end())
    {
        Creature * cr = m_PvP->GetMap()->GetCreature(guid);
        if(!cr)
            return true;
        // if the flag is already taken, then return
        if(m_FlagCarrierGUID)
            return true;
        if(itr->second == ZM_ALLIANCE_FIELD_SCOUT)
        {
            cr->CastSpell(plr,ZM_BATTLE_STANDARD_A, true);
            m_FlagCarrierGUID = plr->GetGUID();
        }
        else if(itr->second == ZM_HORDE_FIELD_SCOUT)
        {
            cr->CastSpell(plr,ZM_BATTLE_STANDARD_H, true);
            m_FlagCarrierGUID = plr->GetGUID();
        }
        UpdateTowerState();
        plr->PlayerTalkClass->SendCloseGossip();
        return true;
    }
    return false;
}

bool OPvPCapturePointZM_GraveYard::HandleDropFlag(Player * plr, uint32 spellId)
{
    switch(spellId)
    {
    case ZM_BATTLE_STANDARD_A:
    case ZM_BATTLE_STANDARD_H:
        m_FlagCarrierGUID.Clear();
        return true;
    }
    return false;
}

void OutdoorPvPZM::FillInitialWorldStates(WorldPacket &data)
{
    data << ZM_WORLDSTATE_UNK_1 << uint32(1);
    for(auto & m_OPvPCapturePoint : m_capturePoints)
    {
        m_OPvPCapturePoint.second->FillInitialWorldStates(data);
    }
}

void OutdoorPvPZM::SendRemoveWorldStates(Player *plr)
{
    plr->SendUpdateWorldState(ZM_WORLDSTATE_UNK_1,WORLD_STATE_ADD);
    plr->SendUpdateWorldState(ZM_UI_TOWER_EAST_N,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(ZM_UI_TOWER_EAST_H,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(ZM_UI_TOWER_EAST_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(ZM_UI_TOWER_WEST_N,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(ZM_UI_TOWER_WEST_H,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(ZM_UI_TOWER_WEST_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(ZM_MAP_TOWER_EAST_N,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(ZM_MAP_TOWER_EAST_H,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(ZM_MAP_TOWER_EAST_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(ZM_MAP_GRAVEYARD_H,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(ZM_MAP_GRAVEYARD_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(ZM_MAP_GRAVEYARD_N,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(ZM_MAP_TOWER_WEST_N,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(ZM_MAP_TOWER_WEST_H,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(ZM_MAP_TOWER_WEST_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(ZM_MAP_HORDE_FLAG_READY,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(ZM_MAP_HORDE_FLAG_NOT_READY,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(ZM_MAP_ALLIANCE_FLAG_NOT_READY,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(ZM_MAP_ALLIANCE_FLAG_READY,WORLD_STATE_REMOVE);
}

class OutdoorPvP_zangarmarsh : public OutdoorPvPScript
{
    public:
        OutdoorPvP_zangarmarsh() : OutdoorPvPScript("outdoorpvp_zm") { }

        OutdoorPvP* GetOutdoorPvP() const override
        {
            return new OutdoorPvPZM();
        }
};

void AddSC_outdoorpvp_zm()
{
    new OutdoorPvP_zangarmarsh();
}