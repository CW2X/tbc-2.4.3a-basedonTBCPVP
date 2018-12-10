/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "TransportMgr.h"
#include "Transport.h"
#include "MoveSpline.h"
#include "MapManager.h"
#include "ObjectMgr.h"
#include "MMapFactory.h"
#include "InstanceScript.h"

TransportTemplate::~TransportTemplate()
{
    // Collect shared pointers into a set to avoid deleting the same memory more than once
    std::set<TransportSpline*> splines;
    for (auto & keyFrame : keyFrames)
        splines.insert(keyFrame.Spline);

    for (auto spline : splines)
        delete spline;
}

TransportMgr::TransportMgr() { }

TransportMgr::~TransportMgr() { }

void TransportMgr::Unload()
{
    _transportTemplates.clear();
}

void TransportMgr::LoadTransportTemplates()
{
    uint32 oldMSTime = GetMSTime();
    
    QueryResult result = WorldDatabase.Query("SELECT entry FROM gameobject_template WHERE type = 15 ORDER BY entry ASC");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded %u transports", 0 );
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();
        uint32 entry = fields[0].GetUInt32();
        GameObjectTemplate const* goInfo = sObjectMgr->GetGameObjectTemplate(entry);
        if (goInfo == nullptr)
        {
            TC_LOG_ERROR("sql.sql", "Transport %u has no associated GameObjectTemplate from `gameobject_template` , skipped.", entry);
            continue;
        }

        if (goInfo->moTransport.taxiPathId >= sTaxiPathNodesByPath.size())
        {
            TC_LOG_ERROR("sql.sql", "Transport %u (name: %s) has an invalid path specified in `gameobject_template`.`data0` (%u) field, skipped.", entry, goInfo->name.c_str(), goInfo->moTransport.taxiPathId);
            continue;
        }

        // paths are generated per template, saves us from generating it again in case of instanced transports
        TransportTemplate& transport = _transportTemplates[entry];
        transport.entry = entry;
        GeneratePath(goInfo, &transport);
        MMAP::MMapFactory::createOrGetMMapManager()->loadGameObject(goInfo->displayId);

        // transports in instance are only on one map
        if (transport.inInstance)
            _instanceTransports[*transport.mapsUsed.begin()].insert(entry);

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u transport templates in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

class SplineRawInitializer
{
public:
    SplineRawInitializer(Movement::PointsArray& points) : _points(points) { }

    void operator()(uint8& mode, bool& cyclic, Movement::PointsArray& points, int& lo, int& hi) const
    {
        mode = Movement::SplineBase::ModeCatmullrom;
        cyclic = false;
        points.assign(_points.begin(), _points.end());
        lo = 1;
        hi = points.size() - 2;
    }

    Movement::PointsArray& _points;
};

void TransportMgr::GeneratePath(GameObjectTemplate const* goInfo, TransportTemplate* transport)
{
    uint32 pathId = goInfo->moTransport.taxiPathId;
    if(!pathId)
    {
        TC_LOG_ERROR("sql.sql","Gameobject %u is marked as transport but has no pathId",goInfo->entry);
        return;
    }
    TaxiPathNodeList const& path = sTaxiPathNodesByPath[pathId];
    std::vector<KeyFrame>& keyFrames = transport->keyFrames;
    Movement::PointsArray splinePath, allPoints;
    bool mapChange = false;
    for (auto i : path)
        allPoints.push_back(G3D::Vector3(i->LocX, i->LocY, i->LocZ));

    // Add extra points to allow derivative calculations for all path nodes
    allPoints.insert(allPoints.begin(), allPoints.front().lerp(allPoints[1], -0.2f));
    allPoints.push_back(allPoints.back().lerp(allPoints[allPoints.size() - 2], -0.2f));
    allPoints.push_back(allPoints.back().lerp(allPoints[allPoints.size() - 2], -1.0f));

    SplineRawInitializer initer(allPoints);
    TransportSpline orientationSpline;
    orientationSpline.init_spline_custom(initer);
    orientationSpline.initLengths();

    for (size_t i = 0; i < path.size(); ++i)
    {
        if (!mapChange)
        {
            TaxiPathNodeEntry const* node_i = path[i];
            if (i != path.size() - 1 && (node_i->actionFlag & 1 || node_i->MapID != path[i + 1]->MapID))
            {
                keyFrames.back().Teleport = true;
                mapChange = true;
            }
            else
            {
                KeyFrame k(node_i);
                G3D::Vector3 h;
                orientationSpline.evaluate_derivative(i + 1, 0.0f, h);
                k.InitialOrientation = Position::NormalizeOrientation(atan2(h.y, h.x) + M_PI);

                keyFrames.push_back(k);
                splinePath.push_back(G3D::Vector3(node_i->LocX, node_i->LocY, node_i->LocZ));
                transport->mapsUsed.insert(k.Node->MapID);
            }
        }
        else
            mapChange = false;
    }

    if (splinePath.size() >= 2)
    {
        // Remove special catmull-rom spline points
        if (!keyFrames.front().IsStopFrame() && !keyFrames.front().Node->arrivalEventID && !keyFrames.front().Node->departureEventID)
        {
            splinePath.erase(splinePath.begin());
            keyFrames.erase(keyFrames.begin());
        }
        if (!keyFrames.back().IsStopFrame() && !keyFrames.back().Node->arrivalEventID && !keyFrames.back().Node->departureEventID)
        {
            splinePath.pop_back();
            keyFrames.pop_back();
        }
    }

    ASSERT(!keyFrames.empty());

    if (transport->mapsUsed.size() > 1)
    {
        for (uint32 itr : transport->mapsUsed)
        {
            ASSERT(!sMapStore.LookupEntry(itr)->Instanceable());
        }

        transport->inInstance = false;
    }
    else
        transport->inInstance = sMapStore.LookupEntry(*transport->mapsUsed.begin())->Instanceable();

    // last to first is always "teleport", even for closed paths
    keyFrames.back().Teleport = true;

    const float speed = float(goInfo->moTransport.moveSpeed);
    const float accel = float(goInfo->moTransport.accelRate);
    const float accel_dist = 0.5f * speed * speed / accel;

    transport->accelTime = speed / accel;
    transport->accelDist = accel_dist;

    int32 firstStop = -1;
    int32 lastStop = -1;

    // first cell is arrived at by teleportation :S
    keyFrames[0].DistFromPrev = 0;
    keyFrames[0].Index = 1;
    if (keyFrames[0].IsStopFrame())
    {
        firstStop = 0;
        lastStop = 0;
    }

    // find the rest of the distances between key points
    // Every path segment has its own spline
    size_t start = 0;
    for (size_t i = 1; i < keyFrames.size(); ++i)
    {
        if (keyFrames[i - 1].Teleport || i + 1 == keyFrames.size())
        {
            size_t extra = !keyFrames[i - 1].Teleport ? 1 : 0;
            auto spline = new TransportSpline();
            spline->init_spline(&splinePath[start], i - start + extra, Movement::SplineBase::ModeCatmullrom, keyFrames[0].InitialOrientation); //not sure about this initial orientation
            spline->initLengths();
            for (size_t j = start; j < i + extra; ++j)
            {
                keyFrames[j].Index = j - start + 1;
                keyFrames[j].DistFromPrev = spline->length(j - start, j + 1 - start);
                if (j > 0)
                    keyFrames[j - 1].NextDistFromPrev = keyFrames[j].DistFromPrev;
                keyFrames[j].Spline = spline;
            }

            if (keyFrames[i - 1].Teleport)
            {
                keyFrames[i].Index = i - start + 1;
                keyFrames[i].DistFromPrev = 0.0f;
                keyFrames[i - 1].NextDistFromPrev = 0.0f;
                keyFrames[i].Spline = spline;
            }

            start = i;
        }

        if (keyFrames[i].IsStopFrame())
        {
            // remember first stop frame
            if (firstStop == -1)
                firstStop = i;
            lastStop = i;
        }
    }

    keyFrames.back().NextDistFromPrev = keyFrames.front().DistFromPrev;

    if (firstStop == -1 || lastStop == -1)
        firstStop = lastStop = 0;

    // at stopping keyframes, we define distSinceStop == 0,
    // and distUntilStop is to the next stopping keyframe.
    // this is required to properly handle cases of two stopping frames in a row (yes they do exist)
    float tmpDist = 0.0f;
    for (size_t i = 0; i < keyFrames.size(); ++i)
    {
        int32 j = (i + lastStop) % keyFrames.size();
        if (keyFrames[j].IsStopFrame() || j == lastStop)
            tmpDist = 0.0f;
        else
            tmpDist += keyFrames[j].DistFromPrev;
        keyFrames[j].DistSinceStop = tmpDist;
    }

    tmpDist = 0.0f;
    for (int32 i = int32(keyFrames.size()) - 1; i >= 0; i--)
    {
        int32 j = (i + firstStop) % keyFrames.size();
        tmpDist += keyFrames[(j + 1) % keyFrames.size()].DistFromPrev;
        keyFrames[j].DistUntilStop = tmpDist;
        if (keyFrames[j].IsStopFrame() || j == firstStop)
            tmpDist = 0.0f;
    }

    for (auto & keyFrame : keyFrames)
    {
        float total_dist = keyFrame.DistSinceStop + keyFrame.DistUntilStop;
        if (total_dist < 2 * accel_dist) // won't reach full speed
        {
            if (keyFrame.DistSinceStop < keyFrame.DistUntilStop) // is still accelerating
            {
                // calculate accel+brake time for this short segment
                float segment_time = 2.0f * sqrt((keyFrame.DistUntilStop + keyFrame.DistSinceStop) / accel);
                // substract acceleration time
                keyFrame.TimeTo = segment_time - sqrt(2 * keyFrame.DistSinceStop / accel);
            }
            else // slowing down
                keyFrame.TimeTo = sqrt(2 * keyFrame.DistUntilStop / accel);
        }
        else if (keyFrame.DistSinceStop < accel_dist) // still accelerating (but will reach full speed)
        {
            // calculate accel + cruise + brake time for this long segment
            float segment_time = (keyFrame.DistUntilStop + keyFrame.DistSinceStop) / speed + (speed / accel);
            // substract acceleration time
            keyFrame.TimeTo = segment_time - sqrt(2 * keyFrame.DistSinceStop / accel);
        }
        else if (keyFrame.DistUntilStop < accel_dist) // already slowing down (but reached full speed)
            keyFrame.TimeTo = sqrt(2 * keyFrame.DistUntilStop / accel);
        else // at full speed
            keyFrame.TimeTo = (keyFrame.DistUntilStop / speed) + (0.5f * speed / accel);
    }

    // calculate tFrom times from tTo times
    float segmentTime = 0.0f;
    for (size_t i = 0; i < keyFrames.size(); ++i)
    {
        int32 j = (i + lastStop) % keyFrames.size();
        if (keyFrames[j].IsStopFrame() || j == lastStop)
            segmentTime = keyFrames[j].TimeTo;
        keyFrames[j].TimeFrom = segmentTime - keyFrames[j].TimeTo;
    }

    // calculate path times
    keyFrames[0].ArriveTime = 0;
    float curPathTime = 0.0f;
    if (keyFrames[0].IsStopFrame())
    {
        curPathTime = float(keyFrames[0].Node->delay);
        keyFrames[0].DepartureTime = uint32(curPathTime * IN_MILLISECONDS);
    }

    for (size_t i = 1; i < keyFrames.size(); ++i)
    {
        curPathTime += keyFrames[i - 1].TimeTo;
        if (keyFrames[i].IsStopFrame())
        {
            keyFrames[i].ArriveTime = uint32(curPathTime * IN_MILLISECONDS);
            keyFrames[i - 1].NextArriveTime = keyFrames[i].ArriveTime;
            curPathTime += float(keyFrames[i].Node->delay);
            keyFrames[i].DepartureTime = uint32(curPathTime * IN_MILLISECONDS);
        }
        else
        {
            curPathTime -= keyFrames[i].TimeTo;
            keyFrames[i].ArriveTime = uint32(curPathTime * IN_MILLISECONDS);
            keyFrames[i - 1].NextArriveTime = keyFrames[i].ArriveTime;
            keyFrames[i].DepartureTime = keyFrames[i].ArriveTime;
        }
    }

    keyFrames.back().NextArriveTime = keyFrames.back().DepartureTime;

    transport->pathTime = keyFrames.back().DepartureTime;
}

void TransportMgr::AddPathNodeToTransport(uint32 transportEntry, uint32 timeSeg, TransportAnimationEntry const* node)
{
    TransportAnimation& animNode = _transportAnimations[transportEntry];
    if (animNode.TotalTime < timeSeg)
        animNode.TotalTime = timeSeg;

    animNode.Path[timeSeg] = node;
}

MotionTransport* TransportMgr::CreateTransport(uint32 entry, ObjectGuid::LowType guid /*= 0*/, Map* map /*= NULL*/)
{
    // instance case, execute GetGameObjectEntry hook
    if (map)
    {
        // SetZoneScript() is called after adding to map, so fetch the script using map
        if (map->IsDungeon())
            if (InstanceScript* instance = static_cast<InstanceMap*>(map)->GetInstanceScript())
                entry = instance->GetGameObjectEntry(0, entry);

        if (!entry)
            return nullptr;
    }

    TransportTemplate const* tInfo = GetTransportTemplate(entry);
    if (!tInfo)
    {
        TC_LOG_ERROR("sql.sql", "Transport %u will not be loaded, `transport_template` missing", entry);
        return nullptr;
    }

    // create transport...
    auto trans = new MotionTransport();

    // ...at first waypoint
    TaxiPathNodeEntry const* startNode = tInfo->keyFrames.begin()->Node;
    uint32 mapId = startNode->MapID;
    float x = startNode->LocX;
    float y = startNode->LocY;
    float z = startNode->LocZ;
    float o = tInfo->keyFrames.begin()->InitialOrientation;

    // initialize the gameobject base
    ObjectGuid::LowType guidLow = guid ? guid : sObjectMgr->GetGenerator<HighGuid::Mo_Transport>().Generate();
    if (!trans->CreateMoTrans(guidLow, entry, mapId, x, y, z, o, 100))
    {
        delete trans;
        return nullptr;
    }

    if (MapEntry const* mapEntry = sMapStore.LookupEntry(mapId))
    {
        if (mapEntry->Instanceable() != tInfo->inInstance)
        {
            TC_LOG_ERROR("entities.transport", "Transport %u (name: %s) attempted creation in instance map (id: %u) but it is not an instanced transport!", entry, trans->GetName().c_str(), mapId);
            delete trans;
            return nullptr;
        }
    }

    // use preset map for instances (need to know which instance)
    trans->SetMap(map ? map : sMapMgr->CreateMap(mapId, nullptr));
    if (map && map->IsDungeon())
        trans->m_zoneScript = map->ToInstanceMap()->GetInstanceScript();

    // Passengers will be loaded once a player is near

    // update in loaded data (changing data only in this place)
    GameObjectData& data = sObjectMgr->NewOrExistGameObjectData(guidLow);

    // data->guid = guid don't must be update at save
    data.id = trans->GetEntry();
    data.spawnPoint.Relocate(trans->GetFloatValue(GAMEOBJECT_POS_X), trans->GetFloatValue(GAMEOBJECT_POS_Y), trans->GetFloatValue(GAMEOBJECT_POS_Z), trans->GetFloatValue(GAMEOBJECT_FACING));
    data.spawnPoint.m_mapId = trans->GetMapId();
    data.rotation = G3D::Quat(
        trans->GetFloatValue(GAMEOBJECT_PARENTROTATION + 0), 
        trans->GetFloatValue(GAMEOBJECT_PARENTROTATION + 1), 
        trans->GetFloatValue(GAMEOBJECT_PARENTROTATION + 2), 
        trans->GetFloatValue(GAMEOBJECT_PARENTROTATION + 3));
    data.spawntimesecs = 0;
    data.animprogress = trans->GetGoAnimProgress();
    data.go_state = trans->GetGoState();
    data.spawnMask = (1 << trans->GetMap()->GetSpawnMode());
    data.ArtKit = trans->GetUInt32Value(GAMEOBJECT_ARTKIT);

    // sunwell: transports are active so passengers can be relocated (grids must be loaded)
    ///trans->SetKeepActive(true);
    // Passengers will be loaded once a player is near
    HashMapHolder<MotionTransport>::Insert(trans);
    trans->GetMap()->AddToMap<MotionTransport>(trans);
    return trans;
}

void TransportMgr::SpawnContinentTransports()
{
    if (_transportTemplates.empty())
        return;

    uint32 oldMSTime = GetMSTime();

    QueryResult result = WorldDatabase.Query("SELECT entry FROM transports");

    uint32 count = 0;
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 entry = fields[0].GetUInt32();

            if (TransportTemplate const* tInfo = GetTransportTemplate(entry))
                if (!tInfo->inInstance)
                    if (CreateTransport(entry))
                        ++count;

        } while (result->NextRow());
    }

    TC_LOG_INFO("server.loading", ">> Spawned %u continent transports in %u ms", count, GetMSTimeDiffToNow(oldMSTime));

    // sunwell: preload grids for continent static transports
  /*  
    oldMSTime = GetMSTime();
    result = WorldDatabase.Query("SELECT map, position_x, position_y FROM gameobject g JOIN gameobject_template t ON g.id = t.entry WHERE t.type = 11");
    count = 0;
    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 mapId = fields[0].GetUInt16();
            float x = fields[1].GetFloat();
            float y = fields[2].GetFloat();

            MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
            if (mapEntry && !mapEntry->Instanceable())
                if (Map* map = sMapMgr->CreateBaseMap(mapId))
                {
                    map->LoadGrid(x, y);
                    ++count;
                }

        } while (result->NextRow());
    }
    TC_LOG_INFO("server.loading", ">> Preloaded grids for %u continent static transports in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    */
}

void TransportMgr::CreateInstanceTransports(Map* map)
{
    TransportInstanceMap::const_iterator mapTransports = _instanceTransports.find(map->GetId());

    // no transports here
    if (mapTransports == _instanceTransports.end() || mapTransports->second.empty())
        return;

    // create transports
    for (uint32 itr : mapTransports->second)
        CreateTransport(itr, 0, map);
}

bool TransportAnimation::GetAnimNode(uint32 time, TransportAnimationEntry const* &curr, TransportAnimationEntry const* &next, float &percPos) const
{
    if (Path.empty())
        return false;

    for (auto itr = Path.rbegin(); itr != Path.rend(); ++itr)
        if (time >= itr->first)
        {
            curr = itr->second;
            ASSERT(itr != Path.rbegin());
            --itr;
            next = itr->second;
            percPos = float(time - curr->TimeSeg) / float(next->TimeSeg - curr->TimeSeg);
            return true;
        }

    return false;
}

#ifdef LICH_KING
void TransportAnimation::GetAnimRotation(uint32 time, G3D::Quat &curr, G3D::Quat &next, float &percRot) const
{
    if (Rotations.empty())
    {
        curr = G3D::Quat(0.0f, 0.0f, 0.0f, 1.0f);
        next = G3D::Quat(0.0f, 0.0f, 0.0f, 1.0f);
        percRot = 0.0f;
        return;
    }

    for (TransportPathRotationContainer::const_reverse_iterator itr = Rotations.rbegin(); itr != Rotations.rend(); ++itr)
        if (time >= itr->first)
        {
            uint32 currSeg = itr->second->TimeSeg, nextSeg;
            curr = G3D::Quat(itr->second->X, itr->second->Y, itr->second->Z, itr->second->W);
            if (itr != Rotations.rbegin())
            {
                --itr;
                next = G3D::Quat(itr->second->X, itr->second->Y, itr->second->Z, itr->second->W);
                nextSeg = itr->second->TimeSeg;
            }
            else
            {
                next = G3D::Quat(Rotations.begin()->second->X, Rotations.begin()->second->Y, Rotations.begin()->second->Z, Rotations.begin()->second->W);
                nextSeg = this->TotalTime;
            }
            percRot = float(time - currSeg) / float(nextSeg - currSeg);
            return;
        }

    curr = G3D::Quat(0.0f, 0.0f, 0.0f, 1.0f);
    next = G3D::Quat(0.0f, 0.0f, 0.0f, 1.0f);
    percRot = 0.0f;
}
#endif