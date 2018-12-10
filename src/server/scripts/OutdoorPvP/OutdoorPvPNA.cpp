/*
 * Copyright (C) 2008 Trinity <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "OutdoorPvPNA.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "WorldPacket.h"
#include "Language.h"
#include "World.h"
#include "MapManager.h"
#include "ScriptMgr.h"

OutdoorPvPNA::OutdoorPvPNA()
{
    m_TypeId = OUTDOOR_PVP_NA;
}

void OutdoorPvPNA::HandleKillImpl(Player* plr, Unit* killed)
{
    if (killed->GetTypeId() == TYPEID_PLAYER && plr->GetTeam() != (killed->ToPlayer())->GetTeam()) {
        plr->KilledMonsterCredit(NA_CREDIT_MARKER, ObjectGuid::Empty);
        if (plr->GetTeam() == TEAM_ALLIANCE)
            plr->CastSpell(plr, NA_KILL_TOKEN_ALLIANCE, true);
        else
            plr->CastSpell(plr, NA_KILL_TOKEN_HORDE, true);
    }
}

uint32 OPvPCapturePointNA::GetAliveGuardsCount()
{
    uint32 cnt = 0;
    for(auto itr = m_Creatures.begin(); itr != m_Creatures.end(); ++itr) {
        switch(itr->first)
        {
        case NA_NPC_GUARD_01:
        case NA_NPC_GUARD_02:
        case NA_NPC_GUARD_03:
        case NA_NPC_GUARD_04:
        case NA_NPC_GUARD_05:
        case NA_NPC_GUARD_06:
        case NA_NPC_GUARD_07:
        case NA_NPC_GUARD_08:
        case NA_NPC_GUARD_09:
        case NA_NPC_GUARD_10:
        case NA_NPC_GUARD_11:
        case NA_NPC_GUARD_12:
        case NA_NPC_GUARD_13:
        case NA_NPC_GUARD_14:
        case NA_NPC_GUARD_15:
        {
            if(Creature * cr = m_PvP->GetMap()->GetCreature(itr->second)) {
                if (cr->IsAlive())
                    ++cnt;
            }
            else if (CreatureData const * cd = sObjectMgr->GetCreatureData(itr->second.GetCounter())) {
                ++cnt;
            }
        }
            break;
        default:
            break;
        }
    }

    return cnt;
}

void OutdoorPvPNA::BuffTeam(uint32 team)
{
    if (team == TEAM_ALLIANCE) {
        for(ObjectGuid itr : m_players[0]) {
            if (Player * plr = ObjectAccessor::FindPlayer(itr)) {
                if (plr->IsInWorld())
                    plr->CastSpell(plr, NA_CAPTURE_BUFF, true);
            }
        }
        for(ObjectGuid itr : m_players[1]) {
            if (Player * plr = ObjectAccessor::FindPlayer(itr)) {
                if (plr->IsInWorld())
                    plr->RemoveAurasDueToSpell(NA_CAPTURE_BUFF);
            }
        }
    }
    else if (team == TEAM_HORDE) {
        for (ObjectGuid itr : m_players[1]) {
            if (Player * plr = ObjectAccessor::FindPlayer(itr)) {
                if (plr->IsInWorld())
                    plr->CastSpell(plr, NA_CAPTURE_BUFF, true);
            }
        }
        for (ObjectGuid itr : m_players[0]) {
            if (Player * plr = ObjectAccessor::FindPlayer(itr)) {
                if (plr->IsInWorld())
                    plr->RemoveAurasDueToSpell(NA_CAPTURE_BUFF);
            }
        }
    }
    else {
        for (ObjectGuid itr : m_players[0]) {
            if (Player * plr = ObjectAccessor::FindPlayer(itr)) {
                if (plr->IsInWorld())
                    plr->RemoveAurasDueToSpell(NA_CAPTURE_BUFF);
            }
        }
        for (ObjectGuid itr : m_players[1]) {
            if (Player * plr = ObjectAccessor::FindPlayer(itr)) {
                if (plr->IsInWorld())
                    plr->RemoveAurasDueToSpell(NA_CAPTURE_BUFF);
            }
        }
    }
}

void OPvPCapturePointNA::SpawnNPCsForTeam(uint32 team)
{
    const creature_type * creatures = nullptr;
    if (team == TEAM_ALLIANCE)
        creatures = AllianceControlNPCs;
    else if (team == TEAM_HORDE)
        creatures = HordeControlNPCs;
    else
        return;

    for(int i = 0; i < NA_CONTROL_NPC_NUM; ++i)
        AddCreature(i, creatures[i].entry, creatures[i].map, creatures[i].x, creatures[i].y, creatures[i].z, creatures[i].o, 1000000);
}

void OPvPCapturePointNA::DeSpawnNPCs()
{
    for (int i = 0; i < NA_CONTROL_NPC_NUM; ++i)
        DelCreature(i);
}

void OPvPCapturePointNA::SpawnGOsForTeam(uint32 team)
{
    const go_type * gos = nullptr;
    if (team == TEAM_ALLIANCE)
        gos = AllianceControlGOs;
    else if (team == TEAM_HORDE)
        gos = HordeControlGOs;
    else
        return;

    for(int i = 0; i < NA_CONTROL_GO_NUM; ++i) {
        if (i == NA_ROOST_S ||
            i == NA_ROOST_W ||
            i == NA_ROOST_N ||
            i == NA_ROOST_E ||
            i == NA_BOMB_WAGON_S ||
            i == NA_BOMB_WAGON_W ||
            i == NA_BOMB_WAGON_N ||
            i == NA_BOMB_WAGON_E)
            continue;   // roosts and bomb wagons are spawned when someone uses the matching destroyed roost

        AddObject(i, gos[i].entry, gos[i].map, gos[i].x, gos[i].y, gos[i].z, gos[i].o, gos[i].rot0, gos[i].rot1, gos[i].rot2, gos[i].rot3);
    }
}

void OPvPCapturePointNA::DeSpawnGOs()
{
    for (int i = 0; i < NA_CONTROL_GO_NUM; ++i)
        DelObject(i);
}

void OPvPCapturePointNA::FactionTakeOver(uint32 team)
{
    if (m_ControllingFaction)
        sObjectMgr->RemoveGraveYardLink(NA_HALAA_GRAVEYARD, NA_HALAA_GRAVEYARD_ZONE, m_ControllingFaction, false);

    m_ControllingFaction = team;
    if (m_ControllingFaction)
        sObjectMgr->AddGraveYardLink(NA_HALAA_GRAVEYARD, NA_HALAA_GRAVEYARD_ZONE, m_ControllingFaction, false);

    DeSpawnGOs();
    DeSpawnNPCs();
    SpawnGOsForTeam(team);
    SpawnNPCsForTeam(team);
    
    m_GuardsAlive = NA_GUARDS_MAX;
    m_capturable = false;

    this->UpdateHalaaWorldState();
    if (team == TEAM_ALLIANCE) {
        m_WyvernStateSouth = WYVERN_NEU_HORDE;
        m_WyvernStateNorth = WYVERN_NEU_HORDE;
        m_WyvernStateEast = WYVERN_NEU_HORDE;
        m_WyvernStateWest = WYVERN_NEU_HORDE;
        m_PvP->SendUpdateWorldState(NA_UI_HORDE_GUARDS_SHOW, WORLD_STATE_REMOVE);
        m_PvP->SendUpdateWorldState(NA_UI_ALLIANCE_GUARDS_SHOW, WORLD_STATE_ADD);
        m_PvP->SendUpdateWorldState(NA_UI_GUARDS_LEFT, m_GuardsAlive);
        m_PvP->SendDefenseMessage(NA_HALAA_GRAVEYARD_ZONE, TEXT_HALAA_TAKEN_ALLIANCE);
    }
    else {
        m_WyvernStateSouth = WYVERN_NEU_ALLIANCE;
        m_WyvernStateNorth = WYVERN_NEU_ALLIANCE;
        m_WyvernStateEast = WYVERN_NEU_ALLIANCE;
        m_WyvernStateWest = WYVERN_NEU_ALLIANCE;
        m_PvP->SendUpdateWorldState(NA_UI_HORDE_GUARDS_SHOW, WORLD_STATE_ADD);
        m_PvP->SendUpdateWorldState(NA_UI_ALLIANCE_GUARDS_SHOW, WORLD_STATE_REMOVE);
        m_PvP->SendUpdateWorldState(NA_UI_GUARDS_LEFT, m_GuardsAlive);
        m_PvP->SendDefenseMessage(NA_HALAA_GRAVEYARD_ZONE, TEXT_HALAA_TAKEN_HORDE);
    }

    this->UpdateWyvernRoostWorldState(NA_ROOST_S);
    this->UpdateWyvernRoostWorldState(NA_ROOST_N);
    this->UpdateWyvernRoostWorldState(NA_ROOST_W);
    this->UpdateWyvernRoostWorldState(NA_ROOST_E);

    ((OutdoorPvPNA*)m_PvP)->BuffTeam(team);
}

OPvPCapturePointNA::OPvPCapturePointNA(OutdoorPvP* pvp) :
OPvPCapturePoint(pvp), m_capturable(true), m_GuardsAlive(0), m_ControllingFaction(0),
m_HalaaState(HALAA_N), m_WyvernStateSouth(0), m_WyvernStateNorth(0), m_WyvernStateWest(0),
m_WyvernStateEast(0), m_RespawnTimer(NA_RESPAWN_TIME), m_GuardCheckTimer(NA_GUARD_CHECK_TIME)
{
    SetCapturePointData(182210, 530, -1572.57, 7945.3, -22.475, 2.05949, 0, 0, 0.857167, 0.515038);
}

bool OutdoorPvPNA::SetupOutdoorPvP()
{
//    m_TypeId = OUTDOOR_PVP_NA; _MUST_ be set in ctor, because of spawns cleanup
    // add the zones affected by the pvp buff
    SetMapFromZone(NA_BUFF_ZONE);
    RegisterZone(NA_BUFF_ZONE);

    // halaa
    m_obj = new OPvPCapturePointNA(this);
    AddCapturePoint(m_obj);

    return true;
}

void OutdoorPvPNA::HandlePlayerEnterZone(Player* plr, uint32 zone)
{
    // add buffs
    if (plr->GetTeam() == m_obj->m_ControllingFaction)
        plr->CastSpell(plr, NA_CAPTURE_BUFF, true);

    OutdoorPvP::HandlePlayerEnterZone(plr, zone);
}

void OutdoorPvPNA::HandlePlayerLeaveZone(Player* plr, uint32 zone)
{
    // remove buffs
    plr->RemoveAurasDueToSpell(NA_CAPTURE_BUFF);
    OutdoorPvP::HandlePlayerLeaveZone(plr, zone);
}

void OutdoorPvPNA::FillInitialWorldStates(WorldPacket &data)
{
    m_obj->FillInitialWorldStates(data);
}

void OPvPCapturePointNA::FillInitialWorldStates(WorldPacket &data)
{
    if (m_ControllingFaction == TEAM_ALLIANCE) {
        data << NA_UI_HORDE_GUARDS_SHOW << uint32(0);
        data << NA_UI_ALLIANCE_GUARDS_SHOW << uint32(1);
    }
    else if (m_ControllingFaction == TEAM_HORDE) {
        data << NA_UI_HORDE_GUARDS_SHOW << uint32(1);
        data << NA_UI_ALLIANCE_GUARDS_SHOW << uint32(0);
    }
    else {
        data << NA_UI_HORDE_GUARDS_SHOW << uint32(0);
        data << NA_UI_ALLIANCE_GUARDS_SHOW << uint32(0);
    }

    data << NA_UI_GUARDS_MAX << NA_GUARDS_MAX;
    data << NA_UI_GUARDS_LEFT << uint32(m_GuardsAlive);

    data << NA_MAP_WYVERN_NORTH_NEU_H << uint32(bool(m_WyvernStateNorth & WYVERN_NEU_HORDE));
    data << NA_MAP_WYVERN_NORTH_NEU_A << uint32(bool(m_WyvernStateNorth & WYVERN_NEU_ALLIANCE));
    data << NA_MAP_WYVERN_NORTH_H << uint32(bool(m_WyvernStateNorth & WYVERN_HORDE));
    data << NA_MAP_WYVERN_NORTH_A << uint32(bool(m_WyvernStateNorth & WYVERN_ALLIANCE));

    data << NA_MAP_WYVERN_SOUTH_NEU_H << uint32(bool(m_WyvernStateSouth & WYVERN_NEU_HORDE));
    data << NA_MAP_WYVERN_SOUTH_NEU_A << uint32(bool(m_WyvernStateSouth & WYVERN_NEU_ALLIANCE));
    data << NA_MAP_WYVERN_SOUTH_H << uint32(bool(m_WyvernStateSouth & WYVERN_HORDE));
    data << NA_MAP_WYVERN_SOUTH_A << uint32(bool(m_WyvernStateSouth & WYVERN_ALLIANCE));

    data << NA_MAP_WYVERN_WEST_NEU_H << uint32(bool(m_WyvernStateWest & WYVERN_NEU_HORDE));
    data << NA_MAP_WYVERN_WEST_NEU_A << uint32(bool(m_WyvernStateWest & WYVERN_NEU_ALLIANCE));
    data << NA_MAP_WYVERN_WEST_H << uint32(bool(m_WyvernStateWest & WYVERN_HORDE));
    data << NA_MAP_WYVERN_WEST_A << uint32(bool(m_WyvernStateWest & WYVERN_ALLIANCE));

    data << NA_MAP_WYVERN_EAST_NEU_H << uint32(bool(m_WyvernStateEast & WYVERN_NEU_HORDE));
    data << NA_MAP_WYVERN_EAST_NEU_A << uint32(bool(m_WyvernStateEast & WYVERN_NEU_ALLIANCE));
    data << NA_MAP_WYVERN_EAST_H << uint32(bool(m_WyvernStateEast & WYVERN_HORDE));
    data << NA_MAP_WYVERN_EAST_A << uint32(bool(m_WyvernStateEast & WYVERN_ALLIANCE));

    data << NA_MAP_HALAA_NEUTRAL << uint32(bool(m_HalaaState & HALAA_N));
    data << NA_MAP_HALAA_NEU_A << uint32(bool(m_HalaaState & HALAA_N_A));
    data << NA_MAP_HALAA_NEU_H << uint32(bool(m_HalaaState & HALAA_N_H));
    data << NA_MAP_HALAA_HORDE << uint32(bool(m_HalaaState & HALAA_H));
    data << NA_MAP_HALAA_ALLIANCE << uint32(bool(m_HalaaState & HALAA_A));
}

void OutdoorPvPNA::SendRemoveWorldStates(Player *plr)
{
    plr->SendUpdateWorldState(NA_UI_HORDE_GUARDS_SHOW,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_UI_ALLIANCE_GUARDS_SHOW,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_UI_GUARDS_MAX,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_UI_GUARDS_LEFT,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_WYVERN_NORTH_NEU_H,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_WYVERN_NORTH_NEU_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_WYVERN_NORTH_H,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_WYVERN_NORTH_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_WYVERN_SOUTH_NEU_H,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_WYVERN_SOUTH_NEU_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_WYVERN_SOUTH_H,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_WYVERN_SOUTH_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_WYVERN_WEST_NEU_H,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_WYVERN_WEST_NEU_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_WYVERN_WEST_H,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_WYVERN_WEST_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_WYVERN_EAST_NEU_H,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_WYVERN_EAST_NEU_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_WYVERN_EAST_H,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_WYVERN_EAST_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_HALAA_NEUTRAL,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_HALAA_NEU_A,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_HALAA_NEU_H,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_HALAA_HORDE,WORLD_STATE_REMOVE);
    plr->SendUpdateWorldState(NA_MAP_HALAA_ALLIANCE,WORLD_STATE_REMOVE);
}

bool OutdoorPvPNA::Update(uint32 diff)
{
    return m_obj->Update(diff);
}

bool OPvPCapturePointNA::HandleCustomSpell(Player* plr, uint32 spellId, GameObject* go)
{
    std::vector<uint32> nodes;
    nodes.resize(2);
    bool retval = false;

    switch(spellId)
    {
    case NA_SPELL_FLY_NORTH:
        nodes[0] = FlightPathStartNodes[NA_ROOST_N];
        nodes[1] = FlightPathEndNodes[NA_ROOST_N];
        plr->ActivateTaxiPathTo(nodes);
        plr->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP);
        plr->UpdatePvP(true, true);
        retval = true;
        break;
    case NA_SPELL_FLY_SOUTH:
        nodes[0] = FlightPathStartNodes[NA_ROOST_S];
        nodes[1] = FlightPathEndNodes[NA_ROOST_S];
        plr->ActivateTaxiPathTo(nodes);
        plr->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP);
        plr->UpdatePvP(true, true);
        retval = true;
        break;
    case NA_SPELL_FLY_WEST:
        nodes[0] = FlightPathStartNodes[NA_ROOST_W];
        nodes[1] = FlightPathEndNodes[NA_ROOST_W];
        plr->ActivateTaxiPathTo(nodes);
        plr->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP);
        plr->UpdatePvP(true, true);
        retval = true;
        break;
    case NA_SPELL_FLY_EAST:
        nodes[0] = FlightPathStartNodes[NA_ROOST_E];
        nodes[1] = FlightPathEndNodes[NA_ROOST_E];
        plr->ActivateTaxiPathTo(nodes);
        plr->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP);
        plr->UpdatePvP(true, true);
        retval = true;
        break;
    default:
        break;
    }

    if (retval) {
        //Adding items
        uint32 noSpaceForCount = 0;

        // check space and find places
        ItemPosCountVec dest;

        int32 count = 10;
        uint32 itemid = 24538;
                                                                // bomb id count
        uint8 msg = plr->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemid, count, &noSpaceForCount);
        if (msg != EQUIP_ERR_OK)                               // convert to possible store amount
            count -= noSpaceForCount;

        if (count == 0 || dest.empty())                         // can't add any
            return true;

        Item* item = plr->StoreNewItem(dest, itemid, true);

        if (count > 0 && item)
            plr->SendNewItem(item, count, true, false); 

        return true;
    }

    return false;
}

int32 OPvPCapturePointNA::HandleOpenGo(Player* plr, ObjectGuid guid)
{
    int32 retval = OPvPCapturePoint::HandleOpenGo(plr, guid);
    if (retval >= 0) {
        const go_type* gos = nullptr;
        if (m_ControllingFaction == TEAM_ALLIANCE)
            gos = AllianceControlGOs;
        else if (m_ControllingFaction == TEAM_HORDE)
            gos = HordeControlGOs;
        else
            return -1;

        int32 del = -1;
        int32 del2 = -1;
        int32 add = -1;
        int32 add2 = -1;

        switch (retval) {
        case NA_DESTROYED_ROOST_S:
            del = NA_DESTROYED_ROOST_S;
            add = NA_ROOST_S;
            add2 = NA_BOMB_WAGON_S;
            if (m_ControllingFaction == TEAM_HORDE)
                m_WyvernStateSouth = WYVERN_ALLIANCE;
            else
                m_WyvernStateSouth = WYVERN_HORDE;

            UpdateWyvernRoostWorldState(NA_ROOST_S);
            break;
        case NA_DESTROYED_ROOST_N:
            del = NA_DESTROYED_ROOST_N;
            add = NA_ROOST_N;
            add2 = NA_BOMB_WAGON_N;
            if (m_ControllingFaction == TEAM_HORDE)
                m_WyvernStateNorth = WYVERN_ALLIANCE;
            else
                m_WyvernStateNorth = WYVERN_HORDE;

            UpdateWyvernRoostWorldState(NA_ROOST_N);
            break;
        case NA_DESTROYED_ROOST_W:
            del = NA_DESTROYED_ROOST_W;
            add = NA_ROOST_W;
            add2 = NA_BOMB_WAGON_W;
            if (m_ControllingFaction == TEAM_HORDE)
                m_WyvernStateWest = WYVERN_ALLIANCE;
            else
                m_WyvernStateWest = WYVERN_HORDE;

            UpdateWyvernRoostWorldState(NA_ROOST_W);
            break;
        case NA_DESTROYED_ROOST_E:
            del = NA_DESTROYED_ROOST_E;
            add = NA_ROOST_E;
            add2 = NA_BOMB_WAGON_E;
            if (m_ControllingFaction == TEAM_HORDE)
                m_WyvernStateEast = WYVERN_ALLIANCE;
            else
                m_WyvernStateEast = WYVERN_HORDE;

            UpdateWyvernRoostWorldState(NA_ROOST_E);
            break;
        case NA_BOMB_WAGON_S:
            del = NA_BOMB_WAGON_S;
            del2 = NA_ROOST_S;
            add = NA_DESTROYED_ROOST_S;
            if (m_ControllingFaction == TEAM_HORDE)
                m_WyvernStateSouth = WYVERN_NEU_ALLIANCE;
            else
                m_WyvernStateSouth = WYVERN_NEU_HORDE;

            UpdateWyvernRoostWorldState(NA_ROOST_S);
            break;
        case NA_BOMB_WAGON_N:
            del = NA_BOMB_WAGON_N;
            del2 = NA_ROOST_N;
            add = NA_DESTROYED_ROOST_N;
            if (m_ControllingFaction == TEAM_HORDE)
                m_WyvernStateNorth = WYVERN_NEU_ALLIANCE;
            else
                m_WyvernStateNorth = WYVERN_NEU_HORDE;

            UpdateWyvernRoostWorldState(NA_ROOST_N);
            break;
        case NA_BOMB_WAGON_W:
            del = NA_BOMB_WAGON_W;
            del2 = NA_ROOST_W;
            add = NA_DESTROYED_ROOST_W;
            if (m_ControllingFaction == TEAM_HORDE)
                m_WyvernStateWest = WYVERN_NEU_ALLIANCE;
            else
                m_WyvernStateWest = WYVERN_NEU_HORDE;

            UpdateWyvernRoostWorldState(NA_ROOST_W);
            break;
        case NA_BOMB_WAGON_E:
            del = NA_BOMB_WAGON_E;
            del2 = NA_ROOST_E;
            add = NA_DESTROYED_ROOST_E;
            if (m_ControllingFaction == TEAM_HORDE)
                m_WyvernStateEast = WYVERN_NEU_ALLIANCE;
            else
                m_WyvernStateEast = WYVERN_NEU_HORDE;

            UpdateWyvernRoostWorldState(NA_ROOST_E);
            break;
        default:
            return -1;
        }

        if (del > -1)
            DelObject(del);

        if (del2 > -1)
            DelObject(del2);

        if (add > -1)
            AddObject(add, gos[add].entry, gos[add].map, gos[add].x, gos[add].y, gos[add].z, gos[add].o, gos[add].rot0, gos[add].rot1, gos[add].rot2, gos[add].rot3);

        if (add2 > -1)
            AddObject(add2, gos[add2].entry, gos[add2].map, gos[add2].x, gos[add2].y, gos[add2].z, gos[add2].o, gos[add2].rot0, gos[add2].rot1, gos[add2].rot2, gos[add2].rot3);

        return retval;
    }

    return -1;
}

bool OPvPCapturePointNA::Update(uint32 diff)
{
    // let the controlling faction advance in phase
    bool capturable = false;
    if (m_ControllingFaction == TEAM_ALLIANCE && m_activePlayers[0].size() > m_activePlayers[1].size())
        capturable = true;
    else if (m_ControllingFaction == TEAM_HORDE && m_activePlayers[0].size() < m_activePlayers[1].size())
        capturable = true;

    if (m_GuardCheckTimer < diff) {
        m_GuardCheckTimer = NA_GUARD_CHECK_TIME;
        uint32 cnt = GetAliveGuardsCount();
        if (cnt != m_GuardsAlive) {
            m_GuardsAlive = cnt;
            if (m_GuardsAlive == 0)
                m_capturable = true;
            // update the guard count for the players in zone
            m_PvP->SendUpdateWorldState(NA_UI_GUARDS_LEFT, m_GuardsAlive);
        }
    }
    else
        m_GuardCheckTimer -= diff;

    if ((m_capturable || capturable) && OPvPCapturePoint::Update(diff)) {
        if (m_RespawnTimer < diff) {
            // if the guards have been killed, then the challenger has one hour to take over halaa.
            // in case they fail to do it, the guards are respawned, and they have to start again.
            if (m_ControllingFaction)
                FactionTakeOver(m_ControllingFaction);

            m_RespawnTimer = NA_RESPAWN_TIME;
        }
        else
            m_RespawnTimer -= diff;
    }

    return false;
}

void OPvPCapturePointNA::ChangeState()
{
    uint32 artkit = 21;
    switch (m_State) {
    case OBJECTIVESTATE_NEUTRAL:
        m_HalaaState = HALAA_N;
        break;
    case OBJECTIVESTATE_ALLIANCE:
        m_HalaaState = HALAA_A;
        FactionTakeOver(TEAM_ALLIANCE);
        artkit = 2;
        break;
    case OBJECTIVESTATE_HORDE:
        m_HalaaState = HALAA_H;
        FactionTakeOver(TEAM_HORDE);
        artkit = 1;
        break;
    case OBJECTIVESTATE_NEUTRAL_ALLIANCE_CHALLENGE:
        m_HalaaState = HALAA_N_A;
        break;
    case OBJECTIVESTATE_NEUTRAL_HORDE_CHALLENGE:
        m_HalaaState = HALAA_N_H;
        break;
    case OBJECTIVESTATE_ALLIANCE_HORDE_CHALLENGE:
        m_HalaaState = HALAA_N_A;
        artkit = 2;
        break;
    case OBJECTIVESTATE_HORDE_ALLIANCE_CHALLENGE:
        m_HalaaState = HALAA_N_H;
        artkit = 1;
        break;
    }

    auto bounds = sMapMgr->FindMap(530, 0)->GetGameObjectBySpawnIdStore().equal_range(m_capturePointSpawnId);
    for (auto itr = bounds.first; itr != bounds.second; ++itr)
        itr->second->SetGoArtKit(artkit);

    UpdateHalaaWorldState();
}

void OPvPCapturePointNA::UpdateHalaaWorldState()
{
    m_PvP->SendUpdateWorldState(NA_MAP_HALAA_NEUTRAL, uint32(bool(m_HalaaState & HALAA_N)));
    m_PvP->SendUpdateWorldState(NA_MAP_HALAA_NEU_A, uint32(bool(m_HalaaState & HALAA_N_A)));
    m_PvP->SendUpdateWorldState(NA_MAP_HALAA_NEU_H, uint32(bool(m_HalaaState & HALAA_N_H)));
    m_PvP->SendUpdateWorldState(NA_MAP_HALAA_HORDE, uint32(bool(m_HalaaState & HALAA_H)));
    m_PvP->SendUpdateWorldState(NA_MAP_HALAA_ALLIANCE, uint32(bool(m_HalaaState & HALAA_A)));
}

void OPvPCapturePointNA::UpdateWyvernRoostWorldState(uint32 roost)
{
    switch (roost) {
    case NA_ROOST_S:
        m_PvP->SendUpdateWorldState(NA_MAP_WYVERN_SOUTH_NEU_H, uint32(bool(m_WyvernStateSouth & WYVERN_NEU_HORDE)));
        m_PvP->SendUpdateWorldState(NA_MAP_WYVERN_SOUTH_NEU_A, uint32(bool(m_WyvernStateSouth & WYVERN_NEU_ALLIANCE)));
        m_PvP->SendUpdateWorldState(NA_MAP_WYVERN_SOUTH_H, uint32(bool(m_WyvernStateSouth & WYVERN_HORDE)));
        m_PvP->SendUpdateWorldState(NA_MAP_WYVERN_SOUTH_A, uint32(bool(m_WyvernStateSouth & WYVERN_ALLIANCE)));
        break;
    case NA_ROOST_N:
        m_PvP->SendUpdateWorldState(NA_MAP_WYVERN_NORTH_NEU_H, uint32(bool(m_WyvernStateNorth & WYVERN_NEU_HORDE)));
        m_PvP->SendUpdateWorldState(NA_MAP_WYVERN_NORTH_NEU_A, uint32(bool(m_WyvernStateNorth & WYVERN_NEU_ALLIANCE)));
        m_PvP->SendUpdateWorldState(NA_MAP_WYVERN_NORTH_H, uint32(bool(m_WyvernStateNorth & WYVERN_HORDE)));
        m_PvP->SendUpdateWorldState(NA_MAP_WYVERN_NORTH_A, uint32(bool(m_WyvernStateNorth & WYVERN_ALLIANCE)));
        break;
    case NA_ROOST_W:
        m_PvP->SendUpdateWorldState(NA_MAP_WYVERN_WEST_NEU_H, uint32(bool(m_WyvernStateWest & WYVERN_NEU_HORDE)));
        m_PvP->SendUpdateWorldState(NA_MAP_WYVERN_WEST_NEU_A, uint32(bool(m_WyvernStateWest & WYVERN_NEU_ALLIANCE)));
        m_PvP->SendUpdateWorldState(NA_MAP_WYVERN_WEST_H, uint32(bool(m_WyvernStateWest & WYVERN_HORDE)));
        m_PvP->SendUpdateWorldState(NA_MAP_WYVERN_WEST_A, uint32(bool(m_WyvernStateWest & WYVERN_ALLIANCE)));
        break;
    case NA_ROOST_E:
        m_PvP->SendUpdateWorldState(NA_MAP_WYVERN_EAST_NEU_H, uint32(bool(m_WyvernStateEast & WYVERN_NEU_HORDE)));
        m_PvP->SendUpdateWorldState(NA_MAP_WYVERN_EAST_NEU_A, uint32(bool(m_WyvernStateEast & WYVERN_NEU_ALLIANCE)));
        m_PvP->SendUpdateWorldState(NA_MAP_WYVERN_EAST_H, uint32(bool(m_WyvernStateEast & WYVERN_HORDE)));
        m_PvP->SendUpdateWorldState(NA_MAP_WYVERN_EAST_A, uint32(bool(m_WyvernStateEast & WYVERN_ALLIANCE)));
        break;
    }
}

class OutdoorPvP_nagrand : public OutdoorPvPScript
{
    public:
        OutdoorPvP_nagrand() : OutdoorPvPScript("outdoorpvp_na") { }

        OutdoorPvP* GetOutdoorPvP() const override
        {
            return new OutdoorPvPNA();
        }
};

void AddSC_outdoorpvp_na()
{
    new OutdoorPvP_nagrand();
}
