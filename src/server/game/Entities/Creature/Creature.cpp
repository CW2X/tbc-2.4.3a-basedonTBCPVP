
#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Creature.h"
#include "QuestDef.h"
#include "QueryPackets.h"
#include "GossipDef.h"
#include "Player.h"
#include "Opcodes.h"
#include "Log.h"
#include "GroupMgr.h"
#include "LootMgr.h"
#include "MapManager.h"
#include "CreatureAI.h"
#include "CreatureAISelector.h"
#include "Formulas.h"
#include "SpellAuras.h"
#include "WaypointMovementGenerator.h"
#include "InstanceScript.h"
#include "BattleGround.h"
#include "Util.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "OutdoorPvPMgr.h"
#include "GameEventMgr.h"
#include "CreatureGroups.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"
#include "MoveSpline.h"
#include "Spell.h"
#include "InstanceScript.h"
#include "Transport.h"
#include "SpellAuraEffects.h"
#include "PoolMgr.h"
#include "SpellAuraEffects.h"
#include "GameTime.h"

std::string CreatureMovementData::ToString() const
{
    char const* const GroundStates[] = { "None", "Run", "Hover" };
    char const* const FlightStates[] = { "None", "DisableGravity", "CanFly" };

    std::ostringstream str;
    str << std::boolalpha
        << "Ground: " << GroundStates[AsUnderlyingType(Ground)]
        << ", Swim: " << Swim
        << ", Flight: " << FlightStates[AsUnderlyingType(Flight)];
    if (Rooted)
        str << ", Rooted";

    return str.str();
}

void TrainerSpellData::Clear()
{
    for (auto & itr : spellList)
        delete itr;

    spellList.clear();
}

TrainerSpell const* TrainerSpellData::Find(uint32 spell_id) const
{
    for(auto itr : spellList)
        if(itr->spell == spell_id)
            return itr;

    return nullptr;
}

bool VendorItemData::RemoveItem( uint32 item_id )
{
    for(auto i = m_items.begin(); i != m_items.end(); ++i )
    {
        if ((*i)->item == item_id)
        {
            m_items.erase(i);
            return true;
        }
    }
    return false;
}

size_t VendorItemData::FindItemSlot(uint32 item_id) const
{
    for(size_t i = 0; i < m_items.size(); ++i )
        if(m_items[i]->item == item_id)
            return i;
    return m_items.size();
}

VendorItem const* VendorItemData::FindItem(uint32 item_id) const
{
    for(auto m_item : m_items)
        if(m_item->item == item_id)
            return m_item;
    return nullptr;
}

uint32 CreatureTemplate::GetRandomValidModelId() const
{
    uint32 c = 0;
    uint32 modelIDs[4];

    if (Modelid1) modelIDs[c++] = Modelid1;
    if (Modelid2) modelIDs[c++] = Modelid2;
    if (Modelid3) modelIDs[c++] = Modelid3;
    if (Modelid4) modelIDs[c++] = Modelid4;

    return ((c>0) ? modelIDs[urand(0,c-1)] : 0);
}

uint32 CreatureTemplate::GetFirstValidModelId() const
{
    if(Modelid1) return Modelid1;
    if(Modelid2) return Modelid2;
    if(Modelid3) return Modelid3;
    if(Modelid4) return Modelid4;
    return 0;
}

uint32 CreatureTemplate::GetFirstInvisibleModel() const
{
    CreatureModelInfo const* modelInfo = sObjectMgr->GetCreatureModelInfo(Modelid1);
    if (modelInfo && modelInfo->is_trigger)
        return Modelid1;

    modelInfo = sObjectMgr->GetCreatureModelInfo(Modelid2);
    if (modelInfo && modelInfo->is_trigger)
        return Modelid2;

    modelInfo = sObjectMgr->GetCreatureModelInfo(Modelid3);
    if (modelInfo && modelInfo->is_trigger)
        return Modelid3;

    modelInfo = sObjectMgr->GetCreatureModelInfo(Modelid4);
    if (modelInfo && modelInfo->is_trigger)
        return Modelid4;

    return 11686;
}

uint32 CreatureTemplate::GetFirstVisibleModel() const
{
    CreatureModelInfo const* modelInfo = sObjectMgr->GetCreatureModelInfo(Modelid1);
    if (modelInfo && !modelInfo->is_trigger)
        return Modelid1;

    modelInfo = sObjectMgr->GetCreatureModelInfo(Modelid2);
    if (modelInfo && !modelInfo->is_trigger)
        return Modelid2;

    modelInfo = sObjectMgr->GetCreatureModelInfo(Modelid3);
    if (modelInfo && !modelInfo->is_trigger)
        return Modelid3;

    modelInfo = sObjectMgr->GetCreatureModelInfo(Modelid4);
    if (modelInfo && !modelInfo->is_trigger)
        return Modelid4;

    return 17519;
}

void CreatureTemplate::InitializeQueryData()
{
    for (uint8 loc = LOCALE_enUS; loc < TOTAL_LOCALES; ++loc)
        QueryData[loc] = BuildQueryData(static_cast<LocaleConstant>(loc));
}

WorldPacket CreatureTemplate::BuildQueryData(LocaleConstant loc) const
{
    WorldPackets::Query::QueryCreatureResponse queryTemp;

    std::string locName = Name, locTitle = Title;
    if (CreatureLocale const* cl = sObjectMgr->GetCreatureLocale(Entry))
    {
        ObjectMgr::GetLocaleString(cl->Name, loc, locName);
        ObjectMgr::GetLocaleString(cl->Title, loc, locTitle);
    }

    queryTemp.CreatureID = Entry;
    queryTemp.Allow = true;

    queryTemp.Stats.Name = locName;
    queryTemp.Stats.NameAlt = locTitle;
    queryTemp.Stats.CursorName = IconName;
    queryTemp.Stats.Flags = type_flags;
    queryTemp.Stats.CreatureType = type;
    queryTemp.Stats.CreatureFamily = family;
    queryTemp.Stats.Classification = rank;
#ifdef LICH_KING
    memcpy(queryTemp.Stats.ProxyCreatureID, KillCredit, sizeof(uint32) * MAX_KILL_CREDIT);
#else
    queryTemp.Stats.PetSpellDataId = PetSpellDataId;
#endif
    queryTemp.Stats.CreatureDisplayID[0] = Modelid1;
    queryTemp.Stats.CreatureDisplayID[1] = Modelid2;
    queryTemp.Stats.CreatureDisplayID[2] = Modelid3;
    queryTemp.Stats.CreatureDisplayID[3] = Modelid4;
    queryTemp.Stats.HpMulti = ModHealth;
    queryTemp.Stats.EnergyMulti = ModMana;
    queryTemp.Stats.Leader = RacialLeader;

#ifdef LICH_KING
    for (uint32 i = 0; i < MAX_CREATURE_QUEST_ITEMS; ++i)
        queryTemp.Stats.QuestItems[i] = 0;

    if (std::vector<uint32> const* items = sObjectMgr->GetCreatureQuestItemList(Entry))
        for (uint32 i = 0; i < MAX_CREATURE_QUEST_ITEMS; ++i)
            if (i < items->size())
                queryTemp.Stats.QuestItems[i] = (*items)[i];

    queryTemp.Stats.CreatureMovementInfoID = movementId;
#endif
    queryTemp.Write();
    queryTemp.ShrinkToFit();
    return queryTemp.Move();
}

CreatureBaseStats const* CreatureBaseStats::GetBaseStats(uint8 level, uint8 unitClass)
{
    return sObjectMgr->GetCreatureBaseStats(level, unitClass);
}

bool AssistDelayEvent::Execute(uint64 /*e_time*/, uint32 /*p_time*/)
{
    Unit* victim = ObjectAccessor::GetUnit(m_owner, m_victim);
    if (victim)
    {
        while (!m_assistants.empty())
        {
            Creature* assistant = ObjectAccessor::GetCreature(m_owner, *m_assistants.begin());
            m_assistants.pop_front();

            if (assistant && assistant->CanAssistTo(&m_owner, victim))
            {
                assistant->SetNoCallAssistance(true);
                assistant->EngageWithTarget(victim);

                if (InstanceScript* instance = ((InstanceScript*)assistant->GetInstanceScript()))
                    instance->MonsterPulled(assistant, victim);
            }
        }
    }
    return true;
}

bool ForcedDespawnDelayEvent::Execute(uint64 /*e_time*/, uint32 /*p_time*/)
{
    m_owner.DespawnOrUnsummon(0, m_respawnTimer);    // since we are here, we are not TempSummon as object type cannot change during runtime
    return true;
}

Creature::Creature(bool isWorldObject) : Unit(isWorldObject), MapObject(),
    m_lootMoney(0), 
    m_lootRecipientGroup(0), 
    m_groupLootTimer(0),
    lootingGroupLowGUID(0),
    m_corpseRemoveTime(0),
    m_respawnTime(0), 
    m_respawnDelay(25),
    m_corpseDelay(60), 
    m_respawnradius(0.0f),
    m_reactState(REACT_AGGRESSIVE), 
    m_defaultMovementType(IDLE_MOTION_TYPE), 
    m_equipmentId(0), 
    m_originalEquipmentId(0),
    m_areaCombatTimer(0), 
    m_relocateTimer(60000),
    m_AlreadyCallAssistance(false), 
    m_regenHealth(true), 
    m_meleeDamageSchoolMask(SPELL_SCHOOL_MASK_NORMAL),
    m_creatureInfo(nullptr), 
    m_creatureData(nullptr),
    m_creatureInfoAddon(nullptr), 
    m_spawnId(0), 
    m_formation(nullptr),
    m_PlayerDamageReq(0),
    m_timeSinceSpawn(0), 
    m_creaturePoolId(0), 
    m_focusSpell(nullptr),
    m_focusDelay(0),
    m_shouldReacquireTarget(false), 
    m_suppressedOrientation(0.0f),
    m_isBeingEscorted(false), 
    _waypointPathId(0),
    _currentWaypointNodeInfo(0, 0),
    m_unreachableTargetTime(0), 
    m_evadingAttacks(false), 
    m_canFly(false),
    m_stealthAlertCooldown(0), 
    m_keepActiveTimer(0), 
    m_homeless(false), 
    m_respawnCompatibilityMode(false),
    m_triggerJustAppeared(false),
    m_boundaryCheckTime(2500), 
    _pickpocketLootRestore(0),
    m_combatPulseTime(0), 
    m_combatPulseDelay(0),
    m_lastDamagedTime(0),
    m_movementFlagsUpdateTimer(MOVEMENT_FLAGS_UPDATE_TIMER),
    m_originalEntry(0),
    m_questPoolId(0),
    m_chosenTemplate(0),
    m_spells(),
    disableReputationGain(false)
{
    m_valuesCount = UNIT_END;

    m_SightDistance = sWorld->getFloatConfig(CONFIG_SIGHT_MONSTER);

    ResetLootMode(); // restore default loot mode
}

Creature::~Creature()
{
    m_vendorItemCounts.clear();
}

void Creature::AddToWorld()
{
    ///- Register the creature for guid lookup
    if(!IsInWorld())
    {
        GetMap()->GetObjectsStore().Insert<Creature>(GetGUID(), this);
        if (m_spawnId)
            GetMap()->GetCreatureBySpawnIdStore().insert(std::make_pair(m_spawnId, this));

        Unit::AddToWorld();
        SearchFormation();

        if(ObjectGuid::LowType guid = GetSpawnId())
        {
            if (CreatureData const* data = sObjectMgr->GetCreatureData(guid))
                m_creaturePoolId = data->poolId;
            if (m_creaturePoolId)
                FindMap()->AddCreatureToPool(this, m_creaturePoolId);
        }

        AIM_Initialize();

#ifdef LICH_KING
        if (IsVehicle())
            GetVehicleKit()->Install();
#endif

        if (GetZoneScript())
            GetZoneScript()->OnCreatureCreate(this);
    }
}

void Creature::RemoveFromWorld()
{
    ///- Remove the creature from the accessor
    if(IsInWorld())
    {
        if (GetZoneScript())
            GetZoneScript()->OnCreatureRemove(this);

        if(m_formation)
            sFormationMgr->RemoveCreatureFromGroup(m_formation, this);

        if (m_creaturePoolId)
            FindMap()->RemoveCreatureFromPool(this, m_creaturePoolId);

        Unit::RemoveFromWorld();

        if (m_spawnId)
            Trinity::Containers::MultimapErasePair(GetMap()->GetCreatureBySpawnIdStore(), m_spawnId, this);

        GetMap()->GetObjectsStore().Remove<Creature>(GetGUID());
    }
}

void Creature::DisappearAndDie()
{
    DespawnOrUnsummon();
}

bool Creature::IsReturningHome() const
{
    if (GetMotionMaster()->GetCurrentMovementGeneratorType() == HOME_MOTION_TYPE)
        return true;

    return false;
}

void Creature::SearchFormation()
{
    ObjectGuid::LowType spawnId = GetSpawnId();
    if(!spawnId)
        return;

    if (FormationInfo const* formationInfo = sFormationMgr->GetFormationInfo(spawnId))
        sFormationMgr->AddCreatureToGroup(formationInfo->groupID, this);
}

bool Creature::IsFormationLeader() const
{
    if (!m_formation)
        return false;

    return m_formation->IsLeader(this);
}

void Creature::SignalFormationMovement(Position const& destination, uint32 id/* = 0*/, uint32 moveType/* = 0*/, bool orientation/* = false*/)
{
    if (!m_formation)
        return;

    if (!m_formation->IsLeader(this))
        return;

    m_formation->LeaderMoveTo(destination, id, moveType, orientation);
}

bool Creature::IsFormationLeaderMoveAllowed() const
{
    if (!m_formation)
        return false;

    return m_formation->CanLeaderStartMoving();
}

void Creature::RemoveCorpse(bool setSpawnTime, bool destroyForNearbyPlayers)
{
    if (GetDeathState() != CORPSE)
        return;

    if (m_respawnCompatibilityMode)
    {
        m_corpseRemoveTime = GetMap()->GetGameTime();
        SetDeathState(DEAD);
        RemoveAllAuras();
        loot.clear();

        if (CreatureAI* ai = AI())
            ai->CorpseRemoved(m_respawnDelay);

        if (destroyForNearbyPlayers)
            DestroyForNearbyPlayers();

        // Should get removed later, just keep "compatibility" with scripts
        if (setSpawnTime)
            m_respawnTime = std::max<time_t>(GetMap()->GetGameTime() + m_respawnDelay, m_respawnTime);

        // if corpse was removed during falling, the falling will continue and override relocation to respawn position
        if (IsFalling())
            StopMoving();

        float x, y, z, o;
        GetRespawnPosition(x, y, z, &o);

        // We were spawned on transport, calculate real position
        if (IsSpawnedOnTransport())
        {
            Position& pos = m_movementInfo.transport.pos;
            pos.m_positionX = x;
            pos.m_positionY = y;
            pos.m_positionZ = z;
            pos.SetOrientation(o);

            if (Transport* transport = GetTransport())
                transport->CalculatePassengerPosition(x, y, z, &o);
        }

        UpdateAllowedPositionZ(x, y, z);
        SetHomePosition(x, y, z, o);
        GetMap()->CreatureRelocation(this, x, y, z, o);
    }
    else
    {
        // In case this is called directly and normal respawn timer not set
        // Since this timer will be longer than the already present time it
        // will be ignored if the correct place added a respawn timer
        if (setSpawnTime)
        {
            uint32 respawnDelay = m_respawnDelay;
            m_respawnTime = std::max<time_t>(GetMap()->GetGameTime() + respawnDelay, m_respawnTime);

            SaveRespawnTime(0, false);
        }

        if (TempSummon* summon = ToTempSummon())
            summon->UnSummon();
        else
            AddObjectToRemoveList();
    }
}

/**
 * change the entry of creature until respawn
 */
bool Creature::InitEntry(uint32 Entry, const CreatureData *data)
{
    CreatureTemplate const* normalInfo = sObjectMgr->GetCreatureTemplate(Entry);
    if(!normalInfo)
    {
        TC_LOG_ERROR("sql.sql","Creature::InitEntry creature entry %u does not exist.", Entry);
        return false;
    }

    // get heroic mode entry
    uint32 actualEntry = Entry;
    CreatureTemplate const *cinfo = normalInfo;
    if(normalInfo->difficulty_entry_1)
    {
        Map *map = sMapMgr->FindMap(GetMapId(), GetInstanceId());
        if(map && map->IsHeroic())
        {
            cinfo = sObjectMgr->GetCreatureTemplate(normalInfo->difficulty_entry_1);
            if(!cinfo)
            {
                TC_LOG_ERROR("sql.sql","Creature::UpdateEntry creature heroic entry %u does not exist.", actualEntry);
                return false;
            }
        }
    }

    SetEntry(Entry);                                        // normal entry always
    m_creatureInfo = cinfo;                                 // map mode related always

    // equal to player Race field, but creature does not have race
    SetByteValue(UNIT_FIELD_BYTES_0, UNIT_BYTES_0_OFFSET_RACE, 0);
#ifndef LICH_KING
    SetByteValue(UNIT_FIELD_BYTES_2, UNIT_BYTES_2_OFFSET_BUFF_LIMIT, UNIT_BYTE2_CREATURE_BUFF_LIMIT);
#endif

    // known valid are: CLASS_WARRIOR, CLASS_PALADIN, CLASS_ROGUE, CLASS_MAGE
    SetByteValue(UNIT_FIELD_BYTES_0, UNIT_BYTES_0_OFFSET_CLASS, uint8(cinfo->unit_class));

    // Cancel load if no model defined
    if (!(cinfo->GetFirstValidModelId()))
    {
        TC_LOG_ERROR("sql.sql","Creature (Entry: %u) has no model defined in table `creature_template`, can't load. ",Entry);
        return false;
    }

    uint32 display_id = ObjectMgr::ChooseDisplayId(GetCreatureTemplate(), data);
    CreatureModelInfo const *minfo = sObjectMgr->GetCreatureModelRandomGender(display_id);
    if (!minfo)
    {
        TC_LOG_ERROR("sql.sql","Creature (Entry: %u) has model %u not found in table `creature_model_info`, can't load. ", Entry, display_id);
        return false;
    }

    SetDisplayId(display_id);
    SetNativeDisplayId(display_id);

    SetByteValue(UNIT_FIELD_BYTES_0, UNIT_BYTES_0_OFFSET_GENDER, minfo->gender);

    LastUsedScriptID = GetScriptId();

    m_homeless = m_creatureInfo->flags_extra & CREATURE_FLAG_EXTRA_HOMELESS;

    // Load creature equipment
    if(!data)
        LoadEquipment(-1); //sunstrider: load random equipment
    else if(data) // override
    {
        uint32 chosenEquipment = data->ChooseEquipmentId(m_chosenTemplate);
        m_originalEquipmentId = chosenEquipment;
        if(auto overrideEquip = sGameEventMgr->GetEquipmentOverride(m_spawnId))
            LoadEquipment(*overrideEquip);
        else
            LoadEquipment(chosenEquipment);
    }

    SetName(normalInfo->Name);                              // at normal entry always

    SetFloatValue(UNIT_MOD_CAST_SPEED, 1.0f);

    SetSpeedRate(MOVE_WALK,   cinfo->speed_walk);
    SetSpeedRate(MOVE_RUN,    cinfo->speed_run);
    SetSpeedRate(MOVE_SWIM,   cinfo->speed_run);
    SetSpeedRate(MOVE_FLIGHT, cinfo->speed_run);

    // Will set UNIT_FIELD_BOUNDINGRADIUS and UNIT_FIELD_COMBATREACH
    SetObjectScale(cinfo->scale);
#ifdef LICH_KING
    SetFloatValue(UNIT_FIELD_HOVERHEIGHT, cinfo->HoverHeight);
#endif

    if(!IsPet())
        _SetCanFly(GetMovementTemplate().IsFlightAllowed());

    // checked at loading
    m_defaultMovementType = MovementGeneratorType(cinfo->MovementType);
    if(!m_respawnradius && m_defaultMovementType==RANDOM_MOTION_TYPE)
        m_defaultMovementType = IDLE_MOTION_TYPE;

    for (uint8 i=0; i < MAX_CREATURE_SPELLS; ++i)
        m_spells[i] = GetCreatureTemplate()->spells[i];

    SetQuestPoolId(normalInfo->QuestPoolId);

    return true;
}

bool Creature::UpdateEntry(uint32 Entry, const CreatureData *data )
{
    if(!InitEntry(Entry,data))
        return false;

    CreatureTemplate const* cInfo = GetCreatureTemplate();
    m_regenHealth = GetCreatureTemplate()->RegenHealth;

    // creatures always have melee weapon ready if any unless specified otherwise
    SetSheath(SHEATH_STATE_MELEE);

    SetByteValue(UNIT_FIELD_BYTES_2, UNIT_BYTES_2_OFFSET_BUFF_LIMIT, UNIT_BYTE2_CREATURE_BUFF_LIMIT);

    SelectLevel();
    SetFaction(cInfo->faction);

    uint32 npcflag, unit_flags, dynamicflags;
    ObjectMgr::ChooseCreatureFlags(cInfo, npcflag, unit_flags, dynamicflags, data);

    if(GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_WORLDEVENT)
        SetUInt32Value(UNIT_NPC_FLAGS, npcflag | sGameEventMgr->GetNPCFlag(this));
    else
        SetUInt32Value(UNIT_NPC_FLAGS, npcflag);


    SetAttackTime(BASE_ATTACK,  cInfo->baseattacktime);
    SetAttackTime(OFF_ATTACK,   cInfo->baseattacktime);
    SetAttackTime(RANGED_ATTACK,cInfo->rangeattacktime);

    // if unit is in combat, keep this flag
    unit_flags &= ~UNIT_FLAG_IN_COMBAT;
    if (IsInCombat())
        unit_flags |= UNIT_FLAG_IN_COMBAT;

    SetUInt32Value(UNIT_FIELD_FLAGS, unit_flags);
    SetUInt32Value(UNIT_FIELD_FLAGS_2, cInfo->unit_flags2);
    SetUInt32Value(UNIT_DYNAMIC_FLAGS, dynamicflags);

    SetMeleeDamageSchool(SpellSchools(cInfo->dmgschool));
    CreatureBaseStats const* stats = sObjectMgr->GetCreatureBaseStats(GetLevel(), cInfo->unit_class);
    float armor = (float)stats->GenerateArmor(cInfo); /// @todo Why is this treated as uint32 when it's a float?
    SetStatFlatModifier(UNIT_MOD_ARMOR,             BASE_VALUE, armor);
    SetStatFlatModifier(UNIT_MOD_RESISTANCE_HOLY,   BASE_VALUE, float(cInfo->resistance[SPELL_SCHOOL_HOLY-1])); //shifted by 1 because SPELL_SCHOOL_NORMAL is not in array
    SetStatFlatModifier(UNIT_MOD_RESISTANCE_FIRE,   BASE_VALUE, float(cInfo->resistance[SPELL_SCHOOL_FIRE-1]));
    SetStatFlatModifier(UNIT_MOD_RESISTANCE_NATURE, BASE_VALUE, float(cInfo->resistance[SPELL_SCHOOL_NATURE-1]));
    SetStatFlatModifier(UNIT_MOD_RESISTANCE_FROST,  BASE_VALUE, float(cInfo->resistance[SPELL_SCHOOL_FROST-1]));
    SetStatFlatModifier(UNIT_MOD_RESISTANCE_SHADOW, BASE_VALUE, float(cInfo->resistance[SPELL_SCHOOL_SHADOW-1]));
    SetStatFlatModifier(UNIT_MOD_RESISTANCE_ARCANE, BASE_VALUE, float(cInfo->resistance[SPELL_SCHOOL_ARCANE-1]));

    if(GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_DUAL_WIELD)
        SetCanDualWield(true);

    SetCanModifyStats(true);
    UpdateAllStats();

    FactionTemplateEntry const* factionTemplate = sFactionTemplateStore.LookupEntry(GetCreatureTemplate()->faction);
    if (factionTemplate)                                    // check and error show at loading templates
    {
        FactionEntry const* factionEntry = sFactionStore.LookupEntry(factionTemplate->faction);
        if (factionEntry)
            if( !IsCivilian() &&
                (factionEntry->team == ALLIANCE || factionEntry->team == HORDE) )
                SetPvP(true);
    }

    // HACK: trigger creature is always not selectable
    if(IsTrigger())
    {
        SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        SetDisableGravity(true);
    }

    InitializeReactState();

    if(cInfo->flags_extra & CREATURE_FLAG_EXTRA_NO_TAUNT)
    {
        ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
        ApplySpellImmune(0, IMMUNITY_EFFECT,SPELL_EFFECT_ATTACK_ME, true);
    }
    if(GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_NO_SPELL_SLOW)
        ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_HASTE_SPELLS, true);

    if (GetMovementTemplate().IsRooted())
        SetControlled(true, UNIT_STATE_ROOT);

    //TrinityCore has this LoadCreatureAddon();
    UpdateMovementFlags();
    LoadTemplateImmunities();

    GetThreatManager().EvaluateSuppressed();

    //We must update last scriptId or it looks like we reloaded a script, breaking some things such as gossip temporarily
    LastUsedScriptID = GetScriptId();

    return true;
}

void Creature::Update(uint32 diff)
{
    if (IsAIEnabled() && m_triggerJustAppeared && m_deathState != DEAD)
    {
#ifdef LICH_KING
        if (m_respawnCompatibilityMode && m_vehicleKit)
            m_vehicleKit->Reset();
#endif
        m_triggerJustAppeared = false;
        AI()->JustAppeared();
            
        Map *map = FindMap();
        if (map && map->IsDungeon() && ((InstanceMap*)map)->GetInstanceScript())
            ((InstanceMap*)map)->GetInstanceScript()->OnCreatureRespawn(this);
    }

    switch(m_deathState)
    {
        case JUST_RESPAWNED:
            // Must not be called, see Creature::setDeathState JUST_RESPAWNED -> ALIVE promoting.
            TC_LOG_ERROR("entities.unit","Creature (GUIDLow: %u Entry: %u ) in wrong state: JUST_RESPAWNED (4)",GetGUID().GetCounter(),GetEntry());
            break;
        case JUST_DIED:
            // Must not be called, see Creature::setDeathState JUST_DIED -> CORPSE promoting.
            TC_LOG_ERROR("entities.unit","Creature (GUIDLow: %u Entry: %u ) in wrong state: JUST_DIED (1)",GetGUID().GetCounter(),GetEntry());
            break;
        case DEAD:
        {
            if (!m_respawnCompatibilityMode)
            {
                TC_LOG_ERROR("entities.unit", "Creature (GUID: %u Entry: %u) in wrong state: DEAD (3)", GetGUID().GetCounter(), GetEntry());
                break;
            }
            time_t now = GetMap()->GetGameTime();
            if( m_respawnTime <= now )
            {
                // Delay respawn if spawn group is not active
                if(m_creatureData && !GetMap()->IsSpawnGroupActive(m_creatureData->spawnGroupData->groupId))
                {
                    m_respawnTime = now + urand(4, 7);
                    break; // Will be rechecked on next Update call after delay expires
                }
                
                Respawn();
            }
            break;
        }
        case CORPSE:
        {
            Unit::Update(diff);

            if (m_corpseRemoveTime <= GetMap()->GetGameTime())
            {
                RemoveCorpse(false);
                TC_LOG_DEBUG("entities.unit","Removing corpse... %u ", GetUInt32Value(OBJECT_FIELD_ENTRY));
            }
            else
            {
                if (m_groupLootTimer && lootingGroupLowGUID)
                {
                    if(diff <= m_groupLootTimer)
                    {
                        m_groupLootTimer -= diff;
                    }
                    else
                    {
                        Group* group = sGroupMgr->GetGroupByGUID(lootingGroupLowGUID);
                        if (group)
                            group->EndRoll(&loot, GetMap());
                        m_groupLootTimer = 0;
                        lootingGroupLowGUID = 0;
                    }
                }
            }

            break;
        }
        case ALIVE:
        {
            Unit::Update(diff);
    
            // creature can be dead after Unit::Update call
            // CORPSE/DEAD state will processed at next tick (in other case death timer will be updated unexpectedly)
            if (!IsAlive())
                break;

            GetThreatManager().Update(diff);

            m_timeSinceSpawn += diff;

            DecreaseTimer(m_stealthAlertCooldown, diff);

            if (DecreaseTimer(m_movementFlagsUpdateTimer, diff))
            {
                UpdateMovementFlags();
                m_movementFlagsUpdateTimer = MOVEMENT_FLAGS_UPDATE_TIMER;
            }

            if(IsInCombat() && 
                (IsWorldBoss() || GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_INSTANCE_BIND) &&
                GetMap()->IsDungeon())
            {
                if(m_areaCombatTimer < diff)
                {
                    for (auto const& pair : GetCombatManager().GetPvECombatRefs())
                        if (Player* player = pair.second->GetOther(this)->ToPlayer())
                        {
                            AreaCombat();
                            break;
                        }
                  
                    m_areaCombatTimer = 5000;
                }else m_areaCombatTimer -= diff;
            }

            if (m_shouldReacquireTarget && !IsFocusing(nullptr, true))
            {
                SetTarget(m_suppressedTarget);

#ifdef LICH_KING
                if (!HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_DISABLE_TURN))
#endif
                {
                    if (m_suppressedTarget)
                    {
                        if (WorldObject const* objTarget = ObjectAccessor::GetWorldObject(*this, m_suppressedTarget))
                            SetFacingToObject(objTarget, false);
                    }
                    else
                        SetFacingTo(m_suppressedOrientation, false);
                }
                m_shouldReacquireTarget = false;
            }
            
            // periodic check to see if the creature has passed an evade boundary
            if (IsAIEnabled() && !IsInEvadeMode() && IsEngaged())
            {
                if (diff >= m_boundaryCheckTime)
                {
                    AI()->CheckInRoom();
                    m_boundaryCheckTime = 2500;
                }
                else
                    m_boundaryCheckTime -= diff;
            }

            // if periodic combat pulse is enabled and we are both in combat and in a dungeon, do this now
            if (m_combatPulseDelay > 0 && IsEngaged() && GetMap()->IsDungeon())
            {
                if (diff > m_combatPulseTime)
                    m_combatPulseTime = 0;
                else
                    m_combatPulseTime -= diff;

                if (m_combatPulseTime == 0)
                {
                    Map::PlayerList const& players = GetMap()->GetPlayers();
                    if (!players.isEmpty())
                        for (Map::PlayerList::const_iterator it = players.begin(); it != players.end(); ++it)
                        {
                            if (Player* player = it->GetSource())
                            {
                                if (player->IsGameMaster())
                                    continue;

                                if (player->IsAlive() && IsHostileTo(player))
                                    EngageWithTarget(player);
                            }
                        }

                    m_combatPulseTime = m_combatPulseDelay * IN_MILLISECONDS;
                }
            }

            Unit::AIUpdateTick(diff);

            HandleUnreachableTarget(diff);

            // creature can be dead after UpdateAI call
            // CORPSE/DEAD state will processed at next tick (in other case death timer will be updated unexpectedly)
            if(!IsAlive())
                break;

            if(!IsInCombat() && GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_PERIODIC_RELOC)
            {
                if(m_relocateTimer < diff)
                {
                     m_relocateTimer = 60000;
                     // forced recreate creature object at clients
                     DestroyForNearbyPlayers();
                     UpdateObjectVisibility();
                } else m_relocateTimer -= diff;
            }
            
            if ((GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_NO_HEALTH_RESET) == 0)
                if(m_regenTimer > 0)
                {
                    if(diff >= m_regenTimer)
                        m_regenTimer = 0;
                    else
                        m_regenTimer -= diff;
                }
            
            if (m_regenTimer == 0 && !m_disabledRegen)
            {
                if (!IsInCombat())
                {
                    if(HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TAPPED))
                        SetUInt32Value(UNIT_DYNAMIC_FLAGS, GetCreatureTemplate()->dynamicflags);
                        
                    RegenerateHealth();
                }
                else if(IsPolymorphed() || IsEvadingAttacks())
                    RegenerateHealth();

                Regenerate(POWER_MANA);

                m_regenTimer = CREATURE_REGEN_INTERVAL;
            }

            //remove keep active if a timer was defined
            if (m_keepActiveTimer != 0)
            {
                if (m_keepActiveTimer <= diff)
                {
                    SetKeepActive(false);
                    m_keepActiveTimer = 0;
                }
                else
                    m_keepActiveTimer -= diff;
            }
            
            break;
        }
        default:
            break;
    }
}

void Creature::Regenerate(Powers power)
{
    if(m_disabledRegen)
        return;

    uint32 curValue = GetPower(power);
    uint32 maxValue = GetMaxPower(power);

#ifdef LICH_KING
    if (!HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_REGENERATE_POWER))
        return;
#endif

    if (curValue >= maxValue)
        return;

    float addvalue = 0.0f;

    switch (power)
    {
        case POWER_FOCUS:
        {
            addvalue = 24 * sWorld->GetRate(RATE_POWER_FOCUS);

            auto const& ModPowerRegenPCTAuras = GetAuraEffectsByType(SPELL_AURA_MOD_POWER_REGEN_PERCENT);
            for (auto ModPowerRegenPCTAura : ModPowerRegenPCTAuras)
                if (ModPowerRegenPCTAura->GetMiscValue() == POWER_FOCUS)
                    addvalue *= (ModPowerRegenPCTAura->GetAmount() + 100) / 100.0f;
            break;
        }
        case POWER_ENERGY:
        {
            // For deathknight's ghoul.
            addvalue = 20;
            break;
        }
        case POWER_MANA:
        {
            // Combat and any controlled creature
            if (IsInCombat() || GetCharmerOrOwnerGUID())
            {
                if (!IsUnderLastManaUseEffect())
                {
                    float ManaIncreaseRate = sWorld->GetRate(RATE_POWER_MANA);
                    float Spirit = GetStat(STAT_SPIRIT);

                    addvalue = uint32((Spirit / 5.0f + 17.0f) * ManaIncreaseRate);
                }
            }
            else
                addvalue = maxValue / 3.0f;

            break;
        }
        default:
            return;
    }

    // Apply modifiers (if any).
    addvalue *= GetTotalAuraMultiplierByMiscValue(SPELL_AURA_MOD_POWER_REGEN_PERCENT, power);

    addvalue += float(GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_POWER_REGEN, power)) * (IsHunterPet() ? PET_FOCUS_REGEN_INTERVAL : CREATURE_REGEN_INTERVAL) / (5 * IN_MILLISECONDS);

    ModifyPower(power, addvalue);
}

void Creature::RegenerateHealth()
{
    if (!isRegeneratingHealth())
        return;

    uint32 curValue = GetHealth();
    uint32 maxValue = GetMaxHealth();

    if (curValue >= maxValue)
        return;

    uint32 addvalue = 0;

    // Not only pet, but any controlled creature (and not polymorphed)
    if(GetCharmerOrOwnerGUID() && !IsPolymorphed())
    {
        float HealthIncreaseRate = sWorld->GetRate(RATE_HEALTH);
        float Spirit = GetStat(STAT_SPIRIT);

        if( GetPower(POWER_MANA) > 0 )
            addvalue = uint32(Spirit * 0.25 * HealthIncreaseRate);
        else
            addvalue = uint32(Spirit * 0.80 * HealthIncreaseRate);
    }
    else
        addvalue = maxValue/3;

    ModifyHealth(addvalue);
}

bool Creature::AIM_Destroy()
{
    SetAI(nullptr);

    return true;
}

bool Creature::AIM_Create(CreatureAI* ai /*= nullptr*/)
{
    Motion_Initialize();

    SetAI(ai ? ai : FactorySelector::SelectAI(this));

    return true;
}

bool Creature::AIM_Initialize(CreatureAI* ai)
{
    if (!AIM_Create(ai))
        return false;

    if (IsAlive()) //sun: we load dead units so make sure we don't call reset hooks for them. That would cause all kinds of weird behaviors in scripts
        AI()->InitializeAI();
#ifdef LICH_KING
    if (GetVehicleKit())
        GetVehicleKit()->Reset();
#endif

    return true;
}

bool Creature::Create(ObjectGuid::LowType guidlow, Map *map, uint32 phaseMask, uint32 entry, Position const& pos, const CreatureData *data, bool dynamic)
{
    ASSERT(map);
    SetMap(map);
    SetPhaseMask(phaseMask, false);

    // Set if this creature can handle dynamic spawns
    if (!dynamic)
        SetRespawnCompatibilityMode();

    CreatureTemplate const* cinfo = sObjectMgr->GetCreatureTemplate(entry);
    if (!cinfo)
    {
        TC_LOG_ERROR("sql.sql", "Creature::Create(): creature template (guidlow: %u, entry: %u) does not exist.", guidlow, entry);
        return false;
    }

    //! Relocate before CreateFromProto, to initialize coords and allow
    //! returning correct zone id for selecting OutdoorPvP/Battlefield script
    Relocate(pos);

    // Check if the position is valid before calling CreateFromProto(), otherwise we might add Auras to Creatures at
    // invalid position, triggering a crash about Auras not removed in the destructor
    if (!IsPositionValid())
    {
        TC_LOG_ERROR("entities.unit", "Creature::Create(): given coordinates for creature (guidlow %d, entry %d) are not valid (X: %f, Y: %f, Z: %f, O: %f)", guidlow, entry, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), pos.GetOrientation());
        return false;
    }

    //oX = x;     oY = y;    dX = x;    dY = y;    m_moveTime = 0;    m_startMove = 0;
    if (!CreateFromProto(guidlow, entry, data))
        return false;

    UpdatePositionData();

    // Allow players to see those units while dead, do it here (may be altered by addon auras)
    if (cinfo->type_flags & CREATURE_TYPE_FLAG_GHOST_VISIBLE)
        m_serverSideVisibility.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_ALIVE | GHOST_VISIBILITY_GHOST);

    if (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_DUNGEON_BOSS && map->IsDungeon())
        m_respawnDelay = 0; // special value, prevents respawn for dungeon bosses unless overridden

    switch (GetCreatureTemplate()->rank)
    {
        case CREATURE_ELITE_RARE:
            m_corpseDelay = sWorld->getConfig(CONFIG_CORPSE_DECAY_RARE);
            break;
        case CREATURE_ELITE_ELITE:
            m_corpseDelay = sWorld->getConfig(CONFIG_CORPSE_DECAY_ELITE);
            break;
        case CREATURE_ELITE_RAREELITE:
            m_corpseDelay = sWorld->getConfig(CONFIG_CORPSE_DECAY_RAREELITE);
            break;
        case CREATURE_ELITE_WORLDBOSS:
            m_corpseDelay = sWorld->getConfig(CONFIG_CORPSE_DECAY_WORLDBOSS);
            break;
        default:
            m_corpseDelay = sWorld->getConfig(CONFIG_CORPSE_DECAY_NORMAL);
            break;
    }
    LoadCreatureAddon();
    InitCreatureAddon();

#ifdef LICH_KING
    m_positionZ += GetHoverOffset();
#endif

    //LastUsedScriptID = GetScriptId(); sunstrider: Moved to InitEntry

    if (IsSpiritHealer() || IsSpiritGuide() || (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_GHOST_VISIBILITY))
    {
        m_serverSideVisibility.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_GHOST);
        m_serverSideVisibilityDetect.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_GHOST);
    }

    /* TC
    if (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_IGNORE_PATHFINDING)
        AddUnitState(UNIT_STATE_IGNORE_PATHFINDING);


    if (GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_IMMUNITY_KNOCKBACK)
    {
        ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
        ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, true);
    }
    */

    GetThreatManager().Initialize();

    return true;
}

Unit* Creature::SelectVictim(bool evade /*= true*/)
{
    Unit* target = nullptr;

    if (CanHaveThreatList())
        target = GetThreatManager().GetCurrentVictim();
    else if (!HasReactState(REACT_PASSIVE))
    {
        // We're a player pet, probably
        target = GetAttackerForHelper();
        if (!target && IsSummon())
        {
            if (Unit* owner = ToTempSummon()->GetOwner())
            {
                if (owner->IsInCombat())
                    target = owner->GetAttackerForHelper();
                if (!target)
                {
                    for (ControlList::const_iterator itr = owner->m_Controlled.begin(); itr != owner->m_Controlled.end(); ++itr)
                    {
                        if ((*itr)->IsInCombat())
                        {
                            target = (*itr)->GetAttackerForHelper();
                            if (target)
                                break;
                        }
                    }
                }
            }
        }
    }
    else
        return nullptr;

    if (target && _IsTargetAcceptable(target) && _CanCreatureAttack(target) == CAN_ATTACK_RESULT_OK)
    {
        if (!IsFocusing(nullptr, true))
            SetInFront(target);
        return target;
    }

#ifdef LICH_KING
    /// @todo a vehicle may eat some mob, so mob should not evade
    if (GetVehicle())
        return nullptr;
#endif

    Unit::AuraEffectList const& iAuras = GetAuraEffectsByType(SPELL_AURA_MOD_INVISIBILITY);
    if (!iAuras.empty())
    {
        for (Unit::AuraEffectList::const_iterator itr = iAuras.begin(); itr != iAuras.end(); ++itr)
        {
            if ((*itr)->GetBase()->IsPermanent())
            {
                if(evade)
                    AI()->EnterEvadeMode(CreatureAI::EVADE_REASON_OTHER);
                break;
            }
        }
        return nullptr;
    }

    if (evade)
    {
        // enter in evade mode in other case
        AI()->EnterEvadeMode(CreatureAI::EVADE_REASON_NO_HOSTILES);
    }

    return nullptr;
}

void Creature::SetReactState(ReactStates st) 
{ 
    m_reactState = st; 
}

void Creature::InitializeReactState()
{
    if (IsTotem() || IsTrigger() || IsCritter() || IsSpiritService())
        SetReactState(REACT_PASSIVE);
    /*
    else if (IsCivilian())
    SetReactState(REACT_DEFENSIVE);
    */
    else
        SetReactState(REACT_AGGRESSIVE);
}

bool Creature::isTrainerFor(Player* pPlayer, bool msg) const
{
    if(!IsTrainer())
        return false;

    TrainerSpellData const* trainer_spells = GetTrainerSpells();

    if(!trainer_spells || trainer_spells->spellList.empty())
    {
        TC_LOG_ERROR("sql.sql","Creature %u (Entry: %u) have UNIT_NPC_FLAG_TRAINER but have empty trainer spell list.",
            GetGUID().GetCounter(),GetEntry());
        return false;
    }

    switch(GetCreatureTemplate()->trainer_type)
    {
        case TRAINER_TYPE_CLASS:
            if(pPlayer->GetClass()!=GetCreatureTemplate()->trainer_class)
            {
                if(msg)
                {
                    pPlayer->PlayerTalkClass->ClearMenus();
                    switch(GetCreatureTemplate()->trainer_class)
                    {
                        case CLASS_DRUID:  pPlayer->PlayerTalkClass->SendGossipMenuTextID( 4913,GetGUID()); break;
                        case CLASS_HUNTER: pPlayer->PlayerTalkClass->SendGossipMenuTextID(10090,GetGUID()); break;
                        case CLASS_MAGE:   pPlayer->PlayerTalkClass->SendGossipMenuTextID(  328,GetGUID()); break;
                        case CLASS_PALADIN:pPlayer->PlayerTalkClass->SendGossipMenuTextID( 1635,GetGUID()); break;
                        case CLASS_PRIEST: pPlayer->PlayerTalkClass->SendGossipMenuTextID( 4436,GetGUID()); break;
                        case CLASS_ROGUE:  pPlayer->PlayerTalkClass->SendGossipMenuTextID( 4797,GetGUID()); break;
                        case CLASS_SHAMAN: pPlayer->PlayerTalkClass->SendGossipMenuTextID( 5003,GetGUID()); break;
                        case CLASS_WARLOCK:pPlayer->PlayerTalkClass->SendGossipMenuTextID( 5836,GetGUID()); break;
                        case CLASS_WARRIOR:pPlayer->PlayerTalkClass->SendGossipMenuTextID( 4985,GetGUID()); break;
                    }
                }
                return false;
            }
            break;
        case TRAINER_TYPE_PETS:
            if(pPlayer->GetClass()!=CLASS_HUNTER)
            {
                pPlayer->PlayerTalkClass->ClearMenus();
                pPlayer->PlayerTalkClass->SendGossipMenuTextID(3620,GetGUID());
                return false;
            }
            break;
        case TRAINER_TYPE_MOUNTS:
            if(GetCreatureTemplate()->trainer_race && pPlayer->GetRace() != GetCreatureTemplate()->trainer_race)
            {
                if(msg)
                {
                    pPlayer->PlayerTalkClass->ClearMenus();
                    switch(GetCreatureTemplate()->trainer_race)
                    {
                        case RACE_DWARF:        pPlayer->PlayerTalkClass->SendGossipMenuTextID(5865,GetGUID()); break;
                        case RACE_GNOME:        pPlayer->PlayerTalkClass->SendGossipMenuTextID(4881,GetGUID()); break;
                        case RACE_HUMAN:        pPlayer->PlayerTalkClass->SendGossipMenuTextID(5861,GetGUID()); break;
                        case RACE_NIGHTELF:     pPlayer->PlayerTalkClass->SendGossipMenuTextID(5862,GetGUID()); break;
                        case RACE_ORC:          pPlayer->PlayerTalkClass->SendGossipMenuTextID(5863,GetGUID()); break;
                        case RACE_TAUREN:       pPlayer->PlayerTalkClass->SendGossipMenuTextID(5864,GetGUID()); break;
                        case RACE_TROLL:        pPlayer->PlayerTalkClass->SendGossipMenuTextID(5816,GetGUID()); break;
                        case RACE_UNDEAD_PLAYER:pPlayer->PlayerTalkClass->SendGossipMenuTextID( 624,GetGUID()); break;
                        case RACE_BLOODELF:     pPlayer->PlayerTalkClass->SendGossipMenuTextID(5862,GetGUID()); break;
                        case RACE_DRAENEI:      pPlayer->PlayerTalkClass->SendGossipMenuTextID(5864,GetGUID()); break;
                    }
                }
                return false;
            }
            break;
        case TRAINER_TYPE_TRADESKILLS:
            if(GetCreatureTemplate()->trainer_spell && !pPlayer->HasSpell(GetCreatureTemplate()->trainer_spell))
            {
                if(msg)
                {
                    pPlayer->PlayerTalkClass->ClearMenus();
                    pPlayer->PlayerTalkClass->SendGossipMenuTextID(11031,GetGUID());
                }
                return false;
            }
            break;
        default:
            return false;                                   // checked and error output at creature_template loading
    }
    return true;
}

bool Creature::isCanTrainingAndResetTalentsOf(Player* player) const
{
    return player->GetLevel() >= 10
        && GetCreatureTemplate()->trainer_type == TRAINER_TYPE_CLASS
        && player->GetClass() == GetCreatureTemplate()->trainer_class;
}

bool Creature::isCanInteractWithBattleMaster(Player* pPlayer, bool msg) const
{
    if(!IsBattleMaster())
        return false;

    BattlegroundTypeId bgTypeId = sObjectMgr->GetBattleMasterBG(GetEntry());
    if(!msg)
        return pPlayer->GetBGAccessByLevel(bgTypeId);

    if(!pPlayer->GetBGAccessByLevel(bgTypeId))
    {
        pPlayer->PlayerTalkClass->ClearMenus();
        switch(bgTypeId)
        {
            case BATTLEGROUND_AV:  pPlayer->PlayerTalkClass->SendGossipMenuTextID(7616,GetGUID()); break;
            case BATTLEGROUND_WS:  pPlayer->PlayerTalkClass->SendGossipMenuTextID(7599,GetGUID()); break;
            case BATTLEGROUND_AB:  pPlayer->PlayerTalkClass->SendGossipMenuTextID(7642,GetGUID()); break;
            case BATTLEGROUND_EY:
            case BATTLEGROUND_NA:
            case BATTLEGROUND_BE:
            case BATTLEGROUND_AA:
            case BATTLEGROUND_RL:  pPlayer->PlayerTalkClass->SendGossipMenuTextID(10024,GetGUID()); break;
            break;
        }
        return false;
    }
    return true;
}

bool Creature::canResetTalentsOf(Player* pPlayer) const
{
    return pPlayer->GetLevel() >= 10
        && GetCreatureTemplate()->trainer_type == TRAINER_TYPE_CLASS
        && pPlayer->GetClass() == GetCreatureTemplate()->trainer_class;
}

Player* Creature::GetLootRecipient() const
{
    if (!m_lootRecipient) 
        return nullptr;

    return ObjectAccessor::FindConnectedPlayer(m_lootRecipient);
}

Group* Creature::GetLootRecipientGroup() const
{
    if (!m_lootRecipient)
        return nullptr;

    return sGroupMgr->GetGroupByGUID(m_lootRecipientGroup);
}

// return true if this creature is tapped by the player or by a member of his group.
bool Creature::isTappedBy(Player const* player) const
{
    if (player->GetGUID() == m_lootRecipient)
        return true;

    Group const* playerGroup = player->GetGroup();
    if (!playerGroup || playerGroup != GetLootRecipientGroup()) // if we dont have a group we arent the recipient
        return false;                                           // if creature doesnt have group bound it means it was solo killed by someone else

    return true;
}

void Creature::SetLootRecipient(Unit* unit, bool withGroup)
{
    // set the player whose group should receive the right
    // to loot the creature after it dies
    // should be set to NULL after the loot disappears

    if (!unit)
    {
        m_lootRecipient.Clear();
        m_lootRecipientGroup = 0;
        RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE|UNIT_DYNFLAG_TAPPED);
        return;
    }

    Player* player = unit->GetCharmerOrOwnerPlayerOrPlayerItself();
    if(!player)                                             // normal creature, no player involved
        return;

    m_lootRecipient = player->GetGUID();
    if (withGroup)
    {
        if (Group* group = player->GetGroup())
            m_lootRecipientGroup = group->GetLowGUID();
    }
    else
        m_lootRecipientGroup = ObjectGuid::Empty;

    SetFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TAPPED);
}

void Creature::SaveToDB()
{
    // this should only be used when the creature has already been loaded
    // preferably after adding to map, because mapid may not be valid otherwise
    CreatureData const *data = sObjectMgr->GetCreatureData(m_spawnId);
    if(!data)
    {
        TC_LOG_ERROR("entities.unit","Creature::SaveToDB failed, cannot get creature data!");
        return;
    }

    SaveToDB(GetMapId(), data->spawnMask);
}

void Creature::SaveToDB(uint32 mapid, uint8 spawnMask)
{
    // update in loaded data
    if (!m_spawnId)
        m_spawnId = sObjectMgr->GenerateCreatureSpawnId();

    CreatureData& data = sObjectMgr->NewOrExistCreatureData(m_spawnId);

    uint32 displayId = GetNativeDisplayId();

    CreatureTemplate const *cinfo = GetCreatureTemplate();
    if(cinfo)
    {
        // check if it's a custom model and if not, use 0 for displayId
        if(displayId == cinfo->Modelid1 || displayId == cinfo->Modelid2 ||
            displayId == cinfo->Modelid3 || displayId == cinfo->Modelid4) displayId = 0;
    }

    data.displayid = displayId;
    if (!GetTransport())
        data.spawnPoint.WorldRelocate(this);
    else
        data.spawnPoint.WorldRelocate(mapid, GetTransOffsetX(), GetTransOffsetY(), GetTransOffsetZ(), GetTransOffsetO());

    data.spawntimesecs = m_respawnDelay;
    // prevent add data integrity problems
    data.spawndist = GetDefaultMovementType()==IDLE_MOTION_TYPE ? 0 : m_respawnradius;
    data.currentwaypoint = 0;
    data.curhealth = GetHealth();
    data.curmana = GetPower(POWER_MANA);
    // prevent add data integrity problems
    data.movementType = !m_respawnradius && GetDefaultMovementType()==RANDOM_MOTION_TYPE
        ? IDLE_MOTION_TYPE : GetDefaultMovementType();
    data.spawnMask = spawnMask;
    data.poolId = m_creaturePoolId;
    if (!data.spawnGroupData)
        data.spawnGroupData = sObjectMgr->GetDefaultSpawnGroup();

    // updated in DB
    //TODO: only save relevant fields? This seems dangerous for no benefit...
    SQLTransaction trans = WorldDatabase.BeginTransaction();

    PreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_CREATURE);
    stmt->setUInt32(0, m_spawnId);
    trans->Append(stmt);

    trans->PAppend("REPLACE INTO creature_entry (`spawnID`,`entry`) VALUES (%u,%u)", m_spawnId, GetEntry());

    std::ostringstream ss;
    ss << "INSERT INTO creature (spawnId,map,spawnMask,modelid,equipment_id,position_x,position_y,position_z,orientation,spawntimesecs,spawndist,currentwaypoint,curhealth,curmana,MovementType, pool_id, patch_min, patch_max) VALUES ("
        << m_spawnId << ","
        << mapid <<","
        << (uint32)spawnMask << ","
        << displayId <<","
        << GetCurrentEquipmentId() <<","
        << data.spawnPoint.GetPositionX() << ","
        << data.spawnPoint.GetPositionY() << ","
        << data.spawnPoint.GetPositionZ() << ","
        << data.spawnPoint.GetOrientation() << ","
        << m_respawnDelay << ","                            //respawn time
        << (float) m_respawnradius << ","                   //spawn distance (float)
        << (uint32) (0) << ","                              //currentwaypoint
        << GetHealth() << ","                               //curhealth
        << GetPower(POWER_MANA) << ","                      //curmana
        << GetDefaultMovementType() << ","                  //default movement generator type
        << m_creaturePoolId << ","                          //creature pool id
        << WOW_PATCH_MIN << ","                             //patch min
        << WOW_PATCH_MAX << ")";                            //patch max

    trans->Append( ss.str( ).c_str( ) );

    WorldDatabase.CommitTransaction(trans);
}

void Creature::UpdateLevelDependantStats()
{
    CreatureTemplate const* cInfo = GetCreatureTemplate();
    // uint32 rank = IsPet() ? 0 : cInfo->rank;
    CreatureBaseStats const* stats = sObjectMgr->GetCreatureBaseStats(GetLevel(), cInfo->unit_class);


    // health
    uint32 health = stats->GenerateHealth(cInfo);

    SetCreateHealth(health);
    SetMaxHealth(health);
    SetHealth(health);
    ResetPlayerDamageReq();

    // mana
    uint32 mana = stats->GenerateMana(cInfo);
    SetCreateMana(mana);

    switch (GetClass())
    {
        case UNIT_CLASS_PALADIN:
        case UNIT_CLASS_MAGE:
            SetMaxPower(POWER_MANA, mana);
            SetFullPower(POWER_MANA);
            break;
        default: // We don't set max power here, 0 makes power bar hidden
            break;
    }

    SetStatFlatModifier(UNIT_MOD_HEALTH, BASE_VALUE, (float)health);
    SetStatFlatModifier(UNIT_MOD_MANA, BASE_VALUE, (float)mana);

    // damage
    float basedamage = stats->GenerateBaseDamage(cInfo);

    float weaponBaseMinDamage = basedamage;
    float weaponBaseMaxDamage = basedamage * (1.0f + GetCreatureTemplate()->BaseVariance);

    SetBaseWeaponDamage(BASE_ATTACK, MINDAMAGE, weaponBaseMinDamage);
    SetBaseWeaponDamage(BASE_ATTACK, MAXDAMAGE, weaponBaseMaxDamage);

    SetBaseWeaponDamage(OFF_ATTACK, MINDAMAGE, weaponBaseMinDamage);
    SetBaseWeaponDamage(OFF_ATTACK, MAXDAMAGE, weaponBaseMaxDamage);

    SetBaseWeaponDamage(RANGED_ATTACK, MINDAMAGE, weaponBaseMinDamage);
    SetBaseWeaponDamage(RANGED_ATTACK, MAXDAMAGE, weaponBaseMaxDamage);

    SetStatFlatModifier(UNIT_MOD_ATTACK_POWER, BASE_VALUE, stats->AttackPower);
    SetStatFlatModifier(UNIT_MOD_ATTACK_POWER_RANGED, BASE_VALUE, stats->RangedAttackPower);

    float armor = (float)stats->GenerateArmor(cInfo); /// @todo Why is this treated as uint32 when it's a float?
    SetStatFlatModifier(UNIT_MOD_ARMOR, BASE_VALUE, armor);
}

void Creature::SelectLevel()
{
    const CreatureTemplate* cInfo = GetCreatureTemplate();
    if(!cInfo)
        return;

    // level
    uint32 minlevel = std::min(cInfo->maxlevel, cInfo->minlevel);
    uint32 maxlevel = std::max(cInfo->maxlevel, cInfo->minlevel);
    uint32 level = minlevel == maxlevel ? minlevel : urand(minlevel, maxlevel);
    SetLevel(level);

    UpdateLevelDependantStats();
}

bool Creature::CreateFromProto(ObjectGuid::LowType guidlow, uint32 entry, const CreatureData *data)
{
    SetZoneScript();
    if (GetZoneScript() && data)
    {
        entry = GetZoneScript()->GetCreatureEntry(guidlow, data, m_chosenTemplate);
        if (!entry)
            return false;
    }

    CreatureTemplate const* cinfo = sObjectMgr->GetCreatureTemplate(entry);
    if(!cinfo)
    {
        TC_LOG_ERROR("sql.sql","Error: creature entry %u does not exist.", entry);
        return false;
    }
    SetOriginalEntry(entry);

    Object::_Create(guidlow, entry, HighGuid::Unit);

    if(!UpdateEntry(entry, data))
        return false;

#ifdef LICH_KING
    if (!vehId)
    {
        if (GetCreatureTemplate()->VehicleId)
        {
            vehId = GetCreatureTemplate()->VehicleId;
            entry = GetCreatureTemplate()->Entry;
        }
        else
            vehId = cinfo->VehicleId;
    }

    if (vehId)
        CreateVehicleKit(vehId, entry);
#endif
    return true;
}

bool Creature::LoadFromDB(uint32 spawnId, Map *map, bool addToMap, bool allowDuplicate)
{
    if (!allowDuplicate)
    {
        // If an alive instance of this spawnId is already found, skip creation
        // If only dead instance(s) exist, despawn them and spawn a new (maybe also dead) version
        const auto creatureBounds = map->GetCreatureBySpawnIdStore().equal_range(spawnId);
        std::vector <Creature*> despawnList;

        if (creatureBounds.first != creatureBounds.second)
        {
            for (auto itr = creatureBounds.first; itr != creatureBounds.second; ++itr)
            {
                if (itr->second->IsAlive())
                {
                    TC_LOG_DEBUG("maps", "Would have spawned %u but it already exists", spawnId);
                    return false;
                }
                else
                {
                    if (!itr->second->GetRespawnCompatibilityMode())
                        continue; // It will be automatically removed with corpse decay
                    despawnList.push_back(itr->second);
                    TC_LOG_DEBUG("maps", "Despawned dead instance of spawn %u", spawnId);
                }
            }

            for (Creature* despawnCreature : despawnList)
            {
                despawnCreature->AddObjectToRemoveList();
            }
        }
    }

    CreatureData const* data = sObjectMgr->GetCreatureData(spawnId);

    if(!data)
    {
        TC_LOG_ERROR("sql.sql","Creature (SpawnID : %u) not found in table `creature`, can't load. ", spawnId);
        return false;
    }
    
    m_chosenTemplate = data->ChooseSpawnEntry();
    // Rare creatures in dungeons have 15% chance to spawn
    CreatureTemplate const *cinfo = sObjectMgr->GetCreatureTemplate(m_chosenTemplate);
    if (cinfo && map->GetInstanceId() != 0 && (cinfo->rank == CREATURE_ELITE_RAREELITE || cinfo->rank == CREATURE_ELITE_RARE)) {
        if (rand()%5 != 0)
            return false;
    }

    m_spawnId = spawnId;

    m_respawnCompatibilityMode = ((data->spawnGroupData->flags & SPAWNGROUP_FLAG_COMPATIBILITY_MODE) != 0);
    m_creatureData = data;
    m_respawnradius = data->spawndist;
    m_respawnDelay = data->spawntimesecs;

    // Is the creature script objecting to us spawning? If yes, delay by a little bit (then re-check in ::Update)
    if (!m_respawnTime && !map->IsSpawnGroupActive(data->spawnGroupData->groupId))
    {
        ASSERT(m_respawnCompatibilityMode, "Creature (SpawnID %u) trying to load in inactive spawn group %s.", spawnId, data->spawnGroupData->name.c_str());
        m_respawnTime = GetMap()->GetGameTime() + urand(4, 7);
    }

    if(!Create(map->GenerateLowGuid<HighGuid::Unit>(), map, PHASEMASK_NORMAL /*data->phaseMask*/, m_chosenTemplate, data->spawnPoint, data, !m_respawnCompatibilityMode))
        return false;

    if(!IsPositionValid())
    {
        TC_LOG_ERROR("sql.sql","ERROR: Creature (guidlow %d, entry %d) not loaded. Suggested coordinates isn't valid (X: %f Y: %f)",GetGUID().GetCounter(),GetEntry(),GetPositionX(),GetPositionY());
        return false;
    }
    //We should set first home position, because then AI calls home movement
    SetHomePosition(*this);

    m_deathState = ALIVE;

    m_respawnTime = GetMap()->GetCreatureRespawnTime(m_spawnId);
    if (sPoolMgr->IsPartOfAPool<Creature>(spawnId)) //sun: just make sure we never spawn dead from pools
        m_respawnTime = 0;

    if (!m_respawnTime && !map->IsSpawnGroupActive(data->spawnGroupData->groupId))
    {
        // @todo pools need fixing! this is just a temporary crashfix, but they violate dynspawn principles
        ASSERT(m_respawnCompatibilityMode || sPoolMgr->IsPartOfAPool<Creature>(spawnId), "Creature (SpawnID %u) trying to load in inactive spawn group %s.", spawnId, data->spawnGroupData->name.c_str());
        m_respawnTime = GetMap()->GetGameTime() + urand(4, 7);
    }

    if (m_respawnTime)                          // respawn on Update
    {
        //Sun: removed this check because of a change in LoadHelper
        //ASSERT(m_respawnCompatibilityMode || sPoolMgr->IsPartOfAPool<Creature>(spawnId), "Creature (SpawnID %u) trying to load despite a respawn timer in progress.", spawnId);
        if (!m_respawnCompatibilityMode)
            m_deathState = CORPSE; //DEAD is used for respawn handling in compat mode
        else
            m_deathState = DEAD;

        if (CanFly())
        {
            float tz = map->GetHeight(GetPhaseMask(), data->spawnPoint, true, MAX_FALL_DISTANCE);
            if (data->spawnPoint.GetPositionZ() - tz > 0.1f && Trinity::IsValidMapCoord(tz))
                Relocate(data->spawnPoint.GetPositionX(), data->spawnPoint.GetPositionY(), tz);
        }
    }

    SetSpawnHealth();

    // checked at creature_template loading
    m_defaultMovementType = MovementGeneratorType(data->movementType);

    if (addToMap && !GetMap()->AddToMap(this))
        return false;

    return true;
}

void Creature::LoadEquipment(int8 id, bool force)
{
    if(id == 0)
    {
        if (force)
        {
            for (uint8 i = WEAPON_SLOT_MAINHAND; i <= WEAPON_SLOT_RANGED; i++)
            {
                SetUInt32Value( UNIT_VIRTUAL_ITEM_SLOT_DISPLAY + i, 0);
                SetUInt32Value( UNIT_VIRTUAL_ITEM_INFO + (i * 2), 0);
                SetUInt32Value( UNIT_VIRTUAL_ITEM_INFO + (i * 2) + 1, 0);
            }
            m_equipmentId = 0;
        }
        return;
    }

    EquipmentInfo const *einfo = sObjectMgr->GetEquipmentInfo(GetEntry(), id);
    if (!einfo)
        return;

    m_equipmentId = id;
    for (uint8 i = WEAPON_SLOT_MAINHAND; i <= WEAPON_SLOT_RANGED; i++)
    {
        SetUInt32Value( UNIT_VIRTUAL_ITEM_SLOT_DISPLAY + i, einfo->equipmodel[i]);
        SetUInt32Value( UNIT_VIRTUAL_ITEM_INFO + (i * 2), einfo->equipinfo[i]);
        SetUInt32Value( UNIT_VIRTUAL_ITEM_INFO + (i * 2) + 1, einfo->equipslot[i]);
    }
}

void Creature::SetSpawnHealth()
{
    uint32 curhealth;
    if (m_creatureData && !m_regenHealth)
    {
        curhealth = m_creatureData->curhealth;
        /*
        if (curhealth)
        {
            curhealth = uint32(curhealth * _GetHealthMod(GetCreatureTemplate()->rank));
            if (curhealth < 1)
                curhealth = 1;
        }*/
        SetPower(POWER_MANA, m_creatureData->curmana);
    }
    else
    {
        curhealth = GetMaxHealth();
        SetPower(POWER_MANA, GetMaxPower(POWER_MANA));
    }

    SetHealth((m_deathState == ALIVE || m_deathState == JUST_RESPAWNED) ? curhealth : 0);
}

void Creature::SetWeapon(WeaponSlot slot, uint32 displayid, ItemSubclassWeapon subclass, InventoryType inventoryType)
{
    SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY+slot, displayid);
    SetUInt32Value(UNIT_VIRTUAL_ITEM_INFO + slot*2, ITEM_CLASS_WEAPON + (subclass << 8));
    SetUInt32Value(UNIT_VIRTUAL_ITEM_INFO + (slot*2)+1, inventoryType);
}

ItemSubclassWeapon Creature::GetWeaponSubclass(WeaponSlot slot)
{
    uint32 itemInfo = GetUInt32Value(UNIT_VIRTUAL_ITEM_INFO + slot * 2);
    ItemSubclassWeapon subclass = ItemSubclassWeapon((itemInfo & 0xFF00) >> 8);
    if (subclass > ITEM_SUBCLASS_WEAPON_FISHING_POLE)
        TC_LOG_ERROR("entities.creature", "Creature (table guid %u) appears to have broken weapon info for slot %u", GetSpawnId(), uint32(slot));

    return subclass;
}

bool Creature::HasMainWeapon() const
{
#ifdef LICH_KING
    return GetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID);
#else
    return GetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_DISPLAY);
#endif
}

bool Creature::HasQuest(uint32 quest_id) const
{
    QuestRelations const& qr = sObjectMgr->mCreatureQuestRelations;
    for(auto itr = qr.lower_bound(GetEntry()); itr != qr.upper_bound(GetEntry()); ++itr)
    {
        if(itr->second==quest_id)
            return true;
    }
    return false;
}

bool Creature::HasInvolvedQuest(uint32 quest_id) const
{
    QuestRelations const& qr = sObjectMgr->mCreatureQuestInvolvedRelations;
    for(auto itr = qr.lower_bound(GetEntry()); itr != qr.upper_bound(GetEntry()); ++itr)
    {
        if(itr->second==quest_id)
            return true;
    }
    return false;
}

void Creature::DeleteFromDB()
{
    if (!m_spawnId)
    {
        TC_LOG_ERROR("entities.unit", "Trying to delete not saved creature! LowGUID: %u, Entry: %u", GetGUID().GetCounter(), GetEntry());
        return;
    }

    // remove any scheduled respawns
    GetMap()->RemoveRespawnTime(SPAWN_TYPE_CREATURE, m_spawnId);

    sObjectMgr->DeleteCreatureData(m_spawnId);

    SQLTransaction trans = WorldDatabase.BeginTransaction();

    PreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_CREATURE);
    stmt->setUInt32(0, m_spawnId);
    trans->Append(stmt);

    stmt = WorldDatabase.GetPreparedStatement(WORLD_DEL_SPAWNGROUP_MEMBER);
    stmt->setUInt8(0, uint8(SPAWN_TYPE_CREATURE));
    stmt->setUInt32(1, m_spawnId);
    trans->Append(stmt);

    trans->PAppend("DELETE FROM creature_addon WHERE spawnID = '%u'", m_spawnId);
    trans->PAppend("DELETE FROM creature_entry WHERE spawnID = '%u'", m_spawnId);
    trans->PAppend("DELETE FROM game_event_creature WHERE guid = '%u'", m_spawnId);
    trans->PAppend("DELETE FROM game_event_model_equip WHERE guid = '%u'", m_spawnId);

    WorldDatabase.CommitTransaction(trans);
}

bool Creature::IsWithinSightDist(Unit const* u) const
{
    return IsWithinDistInMap(u, sWorld->getConfig(CONFIG_SIGHT_MONSTER));
}

/**
Hostile target is in stealth and in warn range
*/
void Creature::StartStealthAlert(Unit const* target)
{
    m_stealthAlertCooldown = STEALTH_ALERT_COOLDOWN;

    GetMotionMaster()->MoveStealthAlert(target, STEALTH_ALERT_DURATION);
    SendAIReaction(AI_REACTION_ALERT);
}

bool Creature::CanDoStealthAlert(Unit const* target) const
{
    if(   IsInCombat()
       || IsWorldBoss()
       || IsTotem()
      )
        return false;

    // cooldown not ready
    if(m_stealthAlertCooldown > 0)
        return false;

    // If this unit isn't an NPC, is already distracted, is in combat, is confused, stunned or fleeing, do nothing
    if (HasUnitState(UNIT_STATE_CONFUSED | UNIT_STATE_STUNNED | UNIT_STATE_FLEEING | UNIT_STATE_DISTRACTED)
        || IsCivilian() || HasReactState(REACT_PASSIVE) || !IsHostileTo(target)
       )
        return false;

    //target is not a player
    if(target->GetTypeId() != TYPEID_PLAYER)
        return false;

    if (!target->HasStealthAura())
        return false;

    if(!CanSeeOrDetect(target, false, true, true))
        return false;

    //only with those movement generators, we don't want to start warning when fleeing, chasing, ...
    MovementGeneratorType currentMovementType = GetMotionMaster()->GetCurrentMovementGeneratorType();
    if(currentMovementType != IDLE_MOTION_TYPE
       && currentMovementType != RANDOM_MOTION_TYPE
       && currentMovementType != WAYPOINT_MOTION_TYPE)
       return false;

    return true;
}

bool Creature::IsInvisibleDueToDespawn() const
{
    if (Unit::IsInvisibleDueToDespawn())
        return true;

    if (IsAlive() || IsDying() || m_corpseRemoveTime > GetMap()->GetGameTime())
        return false;

    return true;
}

bool Creature::CanAlwaysSee(WorldObject const* obj) const
{
    if (IsAIEnabled() && AI()->CanSeeAlways(obj))
        return true;

    return false;
}

CanAttackResult Creature::CanAggro(Unit const* who, bool force /* = false */) const
{
    if(IsCivilian())
        return CAN_ATTACK_RESULT_CIVILIAN;

    // Do not attack non-combat pets
    if (who->GetTypeId() == TYPEID_UNIT && who->GetCreatureType() == CREATURE_TYPE_NON_COMBAT_PET)
        return CAN_ATTACK_RESULT_OTHERS;

    float chaseRange = m_ChaseRange ? m_ChaseRange->MaxRange : 0.0f;
    if(!CanFly() && GetDistanceZ(who) > CREATURE_Z_ATTACK_RANGE + chaseRange)
        return CAN_ATTACK_RESULT_TOO_FAR_Z;

    if(force)
    {
        if(!IsWithinSightDist(who))
            return CAN_ATTACK_RESULT_TOO_FAR;
    } else {
        if (!_IsTargetAcceptable(who))
            return CAN_ATTACK_RESULT_OTHERS;

        if(!IsWithinDistInMap(who, GetAggroRange(who) + GetCombatReach() + who->GetCombatReach()))
            return CAN_ATTACK_RESULT_TOO_FAR;

        //ignore LoS for assist
        if (!IsWithinLOSInMap(who))
            return CAN_ATTACK_RESULT_NOT_IN_LOS;
    }

    CanAttackResult result = _CanCreatureAttack(who, false);
    if(result != CAN_ATTACK_RESULT_OK)
        return result;

    if (!who->isInAccessiblePlaceFor(this))
        return CAN_ATTACK_RESULT_NOT_ACCESSIBLE;

    return CAN_ATTACK_RESULT_OK;
}

// force will use IsFriendlyTo instead of IsHostileTo, so that neutral creatures can also attack players
// force also ignore feign death
CanAttackResult Creature::_CanCreatureAttack(Unit const* target, bool force /*= true*/) const
{
    ASSERT(target);

    if (!target->IsInMap(this))
        return CAN_ATTACK_RESULT_OTHER_MAP;

    if (!IsValidAttackTarget(target))
        return CAN_ATTACK_RESULT_OTHERS;

    if (IsAIEnabled() && !AI()->CanAIAttack(target))
        return CAN_ATTACK_RESULT_OTHERS;

    if (target->GetTypeId() == TYPEID_PLAYER && ((target->ToPlayer())->IsGameMaster() || (target->ToPlayer())->isSpectator()))
        return CAN_ATTACK_RESULT_OTHERS;

    if(target->GetTypeId() == TYPEID_UNIT && target->GetEntry() == 10 && GetTypeId() != TYPEID_PLAYER && !GetCharmerOrOwnerGUID().IsPlayer()) //training dummies
        return CAN_ATTACK_RESULT_OTHERS;

    // we cannot attack in evade mode
    if (IsInEvadeMode())
        return CAN_ATTACK_RESULT_SELF_EVADE;

    // or if enemy is in evade mode
    if (target->GetTypeId() == TYPEID_UNIT && target->ToCreature()->IsInEvadeMode())
        return CAN_ATTACK_RESULT_TARGET_EVADE;

    // feign death case
    if (!force && target->HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH) && !target->IsFeighDeathDetected(this))
    {
        if ((GetTypeId() != TYPEID_PLAYER && !GetOwner()) || (GetOwner() && GetOwner()->GetTypeId() != TYPEID_PLAYER))
            return CAN_ATTACK_RESULT_FEIGN_DEATH;
        // if this == player or owner == player check other conditions
    }
    else if (target->GetTransformSpell() == FORM_SPIRITOFREDEMPTION)
        return CAN_ATTACK_RESULT_OTHERS;

    //Sathrovarr the Corruptor HACK
    if (target->GetEntry() == 24892 && IsPet())
        return CAN_ATTACK_RESULT_OTHERS;

    //changed logic from TC... it does not handle a handful of case
    bool outOfRange = IsOutOfThreatArea(target);
    if (outOfRange)
        return CAN_ATTACK_RESULT_TOO_FAR;

    return CAN_ATTACK_RESULT_OK;
}

// Used to determined if a creature already in combat is still a valid victim
bool Creature::IsOutOfThreatArea(Unit const* target) const
{
    if (GetCharmerOrOwnerGUID().IsPlayer())
        return false;

    if(!GetAI() || GetAI()->IsCombatMovementAllowed()) // only check accessibility for creatures allowed to move
        if (!target->isInAccessiblePlaceFor(this))
            return true;

    if (GetMap()->IsDungeon())
        return false;

    // don't check distance to home position if recently damaged, this should include taunt auras
    if (!IsWorldBoss() && (GetLastDamagedTime() > GetMap()->GetGameTime() || HasAuraType(SPELL_AURA_MOD_TAUNT)))
        return false;

    float dist;
    if (IsHomeless() || IsBeingEscorted())
        dist = target->GetDistance(GetPositionX(), GetPositionY(), GetPositionZ());
    else {
        float x, y, z, o;
        GetHomePosition(x, y, z, o);
        dist = target->GetDistance(x, y, z);
    }
    // include sizes for huge npcs
    dist += GetCombatReach() + target->GetCombatReach();

    // Cap at map visibility range
    dist = std::min<float>(dist, GetMap()->GetVisibilityRange());
    // and no more than 2 * cell size
    dist = std::min<float>(dist, SIZE_OF_GRID_CELL * 2);

    float threatRadius = sWorld->getConfig(CONFIG_THREAT_RADIUS);
    threatRadius = std::max(threatRadius, GetAggroRange(target)); //Use aggro range in distance check if threat radius is lower. This prevents creature bounce in and out of combat every update tick.

    return dist > threatRadius;
}

float Creature::GetAggroRange(Unit const* pl) const
{
    float aggroRate = sWorld->GetRate(RATE_CREATURE_AGGRO);
    if(aggroRate==0)
        return 0.0f;

    int32 playerlevel   = pl->GetLevelForTarget(this);
    int32 creaturelevel = GetLevelForTarget(pl);

    int32 leveldif       = playerlevel - creaturelevel;

    // "The maximum Aggro Radius has a cap of 25 levels under. Example: A level 30 char has the same Aggro Radius of a level 5 char on a level 60 mob."
    if (leveldif < -25)
        leveldif = -25;

    // "The aggro radius of a mob having the same level as the player is roughly 20 yards"
    float aggroRadius = 20;

    // "Aggro Radius varies with level difference at a rate of roughly 1 yard/level"
    // radius grow if playlevel < creaturelevel
    aggroRadius -= (float)leveldif;

    // detect range auras
    aggroRadius += GetTotalAuraModifier(SPELL_AURA_MOD_DETECT_RANGE);

    // detected range auras
    aggroRadius += pl->GetTotalAuraModifier(SPELL_AURA_MOD_DETECTED_RANGE);

    // Just in case, we don't want pets running all over the map
    if (aggroRadius > MAX_AGGRO_RADIUS)
        aggroRadius = MAX_AGGRO_RADIUS;

    //  Minimum Aggro Radius for a mob seems to be combat range (5 yards)
    if(aggroRadius < NOMINAL_MELEE_RANGE)
        aggroRadius = NOMINAL_MELEE_RANGE;

    return (aggroRadius * aggroRate);
}

void Creature::SetDeathState(DeathState s)
{
    Unit::SetDeathState(s);
    if (s == JUST_DIED)
    {
        m_corpseRemoveTime = GetMap()->GetGameTime() + m_corpseDelay;

        uint32 respawnDelay = m_respawnDelay;
        if (uint32 scalingMode = sWorld->getIntConfig(CONFIG_RESPAWN_DYNAMICMODE))
            GetMap()->ApplyDynamicModeRespawnScaling(this, m_spawnId, respawnDelay, scalingMode);
        // @todo remove the boss respawn time hack in a dynspawn follow-up once we have creature groups in instances
        if (m_respawnCompatibilityMode)
        {
            if (IsDungeonBoss() && !m_respawnDelay)
                m_respawnTime = std::numeric_limits<time_t>::max(); // never respawn in this instance
            else
                m_respawnTime = GetMap()->GetGameTime() + respawnDelay + m_corpseDelay;
        }
        else
        {
            if (IsDungeonBoss() && !m_respawnDelay)
                m_respawnTime = std::numeric_limits<time_t>::max(); // never respawn in this instance
            else
                m_respawnTime = GetMap()->GetGameTime() + respawnDelay;
        }

        // always save boss respawn time at death to prevent crash cheating
        if (sWorld->getBoolConfig(CONFIG_SAVE_RESPAWN_TIME_IMMEDIATELY) || IsWorldBoss())
            SaveRespawnTime();
        else if (!m_respawnCompatibilityMode)
            SaveRespawnTime(0, false);

        //sun: notify the PoolMgr of our death, so that it may already consider this creature as removed
        uint32 poolid = GetSpawnId() ? sPoolMgr->IsPartOfAPool<Creature>(GetSpawnId()) : 0;
        if (poolid)
            sPoolMgr->RemoveActiveObject<Creature>(poolid, GetSpawnId());

        Map *map = FindMap();
        if(map && map->IsDungeon() && ((InstanceMap*)map)->GetInstanceScript())
            ((InstanceMap*)map)->GetInstanceScript()->OnCreatureDeath(this);

        ReleaseFocus(nullptr); // remove spellcast focus
        DoNotReacquireTarget(); // cancel delayed re-target
        SetTarget(ObjectGuid::Empty); // drop target - dead mobs shouldn't ever target things

        SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);

        SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, 0);       // if creature is mounted on a virtual mount, remove it at death

        SetKeepActive(false);

        //Dismiss group if is leader
        if (m_formation && m_formation->GetLeader() == this)
            m_formation->FormationReset(true);

        bool needsFalling = ((CanFly() || IsFlying() || IsHovering()) && !HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING));
        SetHover(false);
        if (needsFalling)
            GetMotionMaster()->MoveFall();

        Unit::SetDeathState(CORPSE);
    }
    if(s == JUST_RESPAWNED)
    {
        if (IsPet())
            SetFullHealth();
        else
            SetSpawnHealth();

        SetLootRecipient(nullptr);
        ResetPlayerDamageReq();

        SetCannotReachTarget(false);
        UpdateMovementFlags();

        ClearUnitState(UNIT_STATE_ALL_ERASABLE);

        if (!IsPet())
        {
            RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);

            CreatureData const* creatureData = GetCreatureData();
            CreatureTemplate const* cinfo = GetCreatureTemplate();

            uint32 npcflag, unit_flags, dynamicflags;
            ObjectMgr::ChooseCreatureFlags(cinfo, npcflag, unit_flags, dynamicflags, creatureData);

            SetUInt32Value(UNIT_NPC_FLAGS, npcflag);
            SetUInt32Value(UNIT_FIELD_FLAGS, unit_flags);
            SetUInt32Value(UNIT_DYNAMIC_FLAGS, dynamicflags);

            AddUnitMovementFlag(MOVEMENTFLAG_WALKING);

            SetMeleeDamageSchool(SpellSchools(GetCreatureTemplate()->dmgschool));
        }

        Motion_Initialize();
        Unit::SetDeathState(ALIVE);
        InitCreatureAddon(true);
    }
}

void Creature::SetRespawnTime(uint32 respawn)
{ 
    m_respawnTime = respawn ? GetMap()->GetGameTime() + respawn : 0;
}

void Creature::Respawn(bool force /* = false */)
{
    if (force)
    {
        if (IsAlive())
            SetDeathState(JUST_DIED);
        else if (GetDeathState() != CORPSE)
            SetDeathState(CORPSE);
    }
    if (m_respawnCompatibilityMode)
    {
        DestroyForNearbyPlayers();
        RemoveCorpse(false, false);

        if (!IsInWorld())
            AddToWorld();

        if (GetDeathState() == DEAD)
        {
            if (m_spawnId)
                GetMap()->RemoveRespawnTime(SPAWN_TYPE_CREATURE, m_spawnId);

            TC_LOG_DEBUG("entities.creature", "Respawning...");
            m_respawnTime = 0;
            ResetPickPocketRefillTimer();
            loot.clear();

            if (m_originalEntry != GetEntry())
                UpdateEntry(m_originalEntry);

            SelectLevel();

            SetDeathState(JUST_RESPAWNED);

            GetMotionMaster()->InitializeDefault();

            //Re-initialize reactstate that could be altered by movementgenerators
            InitializeReactState();

            if (UnitAI* ai = AI()) // reset the AI to be sure no dirty or uninitialized values will be used till next tick
                ai->Reset();

            //re rand level & model
            SelectLevel();

            uint32 displayID = GetNativeDisplayId();
            CreatureModelInfo const* minfo = sObjectMgr->GetCreatureModelRandomGender(displayID);
            if (minfo && !IsTotem())                               // Cancel load if no model defined or if totem
            {
                SetNativeDisplayId(displayID);

                Unit::AuraEffectList const& transformAuras = GetAuraEffectsByType(SPELL_AURA_TRANSFORM);
                Unit::AuraEffectList const& shapeshiftAuras = GetAuraEffectsByType(SPELL_AURA_MOD_SHAPESHIFT);
                if (transformAuras.empty() && shapeshiftAuras.empty())
                    SetDisplayId(displayID);
            }

            uint32 poolid = GetSpawnId() ? sPoolMgr->IsPartOfAPool<Creature>(GetSpawnId()) : 0;
            if (poolid)
                sPoolMgr->UpdatePool<Creature>(poolid, GetSpawnId());

            //Call AI respawn virtual function
            if (IsAIEnabled())
                m_triggerJustAppeared = true;//delay event to next tick so all creatures are created on the map before processing
        }
        m_timeSinceSpawn = 0;

        UpdateObjectVisibility();
    }
    else
    {
        if (m_spawnId)
            GetMap()->RemoveRespawnTime(SPAWN_TYPE_CREATURE, m_spawnId, true);
        
        //Map will handle the respawn
    }

    TC_LOG_DEBUG("entities.unit", "Respawning creature %s (%u)",
        GetName().c_str(), GetGUID().GetCounter());
}

void Creature::DespawnOrUnsummon(uint32 msTimeToDespawn /*= 0*/, Seconds forceRespawnTimer /*= 0*/)
{
    if (TempSummon* summon = this->ToTempSummon())
        summon->UnSummon(msTimeToDespawn); 
    else
        ForcedDespawn(msTimeToDespawn, forceRespawnTimer);
}

void Creature::ForcedDespawn(uint32 timeMSToDespawn, Seconds forceRespawnTimer)
{
    if (timeMSToDespawn)
    {
        auto pEvent = new ForcedDespawnDelayEvent(*this, forceRespawnTimer);
        m_Events.AddEvent(pEvent, m_Events.CalculateTime(timeMSToDespawn));
        return;
    }

    if (m_respawnCompatibilityMode)
    {
        uint32 corpseDelay = GetCorpseDelay();
        uint32 respawnDelay = GetRespawnDelay();

        // do it before killing creature
        DestroyForNearbyPlayers();

        bool overrideRespawnTime = false;
        if (IsAlive())
        {
            if (forceRespawnTimer > Seconds::zero())
            {
                SetCorpseDelay(0);
                SetRespawnDelay(forceRespawnTimer.count());
                overrideRespawnTime = true;
            }

            SetDeathState(JUST_DIED);
        }

        // Skip corpse decay time
        RemoveCorpse(!overrideRespawnTime, false);

        SetCorpseDelay(corpseDelay);
        SetRespawnDelay(respawnDelay);
    }
    else 
    {
        if (forceRespawnTimer > Seconds::zero())
            SaveRespawnTime(forceRespawnTimer.count());
        else
        {
            uint32 respawnDelay = m_respawnDelay;
            if (uint32 scalingMode = sWorld->getIntConfig(CONFIG_RESPAWN_DYNAMICMODE))
                GetMap()->ApplyDynamicModeRespawnScaling(this, m_spawnId, respawnDelay, scalingMode);
            m_respawnTime = GetMap()->GetGameTime() + respawnDelay;
            SaveRespawnTime();
        }

        AddObjectToRemoveList();
    }
}

void Creature::LoadTemplateImmunities()
{
    // uint32 max used for "spell id", the immunity system will not perform SpellInfo checks against invalid spells
    // used so we know which immunities were loaded from template
    static uint32 const placeholderSpellId = std::numeric_limits<uint32>::max();

    // unapply template immunities (in case we're updating entry)
    for (uint32 i = MECHANIC_NONE + 1; i < MAX_MECHANIC; ++i)
        ApplySpellImmune(placeholderSpellId, IMMUNITY_MECHANIC, i, false);

    for (uint32 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
        ApplySpellImmune(placeholderSpellId, IMMUNITY_SCHOOL, 1 << i, false);

    // don't inherit immunities for hunter pets
    if (GetOwnerGUID().IsPlayer() && IsHunterPet())
        return;

    if (uint32 mask = GetCreatureTemplate()->MechanicImmuneMask)
    {
        for (uint32 i = MECHANIC_NONE + 1; i < MAX_MECHANIC; ++i)
        {
            if (mask & (1 << (i - 1)))
                ApplySpellImmune(placeholderSpellId, IMMUNITY_MECHANIC, i, true);
        }
    }

    if (uint32 mask = GetCreatureTemplate()->SpellSchoolImmuneMask)
    {
        for (uint8 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
        {
            if (mask & (1 << i))
                ApplySpellImmune(placeholderSpellId, IMMUNITY_SCHOOL, 1 << i, true);
        }
    }
}

bool Creature::IsImmunedToSpell(SpellInfo const* spellInfo, WorldObject const* caster) const
{
    if (!spellInfo)
        return false;

    bool immunedToAllEffects = true;
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (spellInfo->Effects[i].IsEffect() && !IsImmunedToSpellEffect(spellInfo, i, caster))
        {
            immunedToAllEffects = false;
            break;
        }
    }

    if (immunedToAllEffects)
        return true;

    return Unit::IsImmunedToSpell(spellInfo, caster);
}

bool Creature::IsImmunedToSpellEffect(SpellInfo const* spellInfo, uint32 index, WorldObject const* caster) const
{
    if (GetCreatureTemplate()->type == CREATURE_TYPE_MECHANICAL && spellInfo->Effects[index].Effect == SPELL_EFFECT_HEAL)
        return true;

    return Unit::IsImmunedToSpellEffect(spellInfo, index, caster);
}

SpellInfo const *Creature::reachWithSpellAttack(Unit *pVictim)
{
    if(!pVictim)
        return nullptr;

    for(uint32 m_spell : m_spells)
    {
        if(!m_spell)
            continue;
        SpellInfo const *spellInfo = sSpellMgr->GetSpellInfo(m_spell );
        if(!spellInfo)
        {
            TC_LOG_ERROR("FIXME","WORLD: unknown spell id %i\n", m_spell);
            continue;
        }

        bool bcontinue = true;
        for(const auto & Effect : spellInfo->Effects)
        {
            if( (Effect.Effect == SPELL_EFFECT_SCHOOL_DAMAGE )       ||
                (Effect.Effect == SPELL_EFFECT_INSTAKILL)            ||
                (Effect.Effect == SPELL_EFFECT_ENVIRONMENTAL_DAMAGE) ||
                (Effect.Effect == SPELL_EFFECT_HEALTH_LEECH )
                )
            {
                bcontinue = false;
                break;
            }
        }
        if(bcontinue) continue;

        if(spellInfo->ManaCost > GetPower(POWER_MANA))
            continue;
        float range = spellInfo->GetMaxRange(false, this);
        float minrange = spellInfo->GetMinRange(false);
        float dist = GetDistance(pVictim);
        //if(!isInFront( pVictim, range ) && spellInfo->AttributesEx )
        //    continue;
        if( dist > range || dist < minrange )
            continue;
        if(HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED))
            continue;
        return spellInfo;
    }
    return nullptr;
}

SpellInfo const *Creature::reachWithSpellCure(Unit *pVictim)
{
    if(!pVictim)
        return nullptr;

    for(uint32 m_spell : m_spells)
    {
        if(!m_spell)
            continue;
        SpellInfo const *spellInfo = sSpellMgr->GetSpellInfo(m_spell );
        if(!spellInfo)
        {
            TC_LOG_ERROR("FIXME","WORLD: unknown spell id %i\n", m_spell);
            continue;
        }

        bool bcontinue = true;
        for(const auto & Effect : spellInfo->Effects)
        {
            if( (Effect.Effect == SPELL_EFFECT_HEAL ) )
            {
                bcontinue = false;
                break;
            }
        }
        if(bcontinue) continue;

        if(spellInfo->ManaCost > GetPower(POWER_MANA))
            continue;
        float range = spellInfo->GetMaxRange(true, this);
        float minrange = spellInfo->GetMinRange();
        float dist = GetDistance(pVictim);
        //if(!isInFront( pVictim, range ) && spellInfo->AttributesEx )
        //    continue;
        if( dist > range || dist < minrange )
            continue;
        if(HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED))
            continue;
        return spellInfo;
    }
    return nullptr;
}

void Creature::DoFleeToGetAssistance(float radius) // Optional parameter
{
    if (!GetVictim())
        return;
        
    if (HasUnitState(UNIT_STATE_STUNNED))
        return;

    if (HasAuraType(SPELL_AURA_PREVENTS_FLEEING))
        return;

    if (radius <= 0)
        return;
        
    if (IsNonMeleeSpellCast(false))
        InterruptNonMeleeSpells(true);

    Creature* pCreature = nullptr;

    Trinity::NearestAssistCreatureInCreatureRangeCheck u_check(this,GetVictim(),radius);
    Trinity::CreatureLastSearcher<Trinity::NearestAssistCreatureInCreatureRangeCheck> searcher(this, pCreature, u_check);
    Cell::VisitGridObjects(this, searcher, radius);

    if(!pCreature)
        SetControlled(true, UNIT_STATE_FLEEING);
    else
        GetMotionMaster()->MoveSeekAssistance(pCreature->GetPositionX(), pCreature->GetPositionY(), pCreature->GetPositionZ());
}

Unit* Creature::SelectNearestTarget(float dist, bool playerOnly /* = false */, bool furthest /* = false */) const
{
    Unit *target = nullptr;

    {
        if (dist == 0.0f)
            dist = MAX_SEARCHER_DISTANCE;

        Trinity::NearestHostileUnitCheck u_check(this, playerOnly, furthest);
        Trinity::UnitLastSearcher<Trinity::NearestHostileUnitCheck> searcher(this, target, u_check);
        Cell::VisitAllObjects(this, searcher, dist);
    }

    return target;
}


// select nearest hostile unit within the given attack distance (i.e. distance is ignored if > than ATTACK_DISTANCE), regardless of threat list.
Unit* Creature::SelectNearestTargetInAttackDistance(float dist) const
{
    CellCoord p(Trinity::ComputeCellCoord(GetPositionX(), GetPositionY()));
    Cell cell(p);
    cell.SetNoCreate();

    Unit* target = nullptr;

    if (dist < ATTACK_DISTANCE)
        dist = ATTACK_DISTANCE;
    if (dist > MAX_SEARCHER_DISTANCE)
        dist = MAX_SEARCHER_DISTANCE;

    {
        Trinity::NearestHostileUnitInAttackDistanceCheck u_check(this);
        Trinity::UnitLastSearcher<Trinity::NearestHostileUnitInAttackDistanceCheck> searcher(this, target, u_check);

        TypeContainerVisitor<Trinity::UnitLastSearcher<Trinity::NearestHostileUnitInAttackDistanceCheck>, WorldTypeMapContainer > world_unit_searcher(searcher);
        TypeContainerVisitor<Trinity::UnitLastSearcher<Trinity::NearestHostileUnitInAttackDistanceCheck>, GridTypeMapContainer >  grid_unit_searcher(searcher);

        cell.Visit(p, world_unit_searcher, *GetMap(), *this, dist);
        cell.Visit(p, grid_unit_searcher, *GetMap(), *this, dist);
    }

    return target;
}

Unit* Creature::SelectNearestHostileUnitInAggroRange(bool useLOS) const
{
    // Selects nearest hostile target within creature's aggro range. Used primarily by
    //  pets set to aggressive. Will not return neutral or friendly targets.

    Unit* target = NULL;

    {
        Trinity::NearestHostileUnitInAggroRangeCheck u_check(this, useLOS);
        Trinity::UnitSearcher<Trinity::NearestHostileUnitInAggroRangeCheck> searcher(this, target, u_check);

        Cell::VisitAllObjects(this, searcher, MAX_AGGRO_RADIUS); //sun: replaced VisitGridObjects with VisitAllObjects, else we wont get enemy players or pets
    }

    return target;
}

void Creature::CallAssistance()
{
    if( !m_AlreadyCallAssistance && GetVictim() && !IsPet() && !IsCharmed())
    {
        SetNoCallAssistance(true);

        float radius = sWorld->getConfig(CONFIG_CREATURE_FAMILY_ASSISTANCE_RADIUS);
        if(radius > 0)
        {
            std::list<Creature*> assistList;

            {
                CellCoord p(Trinity::ComputeCellCoord(GetPositionX(), GetPositionY()));
                Cell cell(p);
                cell.SetNoCreate();
                cell.data.Part.reserved = ALL_DISTRICT;

                Trinity::AnyAssistCreatureInRangeCheck u_check(this, GetVictim(), radius);
                Trinity::CreatureListSearcher<Trinity::AnyAssistCreatureInRangeCheck> searcher(this, assistList, u_check);

                TypeContainerVisitor<Trinity::CreatureListSearcher<Trinity::AnyAssistCreatureInRangeCheck>, GridTypeMapContainer >  grid_creature_searcher(searcher);

                cell.Visit(p, grid_creature_searcher, *GetMap(), *this, radius);
            }

            // Add creatures from linking DB system
            if (m_creaturePoolId) {
                std::list<Creature*> allCreatures = FindMap()->GetAllCreaturesFromPool(m_creaturePoolId);
                for (auto itr : allCreatures) 
                {
                    if (itr->IsAlive() && itr->IsInWorld())
                        assistList.push_back(itr);
                }
                if(allCreatures.size() == 0)
                    TC_LOG_ERROR("sql.sql","Broken data in table creature_pool_relations for creature pool %u.", m_creaturePoolId);
            }

            if (!assistList.empty())
            {
                uint32 count = 0;
                auto e = new AssistDelayEvent(GetVictim()->GetGUID(), *this);
                while (!assistList.empty())
                {
                    ++count;
                    // Pushing guids because in delay can happen some creature gets despawned => invalid pointer
//                    TC_LOG_INFO("FIXME","ASSISTANCE: calling creature at %p", *assistList.begin());
                    Creature *cr = *assistList.begin();
                    if (!cr->IsInWorld()) {
                        TC_LOG_ERROR("FIXME","ASSISTANCE: ERROR: creature is not in world");
                        assistList.pop_front();
                        continue;
                    }
                    e->AddAssistant((*assistList.begin())->GetGUID());
                    assistList.pop_front();
                    if (GetInstanceId() == 0 && count >= 4)
                        break;
                }
                m_Events.AddEvent(e, m_Events.CalculateTime(sWorld->getConfig(CONFIG_CREATURE_FAMILY_ASSISTANCE_DELAY)));
            }
        }
    }
}

void Creature::CallForHelp(float radius)
{
    if (radius <= 0.0f || !IsEngaged() || IsPet() || IsCharmed())
        return;

    Unit* target = GetThreatManager().GetCurrentVictim();
    if (!target)
        target = GetThreatManager().GetAnyTarget();
    if (!target)
        target = GetCombatManager().GetAnyTarget();
    ASSERT(target, "Creature %u (%s) is engaged without threat list", GetEntry(), GetName().c_str());

    Trinity::CallOfHelpCreatureInRangeDo u_do(this, target, radius);
    Trinity::CreatureWorker<Trinity::CallOfHelpCreatureInRangeDo> worker(this, u_do);
    Cell::VisitGridObjects(this, worker, radius);
}

bool Creature::CanAssistTo(const Unit* u, const Unit* enemy, bool checkFaction /* = true */) const
{
    // is it true?
    if(!HasReactState(REACT_AGGRESSIVE) || (HasJustAppeared() && !IsControlledByPlayer())) //ignore justrespawned if summoned
        return false;

    // we don't need help from zombies :)
    if( !IsAlive() )
        return false;

    // we cannot assist in evade mode
    if (IsInEvadeMode())
        return false;

    // or if enemy is in evade mode
    if (enemy->GetTypeId() == TYPEID_UNIT && enemy->ToCreature()->IsInEvadeMode())
        return false;

    // skip fighting creature
    if( IsInCombat() )
        return false;

    // only from same creature faction
    if (checkFaction && GetFaction() != u->GetFaction())
        return false;

    // only free creature
    if( GetCharmerOrOwnerGUID() )
        return false;

    // skip non hostile to caster enemy creatures
    if( !IsHostileTo(enemy) )
        return false;

    // don't assist to slay innocent critters
    if( enemy->GetTypeId() == CREATURE_TYPE_CRITTER )
        return false;

    return true;
}

// use this function to avoid having hostile creatures attack
// friendlies and other mobs they shouldn't attack
bool Creature::_IsTargetAcceptable(Unit const* target) const
{
    ASSERT(target);

    // if the target cannot be attacked, the target is not acceptable
    if (IsFriendlyTo(target)
        || !target->IsTargetableForAttack(false)
#ifdef LICH_KING
        || (m_vehicle && (IsOnVehicle(target) || m_vehicle->GetBase()->IsOnVehicle(target)))
#endif
        )
        return false;

    if (target->HasUnitState(UNIT_STATE_DIED))
    {
        // guards can detect fake death
        if ((IsGuard() && target->HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH))
            || target->IsFeighDeathDetected(this))
            return true;
        else
            return false;
    }

    // if I'm already fighting target, or I'm hostile towards the target, the target is acceptable
    if (IsEngagedBy(target) || IsHostileTo(target))
        return true;

    // if the target's victim is not friendly, or the target is friendly, the target is not acceptable
    return false;
}

void Creature::SaveRespawnTime(uint32 forceDelay, bool savetodb)
{
    if(IsSummon() || !m_spawnId || (m_creatureData && !m_creatureData->dbData))
        return;

    if (m_respawnCompatibilityMode)
    {
        GetMap()->SaveRespawnTimeDB(SPAWN_TYPE_CREATURE, m_spawnId, m_respawnTime);
        return;
    }

    time_t thisRespawnTime = forceDelay ? GetMap()->GetGameTime() + forceDelay : m_respawnTime;
    GetMap()->SaveRespawnTime(SPAWN_TYPE_CREATURE, m_spawnId, GetEntry(), thisRespawnTime, GetMap()->GetZoneId(GetHomePosition()), Trinity::ComputeGridCoord(GetHomePosition().GetPositionX(), GetHomePosition().GetPositionY()).GetId(), savetodb && m_creatureData && m_creatureData->dbData);
}

void Creature::LoadCreatureAddon()
{
    if (m_spawnId)
    {
        if((m_creatureInfoAddon = sObjectMgr->GetCreatureAddon(m_spawnId)))
            return;
    }

    m_creatureInfoAddon = sObjectMgr->GetCreatureTemplateAddon(GetCreatureTemplate()->Entry);
}

//creature_addon table
bool Creature::InitCreatureAddon(bool reload)
{
    if(!m_creatureInfoAddon)
        return false;

    if (m_creatureInfoAddon->mount != 0)
        Mount(m_creatureInfoAddon->mount);

    if (m_creatureInfoAddon->bytes0 != 0)
        SetUInt32Value(UNIT_FIELD_BYTES_0, m_creatureInfoAddon->bytes0);

    if (m_creatureInfoAddon->bytes1 != 0)
        SetUInt32Value(UNIT_FIELD_BYTES_1, m_creatureInfoAddon->bytes1);

    if (m_creatureInfoAddon->bytes2 != 0)
    {
#ifndef LICH_KING
        //sun: keep buff limit intact
        uint8 const buffLimit = GetByteValue(UNIT_FIELD_BYTES_2, UNIT_BYTES_2_OFFSET_BUFF_LIMIT);
        SetUInt32Value(UNIT_FIELD_BYTES_2, m_creatureInfoAddon->bytes2);
        SetByteValue(UNIT_FIELD_BYTES_2, UNIT_BYTES_2_OFFSET_BUFF_LIMIT, buffLimit);
#else
        SetUInt32Value(UNIT_FIELD_BYTES_2, m_creatureInfoAddon->bytes2);
#endif
    }

    if (m_creatureInfoAddon->emote != 0)
        SetEmoteState(m_creatureInfoAddon->emote);

    if (m_creatureInfoAddon->move_flags != 0)
        SetUnitMovementFlags(m_creatureInfoAddon->move_flags);

    //Load Path
    if (m_creatureInfoAddon->path_id != 0)
        _waypointPathId = m_creatureInfoAddon->path_id;

    for(auto id : m_creatureInfoAddon->auras)
    {
        SpellInfo const *AdditionalSpellInfo = sSpellMgr->GetSpellInfo(id);
        if (!AdditionalSpellInfo)
        {
            TC_LOG_ERROR("sql.sql","Creature (GUIDLow: %u Entry: %u ) has wrong spell %u defined in `auras` field.", GetSpawnId(),GetEntry(),id);
            continue;
        }

        // skip already applied aura
        if(HasAura(id))
        {
            if(!reload)
                TC_LOG_ERROR("sql.sql","Creature (GUIDLow: %u Entry: %u ) has duplicate aura (spell %u) in `auras` field.",GetSpawnId(),GetEntry(),id);

            continue;
        }

        AddAura(id, this);
        TC_LOG_DEBUG("entities.unit", "Spell: %u added to creature (GUID: %u Entry: %u)", id, GetSpawnId(), GetEntry());
    }
    return true;
}

/// Send a message to LocalDefense channel for players opposition team in the zone
void Creature::SendZoneUnderAttackMessage(Player* attacker)
{
    uint32 enemy_team = attacker->GetTeam();
    sWorld->SendZoneUnderAttack(GetZoneId(), (enemy_team==ALLIANCE ? HORDE : ALLIANCE));
}

bool Creature::HasSpell(uint32 spellID) const
{
    return std::find(std::begin(m_spells), std::end(m_spells), spellID) != std::end(m_spells);
}

time_t Creature::GetRespawnTimeEx() const
{
    time_t now = GetMap()->GetGameTime();
    if (m_respawnTime > now)
        return m_respawnTime;
    else
        return now;
}

bool Creature::IsSpawnedOnTransport() const 
{
    return m_creatureData && m_creatureData->spawnPoint.GetMapId() != GetMapId();
}

void Creature::GetRespawnPosition( float &x, float &y, float &z, float* ori, float* dist ) const
{
    if (m_creatureData)
    {
        if (ori)
            m_creatureData->spawnPoint.GetPosition(x, y, z, *ori);
        else
            m_creatureData->spawnPoint.GetPosition(x, y, z);

        if (dist)
            *dist = m_creatureData->spawndist;
    }
    else
    {
        Position const& homePos = GetHomePosition();
        if (ori)
            homePos.GetPosition(x, y, z, *ori);
        else
            homePos.GetPosition(x, y, z);
        if (dist)
            *dist = 0;
    }
}

CreatureMovementData const& Creature::GetMovementTemplate() const
{
    if (CreatureMovementData const* movementOverride = sObjectMgr->GetCreatureMovementOverride(m_spawnId))
        return *movementOverride;

    return GetCreatureTemplate()->Movement;
}

void Creature::AllLootRemovedFromCorpse()
{
    if (loot.loot_type != LOOT_SKINNING && !IsPet() && GetCreatureTemplate()->SkinLootId)
        if (LootTemplates_Skinning.HaveLootFor(GetCreatureTemplate()->SkinLootId))
            SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE);

    time_t now = GetMap()->GetGameTime();
    // Do not reset corpse remove time if corpse is already removed
    if(m_corpseRemoveTime <= now)
        return;
            
    float decayRate = sWorld->GetRate(RATE_CORPSE_DECAY_LOOTED);
    CreatureTemplate const *cinfo = GetCreatureTemplate();

    // corpse skinnable, but without skinning flag, and then skinned, corpse will despawn next update
    if (loot.loot_type == LOOT_SKINNING)
        m_corpseRemoveTime = now;
    else
        m_corpseRemoveTime = now + uint32(m_corpseDelay * decayRate);

    m_respawnTime = std::max<time_t>(m_corpseRemoveTime + m_respawnDelay, m_respawnTime);
}

void Creature::ResetCorpseRemoveTime()
{
    m_corpseRemoveTime = GetMap()->GetGameTime() + m_corpseDelay;
}

uint8 Creature::GetLevelForTarget( WorldObject const* target ) const
{
    if(!IsWorldBoss() || !target->ToUnit())
        return Unit::GetLevelForTarget(target);

    //bosses are always considered 3 level higher
    uint32 level = target->ToUnit()->GetLevel() + 3;
    if(level < 1)
        return 1;
    if(level > 255)
        return 255;
    return uint8(level);
}

std::string Creature::GetScriptName()
{
    return sObjectMgr->GetScriptName(GetScriptId());
}

uint32 Creature::GetScriptId()
{
    if (CreatureData const* creatureData = GetCreatureData())
        if (uint32 scriptId = creatureData->scriptId)
            return scriptId;

    return sObjectMgr->GetCreatureTemplate(GetEntry())->ScriptID;
}

std::string Creature::GetAIName() const
{
    return sObjectMgr->GetCreatureTemplate(GetEntry())->AIName;
}

VendorItemData const* Creature::GetVendorItems() const
{
    return sObjectMgr->GetNpcVendorItemList(GetEntry());
}

uint32 Creature::GetVendorItemCurrentCount(VendorItem const* vItem)
{
    if(!vItem->maxcount)
        return vItem->maxcount;

    auto itr = m_vendorItemCounts.begin();
    for(; itr != m_vendorItemCounts.end(); ++itr)
        if (itr->itemId == vItem->item)
            break;

    if(itr == m_vendorItemCounts.end())
        return vItem->maxcount;

    VendorItemCount* vCount = &*itr;

    time_t ptime = WorldGameTime::GetGameTime();

    if( vCount->lastIncrementTime + vItem->incrtime <= ptime )
    {
        ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(vItem->item);

        uint32 diff = uint32((ptime - vCount->lastIncrementTime)/vItem->incrtime);
        if((vCount->count + diff * pProto->BuyCount) >= vItem->maxcount )
        {
            m_vendorItemCounts.erase(itr);
            return vItem->maxcount;
        }

        vCount->count += diff * pProto->BuyCount;
        vCount->lastIncrementTime = ptime;
    }

    return vCount->count;
}

uint32 Creature::UpdateVendorItemCurrentCount(VendorItem const* vItem, uint32 used_count)
{
    if(!vItem->maxcount)
        return 0;

    auto itr = m_vendorItemCounts.begin();
    for(; itr != m_vendorItemCounts.end(); ++itr)
        if(itr->itemId==vItem->item)
            break;

    if(itr == m_vendorItemCounts.end())
    {
        uint32 new_count = vItem->maxcount > used_count ? vItem->maxcount-used_count : 0;
        m_vendorItemCounts.push_back(VendorItemCount(vItem->item,new_count));
        return new_count;
    }

    VendorItemCount* vCount = &*itr;

    time_t ptime = WorldGameTime::GetGameTime();

    if( vCount->lastIncrementTime + vItem->incrtime <= ptime )
        if (ItemTemplate const* pProto = sObjectMgr->GetItemTemplate(vItem->item))
        {
            uint32 diff = uint32((ptime - vCount->lastIncrementTime)/vItem->incrtime);
            if((vCount->count + diff * pProto->BuyCount) < vItem->maxcount )
                vCount->count += diff * pProto->BuyCount;
            else
                vCount->count = vItem->maxcount;
        }

    vCount->count = vCount->count > used_count ? vCount->count-used_count : 0;
    vCount->lastIncrementTime = ptime;
    return vCount->count;
}

TrainerSpellData const* Creature::GetTrainerSpells() const
{
    return sObjectMgr->GetNpcTrainerSpells(GetEntry());
}

// overwrite WorldObject function for proper name localization
std::string const& Creature::GetNameForLocaleIdx(LocaleConstant loc_idx) const
{
    if (loc_idx != DEFAULT_LOCALE)
    {
        CreatureLocale const *cl = sObjectMgr->GetCreatureLocale(GetEntry());
        if (cl)
        {
            if (cl->Name.size() > loc_idx && !cl->Name[loc_idx].empty())
                return cl->Name[loc_idx];
        }
    }

    return GetName();
}

void Creature::AreaCombat()
{
    if(Map* map = GetMap())
    {
        float range = 0.0f;
        if(GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_INSTANCE_BIND)
            range += 100.0f;
        if(IsWorldBoss())
            range += 100.0f;

        Map::PlayerList const &PlayerList = map->GetPlayers();
        for(const auto & i : PlayerList)
        {
            if (Player* i_pl = i.GetSource())
                if (i_pl->IsAlive() && IsWithinCombatRange(i_pl, range) && _CanCreatureAttack(i_pl, false) == CAN_ATTACK_RESULT_OK)
                    GetThreatManager().AddThreat(i_pl, 0.0f);
        }
    }
}

bool Creature::HadPlayerInThreatListAtDeath(ObjectGuid guid) const
{
    auto itr = m_playerInThreatListAtDeath.find(guid.GetCounter());
    return itr == m_playerInThreatListAtDeath.end();
}

void Creature::ConvertThreatListIntoPlayerListAtDeath()
{
    for (auto itr : GetThreatManager().GetUnsortedThreatList())
        if (Player* player = itr->GetVictim()->ToPlayer())
        {
            if (itr->GetThreat() > 0.0f)
                m_playerInThreatListAtDeath.insert(player->GetGUID().GetCounter());
        }
}

bool Creature::IsBelowHPPercent(float percent)
{
    float healthAtPercent = (GetMaxHealth() / 100.0f) * percent;
    
    return GetHealth() < healthAtPercent;
}

bool Creature::IsAboveHPPercent(float percent)
{
    float healthAtPercent = (GetMaxHealth() / 100.0f) * percent;
    
    return GetHealth() > healthAtPercent;
}

bool Creature::IsBetweenHPPercent(float minPercent, float maxPercent)
{
    float minHealthAtPercent = (GetMaxHealth() / 100.0f) * minPercent;
    float maxHealthAtPercent = (GetMaxHealth() / 100.0f) * maxPercent;

    return GetHealth() > minHealthAtPercent && GetHealth() < maxHealthAtPercent;
}

bool Creature::isMoving()
{
    float x, y ,z;
    return GetMotionMaster()->GetDestination(x,y,z);
}

bool AIMessageEvent::Execute(uint64 /*e_time*/, uint32 /*p_time*/) 
{ 
    if(owner.AI())
        owner.AI()->message(id,data);

    return true; 
}

void Creature::AddMessageEvent(uint64 timer, uint32 messageId, uint64 data)
{
    auto  messageEvent = new AIMessageEvent(*this,messageId,data);
    m_Events.AddEvent(messageEvent, m_Events.CalculateTime(timer), false);
}

float Creature::GetDistanceFromHome() const
{
    return GetDistance(GetHomePosition());
}

void Creature::Motion_Initialize()
{
    if (m_formation)
    {
        if (m_formation->GetLeader() == this)
            m_formation->FormationReset(false);
        else if (m_formation->IsFormed())
        {
            GetMotionMaster()->MoveIdle(); // wait the order of leader
            return;
        }
    }

    GetMotionMaster()->Initialize();
}

void Creature::SetTarget(ObjectGuid guid)
{
    if (IsFocusing(nullptr, true))
        m_suppressedTarget = guid;
    else
        SetGuidValue(UNIT_FIELD_TARGET, guid);
}

void Creature::FocusTarget(Spell const* focusSpell, WorldObject const* target)
{
    // already focused
    if (m_focusSpell)
        return;

    // some spells shouldn't track targets
    if (focusSpell->IsFocusDisabled())
        return;

    SpellInfo const* spellInfo = focusSpell->GetSpellInfo();
#ifdef LICH_KING
    // don't use spell focus for vehicle spells
    if (spellInfo->HasAura(SPELL_AURA_CONTROL_VEHICLE))
        return;
#endif

    if ((!target || target == this) && !focusSpell->GetCastTime()) // instant cast, untargeted (or self-targeted) spell doesn't need any facing updates
        return;

    // store pre-cast values for target and orientation (used to later restore)
    if (!IsFocusing(nullptr, true))
    { // only overwrite these fields if we aren't transitioning from one spell focus to another
        m_suppressedTarget = GetGuidValue(UNIT_FIELD_TARGET);
        m_suppressedOrientation = GetOrientation();
    }

    m_focusSpell = focusSpell;

    // set target, then force send update packet to players if it changed to provide appropriate facing
    ObjectGuid newTarget = target ? target->GetGUID() : ObjectGuid::Empty;
    if (GetGuidValue(UNIT_FIELD_TARGET) != newTarget)
    {
        SetGuidValue(UNIT_FIELD_TARGET, newTarget);

        if ( // here we determine if the (relatively expensive) forced update is worth it, or whether we can afford to wait until the scheduled update tick
            ( // only require instant update for spells that actually have a visual
#ifdef LICH_KING
                spellInfo->SpellVisual[0] ||
                spellInfo->SpellVisual[1]
#else
                spellInfo->SpellVisual
#endif
                ) && (
                    !focusSpell->GetCastTime() || // if the spell is instant cast
                    spellInfo->HasAttribute(SPELL_ATTR5_DONT_TURN_DURING_CAST) // client gets confused if we attempt to turn at the regularly scheduled update packet
                    )
            )
        {
            std::vector<Player*> playersNearby;
            GetPlayerListInGrid(playersNearby, GetVisibilityRange());
            for (Player* player : playersNearby)
            {
                // only update players that are known to the client (have already been created)
                if (player->HaveAtClient(this))
                    SendUpdateToPlayer(player);
            }
        }
    }

    bool const noTurnDuringCast = spellInfo->HasAttribute(SPELL_ATTR5_DONT_TURN_DURING_CAST);

#ifdef LICH_KING
    if (!HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_DISABLE_TURN))
#endif
    {
        // Face the target - we need to do this before the unit state is modified for no-turn spells
        if (target)
            SetFacingToObject(target, false);
        else if (noTurnDuringCast)
            if (Unit* victim = GetVictim())
                SetFacingToObject(victim, false); // ensure orientation is correct at beginning of cast
    }

    if (noTurnDuringCast)
        AddUnitState(UNIT_STATE_FOCUSING);
}

void Creature::ReleaseFocus(Spell const* focusSpell, bool withDelay)
{
    if (!m_focusSpell)
        return;

    // focused to something else
    if (focusSpell && focusSpell != m_focusSpell)
        return;

    if (IsPet()
#ifdef LICH_KING
        && !HasFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_DISABLE_TURN)
#endif
        ) // player pets do not use delay system
    {
        SetGuidValue(UNIT_FIELD_TARGET, m_suppressedTarget);
        if (m_suppressedTarget)
        {
            if (WorldObject const* objTarget = ObjectAccessor::GetWorldObject(*this, m_suppressedTarget))
                SetFacingToObject(objTarget, false);
        }
        else
            SetFacingTo(m_suppressedOrientation, false);
    }
    else
        // tell the creature that it should reacquire its actual target after the delay expires (this is handled in ::Update)
        // player pets don't need to do this, as they automatically reacquire their target on focus release
        MustReacquireTarget();

    if (m_focusSpell->GetSpellInfo()->HasAttribute(SPELL_ATTR5_DONT_TURN_DURING_CAST))
        ClearUnitState(UNIT_STATE_FOCUSING);

    m_focusSpell = nullptr;
    m_focusDelay = (!IsPet() && withDelay) ? GetMap()->GetGameTimeMS() : 0; // don't allow re-target right away to prevent visual bugs
}

bool Creature::IsMovementPreventedByCasting() const
{
    // first check if currently a movement allowed channel is active and we're not casting
    if (Spell* spell = m_currentSpells[CURRENT_CHANNELED_SPELL])
    {
        if (spell->getState() != SPELL_STATE_FINISHED && spell->IsChannelActive())
            if (spell->GetSpellInfo()->IsMoveAllowedChannel())
                return false;
    }

    if (const_cast<Creature*>(this)->IsFocusing(nullptr, true))
        return true;

    if (HasUnitState(UNIT_STATE_CASTING))
        return true;

    return false;
}

void Creature::StartPickPocketRefillTimer()
{
    _pickpocketLootRestore = GetMap()->GetGameTime() + 10 * MINUTE; /*sWorld->getIntConfig(CONFIG_CREATURE_PICKPOCKET_REFILL)*/;
}

bool Creature::CanGeneratePickPocketLoot() const
{
    return _pickpocketLootRestore <= GetMap()->GetGameTime();
}

bool Creature::IsFocusing(Spell const* focusSpell, bool withDelay)
{
    if (!IsAlive()) // dead creatures cannot focus
    {
        ReleaseFocus(nullptr, false);
        return false;
    }

    if (focusSpell && (focusSpell != m_focusSpell))
        return false;

    if (!m_focusSpell)
    {
        if (!withDelay || !m_focusDelay)
            return false;
        if (GetMSTimeDiffToNow(m_focusDelay) > 1000) // @todo figure out if we can get rid of this magic number somehow
        {
            m_focusDelay = 0; // save checks in the future
            return false;
        }
    }

    return true;
}

uint32 Creature::GetPetAutoSpellOnPos(uint8 pos) const
{
    if (pos >= MAX_SPELL_CHARM || m_charmInfo->GetCharmSpell(pos)->GetType() != ACT_ENABLED)
        return 0;
    else
        return m_charmInfo->GetCharmSpell(pos)->GetAction();
}

float Creature::GetPetChaseDistance() const
{
    float range = 0.0f;

    for (uint8 i = 0; i < GetPetAutoSpellSize(); ++i)
    {
        uint32 spellID = GetPetAutoSpellOnPos(i);
        if (!spellID)
            continue;

        if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellID))
        {
            if (spellInfo->GetRecoveryTime() == 0 &&  // No cooldown
                spellInfo->RangeEntry->ID != 1 /*Self*/ && spellInfo->RangeEntry->ID != 2 /*Combat Range*/ &&
                spellInfo->GetMaxRange() > range)
                range = spellInfo->GetMaxRange();
        }
    }

    return range;
}

bool Creature::SetWalk(bool enable)
{
    if (!Unit::SetWalk(enable))
        return false;

    WorldPacket data(enable ? SMSG_SPLINE_MOVE_SET_WALK_MODE : SMSG_SPLINE_MOVE_SET_RUN_MODE, 9);
    data << GetPackGUID();
    SendMessageToSet(&data, false);
    return true;
}


bool Creature::SetSwim(bool enable)
{
    if (!Unit::SetSwim(enable))
        return false;

    if (!movespline->Initialized())
        return true;

    WorldPacket data(enable ? SMSG_SPLINE_MOVE_START_SWIM : SMSG_SPLINE_MOVE_STOP_SWIM);
    data << GetPackGUID();
    SendMessageToSet(&data, true);
    return true;
}

void Creature::SetFlying(bool enable)
{
    Unit::SetFlying(enable);

    //also mark creature as able to fly to avoid getting fly mode removed
    if(enable)
        _SetCanFly(enable, false);
}

void Creature::_SetCanFly(bool enable, bool updateMovementFlags /* = true */) 
{ 
    m_canFly = enable; 
    if (updateMovementFlags)
        UpdateMovementFlags();
}

void Creature::UpdateMovementFlags(bool force /* = false */)
{
    if (!force && GetExactDistSq(m_lastMovementFlagsPosition) < 2.5f*2.5f)
        return;

    // Do not update movement flags if creature is controlled by a player (charm/vehicle)
    if (!m_playerMovingMe.expired())
        return;

    // Set the movement flags if the creature is in that mode. (Only fly if actually in air, only swim if in water, etc)
    float ground = GetFloorZ();

#ifdef LICH_KING
    bool isInAir = (G3D::fuzzyGt(GetPositionZ(), ground + (canHover ? GetFloatValue(UNIT_FIELD_HOVERHEIGHT) : 0.0f) + GROUND_HEIGHT_TOLERANCE) || G3D::fuzzyLt(GetPositionZ(), ground - GROUND_HEIGHT_TOLERANCE)); // Can be underground too, prevent the falling
#else
    bool isInAir = (G3D::fuzzyGt(GetPositionZ(), ground + GROUND_HEIGHT_TOLERANCE) || G3D::fuzzyLt(GetPositionZ(), ground - GROUND_HEIGHT_TOLERANCE)); // Can be underground too, prevent the falling
#endif

    if (CanFly() && isInAir && !IsFalling())
    {
        if (CanWalk())
        {
            SetFlying(true);
            SetDisableGravity(true);
        } 
        else
            SetDisableGravity(true);
    }
    else
    {
        SetFlying(false);
        SetDisableGravity(false);
        if (IsAlive() && (CanHover() || HasAuraType(SPELL_AURA_HOVER)))
            SetHover(true);
    }

    if (!isInAir || CanFly())
        RemoveUnitMovementFlag(MOVEMENTFLAG_JUMPING_OR_FALLING);

    SetSwim(CanSwim() && IsInWater());
    m_lastMovementFlagsPosition = GetPosition();
}

bool Creature::IsInEvadeMode() const 
{ 
    return HasUnitState(UNIT_STATE_EVADE); 
}

void Creature::SetCannotReachTarget(bool cannotReach)
{
    if (cannotReach)
    {
        //this mark creature as unreachable + start counting
        if (!m_unreachableTargetTime)
            m_unreachableTargetTime = 1;
    }
    else {
        m_unreachableTargetTime = 0;
        m_evadingAttacks = false;
    }
}

bool Creature::CannotReachTarget() const
{
    return m_unreachableTargetTime > 0;
}

void Creature::HandleUnreachableTarget(uint32 diff)
{
    if(!AI() || !IsInCombat() || m_unreachableTargetTime == 0 || IsInEvadeMode())
        return;

    if(GetMotionMaster()->GetCurrentMovementGeneratorType() == CHASE_MOTION_TYPE)
        m_unreachableTargetTime += diff;

    if (!m_evadingAttacks)
    {
        uint32 maxTimeBeforeEvadingAttacks = sWorld->getConfig(CONFIG_CREATURE_UNREACHABLE_TARGET_EVADE_ATTACKS_TIME);
        if (maxTimeBeforeEvadingAttacks)
            if (m_unreachableTargetTime > maxTimeBeforeEvadingAttacks)
                m_evadingAttacks = true;
    }

    //evading to home shouldn't happen in instance (not 100% sure in which case this is true on retail but this seems like a good behavior for now)
    if (GetMap()->IsDungeon())
        return;

    uint32 maxTimeBeforeEvadingHome = sWorld->getConfig(CONFIG_CREATURE_UNREACHABLE_TARGET_EVADE_TIME);
    if (maxTimeBeforeEvadingHome)
        if (m_unreachableTargetTime > maxTimeBeforeEvadingHome)
            if (CreatureAI* ai = AI())
                ai->EnterEvadeMode(CreatureAI::EVADE_REASON_NO_PATH);
}

void Creature::ResetCreatureEmote()
{
    if(CreatureAddon const* cAddon = GetCreatureAddon())
        SetEmoteState(cAddon->emote); 
    else
        SetEmoteState(0); 

    SetStandState(UNIT_STAND_STATE_STAND);
}

void Creature::WarnDeathToFriendly()
{
    std::list< std::pair<Creature*,float> > warnList;

    // Check near creatures for assistance
    CellCoord p(Trinity::ComputeCellCoord(GetPositionX(), GetPositionY()));
    Cell cell(p);
    cell.SetNoCreate();

    Trinity::AnyFriendlyUnitInObjectRangeCheckWithRangeReturned u_check(this, this, CREATURE_MAX_DEATH_WARN_RANGE);
    Trinity::CreatureListSearcherWithRange<Trinity::AnyFriendlyUnitInObjectRangeCheckWithRangeReturned> searcher(this, warnList, u_check);
    Cell::VisitGridObjects(this, searcher, CREATURE_MAX_DEATH_WARN_RANGE);

    for(auto itr : warnList) 
        if(CreatureAI* ai = itr.first->AI())
            ai->FriendlyKilled(this, itr.second);    
}

void Creature::SetTextRepeatId(uint8 textGroup, uint8 id)
{
    CreatureTextRepeatIds& repeats = m_textRepeat[textGroup];
    if (std::find(repeats.begin(), repeats.end(), id) == repeats.end())
        repeats.push_back(id);
    else
        TC_LOG_ERROR("sql.sql", "CreatureTextMgr: TextGroup %u for Creature(%s) GuidLow %u Entry %u, id %u already added", uint32(textGroup), GetName().c_str(), GetGUID().GetCounter(), GetEntry(), uint32(id));
}

CreatureTextRepeatIds Creature::GetTextRepeatGroup(uint8 textGroup)
{
    CreatureTextRepeatIds ids;

    CreatureTextRepeatGroup::const_iterator groupItr = m_textRepeat.find(textGroup);
    if (groupItr != m_textRepeat.end())
        ids = groupItr->second;

    return ids;
}

void Creature::ClearTextRepeatGroup(uint8 textGroup)
{
    auto groupItr = m_textRepeat.find(textGroup);
    if (groupItr != m_textRepeat.end())
        groupItr->second.clear();
}

bool Creature::CanGiveExperience() const
{
    return !IsCritter()
        && !IsPet()
        && !IsTotem()
        && !(GetCreatureTemplate()->flags_extra & CREATURE_FLAG_EXTRA_NO_XP_AT_KILL);
}

void Creature::AtEnterCombat()
{
    Unit::AtEnterCombat();

    if (!(GetCreatureTemplate()->type_flags & CREATURE_TYPE_FLAG_MOUNTED_COMBAT_ALLOWED))
        Dismount();

    if (IsPet() || IsGuardian()) // update pets' speed for catchup OOC speed
    {
        UpdateSpeed(MOVE_RUN);
        UpdateSpeed(MOVE_SWIM);
        UpdateSpeed(MOVE_FLIGHT);
    }
}

void Creature::AtExitCombat()
{
    Unit::AtExitCombat();

    ClearUnitState(UNIT_STATE_ATTACK_PLAYER);
    if (HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_TAPPED))
        SetUInt32Value(UNIT_DYNAMIC_FLAGS, GetCreatureTemplate()->dynamicflags);

    if (IsPet() || IsGuardian()) // update pets' speed for catchup OOC speed
    {
        UpdateSpeed(MOVE_RUN);
        UpdateSpeed(MOVE_SWIM);
        UpdateSpeed(MOVE_FLIGHT);
    }
}

void Creature::SetKeepActiveTimer(uint32 timerMS)
{
    if (timerMS == 0)
        return;

    if (GetTypeId() == TYPEID_PLAYER)
        return;

    if (!IsInWorld())
        return;

    SetKeepActive(true);
    m_keepActiveTimer = timerMS;
}

void Creature::SetObjectScale(float scale)
{
    Unit::SetObjectScale(scale);

    if (CreatureModelInfo const* minfo = sObjectMgr->GetCreatureModelInfo(GetDisplayId()))
    {
        SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, (IsPet() ? 1.0f : minfo->bounding_radius) * scale);
        SetFloatValue(UNIT_FIELD_COMBATREACH, (IsPet() ? DEFAULT_PLAYER_COMBAT_REACH : minfo->combat_reach) * scale);
    }
}

void Creature::SetDisplayId(uint32 modelId)
{
    Unit::SetDisplayId(modelId);

    if (CreatureModelInfo const* minfo = sObjectMgr->GetCreatureModelInfo(modelId))
    {
        SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, (IsPet() ? 1.0f : minfo->bounding_radius) * GetObjectScale());
        SetFloatValue(UNIT_FIELD_COMBATREACH, (IsPet() ? DEFAULT_PLAYER_COMBAT_REACH : minfo->combat_reach) * GetObjectScale());

        // Set Gender by modelId. Note: TC has this in Unit::SetDisplayId but this seems wrong for BC
         SetByteValue(UNIT_FIELD_BYTES_0, UNIT_BYTES_0_OFFSET_GENDER, minfo->gender);
    }

    if (Pet *pet = this->ToPet())
    {
        if (pet->isControlled())
        {
            Unit *owner = GetOwner();
            if (owner && (owner->GetTypeId() == TYPEID_PLAYER) && (owner->ToPlayer())->GetGroup())
                (owner->ToPlayer())->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_MODEL_ID);
        }
    }

}

bool Creature::IsEscortNPC(bool onlyIfActive)
{
    if (CreatureAI* ai = AI())
        return ai->IsEscortNPC(onlyIfActive);

    return false;
}

VendorItemCount::VendorItemCount(uint32 _item, uint32 _count) : 
    itemId(_item), 
    count(_count), 
    lastIncrementTime(WorldGameTime::GetGameTime())
{}