
#ifndef TRINITY_MAP_INSTANCED_H
#define TRINITY_MAP_INSTANCED_H

#include "Map.h"
#include "InstanceSaveMgr.h"

class TestMap;

/* 
MapInstanced handles all instances for one particular instanceable map. It has fake cells and shouldn't ever have any players on it.
Actual instances are InstanceMap's.
*/
class TC_GAME_API MapInstanced : public Map
{
    friend class MapManager;
    public:
        typedef std::unordered_map< uint32, Map* > InstancedMaps;

        MapInstanced(uint32 id, time_t expiry);
        ~MapInstanced() override = default;

        // functions overwrite Map versions
        void Update(const uint32&) override;
        void DelayedUpdate(const uint32 diff) override;
        //bool RemoveBones(ObjectGuid guid, float x, float y) override;
        void UnloadAll() override;
        EnterState CannotEnter(Player* /*player*/) override;

		Map* CreateInstanceForPlayer(const uint32 mapId, Player* player, uint32 loginInstanceId = 0);
        Map* FindInstanceMap(uint32 InstanceId);
        //return true of instance was destroyed
        bool DestroyInstance(uint32 InstanceId);
        bool DestroyInstance(InstancedMaps::iterator &itr);

        void AddGridMapReference(GridCoord const& p)
        {
            ++GridMapReference[p.x_coord][p.y_coord];
            SetUnloadReferenceLock(GridCoord((MAX_NUMBER_OF_GRIDS - 1) - p.x_coord, (MAX_NUMBER_OF_GRIDS - 1) - p.y_coord), true);
        }

        void RemoveGridMapReference(GridCoord const& p)
        {
            --GridMapReference[p.x_coord][p.y_coord];
            if (!GridMapReference[p.x_coord][p.y_coord])
                SetUnloadReferenceLock(GridCoord((MAX_NUMBER_OF_GRIDS - 1) - p.x_coord, (MAX_NUMBER_OF_GRIDS - 1) - p.y_coord), false);
        }

        InstancedMaps &GetInstancedMaps() { return m_InstancedMaps; }
        void InitVisibilityDistance() override;

        void MapCrashed(Map* map);
    private:
       
        std::list<Map*> crashedMaps; //those map have crashed, remove them as soon as possible. Contains either BattlegroundMap or InstanceMap

        TestMap* CreateTestInsteance(std::weak_ptr<TestThread>& testThread, uint32 InstanceId, Difficulty difficulty, bool enableMapObjects = false);
        InstanceMap* CreateInstance(uint32 InstanceId, InstanceSave *save, Difficulty difficulty);
        BattlegroundMap* CreateBattleground(uint32 InstanceId, Battleground* bg);

        InstancedMaps m_InstancedMaps;

        uint16 GridMapReference[MAX_NUMBER_OF_GRIDS][MAX_NUMBER_OF_GRIDS];
};
#endif

