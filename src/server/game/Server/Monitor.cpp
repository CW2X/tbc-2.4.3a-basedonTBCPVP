#include "Monitor.h"
#include "World.h"
#include "BattlegroundMgr.h"
#include "Language.h"
#include "Chat.h"

Monitor::Monitor()
    : _worldTickCount(0),
    _generalInfoTimer(0)
{
    _worldTicksInfo.reserve(DAY * 20); //already prepare 1 day worth of 20 updates per seconds
}

void Monitor::Update(uint32 diff)
{
    if (!sWorld->getConfig(CONFIG_MONITORING_ENABLED))
        return;

    UpdateGeneralInfosIfExpired(diff);

    smoothTD.Update(diff);
}

void SmoothedTimeDiff::Update(uint32 diff)
{
    updateTimer += diff;
    count += 1;
    sum += diff;

    if (updateTimer >= UPDATE_SMOOTH_TD)
    {
        lastAVG = uint32(sum / float(count));
        sum = 0;
        count = 0;
        updateTimer = 0;
    }
}


#ifdef TRINITY_DEBUG
    //make sure there is only one thread updating each map at a time
    std::map<std::pair<uint32 /*mapId*/, uint32 /*instanceId*/>, bool> _currentlyUpdating;
#endif

void Monitor::MapUpdateStart(Map const& map)
{
    if (!sWorld->getConfig(CONFIG_MONITORING_ENABLED))
        return;

    if (map.GetMapType() == MAP_TYPE_MAP_INSTANCED)
        return; //ignore these, not true maps

    //this function can be called from several maps at the same time
    std::lock_guard<std::mutex> lock(_currentWorldTickLock);
    #ifdef TRINITY_DEBUG
        auto itr = _currentlyUpdating.find(std::make_pair(map.GetId(), map.GetInstanceId()));
        ASSERT(itr == _currentlyUpdating.end());
        _currentlyUpdating[std::make_pair(map.GetId(), map.GetInstanceId())] = true;
    #endif
    InstanceTicksInfo& updateInfoListForMap = _currentWorldTickInfo.updateInfos[map.GetId()];
    MapTicksInfo& mapsTicksInfo = updateInfoListForMap[map.GetInstanceId()];
    mapsTicksInfo.currentTick++;
    auto& mapTick = mapsTicksInfo.ticks[mapsTicksInfo.currentTick];
    DEBUG_ASSERT(mapTick.endTime == 0);
    mapTick.startTime = GetMSTime();
}

void Monitor::MapUpdateEnd(Map& map)
{
    if (!sWorld->getConfig(CONFIG_MONITORING_ENABLED))
        return;

    if (map.GetMapType() == MAP_TYPE_MAP_INSTANCED)
        return; //ignore these, not true maps

    //this function can be called from several maps at the same time
    _currentWorldTickLock.lock();
    #ifdef TRINITY_DEBUG
        auto itr = _currentlyUpdating.find(std::make_pair(map.GetId(), map.GetInstanceId()));
        if(itr != _currentlyUpdating.end())
            _currentlyUpdating.erase(itr);
    #endif
    MapTicksInfo& mapsTicksInfo = _currentWorldTickInfo.updateInfos[map.GetId()][map.GetInstanceId()];
    auto& mapTick = mapsTicksInfo.ticks[mapsTicksInfo.currentTick];
    if (mapTick.startTime == 0)
        return; //shouldn't happen unless we changed CONFIG_MONITORING_ENABLED while running

    mapTick.endTime = GetMSTime();
    uint32 diff = mapTick.endTime - mapTick.startTime;
    _currentWorldTickLock.unlock();

    _monitDynamicLoS.UpdateForMap(map, diff);

    _lastMapDiffsLock.lock();
    _lastMapDiffs[uint64(&map)] = diff;
    _lastMapDiffsLock.unlock();
}

void Monitor::StartedWorldLoop()
{
    if (!sWorld->getConfig(CONFIG_MONITORING_ENABLED))
        return;

    _worldTickCount++;
    _currentWorldTickInfo.worldTick = _worldTickCount;
    _currentWorldTickInfo.startTime = GetMSTime();
}

void Monitor::FinishedWorldLoop()
{
    if (!sWorld->getConfig(CONFIG_MONITORING_ENABLED))
        return;

    if (_currentWorldTickInfo.startTime == 0)
        return; //shouldn't happen unless we changed CONFIG_MONITORING_ENABLED while running

    _currentWorldTickInfo.endTime = GetMSTime();
    _currentWorldTickInfo.diff = _currentWorldTickInfo.endTime - _currentWorldTickInfo.startTime;
    _currentWorldTickInfo.worldTick = _worldTickCount;

    _monitAutoReboot.Update(_currentWorldTickInfo.diff);
    _monitAlert.UpdateForWorld(_currentWorldTickInfo.diff);


    //Store current world tick and reset it
    _worldTicksInfo.push_back(std::move(_currentWorldTickInfo));
    _currentWorldTickInfo = {};
}

void Monitor::UpdateGeneralInfosIfExpired(uint32 diff)
{
    uint32 generalInfosUpdateTimeout = IN_MILLISECONDS * sWorld->getConfig(CONFIG_MONITORING_GENERALINFOS_UPDATE);
    if (!generalInfosUpdateTimeout)
        return;

    if (_generalInfoTimer > generalInfosUpdateTimeout)
    {
        UpdateGeneralInfos(diff);
        _generalInfoTimer = 0;
    }
    _generalInfoTimer += diff;
}


void Monitor::UpdateGeneralInfos(uint32 diff)
{
    time_t now = time(nullptr);
}

uint32 Monitor::GetAverageWorldDiff(uint32 searchCount)
{
    if (!searchCount)
        return 0;

    std::lock_guard<std::mutex> lock(_worldTicksInfoLock);

    if (_worldTicksInfo.size() < searchCount)
        return 0; //not enough data yet

    uint32 sum = 0;
    for (uint32 i = _worldTicksInfo.size() - searchCount; i != _worldTicksInfo.size(); i++)
        sum += _worldTicksInfo[i].diff;

    uint32 avgTD = uint32(sum / float(searchCount));

    return avgTD;
}

uint32 Monitor::GetAverageDiffForMap(Map const& map, uint32 searchCount)
{
    if (!searchCount)
        return 0;

    std::lock_guard<std::mutex> lock(_worldTicksInfoLock);

    if (_worldTicksInfo.size() < searchCount)
        return 0; //not enough data yet

    uint32 sum = 0;
    uint32 count = 0;
    for (uint32 i = _worldTicksInfo.size() - searchCount; i != _worldTicksInfo.size(); i++)
    {
        auto ticks = _worldTicksInfo[i].updateInfos[map.GetId()][map.GetInstanceId()].ticks;
        for (auto itr : ticks)
        {
            sum += itr.second.diff();
            count++;
        }
    }

    if (!count)
        return 0;

    uint32 avg = uint32(sum / float(count));

    return avg;
}

uint32 Monitor::GetLastDiffForMap(Map const& map)
{
    std::lock_guard<std::mutex> lock(_lastMapDiffsLock);
    auto itr = _lastMapDiffs.find(uint64(&map));
    if (itr == _lastMapDiffs.end())
        return 0;

    return itr->second;
}

void MonitorAutoReboot::Update(uint32 diff)
{
    uint32 searchCount = sWorld->getConfig(CONFIG_MONITORING_LAG_AUTO_REBOOT_COUNT);
    uint32 thresholdDiff = sWorld->getConfig(CONFIG_MONITORING_ABNORMAL_WORLD_UPDATE_DIFF);
    if (!searchCount || !thresholdDiff)
        return;

    checkTimer += diff;

    //not yet time to check
    if (checkTimer < CHECK_INTERVAL)
        return;

    checkTimer = 0;

    uint32 avgDiff = sMonitor->GetAverageWorldDiff(searchCount);
    if (!avgDiff) {
        return; //not enough data atm
    }

    if (avgDiff >= thresholdDiff && !sWorld->IsShuttingDown())
    {
        // Trigger restart
        sWorld->ShutdownServ(15 * MINUTE, SHUTDOWN_MASK_RESTART, RESTART_EXIT_CODE, "Auto-restart triggered due to abnormal server load.");
    }
}

void MonitorDynamicViewDistance::UpdateForMap(Map& map, uint32 diff)
{
    if (!sWorld->getConfig(CONFIG_MONITORING_DYNAMIC_VIEWDIST))
        return;

    uint32 const checkInterval = sWorld->getConfig(CONFIG_MONITORING_DYNAMIC_VIEWDIST_CHECK_INTERVAL) * SECOND * IN_MILLISECONDS;

    //is it time to check?
    _mapCheckTimersLock.lock();
    auto& timer = _mapCheckTimers[uint64(&map)].timer;
    timer += diff;

    if (timer < checkInterval) {
        _mapCheckTimersLock.unlock();
        return;
    }

    timer = 0;
    _mapCheckTimersLock.unlock();

    uint32 const abnormalDiff = sWorld->getConfig(CONFIG_MONITORING_DYNAMIC_VIEWDIST_TRIGGER_DIFF);
    if (!abnormalDiff)
        return;

    //do we have enough data?
    uint32 const avgTD = sMonitor->GetAverageDiffForMap(map, sWorld->getConfig(CONFIG_MONITORING_DYNAMIC_VIEWDIST_AVERAGE_COUNT));
    if (!avgTD)
        return; //not enough data atm

    //is it laggy?
    float const baseVisibilityRange = map.GetDefaultVisibilityDistance();
    float const currentVisibilityRange = map.GetVisibilityRange();
    float const IDEAL_DIFF = sWorld->getConfig(CONFIG_MONITORING_DYNAMIC_VIEWDIST_IDEAL_DIFF);
    if (avgTD >= abnormalDiff)
    {
        // Map is laggy, update visibility distance
        float const minDist = float(sWorld->getConfig(CONFIG_MONITORING_DYNAMIC_VIEWDIST_MINDIST));

        /* New visibility distance calculation. This is a first draft, don't hesitate to tweak it.
        Example with: IDEAL_DIFF = 200; avgTD = 420; currentVisib = 120 :
        dynamicDist = 80
        */
        float dynamicDist = std::max(minDist, (IDEAL_DIFF / float(avgTD)) * 1.4f * currentVisibilityRange);

        map.SetVisibilityDistance(dynamicDist);
    } else if (avgTD <= IDEAL_DIFF)
    {
        //td is normal, restore visibility range if needed
        if (currentVisibilityRange != baseVisibilityRange)
            map.InitVisibilityDistance();
    }
}

void MonitorAlert::UpdateForWorld(uint32 diff)
{
    uint32 searchCount = sWorld->getConfig(CONFIG_MONITORING_ALERT_THRESHOLD_COUNT);
    uint32 abnormalDiff = sWorld->getConfig(CONFIG_MONITORING_ABNORMAL_WORLD_UPDATE_DIFF);
    if (!searchCount || !abnormalDiff)
        return; //not enabled in config

    _worldCheckTimer.timer += diff;
    if (_worldCheckTimer.timer < CHECK_INTERVAL)
        return;

    _worldCheckTimer.timer = 0;

    uint32 avgTD = sMonitor->GetAverageWorldDiff(searchCount);
    if (!avgTD)
        return; //not enough data atm

    if (avgTD < abnormalDiff)
        return; //all okay

    //std::string msg = "/!\\ World updates have been slow for the last " + std::to_string(searchCount) + " updates with an average of " + std::to_string(avgTD);
    //ChatHandler::SendGlobalGMSysMessage(msg.c_str());
}
