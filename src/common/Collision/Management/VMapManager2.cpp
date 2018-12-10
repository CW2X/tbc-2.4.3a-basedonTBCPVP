/*
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2010 MaNGOS <http://getmangos.com/>
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

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include "VMapManager2.h"
#include "MapTree.h"
#include "ModelInstance.h"
#include "WorldModel.h"
#include <G3D/Vector3.h>
#include "Log.h"
#include "VMapDefinitions.h"
#include "Util.h"

using G3D::Vector3;

namespace VMAP
{
    VMapManager2::VMapManager2()
    {
        GetLiquidFlagsPtr = &GetLiquidFlagsDummy;
        IsVMAPDisabledForPtr = &IsVMAPDisabledForDummy;
        thread_safe_environment = true;
    }

    VMapManager2::~VMapManager2()
    {
        for (auto & iInstanceMapTree : iInstanceMapTrees)
        {
            delete iInstanceMapTree.second;
        }
        for (auto & iLoadedModelFile : iLoadedModelFiles)
        {
            delete iLoadedModelFile.second.getModel();
        }
    }

    void VMapManager2::InitializeThreadUnsafe(const std::vector<uint32>& mapIds)
    {
        // the caller must pass the list of all mapIds that will be used in the VMapManager2 lifetime
        for (const uint32& mapId : mapIds)
            iInstanceMapTrees.insert(InstanceTreeMap::value_type(mapId, nullptr));

        thread_safe_environment = false;
    }

    Vector3 VMapManager2::convertPositionToInternalRep(float x, float y, float z) const
    {
        Vector3 pos;
        const float mid = 0.5f * 64.0f * 533.33333333f;
        pos.x = mid - x;
        pos.y = mid - y;
        pos.z = z;

        return pos;
    }

    // move to MapTree too?
    std::string VMapManager2::getMapFileName(unsigned int mapId)
    {
        std::stringstream fname;
        fname.width(3);
        fname << std::setfill('0') << mapId << std::string(MAP_FILENAME_EXTENSION2);

        return fname.str();
    }

    int VMapManager2::loadMap(const char* basePath, unsigned int mapId, int x, int y)
    {
        int result = VMAP_LOAD_RESULT_IGNORED;
        if (isMapLoadingEnabled())
        {
            if (_loadMap(mapId, basePath, x, y))
                result = VMAP_LOAD_RESULT_OK;
            else
                result = VMAP_LOAD_RESULT_ERROR;
        }

        return result;
    }

    InstanceTreeMap::const_iterator VMapManager2::GetMapTree(uint32 mapId) const
    {
        // return the iterator if found or end() if not found/NULL
        auto itr = iInstanceMapTrees.find(mapId);
        if (itr != iInstanceMapTrees.cend() && !itr->second)
            itr = iInstanceMapTrees.cend();

        return itr;
    }

    // load one tile (internal use only)
    bool VMapManager2::_loadMap(uint32 mapId, const std::string& basePath, uint32 tileX, uint32 tileY)
    {
        auto instanceTree = iInstanceMapTrees.find(mapId);
        if (instanceTree == iInstanceMapTrees.end())
        {
            if (thread_safe_environment)
                instanceTree = iInstanceMapTrees.insert(InstanceTreeMap::value_type(mapId, nullptr)).first;
            else
                ASSERT(false, "Invalid mapId %u tile [%u, %u] passed to VMapManager2 after startup in thread unsafe environment",
                mapId, tileX, tileY);
        }

        if (!instanceTree->second)
        {
            std::string mapFileName = getMapFileName(mapId);
            StaticMapTree* newTree = new StaticMapTree(mapId, basePath);
            if (!newTree->InitMap(mapFileName, this))
            {
                delete newTree;
                return false;
            }
            instanceTree->second = newTree;
        }

        return instanceTree->second->LoadMapTile(tileX, tileY, this);
    }

    void VMapManager2::unloadMap(unsigned int mapId)
    {
        auto instanceTree = iInstanceMapTrees.find(mapId);
        if (instanceTree != iInstanceMapTrees.end() && instanceTree->second)
        {
            instanceTree->second->UnloadMap(this);
            if (instanceTree->second->numLoadedTiles() == 0)
            {
                delete instanceTree->second;
                instanceTree->second = nullptr;
            }
        }
    }

    void VMapManager2::unloadMap(unsigned int mapId, int x, int y)
    {
        auto instanceTree = iInstanceMapTrees.find(mapId);
        if (instanceTree != iInstanceMapTrees.end() && instanceTree->second)
        {
            instanceTree->second->UnloadMapTile(x, y, this);
            if (instanceTree->second->numLoadedTiles() == 0)
            {
                delete instanceTree->second;
                instanceTree->second = nullptr;
            }
        }
    }

    bool VMapManager2::isInLineOfSight(unsigned int mapId, float x1, float y1, float z1, float x2, float y2, float z2, ModelIgnoreFlags ignoreFlags)
    {
        if (!isLineOfSightCalcEnabled() || IsVMAPDisabledForPtr(mapId, VMAP_DISABLE_LOS))
            return true;

        auto instanceTree = GetMapTree(mapId);
        if (instanceTree != iInstanceMapTrees.end())
        {
            Vector3 pos1 = convertPositionToInternalRep(x1, y1, z1);
            Vector3 pos2 = convertPositionToInternalRep(x2, y2, z2);
            if (pos1 != pos2)
            {
                return instanceTree->second->isInLineOfSight(pos1, pos2, ignoreFlags);
            }
        }

        return true;
    }

    /* same as getObjectHitPos but a bit more gentle, will try from a bit higher and return collision from there if it gets further */
    bool VMapManager2::getLeapHitPos(unsigned int mapId, float x1, float y1, float z1, float x2, float y2, float z2, float& rx, float &ry, float& rz, float modifyDist)
    {
        bool collision1; //collision from given position
        bool collision2; //collision from 6.0f higher
        float hitX1, hitX2, hitY1, hitY2, hitZ1, hitZ2;

        collision1 = getObjectHitPos(mapId, x1, y1, z1, x2, y2, z2, hitX1, hitY1, hitZ1, modifyDist);
        if(!collision1) //no collision from original position, we're okay
        {
            rx = x2;
            ry = y2;
            rz = z2;
            return false;
        }
        //collision occured, let's try from higher (do not set too high, else this will cause problem with places with multiple floors)
        collision2 = getObjectHitPos(mapId, x1, y1, z1+4.0f, x2, y2, z2, hitX2, hitY2, hitZ2, modifyDist);
        if (collision2)
        {
            //exclude collision2 if it is to high. The further the larger the difference allowed. For example here, at distance 20 the max delta is 12.0f.
            float maxAllowedDistance = GetDistance(x1, y1, hitX2, hitY2) * 0.6f;
            if (hitZ2 > (z2 + maxAllowedDistance))
            {
                rx = hitX1;
                ry = hitY1;
                rz = hitZ1;
                return true;
            }
        }
        else //no collision from there, okay too
        {
            rx = x2;
            ry = y2;
            rz = z2;
            return false;
        }

        //at this point both collided, get the one which got further
        float dist1 = GetDistance(x1,y1,hitX1,hitY1);
        float dist2 = GetDistance(x1,y1,hitX2,hitY2);

        if (   (dist1 > dist2) //if dist1 collided further
            || (std::fabs(dist1 - dist2) < 2.0f) ) //if collisions points were close, prefer collision1. This helps avoiding getting a position 1-2m higher in case where we tried to get a collision when against the wall.
        {
            rx = hitX1;
            ry = hitY1;
            rz = hitZ1;
        }
        else {
            rx = hitX2;
            ry = hitY2;
            rz = hitZ2;
        }

        return true;
    }
    
    bool VMapManager2::getObjectHitPos(unsigned int mapId, float x1, float y1, float z1, float x2, float y2, float z2, float& rx, float &ry, float& rz, float modifyDist)
    {
        if (isLineOfSightCalcEnabled() && !IsVMAPDisabledForPtr(mapId, VMAP_DISABLE_LOS))
        {
            auto instanceTree = GetMapTree(mapId);
            if (instanceTree != iInstanceMapTrees.end())
            {
                Vector3 pos1 = convertPositionToInternalRep(x1, y1, z1);
                Vector3 pos2 = convertPositionToInternalRep(x2, y2, z2);
                Vector3 resultPos;
                bool result = instanceTree->second->getObjectHitPos(pos1, pos2, resultPos, modifyDist);
                resultPos = convertPositionToInternalRep(resultPos.x, resultPos.y, resultPos.z);
                rx = resultPos.x;
                ry = resultPos.y;
                rz = resultPos.z;
                return result;
            }
        }

        rx = x2;
        ry = y2;
        rz = z2;

        return false;
    }

    /**
    Return closest z position for given coordinates within maxSearchDist (search up and down)
    get height or INVALID_HEIGHT if no height available
    */

    float VMapManager2::getHeight(unsigned int mapId, float x, float y, float z, float maxSearchDist)
    {
        if (isHeightCalcEnabled() && !IsVMAPDisabledForPtr(mapId, VMAP_DISABLE_HEIGHT))
        {
            auto instanceTree = GetMapTree(mapId);
            if (instanceTree != iInstanceMapTrees.end())
            {
                Vector3 pos = convertPositionToInternalRep(x, y, z);
                float height = instanceTree->second->getHeight(pos, maxSearchDist);
                if (!(height < G3D::finf()))
                    return height = VMAP_INVALID_HEIGHT_VALUE; // No height

                return height;
            }
        }

        return VMAP_INVALID_HEIGHT_VALUE;
    }

    float VMapManager2::getCeil(unsigned int mapId, float x, float y, float z, float maxSearchDist)
    {
        if (isHeightCalcEnabled() && !IsVMAPDisabledForPtr(mapId, VMAP_DISABLE_HEIGHT))
        {
            InstanceTreeMap::const_iterator instanceTree = GetMapTree(mapId);
            if (instanceTree != iInstanceMapTrees.end())
            {
                Vector3 pos = convertPositionToInternalRep(x, y, z);
                float height = instanceTree->second->getCeil(pos, maxSearchDist);
                if (!(height < G3D::finf()))
                    return height = VMAP_INVALID_CEIL_VALUE; // No height

                return height;
            }
        }

        return VMAP_INVALID_CEIL_VALUE;
    }

    bool VMapManager2::getAreaInfo(uint32 mapId, float x, float y, float& z, uint32& flags, int32& adtId, int32& rootId, int32& groupId) const
    {
        if (!IsVMAPDisabledForPtr(mapId, VMAP_DISABLE_AREAFLAG))
        {
            auto instanceTree = GetMapTree(mapId);
            if (instanceTree != iInstanceMapTrees.end())
            {
                Vector3 pos = convertPositionToInternalRep(x, y, z);
                bool result = instanceTree->second->getAreaInfo(pos, flags, adtId, rootId, groupId);
                // z is not touched by convertPositionToInternalRep(), so just copy
                z = pos.z;
                return result;
            }
        }

        return false;
    }

    bool VMapManager2::isUnderModel(unsigned int pMapId, float x, float y, float z, float* outDist, float* inDist) const
    {
        bool result = false;
        InstanceTreeMap::const_iterator instanceTree = iInstanceMapTrees.find(pMapId);
        if (instanceTree != iInstanceMapTrees.end())
        {
            Vector3 pos = convertPositionToInternalRep(x, y, z);
            result = instanceTree->second->isUnderModel(pos, outDist, inDist);
        }
        return result;
    }

    bool VMapManager2::GetLiquidLevel(uint32 mapId, float x, float y, float z, uint8 reqLiquidTypeMask, float& level, float& floor, LiquidType& type, uint32& mogpFlags) const
    {
        if (IsVMAPDisabledForPtr(mapId, VMAP_DISABLE_LIQUIDSTATUS))
            return false;

        auto instanceTree = GetMapTree(mapId);
        if (instanceTree != iInstanceMapTrees.end())
        {
            LocationInfo info;
            Vector3 pos = convertPositionToInternalRep(x, y, z);
            if (instanceTree->second->GetLocationInfo(pos, info))
            {
                floor = info.ground_Z;
                ASSERT(floor < std::numeric_limits<float>::max());
                ASSERT(info.hitModel);
                type = info.hitModel->GetLiquidType();
                mogpFlags = info.hitModel->GetMogpFlags();
                if (reqLiquidTypeMask && !(GetLiquidFlagsPtr(type) & reqLiquidTypeMask))
                    return false;
                ASSERT(info.hitInstance);
                if (info.hitInstance->GetLiquidLevel(pos, info, level))
                    return true;
            }
        }

        return false;
    }

    void VMapManager2::getAreaAndLiquidData(unsigned int mapId, float x, float y, float z, uint8 ReqLiquidTypeMask, AreaAndLiquidData& data) const
    {
        if (IsVMAPDisabledForPtr(mapId, VMAP_DISABLE_LIQUIDSTATUS))
        {
            data.floorZ = z;
            int32 adtId, rootId, groupId;
            uint32 flags;
            if (getAreaInfo(mapId, x, y, data.floorZ, flags, adtId, rootId, groupId))
                data.areaInfo = boost::in_place(adtId, rootId, groupId, flags);
            return;
        }
        InstanceTreeMap::const_iterator instanceTree = GetMapTree(mapId);
        if (instanceTree != iInstanceMapTrees.end())
        {
            LocationInfo info;
            Vector3 pos = convertPositionToInternalRep(x, y, z);
            if (instanceTree->second->GetLocationInfo(pos, info))
            {
                ASSERT(info.hitModel);
                ASSERT(info.hitInstance);
                data.floorZ = info.ground_Z;
                uint32 liquidType = info.hitModel->GetLiquidType();
                float liquidLevel;
                if (!ReqLiquidTypeMask || (GetLiquidFlagsPtr(liquidType) & ReqLiquidTypeMask))
                    if (info.hitInstance->GetLiquidLevel(pos, info, liquidLevel))
                        data.liquidInfo = boost::in_place(liquidType, liquidLevel);

                if (!IsVMAPDisabledForPtr(mapId, VMAP_DISABLE_AREAFLAG))
                    data.areaInfo = boost::in_place(info.hitInstance->adtId, info.rootId, info.hitModel->GetWmoID(), info.hitModel->GetMogpFlags());
            }
        }
    }

    WorldModel* VMapManager2::acquireModelInstance(const std::string& basepath, const std::string& filename, uint32 flags/* Only used when creating the model */)
    {
        //! Critical section, thread safe access to iLoadedModelFiles
        std::lock_guard<std::mutex> lock(LoadedModelFilesLock);

        auto model = iLoadedModelFiles.find(filename);
        if (model == iLoadedModelFiles.end())
        {
            auto  worldmodel = new WorldModel();
            if (!worldmodel->readFile(basepath + filename + ".vmo"))
            {
                VMAP_ERROR_LOG("misc", "VMapManager2: could not load '%s%s.vmo'", basepath.c_str(), filename.c_str());
                delete worldmodel;
                return nullptr;
            }
            VMAP_DEBUG_LOG("maps", "VMapManager2: loading file '%s%s'", basepath.c_str(), filename.c_str());
            worldmodel->Flags = flags;
            model = iLoadedModelFiles.insert(std::pair<std::string, ManagedModel>(filename, ManagedModel())).first;
            model->second.setModel(worldmodel);
        }
        model->second.incRefCount();
        return model->second.getModel();
    }

    void VMapManager2::releaseModelInstance(const std::string &filename)
    {
        //! Critical section, thread safe access to iLoadedModelFiles
        std::lock_guard<std::mutex> lock(LoadedModelFilesLock);

        auto model = iLoadedModelFiles.find(filename);
        if (model == iLoadedModelFiles.end())
        {
            VMAP_ERROR_LOG("misc", "VMapManager2: trying to unload non-loaded file '%s'", filename.c_str());
            return;
        }
        if (model->second.decRefCount() == 0)
        {
            VMAP_DEBUG_LOG("maps", "VMapManager2: unloading file '%s'", filename.c_str());
            delete model->second.getModel();
            iLoadedModelFiles.erase(model);
        }
    }

    LoadResult VMapManager2::existsMap(const char* basePath, unsigned int mapId, int x, int y)
    {
        return StaticMapTree::CanLoadMap(std::string(basePath), mapId, x, y);
    }

    void VMapManager2::getInstanceMapTree(InstanceTreeMap &instanceMapTree)
    {
        instanceMapTree = iInstanceMapTrees;
    }

} // namespace VMAP
