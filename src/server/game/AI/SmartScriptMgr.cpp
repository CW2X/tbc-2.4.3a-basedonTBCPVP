
#include "DatabaseEnv.h"
#include "ObjectMgr.h"
#include "ObjectDefines.h"
#include "GridDefines.h"
#include "GridNotifiers.h"
#include "SpellMgr.h"
#include "GridNotifiersImpl.h"
#include "Cell.h"
#include "CellImpl.h"
#include "InstanceScript.h"
#include "ScriptedCreature.h"
#include "GameEventMgr.h"
#include "CreatureTextMgr.h"
#include "SmartScriptMgr.h"
#include "WaypointDefines.h"

//from waypoints table
WaypointPath const* SmartWaypointMgr::GetPath(uint32 id)
{
    auto itr = _waypointStore.find(id);
    if (itr != _waypointStore.end())
        return &itr->second;
    return nullptr;
}

void SmartWaypointMgr::LoadFromDB()
{
    uint32 oldMSTime = GetMSTime();

    _waypointStore.clear();

    QueryResult result = WorldDatabase.Query("SELECT entry, pointid, position_x, position_y, position_z FROM waypoints ORDER BY entry, pointid");

    if (!result)
    {
        TC_LOG_INFO("FIXME",">> Loaded 0 SmartAI Waypoint Paths. DB table `waypoints` is empty.");

        return;
    }

    uint32 count = 0;
    uint32 total = 0;
    uint32 last_entry = 0;
    uint32 last_id = 1;

    do
    {
        Field* fields = result->Fetch();
        uint32 entry = fields[0].GetUInt32();
        uint32 id = fields[1].GetUInt32();
        float x = fields[2].GetFloat();
        float y = fields[3].GetFloat();
        float z = fields[4].GetFloat();
    
        if (last_entry != entry)
        {
            last_id = 1;
            count++;
        }

        if (last_id != id)
            TC_LOG_ERROR("sql.sql","SmartWaypointMgr::LoadFromDB: Path entry %u, unexpected point id %u, expected %u.", entry, id, last_id);

        last_id++;
        WaypointPath& path = _waypointStore[entry];
        path.id = entry;
        path.nodes.emplace_back(id, x, y, z);

        last_entry = entry;
        total++;
    }
    while (result->NextRow());

    GetMSTime();
    TC_LOG_INFO("server.loading",">> Loaded %u SmartAI waypoint paths (total %u waypoints) in %u ms", count, total, GetMSTimeDiffToNow(oldMSTime));

}

SmartWaypointMgr::~SmartWaypointMgr()
{
}

int32 debugentryOrGuid = 0;

void SmartAIMgr::LoadSmartAIFromDB()
{
    databaseErrors.clear();
    LoadHelperStores();

    uint32 oldMSTime = GetMSTime();

    for (auto & i : mEventMap)
        i.clear();  //Drop Existing SmartAI List

    PreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_SMART_SCRIPTS);
    PreparedQueryResult result = WorldDatabase.Query(stmt);

    if (!result)
    {
        TC_LOG_INFO("sql.sql",">> Loaded 0 SmartAI scripts. DB table `smartai_scripts` is empty.");
        return;
    }

    uint32 loadedCount = 0;

    do
    {
        Field* fields = result->Fetch();

        SmartScriptHolder temp;

        uint32 count = 0;
        temp.entryOrGuid = fields[count++].GetInt32();

        if(debugentryOrGuid && temp.entryOrGuid == debugentryOrGuid)
        {
            TC_LOG_ERROR("sql.sql","debug");
        }

        SmartScriptType source_type = (SmartScriptType)fields[count++].GetUInt8();
        if (source_type >= SMART_SCRIPT_TYPE_MAX)
        {
            TC_LOG_ERROR("sql.sql","SmartAIMgr::LoadSmartAIFromDB: invalid source_type (%u), skipped loading.", uint32(source_type));
            continue;
        }
        if (temp.entryOrGuid > 0)
        {
            switch (source_type)
            {
                case SMART_SCRIPT_TYPE_CREATURE:
                {
                    CreatureTemplate const* creatureInfo = sObjectMgr->GetCreatureTemplate((uint32)temp.entryOrGuid);
                    if (!creatureInfo)
                    {
                        SMARTAI_DB_ERROR(temp.entryOrGuid, "SmartAIMgr::LoadSmartAIFromDB: Creature entry (%u) does not exist, skipped loading.", uint32(temp.entryOrGuid));
                        continue;
                    }

                    if (creatureInfo->AIName != SMARTAI_AI_NAME)
                    {
                        SMARTAI_DB_ERROR(temp.entryOrGuid, "SmartAIMgr::LoadSmartAIFromDB: Creature entry (%u) is not using SmartAI, skipped loading.", uint32(temp.entryOrGuid));
                        continue;
                    }
                    break;
                }
                case SMART_SCRIPT_TYPE_GAMEOBJECT:
                {
                    if (!sObjectMgr->GetGameObjectTemplate((uint32)temp.entryOrGuid))
                    {
                        SMARTAI_DB_ERROR(temp.entryOrGuid,"SmartAIMgr::LoadSmartAIFromDB: GameObject entry (%u) does not exist, skipped loading.", uint32(temp.entryOrGuid));
                        continue;
                    }
                    break;
                }
                case SMART_SCRIPT_TYPE_AREATRIGGER:
                {
                    if (!sAreaTriggerStore.LookupEntry((uint32)temp.entryOrGuid))
                    {
                        SMARTAI_DB_ERROR(temp.entryOrGuid,"SmartAIMgr::LoadSmartAIFromDB: AreaTrigger entry (%u) does not exist, skipped loading.", uint32(temp.entryOrGuid));
                        continue;
                    }
                    break;
                }
                case SMART_SCRIPT_TYPE_TIMED_ACTIONLIST:
                    break;//nothing to check, really
                default:
                    SMARTAI_DB_ERROR(temp.entryOrGuid,"SmartAIMgr::LoadSmartAIFromDB: not yet implemented source_type %u", (uint32)source_type);
                    continue;
            }
        }
        else
        {
            switch (source_type)
            {
                case SMART_SCRIPT_TYPE_CREATURE:
                {
                    CreatureData const* creature = sObjectMgr->GetCreatureData(uint32(std::abs(temp.entryOrGuid)));
                    if (!creature)
                    {
                        SMARTAI_DB_ERROR(uint32(abs(temp.entryOrGuid)), "SmartAIMgr::LoadSmartAIFromDB: Creature spawnId (%u) does not exist, skipped loading.", uint32(abs(temp.entryOrGuid)));
                        continue;
                    }

                    CreatureTemplate const* creatureInfo = sObjectMgr->GetCreatureTemplate(creature->GetFirstSpawnEntry());
                    if (!creatureInfo)
                    {
                        TC_LOG_ERROR("sql.sql", "SmartAIMgr::LoadSmartAIFromDB: Creature entry (%u) spawnId (%u) does not exist, skipped loading.", creature->GetFirstSpawnEntry(), uint32(std::abs(temp.entryOrGuid)));
                        continue;
                    }

                    if (creatureInfo->AIName != SMARTAI_AI_NAME)
                    {
                        TC_LOG_ERROR("sql.sql", "SmartAIMgr::LoadSmartAIFromDB: Creature entry (%u) spawnId (%u) is not using " SMARTAI_AI_NAME ", skipped loading.", creature->GetFirstSpawnEntry(), uint32(std::abs(temp.entryOrGuid)));
                        continue;
                    }
                    break;
                }
                case SMART_SCRIPT_TYPE_GAMEOBJECT:
                {
                    GameObjectData const* gameObject = sObjectMgr->GetGameObjectData(uint32(std::abs(temp.entryOrGuid)));
                    if (!gameObject)
                    {
                        TC_LOG_ERROR("sql.sql", "SmartAIMgr::LoadSmartAIFromDB: GameObject spawnId (%u) does not exist, skipped loading.", uint32(std::abs(temp.entryOrGuid)));
                        continue;
                    }

                    GameObjectTemplate const* gameObjectInfo = sObjectMgr->GetGameObjectTemplate(gameObject->id);
                    if (!gameObjectInfo)
                    {
                        TC_LOG_ERROR("sql.sql", "SmartAIMgr::LoadSmartAIFromDB: GameObject entry (%u) spawnId (%u) does not exist, skipped loading.", gameObject->id, uint32(std::abs(temp.entryOrGuid)));
                        continue;
                    }

                    if (gameObjectInfo->AIName != SMARTAI_GOBJECT_AI_NAME)
                    {
                        TC_LOG_ERROR("sql.sql", "SmartAIMgr::LoadSmartAIFromDB: GameObject entry (%u) spawnId (%u) is not using " SMARTAI_GOBJECT_AI_NAME ", skipped loading.", gameObject->id, uint32(std::abs(temp.entryOrGuid)));
                        continue;
                    }
                    break;
                } 
                default:
                    TC_LOG_ERROR("sql.sql", "SmartAIMgr::LoadSmartAIFromDB: SpawnId-specific scripting not yet implemented for source_type %u", (uint32)source_type);
                    continue;
            }
        }

        temp.source_type = source_type;
        temp.event_id = fields[count++].GetUInt16();
        temp.link = fields[count++].GetUInt16();
        temp.event.type = (SMART_EVENT)fields[count++].GetUInt8();
        temp.event.event_phase_mask = fields[count++].GetUInt16();
        temp.event.event_chance = fields[count++].GetUInt8();
        temp.event.event_flags = fields[count++].GetUInt16();

        temp.event.raw.param1 = fields[count++].GetUInt32();
        temp.event.raw.param2 = fields[count++].GetUInt32();
        temp.event.raw.param3 = fields[count++].GetUInt32();
        temp.event.raw.param4 = fields[count++].GetUInt32();
        temp.event.raw.param5 = fields[count++].GetUInt32();

        temp.action.type = (SMART_ACTION)fields[count++].GetUInt8();
        temp.action.raw.param1 = fields[count++].GetUInt32();
        temp.action.raw.param2 = fields[count++].GetUInt32();
        temp.action.raw.param3 = fields[count++].GetUInt32();
        temp.action.raw.param4 = fields[count++].GetUInt32();
        temp.action.raw.param5 = fields[count++].GetUInt32();
        temp.action.raw.param6 = fields[count++].GetUInt32();

        temp.target.type = (SMARTAI_TARGETS)fields[count++].GetUInt8();
        temp.target.flags = (SMARTAI_TARGETS_FLAGS)fields[count++].GetUInt8();
        temp.target.raw.param1 = fields[count++].GetUInt32();
        temp.target.raw.param2 = fields[count++].GetUInt32();
        temp.target.raw.param3 = fields[count++].GetUInt32();
        temp.target.raw.param4 = fields[count++].GetUInt32();
        temp.target.x = fields[count++].GetFloat();
        temp.target.y = fields[count++].GetFloat();
        temp.target.z = fields[count++].GetFloat();
        temp.target.o = fields[count++].GetFloat();

        //check target
        if (!IsTargetValid(temp))
            continue;

        // check all event and action params
        if (!IsEventValid(temp))
            continue;

        // specific check for timed events
        switch (temp.event.type)
        {
        case SMART_EVENT_UPDATE:
        case SMART_EVENT_UPDATE_OOC:
        case SMART_EVENT_UPDATE_IC:
        case SMART_EVENT_HEALT_PCT:
        case SMART_EVENT_TARGET_HEALTH_PCT:
        case SMART_EVENT_MANA_PCT:
        case SMART_EVENT_TARGET_MANA_PCT:
        case SMART_EVENT_RANGE:
        case SMART_EVENT_FRIENDLY_HEALTH:
        case SMART_EVENT_FRIENDLY_HEALTH_PCT:
        case SMART_EVENT_FRIENDLY_MISSING_BUFF:
        case SMART_EVENT_HAS_AURA:
        case SMART_EVENT_TARGET_BUFFED:
            if (temp.event.minMaxRepeat.repeatMin == 0 && temp.event.minMaxRepeat.repeatMax == 0 && !(temp.event.event_flags & SMART_EVENT_FLAG_NOT_REPEATABLE) && temp.source_type != SMART_SCRIPT_TYPE_TIMED_ACTIONLIST)
            {
                temp.event.event_flags |= SMART_EVENT_FLAG_NOT_REPEATABLE;
                SMARTAI_DB_ERROR(temp.entryOrGuid, "SmartAIMgr::LoadSmartAIFromDB: Entry %d SourceType %u, Event %u, Missing Repeat flag.",
                    temp.entryOrGuid, temp.GetScriptType(), temp.event_id);
            }
            break;
        case SMART_EVENT_VICTIM_CASTING:
        case SMART_EVENT_IS_BEHIND_TARGET:
            if (temp.event.minMaxRepeat.min == 0 && temp.event.minMaxRepeat.max == 0 && !(temp.event.event_flags & SMART_EVENT_FLAG_NOT_REPEATABLE) && temp.source_type != SMART_SCRIPT_TYPE_TIMED_ACTIONLIST)
            {
                temp.event.event_flags |= SMART_EVENT_FLAG_NOT_REPEATABLE;
                SMARTAI_DB_ERROR(temp.entryOrGuid, "SmartAIMgr::LoadSmartAIFromDB: Entry %d SourceType %u, Event %u, Missing Repeat flag.",
                    temp.entryOrGuid, temp.GetScriptType(), temp.event_id);
            }
            break;
        case SMART_EVENT_FRIENDLY_IS_CC:
            if (temp.event.friendlyCC.repeatMin == 0 && temp.event.friendlyCC.repeatMax == 0 && !(temp.event.event_flags & SMART_EVENT_FLAG_NOT_REPEATABLE) && temp.source_type != SMART_SCRIPT_TYPE_TIMED_ACTIONLIST)
            {
                temp.event.event_flags |= SMART_EVENT_FLAG_NOT_REPEATABLE;
                SMARTAI_DB_ERROR(temp.entryOrGuid, "SmartAIMgr::LoadSmartAIFromDB: Entry %d SourceType %u, Event %u, Missing Repeat flag.",
                    temp.entryOrGuid, temp.GetScriptType(), temp.event_id);
            }
            break;
        default:
            break;
        }

        // creature entry / guid not found in storage, create empty event list for it and increase counters
        if (mEventMap[source_type].find(temp.entryOrGuid) == mEventMap[source_type].end())
        {
            ++loadedCount;
            SmartAIEventList eventList;
            mEventMap[source_type][temp.entryOrGuid] = eventList;
        }
        // store the new event
        mEventMap[source_type][temp.entryOrGuid].push_back(std::move(temp));
    }
    while (result->NextRow());

    // Post Loading Validation
    for (uint8 i = 0; i < SMART_SCRIPT_TYPE_MAX; ++i)
    {
        for (SmartAIEventMap::iterator itr = mEventMap[i].begin(); itr != mEventMap[i].end(); ++itr)
        {
            for (SmartScriptHolder const& e : itr->second)
            {
                if (e.link)
                {
                    if (!FindLinkedEvent(itr->second, e.link))
                    {
                        TC_LOG_ERROR("sql.sql", "SmartAIMgr::LoadSmartAIFromDB: Entry %d SourceType %u, Event %u, Link Event %u not found or invalid.",
                            e.entryOrGuid, e.GetScriptType(), e.event_id, e.link);
                    }
                }

                if (e.GetEventType() == SMART_EVENT_LINK)
                {
                    if (!FindLinkedSourceEvent(itr->second, e.event_id))
                    {
                        TC_LOG_ERROR("sql.sql", "SmartAIMgr::LoadSmartAIFromDB: Entry %d SourceType %u, Event %u, Link Source Event not found or invalid. Event will never trigger.",
                            e.entryOrGuid, e.GetScriptType(), e.event_id);
                    }
                }
            }
        }
    }

    TC_LOG_INFO("server.loading",">> Loaded %u SmartAI scripts in %u ms", loadedCount, GetMSTimeDiffToNow(oldMSTime));

    UnLoadHelperStores();
}

bool SmartAIMgr::IsTargetValid(SmartScriptHolder const& e)
{
    if (e.GetActionType() == SMART_ACTION_INSTALL_AI_TEMPLATE)
        return true; // AI template has special handling

    switch (e.GetTargetType())
    {
        case SMART_TARGET_CREATURE_DISTANCE:
        case SMART_TARGET_CREATURE_RANGE:
        {
            if (e.target.unitDistance.creature && !sObjectMgr->GetCreatureTemplate(e.target.unitDistance.creature))
            {
                SMARTAI_DB_ERROR(e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Creature entry %u as target_param1, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.target.unitDistance.creature);
                return false;
            }
            break;
        }
        case SMART_TARGET_GAMEOBJECT_DISTANCE:
        case SMART_TARGET_GAMEOBJECT_RANGE:
        {
            if (e.target.goDistance.entry && !sObjectMgr->GetGameObjectTemplate(e.target.goDistance.entry))
            {
                SMARTAI_DB_ERROR(e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent GameObject entry %u as target_param1, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.target.goDistance.entry);
                return false;
            }
            break;
        }
        case SMART_TARGET_CREATURE_GUID:
        {
            if (e.target.unitGUID.entry && !IsCreatureValid(e, e.target.unitGUID.entry))
                return false;
            break;
        }
        case SMART_TARGET_GAMEOBJECT_GUID:
        {
            if (e.target.goGUID.entry && !IsGameObjectValid(e, e.target.goGUID.entry))
                return false;
            break;
        }
        case SMART_TARGET_PLAYER_DISTANCE:
        case SMART_TARGET_PLAYER_CASTING_DISTANCE:
        case SMART_TARGET_CLOSEST_PLAYER:
        {
            if (e.target.playerDistance.dist == 0)
            {
                SMARTAI_DB_ERROR(e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u has maxDist 0 as target_param1, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                return false;
            }
            break;
        }
        case SMART_TARGET_FRIENDLY_HEALTH_PCT:
        {
            if (e.target.friendlyHealthPct.maxDist == 0)
            {
                SMARTAI_DB_ERROR(e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u has maxDist 0 as target_param1, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                return false;
            }

            if (e.target.friendlyHealthPct.percentBelow == 0)
            {
                SMARTAI_DB_ERROR(e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u has percentBelow 0 as target_param1, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                return false;
            }
            break;
        }
        case SMART_TARGET_ACTION_INVOKER:
#ifdef LICH_KING
        case SMART_TARGET_ACTION_INVOKER_VEHICLE:
#endif
        case SMART_TARGET_INVOKER_PARTY:
            if (e.GetScriptType() != SMART_SCRIPT_TYPE_TIMED_ACTIONLIST && e.GetEventType() != SMART_EVENT_LINK && !EventHasInvoker(e.event.type))
            {
                TC_LOG_ERROR("sql.sql", "SmartAIMgr: Entry %d SourceType %u Event %u Action %u has invoker target, but event does not provide any invoker!", e.entryOrGuid, e.GetScriptType(), e.GetEventType(), e.GetActionType());
                // allow this to load for now
                // return false;
            }
            break;
        case SMART_TARGET_PLAYER_RANGE:
        case SMART_TARGET_SELF:
        case SMART_TARGET_VICTIM:
        case SMART_TARGET_HOSTILE_SECOND_AGGRO:
        case SMART_TARGET_HOSTILE_LAST_AGGRO:
        case SMART_TARGET_HOSTILE_RANDOM:
        case SMART_TARGET_HOSTILE_RANDOM_NOT_TOP:
        case SMART_TARGET_POSITION:
        case SMART_TARGET_NONE:
#ifdef LICH_KING
        case SMART_TARGET_ACTION_INVOKER_VEHICLE:
#endif
        case SMART_TARGET_OWNER_OR_SUMMONER:
        case SMART_TARGET_THREAT_LIST:
        case SMART_TARGET_CLOSEST_GAMEOBJECT:
        case SMART_TARGET_CLOSEST_CREATURE:
        case SMART_TARGET_CLOSEST_ENEMY:
        case SMART_TARGET_CLOSEST_FRIENDLY:
        case SMART_TARGET_STORED:
        case SMART_TARGET_LOOT_RECIPIENTS:
        case SMART_TARGET_FARTHEST:
            break;
        default:
            SMARTAI_DB_ERROR(e.entryOrGuid, "SmartAIMgr: Not handled target_type(%u), Entry %d SourceType %u Event %u Action %u, skipped.", e.GetTargetType(), e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
            return false;
    }

    return true;
}

bool SmartAIMgr::IsEventValid(SmartScriptHolder& e)
{
    if (e.event.type >= SMART_EVENT_END)
    {
        SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: EntryOrGuid %d using event(%u) has invalid event type (%u), skipped.", e.entryOrGuid, e.event_id, e.GetEventType());
        return false;
    }
    // in SMART_SCRIPT_TYPE_TIMED_ACTIONLIST all event types are overriden by core
    if (e.GetScriptType() != SMART_SCRIPT_TYPE_TIMED_ACTIONLIST && !(SmartAIEventMask.at(e.event.type) & SmartAITypeMask.at(e.GetScriptType())))
    {
        SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: EntryOrGuid %d, event type %u can not be used for Script type %u", e.entryOrGuid, e.GetEventType(), e.GetScriptType());
        return false;
    }
    if (e.action.type <= 0 || e.action.type >= SMART_ACTION_END)
    {
        SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: EntryOrGuid %d using event(%u) has invalid action type (%u), skipped.", e.entryOrGuid, e.event_id, e.GetActionType());
        return false;
    }
    if (e.event.event_phase_mask > SMART_EVENT_PHASE_ALL)
    {
        SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: EntryOrGuid %d using event(%u) has invalid phase mask (%u), skipped.", e.entryOrGuid, e.event_id, uint32(e.event.event_phase_mask));
        return false;
    }
    if (e.event.event_flags > SMART_EVENT_FLAGS_ALL)
    {
        SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: EntryOrGuid %d using event(%u) has invalid event flags (%u), skipped.", e.entryOrGuid, e.event_id, e.event.event_flags);
        return false;
    }
    if (e.GetScriptType() == SMART_SCRIPT_TYPE_TIMED_ACTIONLIST)
    {
        e.event.type = SMART_EVENT_UPDATE_OOC;//force default OOC, can change when calling the script!
        if (!IsMinMaxValid(e, e.event.minMaxRepeat.min, e.event.minMaxRepeat.max))
            return false;

        if (!IsMinMaxValid(e, e.event.minMaxRepeat.repeatMin, e.event.minMaxRepeat.repeatMax))
            return false;
    }
    else
    {
        uint32 type = e.event.type;
        switch (type)
        {
            case SMART_EVENT_UPDATE:
            case SMART_EVENT_UPDATE_IC:
            case SMART_EVENT_UPDATE_OOC:
            case SMART_EVENT_HEALT_PCT:
            case SMART_EVENT_MANA_PCT:
            case SMART_EVENT_TARGET_HEALTH_PCT:
            case SMART_EVENT_TARGET_MANA_PCT:
            case SMART_EVENT_RANGE:
            case SMART_EVENT_DAMAGED:
            case SMART_EVENT_DAMAGED_TARGET:
            case SMART_EVENT_RECEIVE_HEAL:
                if (!IsMinMaxValid(e, e.event.minMaxRepeat.min, e.event.minMaxRepeat.max))
                    return false;

                if(!(e.event.event_flags & SMART_EVENT_FLAG_NOT_REPEATABLE))
                    if (!IsMinMaxValid(e, e.event.minMaxRepeat.repeatMin, e.event.minMaxRepeat.repeatMax))
                        return false;
                break;
            case SMART_EVENT_SPELLHIT:
            case SMART_EVENT_SPELLHIT_TARGET:
                if (e.event.spellHit.spell)
                {
                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(e.event.spellHit.spell);
                    if (!spellInfo)
                    {
                        SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Spell entry %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.event.spellHit.spell);
                        return false;
                    }
                    if (e.event.spellHit.school && (e.event.spellHit.school & spellInfo->SchoolMask) != spellInfo->SchoolMask)
                    {
                        SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses Spell entry %u with invalid school mask, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.event.spellHit.spell);
                        return false;
                    }
                }
                if (!IsMinMaxValid(e, e.event.spellHit.cooldownMin, e.event.spellHit.cooldownMax))
                    return false;
                break;
            case SMART_EVENT_OOC_LOS:
            case SMART_EVENT_IC_LOS:
                if (!IsMinMaxValid(e, e.event.los.cooldownMin, e.event.los.cooldownMax))
                    return false;
                break;
            case SMART_EVENT_RESPAWN:
                if (e.event.respawn.type == SMART_SCRIPT_RESPAWN_CONDITION_MAP && !sMapStore.LookupEntry(e.event.respawn.map))
                {
                    SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Map entry %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.event.respawn.map);
                    return false;
                }
                if (e.event.respawn.type == SMART_SCRIPT_RESPAWN_CONDITION_AREA && !sAreaTableStore.LookupEntry(e.event.respawn.area))
                {
                    SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Area entry %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.event.respawn.area);
                    return false;
                }
                break;
            case SMART_EVENT_FRIENDLY_HEALTH:
                if (!NotNULL(e, e.event.friendlyHealth.radius))
                    return false;

                if (!IsMinMaxValid(e, e.event.friendlyHealth.repeatMin, e.event.friendlyHealth.repeatMax))
                    return false;
                break;
            case SMART_EVENT_FRIENDLY_IS_CC:
                if (!IsMinMaxValid(e, e.event.friendlyCC.repeatMin, e.event.friendlyCC.repeatMax))
                    return false;
                break;
            case SMART_EVENT_FRIENDLY_MISSING_BUFF:
            {
                if (!IsSpellValid(e, e.event.missingBuff.spell))
                    return false;

                if (!NotNULL(e, e.event.missingBuff.radius))
                    return false;

                if (!IsMinMaxValid(e, e.event.missingBuff.repeatMin, e.event.missingBuff.repeatMax))
                    return false;
                break;
            }
            case SMART_EVENT_KILL:
                if (!IsMinMaxValid(e, e.event.kill.cooldownMin, e.event.kill.cooldownMax))
                    return false;

                if (e.event.kill.creature && !IsCreatureValid(e, e.event.kill.creature))
                    return false;
                break;
            case SMART_EVENT_VICTIM_CASTING:
                if (e.event.targetCasting.spellId > 0 && !sSpellMgr->GetSpellInfo(e.event.targetCasting.spellId))
                {
                    SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Spell entry %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.event.spellHit.spell);
                    return false;
                }

                if (!IsMinMaxValid(e, e.event.targetCasting.repeatMin, e.event.targetCasting.repeatMax))
                    return false;
                break;
            case SMART_EVENT_PASSENGER_BOARDED:
            case SMART_EVENT_PASSENGER_REMOVED:
                if (!IsMinMaxValid(e, e.event.minMax.repeatMin, e.event.minMax.repeatMax))
                    return false;
                break;
            case SMART_EVENT_SUMMON_DESPAWNED:
            case SMART_EVENT_SUMMONED_UNIT:
                if (e.event.summoned.creature && !IsCreatureValid(e, e.event.summoned.creature))
                    return false;

                if (!IsMinMaxValid(e, e.event.summoned.cooldownMin, e.event.summoned.cooldownMax))
                    return false;
                break;
            case SMART_EVENT_ACCEPTED_QUEST:
            case SMART_EVENT_REWARD_QUEST:
                if (e.event.quest.quest && !IsQuestValid(e, e.event.quest.quest))
                    return false;
                break;
            case SMART_EVENT_RECEIVE_EMOTE:
            {
                if (e.event.emote.emote && !IsTextEmoteValid(e, e.event.emote.emote))
                    return false;

                if (!IsMinMaxValid(e, e.event.emote.cooldownMin, e.event.emote.cooldownMax))
                    return false;
                break;
            }
            case SMART_EVENT_HAS_AURA:
            case SMART_EVENT_TARGET_BUFFED:
            {
                if (!IsSpellValid(e, e.event.aura.spell))
                    return false;

                if (!IsMinMaxValid(e, e.event.aura.repeatMin, e.event.aura.repeatMax))
                    return false;
                break;
            }
            case SMART_EVENT_TRANSPORT_ADDCREATURE:
            {
                if (e.event.transportAddCreature.creature && !IsCreatureValid(e, e.event.transportAddCreature.creature))
                    return false;
                break;
            }
            case SMART_EVENT_MOVEMENTINFORM:
            {
                if (IsInvalidMovementGeneratorType(e.event.movementInform.type))
                {
                    SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses invalid Motion type %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.event.movementInform.type);
                    return false;
                }
                break;
            }
            case SMART_EVENT_DATA_SET:
            {
                if (!IsMinMaxValid(e, e.event.dataSet.cooldownMin, e.event.dataSet.cooldownMax))
                    return false;
                break;
            }
            case SMART_EVENT_AREATRIGGER_ONTRIGGER:
            {
                if (e.event.areatrigger.id && !IsAreaTriggerValid(e, e.event.areatrigger.id))
                    return false;
                break;
            }
            case SMART_EVENT_TEXT_OVER:
                if (!IsTextValid(e, e.event.textOver.textGroupID))
                    return false;
                break;
            case SMART_EVENT_LINK:
            {
                if (e.link && e.link == e.event_id)
                {
                    SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u, Event %u, Link Event is linking self (infinite loop), skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id);
                    return false;
                }
                break;
            }
            case SMART_EVENT_EVENT_PHASE_CHANGE:
            case SMART_EVENT_EVENT_TEMPLATE_PHASE_CHANGE:
            {
                if (!e.event.eventPhaseChange.phasemask)
                {
                    TC_LOG_ERROR("sql.sql", "SmartAIMgr: Entry %d SourceType %u Event %u Action %u has no param set, event won't be executed!.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                    return false;
                }

                if (e.event.eventPhaseChange.phasemask > SMART_EVENT_PHASE_ALL)
                {
                    TC_LOG_ERROR("sql.sql", "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses invalid phasemask %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.event.eventPhaseChange.phasemask);
                    return false;
                }

                if (e.event.event_phase_mask && !(e.event.event_phase_mask & e.event.eventPhaseChange.phasemask))
                {
                    TC_LOG_ERROR("sql.sql", "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses event phasemask %u and incompatible event_param1 %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), uint32(e.event.event_phase_mask), uint32(e.event.eventPhaseChange.phasemask));
                    return false;
                }
                break;
            }
            case SMART_EVENT_IS_BEHIND_TARGET:
            {
                if (!IsMinMaxValid(e, e.event.behindTarget.cooldownMin, e.event.behindTarget.cooldownMax))
                    return false;
                break;
            }
            case SMART_EVENT_GAME_EVENT_START:
            case SMART_EVENT_GAME_EVENT_END:
            {
                GameEventMgr::GameEventDataMap const& events = sGameEventMgr->GetEventMap();
                if (e.event.gameEvent.gameEventId >= events.size() || !events[e.event.gameEvent.gameEventId].isValid())
                    return false;
                break;
            }
            case SMART_EVENT_ACTION_DONE:
            {
                if (e.event.doAction.eventId > 1003 /*EVENT_CHARGE*/)
                {
                    SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses invalid event id %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.event.doAction.eventId);
                    return false;
                }
                break;
            }
            case SMART_EVENT_FRIENDLY_HEALTH_PCT:
                if (!IsMinMaxValid(e, e.event.friendlyHealthPct.repeatMin, e.event.friendlyHealthPct.repeatMax))
                    return false;

                if (e.event.friendlyHealthPct.maxHpPct > 100 || e.event.friendlyHealthPct.minHpPct > 100)
                {
                    SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u has pct value above 100, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                    return false;
                }

                switch (e.GetTargetType())
                {
                case SMART_TARGET_CREATURE_RANGE:
                case SMART_TARGET_CREATURE_GUID:
                case SMART_TARGET_CREATURE_DISTANCE:
                case SMART_TARGET_CLOSEST_CREATURE:
                case SMART_TARGET_CLOSEST_PLAYER:
                case SMART_TARGET_PLAYER_RANGE:
                case SMART_TARGET_PLAYER_DISTANCE:
                case SMART_TARGET_PLAYER_CASTING_DISTANCE:
                    break;
                default:
                    SMARTAI_DB_ERROR(e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u has invalid target type for event, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                    return false;
                }

                break;
            case SMART_EVENT_DISTANCE_CREATURE:
                if (e.event.distance.guid == 0 && e.event.distance.entry == 0)
                {
                    TC_LOG_ERROR("sql.sql", "SmartAIMgr: Event SMART_EVENT_DISTANCE_CREATURE did not provide creature guid or entry, skipped.");
                    return false;
                }

                if (e.event.distance.guid != 0 && e.event.distance.entry != 0)
                {
                    TC_LOG_ERROR("sql.sql", "SmartAIMgr: Event SMART_EVENT_DISTANCE_CREATURE provided both an entry and guid, skipped.");
                    return false;
                }

                if (e.event.distance.guid != 0 && !sObjectMgr->GetCreatureData(e.event.distance.guid))
                {
                    TC_LOG_ERROR("sql.sql", "SmartAIMgr: Event SMART_EVENT_DISTANCE_CREATURE using invalid creature guid %u, skipped.", e.event.distance.guid);
                    return false;
                }

                if (e.event.distance.entry != 0 && !sObjectMgr->GetCreatureTemplate(e.event.distance.entry))
                {
                    TC_LOG_ERROR("sql.sql", "SmartAIMgr: Event SMART_EVENT_DISTANCE_CREATURE using invalid creature entry %u, skipped.", e.event.distance.entry);
                    return false;
                }
                break;
            case SMART_EVENT_DISTANCE_GAMEOBJECT:
                if (e.event.distance.guid == 0 && e.event.distance.entry == 0)
                {
                    TC_LOG_ERROR("sql.sql", "SmartAIMgr: Event SMART_EVENT_DISTANCE_GAMEOBJECT did not provide gameobject guid or entry, skipped.");
                    return false;
                }

                if (e.event.distance.guid != 0 && e.event.distance.entry != 0)
                {
                    TC_LOG_ERROR("sql.sql", "SmartAIMgr: Event SMART_EVENT_DISTANCE_GAMEOBJECT provided both an entry and guid, skipped.");
                    return false;
                }

                if (e.event.distance.guid != 0 && !sObjectMgr->GetGameObjectData(e.event.distance.guid))
                {
                    TC_LOG_ERROR("sql.sql", "SmartAIMgr: Event SMART_EVENT_DISTANCE_GAMEOBJECT using invalid gameobject guid %u, skipped.", e.event.distance.guid);
                    return false;
                }

                if (e.event.distance.entry != 0 && !sObjectMgr->GetGameObjectTemplate(e.event.distance.entry))
                {
                    TC_LOG_ERROR("sql.sql", "SmartAIMgr: Event SMART_EVENT_DISTANCE_GAMEOBJECT using invalid gameobject entry %u, skipped.", e.event.distance.entry);
                    return false;
                }
                break;
            case SMART_EVENT_COUNTER_SET:
                if (!IsMinMaxValid(e, e.event.counter.cooldownMin, e.event.counter.cooldownMax))
                    return false;

                if (e.event.counter.id == 0)
                {
                    TC_LOG_ERROR("sql.sql", "SmartAIMgr: Event SMART_EVENT_COUNTER_SET using invalid counter id %u, skipped.", e.event.counter.id);
                    return false;
                }

                if (e.event.counter.value == 0)
                {
                    TC_LOG_ERROR("sql.sql", "SmartAIMgr: Event SMART_EVENT_COUNTER_SET using invalid value %u, skipped.", e.event.counter.value);
                    return false;
                }
                break;
            case SMART_EVENT_FRIENDLY_KILLED:
            {
                if(e.event.friendlyDeath.range > CREATURE_MAX_DEATH_WARN_RANGE)
                {
                    SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u has range (%u) above max possible value, range has been capped to %u.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.event.friendlyDeath.range, uint32(CREATURE_MAX_DEATH_WARN_RANGE));
                    e.event.friendlyDeath.range = (uint32)CREATURE_MAX_DEATH_WARN_RANGE;
                }

                switch (e.GetTargetType())
                {
                    case SMART_TARGET_ACTION_INVOKER:
                        break;
                    default:
                        SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses invalid target_type %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.GetTargetType());
                        return false;
                }
            }
            case SMART_EVENT_AFFECTED_BY_MECHANIC:
            {
                if (e.event.affectedByMechanic.mechanicMask >= (1 << MAX_MECHANIC))
                {
                    SMARTAI_DB_ERROR(e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses invalid mechanic mask %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.event.affectedByMechanic.mechanicMask);
                    return false;
                }
            }
            case SMART_ACTION_ENABLE_TEMP_GOBJ:
                if (!e.action.enableTempGO.duration)
                {
                    TC_LOG_ERROR("sql.sql", "Entry %u SourceType %u Event %u Action %u does not specify duration", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                    return false;
                }
            }
    }

    if(e.GetEventType() >= SMART_EVENT_END)
    {
        SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Not handled event_type(%u), Entry %d SourceType %u Event %u Action %u, skipped.", e.GetEventType(), e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
        return false;
    }


    switch (e.GetActionType())
    {
        case SMART_ACTION_TALK:
        case SMART_ACTION_SIMPLE_TALK:
            if (!IsTextValid(e, e.action.talk.textGroupID))
            {
                SMARTAI_DB_ERROR(e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent text %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.talk.textGroupID);
                return false;
            }

            break;
        case SMART_ACTION_SET_FACTION:
            if (e.action.faction.factionID && !sFactionTemplateStore.LookupEntry(e.action.faction.factionID))
            {
                SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Faction %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.faction.factionID);
                return false;
            }
            break;
        case SMART_ACTION_MORPH_TO_ENTRY_OR_MODEL:
        case SMART_ACTION_MOUNT_TO_ENTRY_OR_MODEL:
            if (e.action.morphOrMount.creature || e.action.morphOrMount.model)
            {
                if (e.action.morphOrMount.creature > 0 && !sObjectMgr->GetCreatureTemplate(e.action.morphOrMount.creature))
                {
                    SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Creature entry %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.morphOrMount.creature);
                    return false;
                }

                if (e.action.morphOrMount.model)
                {
                    if (e.action.morphOrMount.creature)
                    {
                        SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u has ModelID set with also set CreatureId, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                        return false;
                    }
                    else if (!sCreatureDisplayInfoStore.LookupEntry(e.action.morphOrMount.model))
                    {
                        SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Model id %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.morphOrMount.model);
                        return false;
                    }
                }
            }
            break;
        case SMART_ACTION_SOUND:
            if (!IsSoundValid(e, e.action.sound.sound))
                return false;
            break;
        case SMART_ACTION_SET_EMOTE_STATE:
        case SMART_ACTION_PLAY_EMOTE:
            if (!IsEmoteValid(e, e.action.emote.emote))
                return false;
            break;
        case SMART_ACTION_FAIL_QUEST:
        case SMART_ACTION_ADD_QUEST:
            if (!e.action.quest.quest || !IsQuestValid(e, e.action.quest.quest))
                return false;
            break;
        case SMART_ACTION_ACTIVATE_TAXI:
            {
                if (!sTaxiPathStore.LookupEntry(e.action.taxi.id))
                {
                    SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses invalid Taxi path ID %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.taxi.id);
                    return false;
                }
                break;
            }
        case SMART_ACTION_RANDOM_EMOTE:
            if (e.action.randomEmote.emote1 && !IsEmoteValid(e, e.action.randomEmote.emote1))
                return false;

            if (e.action.randomEmote.emote2 && !IsEmoteValid(e, e.action.randomEmote.emote2))
                return false;

            if (e.action.randomEmote.emote3 && !IsEmoteValid(e, e.action.randomEmote.emote3))
                return false;

            if (e.action.randomEmote.emote4 && !IsEmoteValid(e, e.action.randomEmote.emote4))
                return false;

            if (e.action.randomEmote.emote5 && !IsEmoteValid(e, e.action.randomEmote.emote5))
                return false;

            if (e.action.randomEmote.emote6 && !IsEmoteValid(e, e.action.randomEmote.emote6))
                return false;
            break;
        case SMART_ACTION_CROSS_CAST:
        {
            if (!IsSpellValid(e, e.action.crossCast.spell))
                return false;
            break;
        }
        case SMART_ACTION_INVOKER_CAST:
        {
            if (e.GetScriptType() != SMART_SCRIPT_TYPE_TIMED_ACTIONLIST && e.GetEventType() != SMART_EVENT_LINK && !EventHasInvoker(e.event.type))
            {
                TC_LOG_ERROR("sql.sql", "SmartAIMgr: Entry %d SourceType %u Event %u Action %u has invoker cast action, but event does not provide any invoker!", e.entryOrGuid, e.GetScriptType(), e.GetEventType(), e.GetActionType());
                // allow this to load for now
                // return false;
            }
            [[fallthrough]];
        }
        case SMART_ACTION_ADD_AURA:
        case SMART_ACTION_SELF_CAST:
        case SMART_ACTION_CAST:
        {
            if (!IsSpellValid(e, e.action.cast.spell))
                return false;
            break;
        }
        case SMART_ACTION_CALL_AREAEXPLOREDOREVENTHAPPENS:
        case SMART_ACTION_CALL_GROUPEVENTHAPPENS:
            if (Quest const* qid = sObjectMgr->GetQuestTemplate(e.action.quest.quest))
            {
                if (!qid->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT))
                {
                    SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u SpecialFlags for Quest entry %u does not include FLAGS_EXPLORATION_OR_EVENT(2), skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.quest.quest);
                    return false;
                }
            }
            else
            {
                SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Quest entry %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.quest.quest);
                return false;
            }
            break;
        case SMART_ACTION_SET_EVENT_PHASE:
            if (e.action.setEventPhase.phase >=  SMART_EVENT_PHASE_MAX)
            {
                SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u attempts to set phase %u. Phase mask cannot be used past phase %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.setEventPhase.phase, SMART_EVENT_PHASE_MAX-1);
                return false;
            }
            break;
        case SMART_ACTION_INC_EVENT_PHASE:
            if (!e.action.incEventPhase.inc && !e.action.incEventPhase.dec)
            {
                SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u is incrementing phase by 0, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                return false;
            }
            else if (e.action.incEventPhase.inc > SMART_EVENT_PHASE_MAX || e.action.incEventPhase.dec > SMART_EVENT_PHASE_MAX)
            {
                SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u attempts to increment phase by too large value, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                return false;
            }
            break;
        case SMART_ACTION_REMOVEAURASFROMSPELL:
            if (e.action.removeAura.spell != 0 && !IsSpellValid(e, e.action.removeAura.spell))
                return false;
            break;
        case SMART_ACTION_RANDOM_PHASE:
            {
                if (e.action.randomPhase.phase1 >= SMART_EVENT_PHASE_MAX ||
                    e.action.randomPhase.phase2 >= SMART_EVENT_PHASE_MAX ||
                    e.action.randomPhase.phase3 >= SMART_EVENT_PHASE_MAX ||
                    e.action.randomPhase.phase4 >= SMART_EVENT_PHASE_MAX ||
                    e.action.randomPhase.phase5 >= SMART_EVENT_PHASE_MAX ||
                    e.action.randomPhase.phase6 >= SMART_EVENT_PHASE_MAX)
                {
                    SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u attempts to set invalid phase, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                    return false;
                }
            }
            break;
        case SMART_ACTION_RANDOM_PHASE_RANGE:       //PhaseMin, PhaseMax
            {
                if (e.action.randomPhaseRange.phaseMin >= SMART_EVENT_PHASE_MAX ||
                    e.action.randomPhaseRange.phaseMax >= SMART_EVENT_PHASE_MAX)
                {
                    SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u attempts to set invalid phase, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                    return false;
                }
                if (!IsMinMaxValid(e, e.action.randomPhaseRange.phaseMin, e.action.randomPhaseRange.phaseMax))
                    return false;
                break;
            }
        case SMART_ACTION_SUMMON_CREATURE:
        {
            if (!IsCreatureValid(e, e.action.summonCreature.creature))
                return false;

            CacheSpellContainerBounds sBounds = GetSummonCreatureSpellContainerBounds(e.action.summonCreature.creature);
            for (auto itr = sBounds.first; itr != sBounds.second; ++itr)
                SMARTAI_DB_WARNING( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u creature summon: There is a summon spell for creature entry %u (SpellId: %u, effect: %u)",
                                e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.summonCreature.creature, itr->second.first, itr->second.second);

            if(e.action.summonCreature.attackInvoker && e.action.summonCreature.attackVictim)
            {
                SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u cant attack invoker and victim at the same time, removed attack victim, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType());
                e.action.summonCreature.attackVictim = 0;
            }

            if (e.action.summonCreature.type < TEMPSUMMON_TIMED_OR_DEAD_DESPAWN || e.action.summonCreature.type > TEMPSUMMON_MANUAL_DESPAWN)
            {
                SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses incorrect TempSummonType %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.summonCreature.type);
                return false;
            }
            break;
        }
        case SMART_ACTION_CALL_KILLEDMONSTER:
        {
            if (!IsCreatureValid(e, e.action.killedMonster.creature))
                return false;

            CacheSpellContainerBounds sBounds = GetKillCreditSpellContainerBounds(e.action.killedMonster.creature);
            for (auto itr = sBounds.first; itr != sBounds.second; ++itr)
                SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u Kill Credit: There is a killcredit spell for creatureEntry %u (SpellId: %u effect: %u)",
                                e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.killedMonster.creature, itr->second.first, itr->second.second);

            if (e.GetTargetType() == SMART_TARGET_POSITION)
            {
                SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses incorrect TargetType %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.GetTargetType());
                return false;
            }
            break;
        }
        case SMART_ACTION_UPDATE_TEMPLATE:
            if (!IsCreatureValid(e, e.action.updateTemplate.creature))
                return false;
            break;
        case SMART_ACTION_SET_SHEATH:
            if (e.action.setSheath.sheath && e.action.setSheath.sheath >= MAX_SHEATH_STATE)
            {
                SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses incorrect Sheath state %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.setSheath.sheath);
                return false;
            }
            break;
        case SMART_ACTION_SET_REACT_STATE:
            {
                if (e.action.react.state > REACT_AGGRESSIVE)
                {
                    SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Creature %d Event %u Action %u uses invalid React State %u, skipped.", e.entryOrGuid, e.event_id, e.GetActionType(), e.action.react.state);
                    return false;
                }
                break;
            }
        case SMART_ACTION_SUMMON_GO:
        {
            if (!IsGameObjectValid(e, e.action.summonGO.entry))
                return false;

            CacheSpellContainerBounds sBounds = GetSummonGameObjectSpellContainerBounds(e.action.summonGO.entry);
            for (auto itr = sBounds.first; itr != sBounds.second; ++itr)
                SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Warning: Entry %d SourceType %u Event %u Action %u gameobject summon: There is a summon spell for gameobject entry %u (SpellId: %u, effect: %u)",
                                e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.summonGO.entry, itr->second.first, itr->second.second);

            break;
        }
        case SMART_ACTION_ADD_ITEM:
        case SMART_ACTION_REMOVE_ITEM:
            if (!IsItemValid(e, e.action.item.entry))
                return false;

            if (!NotNULL(e, e.action.item.count))
                return false;
            break;
        case SMART_ACTION_TELEPORT:
            if (!sMapStore.LookupEntry(e.action.teleport.mapID))
            {
                SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Map entry %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.teleport.mapID);
                return false;
            }
            break;
        case SMART_ACTION_INSTALL_AI_TEMPLATE:
            if (e.action.installTemplate.id >= SMARTAI_TEMPLATE_END)
            {
                SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Creature %d Event %u Action %u uses non-existent AI template id %u, skipped.", e.entryOrGuid, e.event_id, e.GetActionType(), e.action.installTemplate.id);
                return false;
            }
            break;
        case SMART_ACTION_WP_STOP:
            if (e.action.wpStop.quest && !IsQuestValid(e, e.action.wpStop.quest))
                return false;
            break;
        case SMART_ACTION_WP_START:
            {
                auto const path = sSmartWaypointMgr->GetPath(e.action.wpStart.pathID);
                if (!path || path->nodes.empty())
                {
                    SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Creature %d Event %u Action %u uses non-existent or empty WaypointPath id %u, skipped.", e.entryOrGuid, e.event_id, e.GetActionType(), e.action.wpStart.pathID);
                    return false;
                }
                if (e.action.wpStart.quest && !IsQuestValid(e, e.action.wpStart.quest))
                    return false;
                if (e.action.wpStart.reactState > REACT_AGGRESSIVE)
                {
                    SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Creature %d Event %u Action %u uses invalid React State %u, skipped.", e.entryOrGuid, e.event_id, e.GetActionType(), e.action.wpStart.reactState);
                    return false;
                }
                break;
            }
        case SMART_ACTION_CREATE_TIMED_EVENT:
        {
            if (!IsMinMaxValid(e, e.action.timeEvent.min, e.action.timeEvent.max))
                return false;

            if (!IsMinMaxValid(e, e.action.timeEvent.repeatMin, e.action.timeEvent.repeatMax))
                return false;
            break;
        }
        case SMART_ACTION_SET_POWER:
        case SMART_ACTION_ADD_POWER:
        case SMART_ACTION_REMOVE_POWER:
            if (e.action.power.powerType > MAX_POWERS)
            {
                SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Power %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.power.powerType);
                return false;
            }
            break;
        case SMART_ACTION_GAME_EVENT_STOP:
        {
            uint32 eventId = e.action.gameEventStop.id;

             GameEventMgr::GameEventDataMap const& events = sGameEventMgr->GetEventMap();
            if (eventId < 1 || eventId >= events.size())
            {
                SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %u SourceType %u Event %u Action %u uses non-existent event, eventId %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.gameEventStop.id);
                return false;
            }

            GameEventData const& eventData = events[eventId];
            if (!eventData.isValid())
            {
                SMARTAI_DB_ERROR( e.entryOrGuid,"SmartAIMgr: Entry %u SourceType %u Event %u Action %u uses non-existent event, eventId %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.gameEventStop.id);
                return false;
            }
            break;
        }
        case SMART_ACTION_GAME_EVENT_START:
        {
            uint32 eventId = e.action.gameEventStart.id;

             GameEventMgr::GameEventDataMap const& events = sGameEventMgr->GetEventMap();
            if (eventId < 1 || eventId >= events.size())
            {
                SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %u SourceType %u Event %u Action %u uses non-existent event, eventId %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.gameEventStart.id);
                return false;
            }

            GameEventData const& eventData = events[eventId];
            if (!eventData.isValid())
            {
                SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Entry %u SourceType %u Event %u Action %u uses non-existent event, eventId %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.gameEventStart.id);
                return false;
            }
            break;
        }
        case SMART_ACTION_CALL_RANDOM_RANGE_TIMED_ACTIONLIST:
        {
            if (!IsMinMaxValid(e, e.action.randTimedActionList.entry1, e.action.randTimedActionList.entry2))
                return false;
            break;
        }
        case SMART_ACTION_SET_INST_DATA:
        {
            if (e.action.setInstanceData.type > 1)
            {
                TC_LOG_ERROR("sql.sql", "Entry %u SourceType %u Event %u Action %u uses invalid data type %u (value range 0-1), skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.setInstanceData.type);
                return false;
            }
            else if (e.action.setInstanceData.type == 1)
            {
                if (e.action.setInstanceData.data > TO_BE_DECIDED)
                {
                    TC_LOG_ERROR("sql.sql", "Entry %u SourceType %u Event %u Action %u uses invalid boss state %u (value range 0-5), skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.setInstanceData.data);
                    return false;
                }
            }
            break;
        }
        case SMART_ACTION_REMOVE_AURAS_BY_TYPE:
        {
            if (e.action.auraType.type >= TOTAL_AURAS)
            {
                TC_LOG_ERROR("sql.sql", "Entry %u SourceType %u Event %u Action %u uses invalid data type %u (value range 0-TOTAL_AURAS), skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.auraType.type);
                return false;
            }
            break;
        }
        case SMART_ACTION_RESPAWN_BY_SPAWNID:
            if (!sObjectMgr->GetSpawnData(SpawnObjectType(e.action.respawnData.spawnType), e.action.respawnData.spawnId))
            {
                TC_LOG_ERROR("sql.sql", "Entry %u SourceType %u Event %u Action %u specifies invalid spawn data (%u,%u)", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), e.action.respawnData.spawnType, e.action.respawnData.spawnId);
                return false;
            }
            break;
    }

    if(e.GetActionType() >= SMART_ACTION_END)
    {
        SMARTAI_DB_ERROR( e.entryOrGuid, "SmartAIMgr: Not handled action_type(%u), event_type(%u), Entry %d SourceType %u Event %u, skipped.", e.GetActionType(), e.GetEventType(), e.entryOrGuid, e.GetScriptType(), e.event_id);
        return false;
    }

    return true;
}

bool SmartAIMgr::IsTextValid(SmartScriptHolder const& e, uint32 id)
{
    if (e.GetScriptType() != SMART_SCRIPT_TYPE_CREATURE)
        return true;

    uint32 entry = 0;

    if (e.GetEventType() == SMART_EVENT_TEXT_OVER)
    {
        entry = e.event.textOver.creatureEntry;
    }
    else
    {
        switch (e.GetTargetType())
        {
            case SMART_TARGET_CREATURE_DISTANCE:
            case SMART_TARGET_CREATURE_RANGE:
            case SMART_TARGET_CLOSEST_CREATURE:
                return true; // ignore
            default:
                if (e.entryOrGuid < 0)
                {
                    uint32 guid = -e.entryOrGuid;
                    CreatureData const* data = sObjectMgr->GetCreatureData(guid);
                    if (!data)
                    {
                        TC_LOG_ERROR("sql.sql", "SmartAIMgr: Entry %d SourceType %u Event %u Action %u using non-existent Creature guid %d, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), guid);
                        SMARTAI_DB_ERROR(e.entryOrGuid, "SmartAIMgr: Entry %d SourceType %u Event %u Action %u using non-existent Creature guid %d, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), guid);
                        return false;
                    }
                    else
                        entry = data->GetFirstSpawnEntry();
                }
                else
                    entry = uint32(e.entryOrGuid);
                break;
        }
    }

    if (!entry || !sCreatureTextMgr->TextExist(entry, uint8(id)))
    {
        TC_LOG_ERROR("sql.sql", "SmartAIMgr: Entry %d SourceType %u Event %u Action %u using non-existent Text id %d, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), id);
        return false;
    }

    return true;
}

void SmartAIMgr::LoadHelperStores()
{
    uint32 oldMSTime = GetMSTime();

    SpellInfo const* spellInfo = nullptr;// sSpellMgr->GetSpellInfo(e.event.spellHit.spell);
    for (auto itr = sObjectMgr->GetSpellStore().begin(); itr != sObjectMgr->GetSpellStore().end(); ++itr)
    {
        uint32 id = itr->first;
        spellInfo = sSpellMgr->GetSpellInfo(id);
        if (!spellInfo)
            continue;

        for (uint32 j = 0; j < MAX_SPELL_EFFECTS; ++j)
        {
            if (spellInfo->Effects[j].Effect == SPELL_EFFECT_SUMMON)
                SummonCreatureSpellStore.insert(std::make_pair(uint32(spellInfo->Effects[j].MiscValue), std::make_pair(id, SpellEffIndex(j))));

            else if (spellInfo->Effects[j].Effect == SPELL_EFFECT_SUMMON_OBJECT_WILD)
                SummonGameObjectSpellStore.insert(std::make_pair(uint32(spellInfo->Effects[j].MiscValue), std::make_pair(id, SpellEffIndex(j))));

            else if (spellInfo->Effects[j].Effect == SPELL_EFFECT_KILL_CREDIT)
                KillCreditSpellStore.insert(std::make_pair(uint32(spellInfo->Effects[j].MiscValue), std::make_pair(id, SpellEffIndex(j))));
        }
    }

    TC_LOG_INFO("server.loading",">> Loaded SmartAIMgr Helpers in %u ms", GetMSTimeDiffToNow(oldMSTime));
}

void SmartAIMgr::UnLoadHelperStores()
{
    SummonCreatureSpellStore.clear();
    SummonGameObjectSpellStore.clear();
    KillCreditSpellStore.clear();
}

SmartScriptHolder& SmartAIMgr::FindLinkedSourceEvent(SmartAIEventList& list, uint32 eventId)
{
    SmartAIEventList::iterator itr = std::find_if(list.begin(), list.end(),
        [eventId](SmartScriptHolder& source) { return source.link == eventId; });

    if (itr != list.end())
        return *itr;

    static SmartScriptHolder SmartScriptHolderDummy;
    return SmartScriptHolderDummy;
}

SmartScriptHolder& SmartAIMgr::FindLinkedEvent(SmartAIEventList& list, uint32 link)
{
    SmartAIEventList::iterator itr = std::find_if(list.begin(), list.end(),
        [link](SmartScriptHolder& linked) { return linked.event_id == link && linked.GetEventType() == SMART_EVENT_LINK; });

    if (itr != list.end())
        return *itr;

    static SmartScriptHolder SmartScriptHolderDummy;
    return SmartScriptHolderDummy;
}

/*static*/ bool SmartAIMgr::EventHasInvoker(SMART_EVENT event)
{
    switch (event)
    { // white list of events that actually have an invoker passed to them
        case SMART_EVENT_AGGRO:
        case SMART_EVENT_DEATH:
        case SMART_EVENT_KILL:
        case SMART_EVENT_SUMMONED_UNIT:
        case SMART_EVENT_SPELLHIT:
        case SMART_EVENT_SPELLHIT_TARGET:
        case SMART_EVENT_DAMAGED:
        case SMART_EVENT_RECEIVE_HEAL:
        case SMART_EVENT_RECEIVE_EMOTE:
        case SMART_EVENT_JUST_SUMMONED:
        case SMART_EVENT_DAMAGED_TARGET:
        case SMART_EVENT_SUMMON_DESPAWNED:
        case SMART_EVENT_PASSENGER_BOARDED:
        case SMART_EVENT_PASSENGER_REMOVED:
        case SMART_EVENT_GOSSIP_HELLO:
        case SMART_EVENT_GOSSIP_SELECT:
        case SMART_EVENT_ACCEPTED_QUEST:
        case SMART_EVENT_REWARD_QUEST:
        case SMART_EVENT_FOLLOW_COMPLETED:
        case SMART_EVENT_ON_SPELLCLICK:
        case SMART_EVENT_GO_LOOT_STATE_CHANGED:
        case SMART_EVENT_AREATRIGGER_ONTRIGGER:
        case SMART_EVENT_IC_LOS:
        case SMART_EVENT_OOC_LOS:
        case SMART_EVENT_DISTANCE_CREATURE:
        case SMART_EVENT_FRIENDLY_HEALTH:
        case SMART_EVENT_FRIENDLY_HEALTH_PCT:
        case SMART_EVENT_FRIENDLY_IS_CC:
        case SMART_EVENT_FRIENDLY_MISSING_BUFF:
        case SMART_EVENT_ACTION_DONE:
        case SMART_EVENT_TARGET_HEALTH_PCT:
        case SMART_EVENT_TARGET_MANA_PCT:
        case SMART_EVENT_RANGE:
        case SMART_EVENT_VICTIM_CASTING:
        case SMART_EVENT_TARGET_BUFFED:
        case SMART_EVENT_IS_BEHIND_TARGET:
        case SMART_EVENT_INSTANCE_PLAYER_ENTER:
        case SMART_EVENT_TRANSPORT_ADDCREATURE:
        case SMART_EVENT_DATA_SET:
            return true;
        default:
            return false;
    }
}

std::list<std::string> const& SmartAIMgr::GetErrorList(int32 entryOrGuid)
{
    return databaseErrors[entryOrGuid];
}

void SmartAIMgr::LogSmartAIDBError(int32 entryOrGuid, const char* cStr, ...)
{
    if(!entryOrGuid)
        return;

    va_list ap;
    va_start(ap, cStr);

    char text[MAX_QUERY_LEN];
    vsnprintf(text, MAX_QUERY_LEN, cStr, ap);
    
    std::string str(text);

    va_end(ap);

    databaseErrors[entryOrGuid].push_back(str);
}

CacheSpellContainerBounds SmartAIMgr::GetSummonCreatureSpellContainerBounds(uint32 creatureEntry) const
{
    return SummonCreatureSpellStore.equal_range(creatureEntry);
}

CacheSpellContainerBounds SmartAIMgr::GetSummonGameObjectSpellContainerBounds(uint32 gameObjectEntry) const
{
    return SummonGameObjectSpellStore.equal_range(gameObjectEntry);
}

CacheSpellContainerBounds SmartAIMgr::GetKillCreditSpellContainerBounds(uint32 killCredit) const
{
    return KillCreditSpellStore.equal_range(killCredit);
}

void SmartAIMgr::ReloadCreaturesScripts(Map* map)
{
    //get creature list then release lock, as function inside AIM_Init may need this lock as well
    std::list<Creature*> updateList;
    {
        
        /*
        FIXME
        boost::shared_lock<boost::shared_mutex> lock(*HashMapHolder<Creature>::GetLock());
        auto creatures = ObjectAccessor::GetCreatures();

        for (auto pair : creatures)
            updateList.push_back(pair.second);
            */ 
    }
    for (auto c : updateList)
    {
        if (c->GetAIName() == SMARTAI_AI_NAME)
            c->AIM_Initialize();
    }
}

bool SmartAIMgr::IsItemValid(SmartScriptHolder const& e, uint32 entry)
{
    if (!sItemStore.LookupEntry(entry))
    {
        TC_LOG_ERROR("sql.sql", "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Item entry %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), entry);
        return false;
    }
    return true;
}

bool SmartAIMgr::IsTextEmoteValid(SmartScriptHolder const& e, uint32 entry)
{
    if (!sEmotesTextStore.LookupEntry(entry))
    {
        TC_LOG_ERROR("sql.sql", "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Text Emote entry %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), entry);
        return false;
    }
    return true;
}

bool SmartAIMgr::IsEmoteValid(SmartScriptHolder const& e, uint32 entry)
{
    if (!sEmotesStore.LookupEntry(entry))
    {
        TC_LOG_ERROR("sql.sql","SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Emote entry %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), entry);
        return false;
    }
    return true;
}

bool SmartAIMgr::IsAreaTriggerValid(SmartScriptHolder const& e, uint32 entry)
{
    if (!sAreaTriggerStore.LookupEntry(entry))
    {
        TC_LOG_ERROR("sql.sql", "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent AreaTrigger entry %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), entry);
        return false;
    }
    return true;
}

bool SmartAIMgr::IsSoundValid(SmartScriptHolder const& e, uint32 entry)
{
    if (!sSoundEntriesStore.LookupEntry(entry))
    {
        TC_LOG_ERROR("sql.sql", "SmartAIMgr: Entry %d SourceType %u Event %u Action %u uses non-existent Sound entry %u, skipped.", e.entryOrGuid, e.GetScriptType(), e.event_id, e.GetActionType(), entry);
        return false;
    }
    return true;
}

ObjectGuidVector::ObjectGuidVector(ObjectVector const& objectVector) : _objectVector(objectVector)
{
    _guidVector.reserve(_objectVector.size());
    for (WorldObject* obj : _objectVector)
        _guidVector.push_back(obj->GetGUID());
}

void ObjectGuidVector::UpdateObjects(WorldObject const& ref) const
{
    _objectVector.clear();

    for (ObjectGuid const& guid : _guidVector)
        if (WorldObject* obj = ObjectAccessor::GetWorldObject(ref, guid))
            _objectVector.push_back(obj);
}
