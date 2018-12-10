/*
Current bugs :
- MotionTransports are at 0,0,0 on client until first teleport
    to try : dumping all fields at first send then after first teleport, see what changed for client
- MotionTransports have some strange behavior at client after a while ? (transport randomly stopping, broken player movement when aboard)
- No MMaps handling on transports : Not so important, creatures force destination for now
 */

#include "Common.h"
#include "Transport.h"
#include "MapManager.h"
#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "WorldPacket.h"
#include "DBCStores.h"
#include "World.h"
#include "GameObjectAI.h"
#include "MapReference.h"
#include "Player.h"
#include "Cell.h"
#include "CellImpl.h"
#include "Totem.h"
#include "Spell.h"
#include "ZoneScript.h"

uint32 Transport::GetPathProgress() const 
{
    return GetGOValue()->Transport.PathProgress;
}

MotionTransport::MotionTransport() : 
    Transport(), 
    _updating(false),
    _transportInfo(nullptr), 
    _isMoving(true), 
    _pendingStop(false), 
    _triggeredArrivalEvent(false), 
    _triggeredDepartureEvent(false), 
    _passengersLoaded(false),
    _delayedTeleport(false), 
    _delayedAddModel(false)
{
#ifdef LICH_KING
    m_updateFlag = UPDATEFLAG_TRANSPORT | UPDATEFLAG_LOWGUID | UPDATEFLAG_STATIONARY_POSITION | UPDATEFLAG_ROTATION;
#else
    m_updateFlag = UPDATEFLAG_TRANSPORT | UPDATEFLAG_LOWGUID | UPDATEFLAG_HIGHGUID | UPDATEFLAG_STATIONARY_POSITION;
#endif
}

MotionTransport::~MotionTransport()
{
    ASSERT(_passengers.empty());
    UnloadStaticPassengers();
}

bool MotionTransport::CreateMoTrans(ObjectGuid::LowType guidlow, uint32 entry, uint32 mapid, float x, float y, float z, float ang, uint32 animprogress)
{
    Relocate(x, y, z, ang);

    if (!IsPositionValid())
    {
        TC_LOG_ERROR("entities.transport", "Transport (GUID: %u) not created. Suggested coordinates isn't valid (X: %f Y: %f)",
            guidlow, x, y);
        return false;
    }

    Object::_Create(guidlow, 0, HighGuid::Mo_Transport);

    GameObjectTemplate const* goinfo = sObjectMgr->GetGameObjectTemplate(entry);

    if (!goinfo)
    {
        TC_LOG_ERROR("entities.transport", "Transport not created: entry in `gameobject_template` not found, guidlow: %u map: %u  (X: %f Y: %f Z: %f) ang: %f", guidlow, mapid, x, y, z, ang);
        return false;
    }

    m_goInfo = goinfo;

    TransportTemplate const* tInfo = sTransportMgr->GetTransportTemplate(entry);
    if (!tInfo)
    {
        TC_LOG_ERROR("entities.transport", "Transport %u (name: %s) will not be created, missing `transport_template` entry.", entry, goinfo->name.c_str());
        return false;
    }

    _transportInfo = tInfo;

    // initialize waypoints
    _nextFrame = tInfo->keyFrames.begin();
    _currentFrame = _nextFrame++;
    _triggeredArrivalEvent = false;
    _triggeredDepartureEvent = false;

    SetObjectScale(goinfo->size);
    SetUInt32Value(GAMEOBJECT_FACTION, goinfo->faction);
    SetUInt32Value(GAMEOBJECT_FLAGS, goinfo->flags);
    //Do not start at 0, hackfix for ships appearing at wrong coordinates at server boot
    SetPathProgress(120000);
    SetPeriod(tInfo->pathTime);
    SetEntry(goinfo->entry);
    SetDisplayId(goinfo->displayId);
    //Ships on BC cannot be paused and are always GO_STATE_READY
#ifdef LICH_KING
    SetGoState(!goinfo->moTransport.canBeStopped ? GO_STATE_READY : GO_STATE_ACTIVE);
#else
    SetGoState(GO_STATE_READY);
#endif
    SetGoType(GAMEOBJECT_TYPE_MO_TRANSPORT);
    SetGoAnimProgress(animprogress);
    SetName(goinfo->name);

#ifdef LICH_KING
    // sunwell: no WorldRotation for MotionTransports
    SetWorldRotation(G3D::Quat());
#endif
    // sunwell: no PathRotation for MotionTransports
    SetTransportPathRotation(G3D::Quat(0.0f, 0.0f, 0.0f, 1.0f));

    m_model = CreateModel();
    return true;
}

void MotionTransport::CleanupsBeforeDelete(bool finalCleanup /*= true*/)
{
    UnloadStaticPassengers();
    while (!_passengers.empty())
    {
        WorldObject* obj = *_passengers.begin();
        RemovePassenger(obj);
        obj->SetTransport(nullptr);
    }

    GameObject::CleanupsBeforeDelete(finalCleanup);
}

void MotionTransport::BuildUpdate(UpdateDataMapType& data_map, UpdatePlayerSet&)
{
    Map::PlayerList const& players = GetMap()->GetPlayers();
    if (players.isEmpty())
        return;

    for (const auto & player : players)
        BuildFieldsUpdate(player.GetSource(), data_map);

    ClearUpdateMask(true);
}

void MotionTransport::Update(uint32 diff)
{
    ASSERT(_updating == false);
    _updating = true;
    if (_delayedTeleport) //on sunstrider some maps may be updated several times while continent update only once. In case of teleport, this prevent keeping updating the transport again until we finished a full world update (and thus processed the delayed updates and teleports)
    {
        _updating = false;
        return;
    }

    uint32 const positionUpdateDelay = 1;

    if (AI())
        AI()->UpdateAI(diff);
    else if (!AIM_Initialize())
        TC_LOG_ERROR("entities.transport", "Could not initialize GameObjectAI for Transport");

    if (GetKeyFrames().size() <= 1)
    {
        _updating = false;
        return;
    }

    if (IsMoving() || !_pendingStop)
        SetPathProgress(GetPathProgress() + diff);

    uint32 timer = GetPathProgress() % GetPeriod();
    bool justStopped = false;

    // Set current waypoint
    // Desired outcome: _currentFrame->DepartureTime < timer < _nextFrame->ArriveTime
    // ... arrive | ... delay ... | departure
    //      event /         event /
    for (;;)
    {
        if (timer >= _currentFrame->ArriveTime)
        {
            if (!_triggeredArrivalEvent)
            {
                DoEventIfAny(*_currentFrame, false);
                _triggeredArrivalEvent = true;
            }

            if (timer < _currentFrame->DepartureTime)
            {
                justStopped = IsMoving();
                SetMoving(false);
#ifdef LICH_KING
                if (_pendingStop && GetGoState() != GO_STATE_READY)
                {
                    SetGoState(GO_STATE_READY);
                    SetPathProgress(GetPathProgress() / GetPeriod());
                    SetPathProgress(GetPathProgress() * GetPeriod());
                    SetPathProgress(GetPathProgress() + _currentFrame->ArriveTime);
                }
#endif
                break;  // its a stop frame and we are waiting
            }
        }

        if (timer >= _currentFrame->DepartureTime && !_triggeredDepartureEvent)
        {
            DoEventIfAny(*_currentFrame, true); // departure event
            _triggeredDepartureEvent = true;
        }

        // not waiting anymore
        SetMoving(true);

        // Enable movement //No pause for ships on BC
#ifdef LICH_KING
        if (GetGOInfo()->moTransport.canBeStopped)
            SetGoState(GO_STATE_ACTIVE);
#endif

        if (timer >= _currentFrame->DepartureTime && timer < _currentFrame->NextArriveTime)
            break;  // found current waypoint

        MoveToNextWaypoint();

        // sScriptMgr->OnRelocate(this, _currentFrame->Node->NodeIndex, _currentFrame->Node->MapID, _currentFrame->Node->LocX, _currentFrame->Node->LocY, _currentFrame->Node->LocZ);

        //TC_LOG_DEBUG("entities.transport", "Transport %u (%s) moved to node %u %u %f %f %f", GetEntry(), GetName().c_str(), _currentFrame->Node->index, _currentFrame->Node->mapid, _currentFrame->Node->x, _currentFrame->Node->y, _currentFrame->Node->z);

        // Departure event
        if (_currentFrame->IsTeleportFrame())
            if (TeleportTransport(_nextFrame->Node->MapID, _nextFrame->Node->LocX, _nextFrame->Node->LocY, _nextFrame->Node->LocZ, _nextFrame->InitialOrientation))
            {
                _updating = false;
                return; // Update more in new map thread
            }
    }

    // Add model to map after we are fully done with moving maps
    if (_delayedAddModel)
    {
        _delayedAddModel = false;
        if (m_model)
            GetMap()->InsertGameObjectModel(*m_model);
    }

    // Set position
    _positionChangeTimer.Update(diff);
    if (_positionChangeTimer.Passed())
    {
        _positionChangeTimer.Reset(positionUpdateDelay);
        if (IsMoving())
        {
            float t = !justStopped ? CalculateSegmentPos(float(timer) * 0.001f) : 1.0f;
            G3D::Vector3 pos, dir;
            _currentFrame->Spline->evaluate_percent(_currentFrame->Index, t, pos);
            _currentFrame->Spline->evaluate_derivative(_currentFrame->Index, t, dir);
            UpdatePosition(pos.x, pos.y, pos.z, NormalizeOrientation(atan2(dir.y, dir.x) + M_PI));
        }
        else if (justStopped)
        {
            UpdatePosition(_currentFrame->Node->LocX, _currentFrame->Node->LocY, _currentFrame->Node->LocZ, _currentFrame->InitialOrientation);
            JustStopped();
        }
        else
        {
            /* There are four possible scenarios that trigger loading/unloading passengers:
            1. transport moves from inactive to active grid
            2. the grid that transport is currently in becomes active
            3. transport moves from active to inactive grid
            4. the grid that transport is currently in unloads
            */
            bool gridActive = GetMap()->IsGridLoaded(GetPositionX(), GetPositionY());

            if (_staticPassengers.empty() && gridActive) // 2.
                LoadStaticPassengers();
            else if (!_staticPassengers.empty() && !gridActive)
                // 4. - if transports stopped on grid edge, some passengers can remain in active grids
                //      unload all static passengers otherwise passengers won't load correctly when the grid that transport is currently in becomes active
                UnloadStaticPassengers();
        }
    }

    sScriptMgr->OnTransportUpdate(this, diff);
    _updating = false;
}

void MotionTransport::DelayedUpdate(uint32 diff)
{
    ASSERT(_updating == false);
    _updating = true;
    if (GetKeyFrames().size() <= 1)
    {
        _updating = false;
        return;
    }

    DelayedTeleportTransport();

    _updating = false;
}

void MotionTransport::UpdatePosition(float x, float y, float z, float o)
{
    bool newActive = GetMap()->IsGridLoaded(x, y);
    Cell oldCell(GetPositionX(), GetPositionY());

    Relocate(x, y, z, o);
    UpdateModelPosition();
    
    UpdatePassengerPositions(_passengers);

    /* There are four possible scenarios that trigger loading/unloading passengers:
    1. transport moves from inactive to active grid
    2. the grid that transport is currently in becomes active
    3. transport moves from active to inactive grid
    4. the grid that transport is currently in unloads
    */
    if (_staticPassengers.empty()) // 1.
        LoadStaticPassengers();
    else if (!_staticPassengers.empty() && !newActive && oldCell.DiffGrid(Cell(GetPositionX(), GetPositionY()))) // 3.
        UnloadStaticPassengers();
    else
        UpdatePassengerPositions(_staticPassengers);
    // 4. is handed by grid unload
}

void MotionTransport::AddPassenger(WorldObject* passenger, bool calcPassengerPosition /*= false*/)
{
    if (!IsInWorld())
        return;

    ASSERT(Lock.try_lock()); //almost sure we don't need this lock anymore, but lets do this for a while, this will crash if there is any concurrency for this function
    if (_passengers.insert(passenger).second)
    {
        passenger->SetTransport(this);
        if (calcPassengerPosition)
        {
            float x, y, z, o;
            passenger->GetPosition(x, y, z, o);
            CalculatePassengerOffset(x, y, z, &o);
            passenger->SetTransOffset(x, y, z, o);
        }

        if (Player* plr = passenger->ToPlayer())
            sScriptMgr->OnAddPassenger(ToTransport(), plr);
    }
    Lock.unlock();
}

void MotionTransport::RemovePassenger(WorldObject* passenger)
{
    ASSERT(Lock.try_lock()); //almost sure we don't need this lock anymore, but lets do this for a while, this will crash if there is any concurrency for this function
    if (_passengers.erase(passenger) || _staticPassengers.erase(passenger))
{
        passenger->SetTransport(nullptr);
        //TC_LOG_DEBUG("entities.transport", "Object %s removed from transport %s.", passenger->GetName().c_str(), GetName().c_str());

        if (Player* plr = passenger->ToPlayer())
        {
            sScriptMgr->OnRemovePassenger(ToTransport(), plr);
            plr->SetFallInformation(0, plr->GetPositionZ());
        }
    }
    Lock.unlock();
}

Creature* MotionTransport::CreateNPCPassenger(ObjectGuid::LowType guid, CreatureData const* data)
{
    Map* map = GetMap();
    if (map->GetCreatureRespawnTime(guid))
        return nullptr;

    auto  creature = new Creature();

    if (!creature->LoadFromDB(guid, map, false, false)) //do not add to map yet
    {
        delete creature;
        return nullptr;
    }

    float x, y, z, o;
    data->spawnPoint.GetPosition(x, y, z, o);

    creature->SetTransport(this);
    creature->SetTransOffset(x, y, z, o);
    CalculatePassengerPosition(x, y, z, &o);
    creature->Relocate(x, y, z, o);
    creature->SetHomePosition(creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ(), creature->GetOrientation());
    creature->SetTransportHomePosition(creature->GetMovementInfo().transport.pos);

#ifndef LICH_KING
    //keep these mobs as purely aesthetic for BC as the ship crews should not even be there on BC
    creature->SetReactState(REACT_PASSIVE);
    creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
    //also prevent using vendors
    creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_VENDOR | UNIT_NPC_FLAG_VENDOR_REAGENT | UNIT_NPC_FLAG_VENDOR_AMMO | UNIT_NPC_FLAG_VENDOR_FOOD | UNIT_NPC_FLAG_VENDOR_POISON | UNIT_NPC_FLAG_REPAIR);
#endif

    if (!creature->IsPositionValid())
    {
        TC_LOG_ERROR("entities.transport", "Creature (guidlow %d, entry %d) not created. Suggested coordinates aren't valid (X: %f Y: %f)", creature->GetGUID().GetCounter(), creature->GetEntry(), creature->GetPositionX(), creature->GetPositionY());
        delete creature;
        return nullptr;
    }

    if (!map->AddToMap(creature))
    {
        delete creature;
        return nullptr;
    }

    _staticPassengers.insert(creature);
    sScriptMgr->OnAddCreaturePassenger(this, creature);
    return creature;
}

GameObject* MotionTransport::CreateGOPassenger(ObjectGuid::LowType guid, GameObjectData const* data)
{
#ifdef LICH_KING
    Map* map = GetMap();
    if (map->GetGORespawnTime(guid))
        return nullptr;

    GameObject* go = new GameObject();
    ASSERT(m_goInfo->type != GAMEOBJECT_TYPE_TRANSPORT));

    if (!go->LoadGameObjectFromDB(guid, map, false))
    {
        delete go;
        return NULL;
    }

    float x = data->posX;
    float y = data->posY;
    float z = data->posZ;
    float o = data->orientation;

    go->SetTransport(this);
    go->SetTransOffset(x, y, z, o);
    CalculatePassengerPosition(x, y, z, &o);
    go->Relocate(x, y, z, o);

    if (!go->IsPositionValid())
    {
        TC_LOG_ERROR("entities.transport", "GameObject (guidlow %d, entry %d) not created. Suggested coordinates aren't valid (X: %f Y: %f)", go->GetGUID().GetCounter(), go->GetEntry(), go->GetPositionX(), go->GetPositionY());
        delete go;
        return NULL;
    }

    if (!map->AddToMap(go))
    {
        delete go;
        return NULL;
    }

    _staticPassengers.insert(go);
    return go;
#else
    //no gameobject on transports before LK ?
    return nullptr;
#endif
}

void MotionTransport::LoadStaticPassengers()
{
    if (PassengersLoaded())
        return;
    SetPassengersLoaded(true);
    if (uint32 mapId = GetGOInfo()->moTransport.mapID)
    {
        CellObjectGuidsMap const& cells = sObjectMgr->GetMapObjectGuids(mapId, GetMap()->GetSpawnMode());
        CellGuidSet::const_iterator guidEnd;
        for (const auto & cell : cells)
        {
            // Creatures on transport
            guidEnd = cell.second.creatures.end();
            for (auto guidItr = cell.second.creatures.begin(); guidItr != guidEnd; ++guidItr)
                CreateNPCPassenger(*guidItr, sObjectMgr->GetCreatureData(*guidItr));

            // GameObjects on transport
            guidEnd = cell.second.gameobjects.end();
            for (auto guidItr = cell.second.gameobjects.begin(); guidItr != guidEnd; ++guidItr)
                CreateGOPassenger(*guidItr, sObjectMgr->GetGameObjectData(*guidItr));
        }
    }
}

void MotionTransport::UnloadStaticPassengers()
{
    SetPassengersLoaded(false);
    while (!_staticPassengers.empty())
    {
        WorldObject* obj = *_staticPassengers.begin();
        obj->AddObjectToRemoveList();   // also removes from _staticPassengers
    }
}

void MotionTransport::UnloadNonStaticPassengers()
{
    for (auto itr = _passengers.begin(); itr != _passengers.end(); )
    {
        if ((*itr)->GetTypeId() == TYPEID_PLAYER)
        {
            ++itr;
            continue;
        }
        auto itr2 = itr++;
        (*itr2)->AddObjectToRemoveList();
    }
}

void MotionTransport::EnableMovement(bool enabled)
{
#ifdef LICH_KING
    if (!GetGOInfo()->moTransport.canBeStopped)
        return;

    _pendingStop = !enabled;
#endif
}

void MotionTransport::MoveToNextWaypoint()
{
    // Clear events flagging
    _triggeredArrivalEvent = false;
    _triggeredDepartureEvent = false;

    // Set frames
    _currentFrame = _nextFrame++;
    if (_nextFrame == GetKeyFrames().end())
        _nextFrame = GetKeyFrames().begin();
}

float MotionTransport::CalculateSegmentPos(float now)
{
    KeyFrame const& frame = *_currentFrame;
    const float speed = float(m_goInfo->moTransport.moveSpeed);
    const float accel = float(m_goInfo->moTransport.accelRate);
    float timeSinceStop = frame.TimeFrom + (now - (1.0f / IN_MILLISECONDS) * frame.DepartureTime);
    float timeUntilStop = frame.TimeTo - (now - (1.0f / IN_MILLISECONDS) * frame.DepartureTime);
    float segmentPos, dist;
    float accelTime = _transportInfo->accelTime;
    float accelDist = _transportInfo->accelDist;
    // calculate from nearest stop, less confusing calculation...
    if (timeSinceStop < timeUntilStop)
    {
        if (timeSinceStop < accelTime)
            dist = 0.5f * accel * timeSinceStop * timeSinceStop;
        else
            dist = accelDist + (timeSinceStop - accelTime) * speed;
        segmentPos = dist - frame.DistSinceStop;
    }
    else
    {
        if (timeUntilStop < _transportInfo->accelTime)
            dist = 0.5f * accel * timeUntilStop * timeUntilStop;
        else
            dist = accelDist + (timeUntilStop - accelTime) * speed;
        segmentPos = frame.DistUntilStop - dist;
    }

    return segmentPos / frame.NextDistFromPrev;
}

bool MotionTransport::TeleportTransport(uint32 newMapid, float x, float y, float z, float o)
{
    Map const* oldMap = GetMap();

    if (oldMap->GetId() != newMapid)
    {
        _delayedTeleport = true;
        UnloadStaticPassengers();
        return true;
    }
    else
    {
        /*
        sun: Disabled, dunno why some transports have strange teleport frames (Grom'gol/Undercity). Enabling this make players leave the transport

        // Teleport players, they need to know it
        for (PassengerSet::iterator itr = _passengers.begin(); itr != _passengers.end(); ++itr)
        {
            if ((*itr)->GetTypeId() == TYPEID_PLAYER)
            {
                float destX, destY, destZ, destO;
                (*itr)->GetMovementInfo().transport.pos.GetPosition(destX, destY, destZ, destO);
                TransportBase::CalculatePassengerPosition(destX, destY, destZ, &destO, x, y, z, o);

                (*itr)->ToUnit()->NearTeleportTo(destX, destY, destZ, destO);
            }
        }

        */
        UpdatePosition(x, y, z, o);

        return false;
    }
}

void MotionTransport::DelayedTeleportTransport()
{
    if (!_delayedTeleport)
        return;

    _delayedTeleport = false;

    uint32 newMapId = _nextFrame->Node->MapID;

    float x = _nextFrame->Node->LocX,
        y = _nextFrame->Node->LocY,
        z = _nextFrame->Node->LocZ,
        o = _nextFrame->InitialOrientation;

    Map* newMap = sMapMgr->CreateBaseMap(newMapId);
    GetMap()->RemoveFromMap<MotionTransport>(this, false);
    newMap->LoadGrid(x, y); // sunwell: load before adding passengers to new map
    SetMap(newMap);

    PassengerSet _passengersCopy = _passengers;
    for (auto itr = _passengersCopy.begin(); itr != _passengersCopy.end(); )
    {
        WorldObject* obj = (*itr++);

        if (_passengers.find(obj) == _passengers.end())
            continue;

        switch (obj->GetTypeId())
        {
        case TYPEID_UNIT:
            _passengers.erase(obj);
            if (!obj->ToCreature()->IsPet())
                obj->ToCreature()->DespawnOrUnsummon();
            break;
        case TYPEID_GAMEOBJECT:
            _passengers.erase(obj);
            obj->ToGameObject()->Delete();
            break;
        case TYPEID_DYNAMICOBJECT:
            _passengers.erase(obj);
            if (Unit* caster = obj->ToDynObject()->GetCaster())
                if (Spell* s = caster->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
                    if (obj->ToDynObject()->GetSpellId() == s->GetSpellInfo()->Id)
                    {
                        s->SendChannelUpdate(0);
                        s->SendInterrupted(0);
                        caster->RemoveOwnedAura(s->GetSpellInfo()->Id, caster->GetGUID());
                    }
            obj->AddObjectToRemoveList();
            break;
        case TYPEID_PLAYER:
        {
            float destX, destY, destZ, destO;
            obj->GetMovementInfo().transport.pos.GetPosition(destX, destY, destZ, destO);
            TransportBase::CalculatePassengerPosition(destX, destY, destZ, &destO, x, y, z, o);
            if (!obj->ToPlayer()->TeleportTo(newMapId, destX, destY, destZ, destO, TELE_TO_NOT_LEAVE_TRANSPORT | TELE_TO_NOT_UNSUMMON_PET | TELE_TO_TRANSPORT_TELEPORT))
                _passengers.erase(obj);
        }
        break;
        default:
            break;
        }
    }

    Relocate(x, y, z, o);
    GetMap()->AddToMap<MotionTransport>(this);

    LoadStaticPassengers();
}

void MotionTransport::UpdatePassengerPositions(PassengerSet& passengers)
{
    for (auto passenger : passengers)
    {
        // transport teleported but passenger not yet (can happen for players)
        if (passenger->GetMap() != GetMap())
            continue;

#ifdef LICH_KING
        // if passenger is on vehicle we have to assume the vehicle is also on transport and its the vehicle that will be updating its passengers
        if (Unit* unit = passenger->ToUnit())
            if (unit->GetVehicle())
                continue;
#endif

        // Do not use Unit::UpdatePosition here, we don't want to remove auras as if regular movement occurred
        float x, y, z, o;
        passenger->GetMovementInfo().transport.pos.GetPosition(x, y, z, o);
        CalculatePassengerPosition(x, y, z, &o);

        // check if position is valid
        if (!Trinity::IsValidMapCoord(x, y, z))
            continue;

        switch (passenger->GetTypeId())
        {
        case TYPEID_UNIT:
        {
            Creature* creature = passenger->ToCreature();
            GetMap()->CreatureRelocation(creature, x, y, z, o);

            creature->GetTransportHomePosition(x, y, z, o);
            CalculatePassengerPosition(x, y, z, &o);
            creature->SetHomePosition(x, y, z, o);
        }
        break;
        case TYPEID_PLAYER:
            if (passenger->IsInWorld() && !passenger->ToPlayer()->IsBeingTeleported()) 
            {
                GetMap()->PlayerRelocation(passenger->ToPlayer(), x, y, z, o);
                passenger->ToPlayer()->SetFallInformation(0, passenger->GetPositionZ());
            }
            break;
        case TYPEID_GAMEOBJECT:
            GetMap()->GameObjectRelocation(passenger->ToGameObject(), x, y, z, o);
            break;
        case TYPEID_DYNAMICOBJECT:
            GetMap()->DynamicObjectRelocation(passenger->ToDynObject(), x, y, z, o);
            break;
        default:
            break;
        }
    }
}

void MotionTransport::JustStopped()
{
    //not working for now... Player does not receive sound packet because of check player->HaveAtClient in MessageDistDeliverer::SendPacket
    /*
    if (sWorld->getConfig(CONFIG_TRANSPORT_DOCKING_SOUNDS))
    {
        switch (GetEntry())
        {
        case 176495:
        case 164871:
        case 175080:
            PlayDirectSound(11804); break;		// ZeppelinDocked
        case 20808:
        case 181646:
        case 176231:
        case 176244:
        case 176310:
        case 177233:
            PlayDirectSound(5154); break;		// ShipDocked
        default:
            PlayDirectSound(5495); break;		// BoatDockingWarning
        }
    }
    */
}

void MotionTransport::DoEventIfAny(KeyFrame const& node, bool departure)
{
    if (uint32 eventid = departure ? node.Node->departureEventID : node.Node->arrivalEventID)
    {
        //TC_LOG_DEBUG("maps.script", "Taxi %s event %u of node %u of %s path", departure ? "departure" : "arrival", eventid, node.Node->index, GetName().c_str());
        GetMap()->ScriptsStart(sEventScripts, eventid, this, this);
        EventInform(eventid);
    }
}

// sunwell: StaticTransport below

StaticTransport::StaticTransport() : Transport(), _needDoInitialRelocation(false)
{
#ifdef LICH_KING
    m_updateFlag = UPDATEFLAG_TRANSPORT | UPDATEFLAG_LOWGUID | UPDATEFLAG_STATIONARY_POSITION | UPDATEFLAG_ROTATION;
#else
    m_updateFlag = UPDATEFLAG_TRANSPORT | UPDATEFLAG_LOWGUID | UPDATEFLAG_HIGHGUID | UPDATEFLAG_STATIONARY_POSITION;
#endif
}

StaticTransport::~StaticTransport()
{
    ASSERT(_passengers.empty());
}

bool StaticTransport::Create(ObjectGuid::LowType guidlow, uint32 name_id, Map* map, uint32 phaseMask, Position const& pos, G3D::Quat const& rotation, uint32 animprogress, GOState go_state, uint32 artKit, bool dynamic, uint32 spawnid)
{
    ASSERT(map);
    SetMap(map);

    Relocate(pos);
    m_stationaryPosition.Relocate(pos);

    if (!IsPositionValid())
    {
        TC_LOG_ERROR("entities.gameobject", "Gameobject (GUID: %u Entry: %u) not created. Suggested coordinates isn't valid (X: %f Y: %f)", guidlow, name_id, pos.GetPositionX(), pos.GetPositionY());
        return false;
    }

    SetPhaseMask(phaseMask, false);
    SetZoneScript();
    if (m_zoneScript)
    {
        name_id = m_zoneScript->GetGameObjectEntry(guidlow, name_id);
        if (!name_id)
            return false;
    }

    GameObjectTemplate const* goinfo = sObjectMgr->GetGameObjectTemplate(name_id);
    if (!goinfo)
    {
        TC_LOG_ERROR("sql.sql", "Gameobject (GUID: %u Entry: %u) not created: non-existing entry in `gameobject_template`. Map: %u (X: %f Y: %f Z: %f)", guidlow, name_id, map->GetId(), pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ());
        return false;
    }

#ifdef LICH_KING
    Object::_Create(guidlow, 0, HighGuid::Transport);
#else
    //Object::_Create(guidlow, goinfo->entry, HighGuid::GameObject); //entry doesn't seem necessary, but keeping this in comment in case this causes problem. If you change this, you'll also need to change it in Player::LoadFromDB
    Object::_Create(guidlow, 0, HighGuid::GameObject);
#endif

    m_goInfo = goinfo;

    if (goinfo->type >= MAX_GAMEOBJECT_TYPE)
    {
        TC_LOG_ERROR("sql.sql", "Gameobject (GUID: %u Entry: %u) not created: non-existing GO type '%u' in `gameobject_template`. It will crash client if created.", guidlow, name_id, goinfo->type);
        return false;
    }

    // sunwell: temporarily calculate WorldRotation from orientation, do so until values in db are correct
    //SetWorldRotation( /*for StaticTransport we need 2 rotation Quats in db for World- and Path- Rotation*/ );
#ifdef LICH_KING
    SetWorldRotationAngles(NormalizeOrientation(GetOrientation()), 0.0f, 0.0f);
#else
    SetFloatValue(GAMEOBJECT_POS_X, pos.GetPositionX());
    SetFloatValue(GAMEOBJECT_POS_Y, pos.GetPositionY());
    SetFloatValue(GAMEOBJECT_POS_Z, pos.GetPositionZ());
    SetFloatValue(GAMEOBJECT_FACING, pos.GetOrientation());                  //this is not facing angle
#endif
    // sunwell: PathRotation for StaticTransport (only StaticTransports have PathRotation)
    SetTransportPathRotation(rotation);

    SetObjectScale(goinfo->size);

    SetUInt32Value(GAMEOBJECT_FACTION, goinfo->faction);
    SetUInt32Value(GAMEOBJECT_FLAGS, goinfo->flags);

    SetEntry(goinfo->entry);
    SetName(goinfo->name);

    SetDisplayId(goinfo->displayId);

    if (!m_model)
        m_model = CreateModel();

    SetGoType(GAMEOBJECT_TYPE_TRANSPORT);
    //SetGoState(goinfo->transport.startOpen ? GO_STATE_ACTIVE : GO_STATE_READY);
    SetGoAnimProgress(animprogress);

    SetGoArtKit(artKit);
    
    m_goValue.Transport.AnimationInfo = sTransportMgr->GetTransportAnimInfo(goinfo->entry);
    //ASSERT(m_goValue.Transport.AnimationInfo);
    //ASSERT(m_goValue.Transport.AnimationInfo->TotalTime > 0);
    SetPauseTime(goinfo->transport.pauseAtTime);
    if (goinfo->transport.startOpen && goinfo->transport.pauseAtTime)
    {
        SetPathProgress(goinfo->transport.pauseAtTime);
        _needDoInitialRelocation = true;
    }
    else
        SetPathProgress(0);

    LastUsedScriptID = GetScriptId();
    AIM_Initialize();

    if (spawnid)
        m_spawnId = spawnid;

    if(m_name == "Subway") //I'd like to remove this but we need a way to keep the position updating when no players are nearby
        this->SetKeepActive(true); 
    return true;
}

void StaticTransport::CleanupsBeforeDelete(bool finalCleanup /*= true*/)
{
    while (!_passengers.empty())
    {
        WorldObject* obj = *_passengers.begin();
        RemovePassenger(obj);
    }

    GameObject::CleanupsBeforeDelete(finalCleanup);
}

void StaticTransport::BuildUpdate(UpdateDataMapType& data_map, UpdatePlayerSet&)
{
    Map::PlayerList const& players = GetMap()->GetPlayers();
    if (players.isEmpty())
        return;

    for (const auto & player : players)
        BuildFieldsUpdate(player.GetSource(), data_map);

    ClearUpdateMask(true);
}

void StaticTransport::Update(uint32 diff)
{
    GameObject::Update(diff);

    if (!IsInWorld())
        return;

    if (!m_goValue.Transport.AnimationInfo)
        return;

    if (_needDoInitialRelocation)
    {
        _needDoInitialRelocation = false;
        RelocateToProgress(GetPathProgress());
    }

    if (GetPauseTime())
    {
        if (GetGoState() == GO_STATE_READY)
        {
            if (GetPathProgress() == 0) // waiting at it's destination for state change, do nothing
                return;

            if (GetPathProgress() < GetPauseTime()) // GOState has changed before previous state was reached, move to new destination immediately
                SetPathProgress(0);
            else if (GetPathProgress() + diff < GetPeriod())
                SetPathProgress(GetPathProgress() + diff);
            else
                SetPathProgress(0);
        }
        else
        {
            if (GetPathProgress() == GetPauseTime()) // waiting at it's destination for state change, do nothing
                return;

            if (GetPathProgress() > GetPauseTime()) // GOState has changed before previous state was reached, move to new destination immediately
                SetPathProgress(GetPauseTime());
            else if (GetPathProgress() + diff < GetPauseTime())
                SetPathProgress(GetPathProgress() + diff);
            else
                SetPathProgress(GetPauseTime());
        }
    }
    else
    {
        SetPathProgress(GetPathProgress() + diff);
        if (GetPathProgress() >= GetPeriod())
            SetPathProgress(GetPathProgress() % GetPeriod());
    }

    RelocateToProgress(GetPathProgress());
}

void StaticTransport::RelocateToProgress(uint32 progress)
{
    TransportAnimationEntry const *curr = nullptr, *next = nullptr;
    float percPos;
    if (m_goValue.Transport.AnimationInfo->GetAnimNode(progress, curr, next, percPos))
    {
        // curr node offset
        G3D::Vector3 pos = G3D::Vector3(curr->X, curr->Y, curr->Z);

        // move by percentage of segment already passed
        pos += G3D::Vector3(percPos * (next->X - curr->X), percPos * (next->Y - curr->Y), percPos * (next->Z - curr->Z));

        // rotate path by PathRotation
        // sunwell: PathRotation in db is only simple orientation rotation, so don't use sophisticated and not working code
        // reminder: WorldRotation only influences model rotation, not the path
        float sign = GetFloatValue(GAMEOBJECT_PARENTROTATION + 2) >= 0.0f ? 1.0f : -1.0f;
        float pathRotAngle = sign * 2.0f * acos(GetFloatValue(GAMEOBJECT_PARENTROTATION + 3));
        float cs = cos(pathRotAngle), sn = sin(pathRotAngle);
        float nx = pos.x * cs - pos.y * sn;
        float ny = pos.x * sn + pos.y * cs;
        pos.x = nx;
        pos.y = ny;

        // add stationary position to the calculated offset
        pos += G3D::Vector3(GetStationaryX(), GetStationaryY(), GetStationaryZ());

        // rotate by AnimRotation at current segment
        // sunwell: AnimRotation in dbc is only simple orientation rotation, so don't use sophisticated and not working code
#ifdef LICH_KING
        G3D::Quat currRot, nextRot;
        float percRot;
        m_goValue.Transport.AnimationInfo->GetAnimRotation(progress, currRot, nextRot, percRot);
        float signCurr = currRot.z >= 0.0f ? 1.0f : -1.0f;
        float oriRotAngleCurr = signCurr * 2.0f * acos(currRot.w);
        float signNext = nextRot.z >= 0.0f ? 1.0f : -1.0f;
        float oriRotAngleNext = signNext * 2.0f * acos(nextRot.w);
        float oriRotAngle = oriRotAngleCurr + percRot * (oriRotAngleNext - oriRotAngleCurr);
#else
        float oriRotAngle = 0.0f;
#endif
        // check if position is valid
        if (!Trinity::IsValidMapCoord(pos.x, pos.y, pos.z))
            return;

        // update position to new one
        // also adding simplified orientation rotation here
        UpdatePosition(pos.x, pos.y, pos.z, NormalizeOrientation(GetStationaryO() + oriRotAngle));
    }
}

void StaticTransport::UpdatePosition(float x, float y, float /*z*/, float o)
{
    if (!GetMap()->IsGridLoaded(x, y)) // sunwell: should not happen, but just in case
        GetMap()->LoadGrid(x, y);

    //currently broken so, let client handle position for now
#ifdef LICH_KING
    //I'm pretty sure we're not supposed to send anything on BC
    GetMap()->GameObjectRelocation(this, x, y, z, o); // this also relocates the model
#endif
    //SummonCreature(VISUAL_WAYPOINT, { x, y, z }, TEMPSUMMON_TIMED_DESPAWN, 10000);
    //UpdatePassengerPositions();
}

void StaticTransport::UpdatePassengerPositions()
{
    for (auto passenger : _passengers)
    {
        
#ifdef LICH_KING
        // if passenger is on vehicle we have to assume the vehicle is also on transport and its the vehicle that will be updating its passengers
        if (Unit* unit = passenger->ToUnit())
            if (unit->GetVehicle())
                continue;
#endif

        // Do not use Unit::UpdatePosition here, we don't want to remove auras as if regular movement occurred
        float x, y, z, o;
        passenger->GetMovementInfo().transport.pos.GetPosition(x, y, z, o);
        CalculatePassengerPosition(x, y, z, &o);

        // check if position is valid
        if (!Trinity::IsValidMapCoord(x, y, z))
            continue;

        switch (passenger->GetTypeId())
        {
        case TYPEID_UNIT:
            GetMap()->CreatureRelocation(passenger->ToCreature(), x, y, z, o);
            break;
        case TYPEID_PLAYER:
            if (passenger->IsInWorld())
                GetMap()->PlayerRelocation(passenger->ToPlayer(), x, y, z, o);
            break;
        case TYPEID_GAMEOBJECT:
            GetMap()->GameObjectRelocation(passenger->ToGameObject(), x, y, z, o);
            break;
        case TYPEID_DYNAMICOBJECT:
            GetMap()->DynamicObjectRelocation(passenger->ToDynObject(), x, y, z, o);
            break;
        default:
            break;
        }
    }
}

void StaticTransport::AddPassenger(WorldObject* passenger, bool calcPassengerPosition /*= false*/)
{
    if (_passengers.insert(passenger).second)
    {
        if (Transport* t = passenger->GetTransport()) // SHOULD NEVER HAPPEN
            t->RemovePassenger(passenger);

        passenger->SetTransport(this);
        if (calcPassengerPosition)
        {
            float x, y, z, o;
            passenger->GetPosition(x, y, z, o);
            CalculatePassengerOffset(x, y, z, &o);
            passenger->SetTransOffset(x, y, z, o);
        }

        if (Player* plr = passenger->ToPlayer())
            sScriptMgr->OnAddPassenger(ToTransport(), plr);
    }
}

void StaticTransport::RemovePassenger(WorldObject* passenger)
{
    if (_passengers.erase(passenger))
    {
        passenger->SetTransport(nullptr);

        if (Player* plr = passenger->ToPlayer())
        {
            sScriptMgr->OnRemovePassenger(ToTransport(), plr);
            plr->SetFallInformation(0, plr->GetPositionZ());
        }
    }
}