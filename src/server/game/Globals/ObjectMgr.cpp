
#include "Common.h"
#include "Database/DatabaseEnv.h"

#include "Log.h"
#include "CreatureAIFactory.h"
#include "GameObjectAIFactory.h"
#include "MapManager.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "UpdateMask.h"
#include "World.h"
#include "WorldSession.h"
#include "Group.h"
#include "GroupMgr.h"
#include "Guild.h"
#include "ArenaTeam.h"
#include "Transport.h"
#include "Language.h"
#include "GameEventMgr.h"
#include "Spell.h"
#include "Chat.h"
#include "AccountMgr.h"
#include "InstanceSaveMgr.h"
#include "SpellAuras.h"
#include "Util.h"
#include "WaypointManager.h"
#include "InstanceScript.h" //for condition_instance_data
#include "ItemEnchantmentMgr.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "GameObject.h"
#include "Formulas.h"
#include "SpawnData.h"
#include "GameTime.h"
#include "QueryPackets.h"
#include "GuildMgr.h"
#include "ReputationMgr.h"

ScriptMapMap sQuestEndScripts;
ScriptMapMap sQuestStartScripts;
ScriptMapMap sSpellScripts;
ScriptMapMap sGameObjectScripts;
ScriptMapMap sEventScripts;
ScriptMapMap sWaypointScripts;

bool normalizePlayerName(std::string& name)
{
    if(name.empty())
        return false;

    wchar_t wstr_buf[MAX_INTERNAL_PLAYER_NAME+1];
    size_t wstr_len = MAX_INTERNAL_PLAYER_NAME;

    if(!Utf8toWStr(name,&wstr_buf[0],wstr_len))
        return false;

    wstr_buf[0] = wcharToUpper(wstr_buf[0]);
    for(size_t i = 1; i < wstr_len; ++i)
        wstr_buf[i] = wcharToLower(wstr_buf[i]);

    if(!WStrToUtf8(wstr_buf,wstr_len,name))
        return false;

    return true;
}

LanguageDesc lang_description[LANGUAGES_COUNT] =
{
    { LANG_ADDON,           0, 0                       },
    { LANG_UNIVERSAL,       0, 0                       },
    { LANG_ORCISH,        669, SKILL_LANG_ORCISH       },
    { LANG_DARNASSIAN,    671, SKILL_LANG_DARNASSIAN   },
    { LANG_TAURAHE,       670, SKILL_LANG_TAURAHE      },
    { LANG_DWARVISH,      672, SKILL_LANG_DWARVEN      },
    { LANG_COMMON,        668, SKILL_LANG_COMMON       },
    { LANG_DEMONIC,       815, SKILL_LANG_DEMON_TONGUE },
    { LANG_TITAN,         816, SKILL_LANG_TITAN        },
    { LANG_THALASSIAN,    813, SKILL_LANG_THALASSIAN   },
    { LANG_DRACONIC,      814, SKILL_LANG_DRACONIC     },
    { LANG_KALIMAG,       817, SKILL_LANG_OLD_TONGUE   },
    { LANG_GNOMISH,      7340, SKILL_LANG_GNOMISH      },
    { LANG_TROLL,        7341, SKILL_LANG_TROLL        },
    { LANG_GUTTERSPEAK, 17737, SKILL_LANG_GUTTERSPEAK  },
    { LANG_DRAENEI,     29932, SKILL_LANG_DRAENEI      },
    { LANG_ZOMBIE,          0, 0                       },
    { LANG_GNOMISH_BINARY,  0, 0                       },
    { LANG_GOBLIN_BINARY,   0, 0                       }
};

LanguageDesc const* GetLanguageDescByID(uint32 lang)
{
    for(auto & i : lang_description)
    {
        if(uint32(i.lang_id) == lang)
            return &i;
    }

    return nullptr;
}

ObjectMgr::ObjectMgr() :
    _creatureSpawnId(1),
    _gameObjectSpawnId(1),
    _hiPetNumber(1),
    _ItemTextId(1),
    _mailid(1),
    _auctionId(1),
    maxSpellId(0),
    _GMticketid(0),
    DBCLocaleIndex(LOCALE_enUS)
{
    for (uint8 i = 0; i < MAX_CLASSES; ++i)
    {
        _playerClassInfo[i] = nullptr;
        for (uint8 j = 0; j < MAX_RACES; ++j)
            _playerInfo[j][i] = nullptr;
    }
}

ObjectMgr::~ObjectMgr()
{
    _questTemplates.clear( );

    _areaTriggerStore.clear();

    for (auto & itr : m_mCacheVendorItemMap)
        itr.second.Clear();

    for (auto & itr : m_mCacheTrainerSpellMap)
        itr.second.Clear();

    for (auto itr : spellTemplates)
        delete itr.second;
}

CreatureTemplate const* ObjectMgr::GetCreatureTemplate(uint32 entry)
{
    return Trinity::Containers::MapGetValuePtr(_creatureTemplateStore, entry);
}

GameObjectTemplate const* ObjectMgr::GetGameObjectTemplate(uint32 entry)
{
    return Trinity::Containers::MapGetValuePtr(_gameObjectTemplateStore, entry);
}

void ObjectMgr::LoadCreatureLocales()
{
    uint32 oldMSTime = GetMSTime();

    _creatureLocaleStore.clear();                              // need for reload case
//                                                    0        1                     3                      5                      7                      9                       11                    13                      15                
    QueryResult result = WorldDatabase.Query("SELECT entry,name_loc1,subname_loc1,name_loc2,subname_loc2,name_loc3,subname_loc3,name_loc4,subname_loc4,name_loc5,subname_loc5,name_loc6,subname_loc6,name_loc7,subname_loc7,name_loc8,subname_loc8 FROM locales_creature");

    if(!result)
    {
        TC_LOG_INFO("sql.sql",">> Loaded 0 creature locale strings. DB table `locales_creature` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        CreatureLocale& data = _creatureLocaleStore[entry];

        for (uint8 i = MAX_LOCALE; i > 0; --i)
        {
            LocaleConstant locale = (LocaleConstant) i;
            AddLocaleString(fields[1 + 2 * (i - 1)].GetString(), locale, data.Name);
            AddLocaleString(fields[1 + 2 * (i - 1) + 1].GetString(), locale, data.Title);
        }
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u creature locale strings in %u ms", uint32(_creatureLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadGossipMenuItemsLocales()
{
    uint32 oldMSTime = GetMSTime();

    _gossipMenuItemsLocaleStore.clear();                              // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT menu_id, id,"
        "option_text_loc1,box_text_loc1,option_text_loc2,box_text_loc2,"
        "option_text_loc3,box_text_loc3,option_text_loc4,box_text_loc4,"
        "option_text_loc5,box_text_loc5,option_text_loc6,box_text_loc6,"
        "option_text_loc7,box_text_loc7,option_text_loc8,box_text_loc8 "
        "FROM locales_gossip_menu_option");

    if(!result)
    {
        TC_LOG_INFO("sql.sql",">> Loaded 0 locales_gossip_menu_option locale strings. DB table `locales_gossip_menu_option` is empty.");
        return;
    }

     do
    {
        Field* fields = result->Fetch();

        uint16 menuId   = fields[0].GetUInt16();
        uint16 id       = fields[1].GetUInt16();

        GossipMenuItemsLocale& data = _gossipMenuItemsLocaleStore[MAKE_PAIR32(menuId, id)];

        for (uint8 i = TOTAL_LOCALES - 1; i > 0; --i)
        {
            LocaleConstant locale = (LocaleConstant) i;
            AddLocaleString(fields[2 + 2 * (i - 1)].GetString(), locale, data.OptionText);
            AddLocaleString(fields[2 + 2 * (i - 1) + 1].GetString(), locale, data.BoxText);
        }
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u gossip_menu_option locale strings in %u ms", uint32(_gossipMenuItemsLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadCreatureTemplates(bool reload /* = false */)
{
    uint32 oldMSTime = GetMSTime();

    //                                                 
    QueryResult result = WorldDatabase.PQuery("SELECT entry, difficulty_entry_1, modelid1, modelid2, modelid3, "
                                             //   5
                                             "modelid4, name, subname, IconName, gossip_menu_id, minlevel, maxlevel, exp, faction, npcflag, speed_walk, speed_run, "
                                             //
                                             "scale, `rank`, dmgschool, BaseAttackTime, RangeAttackTime, BaseVariance, RangeVariance, unit_class, unit_flags, unit_flags2, dynamicflags, family,"
                                             //   
                                             "trainer_type, trainer_spell, trainer_class, trainer_race, type,"
                                             //  
                                             "type_flags, lootid, pickpocketloot, skinloot, resistance1, resistance2, resistance3, resistance4, resistance5, resistance6, spell1, "
                                             //
                                             "spell2, spell3, spell4, spell5, spell6, spell7, spell8, PetSpellDataId, mingold, maxgold, AIName, MovementType, "
                                             //           
                                             "HealthModifier, ManaModifier, ArmorModifier, DamageModifier, ExperienceModifier, RacialLeader, RegenHealth, "
                                             //   
                                             "mechanic_immune_mask, spell_school_immune_mask, flags_extra, ScriptName, ctm.Ground, ctm.Swim, ctm.Flight, ctm.Rooted, patch "
                                             //   
                                             "FROM creature_template ct LEFT JOIN creature_template_movement ctm ON ct.entry = ctm.CreatureId WHERE patch=(SELECT max(patch) FROM creature_template t2 WHERE ct.entry=t2.entry && patch <= %u)", sWorld->GetWowPatch());

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 creature template definitions. DB table `creature_template` is empty.");
        return;
    }

    if(!reload)
        _creatureTemplateStore.rehash(result->GetRowCount());

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();
        LoadCreatureTemplate(fields);
        ++count;
    }
    while (result->NextRow());

    // Checking needs to be done after loading because of the difficulty self referencing
    for (CreatureTemplateContainer::const_iterator itr = _creatureTemplateStore.begin(); itr != _creatureTemplateStore.end(); ++itr)
        CheckCreatureTemplate(&itr->second);

    TC_LOG_INFO("server.loading", ">> Loaded %u creature definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadCreatureTemplate(Field* fields)
{
    uint8 f = 0;
    uint32 entry = fields[f++].GetUInt32();

    CreatureTemplate& creatureTemplate = _creatureTemplateStore[entry];

    creatureTemplate.Entry = entry;
    creatureTemplate.difficulty_entry_1 = fields[f++].GetUInt32();

    creatureTemplate.Modelid1         = fields[f++].GetUInt32();
    creatureTemplate.Modelid2         = fields[f++].GetUInt32();
    creatureTemplate.Modelid3         = fields[f++].GetUInt32();
    creatureTemplate.Modelid4         = fields[f++].GetUInt32();
    creatureTemplate.Name             = fields[f++].GetString();
    creatureTemplate.Title            = fields[f++].GetString();
    creatureTemplate.IconName         = fields[f++].GetString();
    creatureTemplate.GossipMenuId     = fields[f++].GetUInt16();
    creatureTemplate.minlevel         = fields[f++].GetUInt8();
    creatureTemplate.maxlevel         = fields[f++].GetUInt8();
    creatureTemplate.expansion        = fields[f++].GetUInt16();
    creatureTemplate.faction          = fields[f++].GetUInt16();
    creatureTemplate.npcflag          = fields[f++].GetUInt32();
    creatureTemplate.speed_walk       = fields[f++].GetFloat();
    creatureTemplate.speed_run        = fields[f++].GetFloat();
    creatureTemplate.scale            = fields[f++].GetFloat();
    creatureTemplate.rank             = fields[f++].GetUInt8();
    creatureTemplate.dmgschool        = uint32(fields[f++].GetInt8());
    creatureTemplate.baseattacktime   = fields[f++].GetUInt32();
    creatureTemplate.rangeattacktime  = fields[f++].GetUInt32();
    creatureTemplate.BaseVariance     = fields[f++].GetFloat();
    creatureTemplate.RangeVariance    = fields[f++].GetFloat();
    creatureTemplate.unit_class       = fields[f++].GetUInt8();
    creatureTemplate.unit_flags       = fields[f++].GetUInt32();
    creatureTemplate.unit_flags2      = fields[f++].GetUInt32();
    creatureTemplate.dynamicflags     = fields[f++].GetUInt32();
    creatureTemplate.family           = CreatureFamily(fields[f++].GetUInt8());
    creatureTemplate.trainer_type     = fields[f++].GetUInt8();
    creatureTemplate.trainer_spell    = fields[f++].GetUInt32();
    creatureTemplate.trainer_class    = fields[f++].GetUInt8();
    creatureTemplate.trainer_race     = fields[f++].GetUInt8();
    creatureTemplate.type             = fields[f++].GetUInt8();
    creatureTemplate.type_flags       = fields[f++].GetUInt32();
    creatureTemplate.lootid           = fields[f++].GetUInt32();
    creatureTemplate.pickpocketLootId = fields[f++].GetUInt32();
    creatureTemplate.SkinLootId       = fields[f++].GetUInt32();

    for (int & i : creatureTemplate.resistance) //SPELL_SCHOOL_NORMAL is not handled here so, -1
        i = fields[f++].GetInt16();

    for (uint32 & spell : creatureTemplate.spells)
        spell = fields[f++].GetUInt32();

    creatureTemplate.PetSpellDataId = fields[f++].GetUInt32();
    creatureTemplate.mingold        = fields[f++].GetUInt32();
    creatureTemplate.maxgold        = fields[f++].GetUInt32();
    creatureTemplate.AIName         = fields[f++].GetString();
    creatureTemplate.MovementType   = fields[f++].GetUInt8();
    creatureTemplate.ModHealth      = fields[f++].GetFloat();
    creatureTemplate.ModMana        = fields[f++].GetFloat();
    creatureTemplate.ModArmor       = fields[f++].GetFloat();
    creatureTemplate.ModDamage      = fields[f++].GetFloat();
    creatureTemplate.ModExperience  = fields[f++].GetFloat();
    creatureTemplate.RacialLeader   = fields[f++].GetBool();
    creatureTemplate.RegenHealth    = fields[f++].GetBool();
    creatureTemplate.MechanicImmuneMask = fields[f++].GetUInt32();
    creatureTemplate.SpellSchoolImmuneMask = fields[f++].GetUInt32();
    creatureTemplate.flags_extra    = fields[f++].GetUInt32();
    creatureTemplate.ScriptID       = GetScriptId(fields[f++].GetCString());

    if (!fields[f].IsNull())
        creatureTemplate.Movement.Ground = static_cast<CreatureGroundMovementType>(fields[f].GetUInt8());
    f++;
    if (!fields[f].IsNull())
        creatureTemplate.Movement.Swim = fields[f].GetBool();
    f++;
    if (!fields[f].IsNull())
        creatureTemplate.Movement.Flight = static_cast<CreatureFlightMovementType>(fields[f].GetUInt8());
    f++;
    if (!fields[f].IsNull())
        creatureTemplate.Movement.Rooted = fields[f].GetBool();
    f++;
}

void ObjectMgr::CheckCreatureTemplate(CreatureTemplate const* cInfo)
{
    if(cInfo->difficulty_entry_1)
    {
        uint32 diff = DUNGEON_DIFFICULTY_HEROIC;
        uint32 i = cInfo->Entry;
        CreatureTemplate const* difficultyInfo = GetCreatureTemplate(cInfo->difficulty_entry_1);
        if(!difficultyInfo)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) have `difficulty_entry_1`=%u but the creature does not exist.", i, cInfo->difficulty_entry_1);
        }
        /*
        if(heroicEntries.find(i)!=heroicEntries.end())
        {
            TC_LOG_ERROR("sql.sql","Creature (Entry: %u) listed as heroic but have value in `heroic_entry`.",i);
        }
        if(heroicEntries.find(cInfo->difficulty_entry_1)!=heroicEntries.end())
        {
            TC_LOG_ERROR("sql.sql","Creature (Entry: %u) already listed as heroic for another entry.",cInfo->difficulty_entry_1);
        }
        if(hasHeroicEntries.find(cInfo->difficulty_entry_1)!=hasHeroicEntries.end())
        {
            TC_LOG_ERROR("sql.sql","Creature (Entry: %u) have `difficulty_entry_1`=%u but creature entry %u have heroic entry also.",i,cInfo->difficulty_entry_1,cInfo->difficulty_entry_1);
        }*/

        if(difficultyInfo)
        {
        if(cInfo->npcflag != difficultyInfo->npcflag)
        {
            TC_LOG_ERROR("sql.sql","Creature (Entry: %u) listed in `creature_template_substitution` has different `npcflag` in heroic mode.",i);
        }

        if(cInfo->trainer_class != difficultyInfo->trainer_class)
        {
            TC_LOG_ERROR("sql.sql","Creature (Entry: %u) listed in `creature_template_substitution` has different `trainer_class` in heroic mode.",i);
        }

        if(cInfo->trainer_race != difficultyInfo->trainer_race)
        {
            TC_LOG_ERROR("sql.sql","Creature (Entry: %u) listed in `creature_template_substitution` has different `race` in heroic mode.",i);
        }

        if(cInfo->trainer_type != difficultyInfo->trainer_type)
        {
            TC_LOG_ERROR("sql.sql","Creature (Entry: %u) listed in `creature_template_substitution` has different `trainer_type` in heroic mode.",i);
        }

        if(cInfo->trainer_spell != difficultyInfo->trainer_spell)
        {
            TC_LOG_ERROR("sql.sql","Creature (Entry: %u) listed in `creature_template_substitution` has different `trainer_spell` in heroic mode.",i);
        }

        uint32 differenceMask = cInfo->npcflag ^ difficultyInfo->npcflag;
        if (cInfo->npcflag != difficultyInfo->npcflag)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u, `npcflag`: %u) has different `npcflag` in difficulty %u mode (Entry: %u, `npcflag`: %u).",
                cInfo->Entry, cInfo->npcflag, diff + 1, cInfo->difficulty_entry_1, difficultyInfo->npcflag);
            TC_LOG_ERROR("sql.sql", "Possible FIX: UPDATE `creature_template` SET `npcflag`=`npcflag`^%u WHERE `entry`=%u;",
                differenceMask, cInfo->difficulty_entry_1);
        }

        if (cInfo->dmgschool != difficultyInfo->dmgschool)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u, `dmgschool`: %u) has different `dmgschool` in difficulty %u mode (Entry: %u, `dmgschool`: %u).",
                cInfo->Entry, cInfo->dmgschool, diff + 1, cInfo->difficulty_entry_1, difficultyInfo->dmgschool);
            TC_LOG_ERROR("sql.sql", "Possible FIX: UPDATE `creature_template` SET `dmgschool`=%u WHERE `entry`=%u;",
                cInfo->dmgschool, cInfo->difficulty_entry_1);
        }

        differenceMask = cInfo->unit_flags2 ^ difficultyInfo->unit_flags2;
        if (cInfo->unit_flags2 != difficultyInfo->unit_flags2)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u, `unit_flags2`: %u) has different `unit_flags2` in difficulty %u mode (Entry: %u, `unit_flags2`: %u).",
                cInfo->Entry, cInfo->unit_flags2, diff + 1, cInfo->difficulty_entry_1, difficultyInfo->unit_flags2);
            TC_LOG_ERROR("sql.sql", "Possible FIX: UPDATE `creature_template` SET `unit_flags2`=`unit_flags2`^%u WHERE `entry`=%u;",
                differenceMask, cInfo->difficulty_entry_1);
        }

        /*
        hasHeroicEntries.insert(i);
        heroicEntries.insert(cInfo->difficulty_entry_1);**/
        }
    }

    if (cInfo->mingold > cInfo->maxgold)
    {
        TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has `mingold` %u which is greater than `maxgold` %u, setting `maxgold` to %u.",
            cInfo->Entry, cInfo->mingold, cInfo->maxgold, cInfo->mingold);
        const_cast<CreatureTemplate*>(cInfo)->maxgold = cInfo->mingold;
    }

    if (!cInfo->AIName.empty())
    {
        auto registryItem = sCreatureAIRegistry->GetRegistryItem(cInfo->AIName);
        if (!registryItem)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has non-registered `AIName` '%s' set, removing", cInfo->Entry, cInfo->AIName.c_str());
            const_cast<CreatureTemplate*>(cInfo)->AIName.clear();
        }
        else
        {
            DBPermit const* permit = dynamic_cast<DBPermit const*>(registryItem);
            if (!ASSERT_NOTNULL(permit)->IsScriptNameAllowedInDB())
            {
                TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has not-allowed `AIName` '%s' set, removing", cInfo->Entry, cInfo->AIName.c_str());
                const_cast<CreatureTemplate*>(cInfo)->AIName.clear();
            }
        }
    }

    if (!cInfo->AIName.empty() && !sCreatureAIRegistry->HasItem(cInfo->AIName))
    {
        TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has non-registered `AIName` '%s' set, removing", cInfo->Entry, cInfo->AIName.c_str());
        const_cast<CreatureTemplate*>(cInfo)->AIName.clear();
    }

    FactionTemplateEntry const* factionTemplate = sFactionTemplateStore.LookupEntry(cInfo->faction);
    if(!factionTemplate) {
        TC_LOG_FATAL("sql.sql", "Creature (Entry: %u) has non-existing faction template (%u). This can lead to crashes, aborting.", cInfo->Entry, cInfo->faction);
        ABORT();
    }

    // check model ids, supplying and sending non-existent ids to the client might crash them
    if(cInfo->Modelid1 && !GetCreatureModelInfo(cInfo->Modelid1))
    {
        TC_LOG_ERROR("sql.sql","Creature (Entry: %u) has non-existing Modelid1 (%u), setting it to 0", cInfo->Entry, cInfo->Modelid1);
        const_cast<CreatureTemplate*>(cInfo)->Modelid1 = 0;
    }
    if(cInfo->Modelid2 && !GetCreatureModelInfo(cInfo->Modelid2))
    {
        TC_LOG_ERROR("sql.sql","Creature (Entry: %u) has non-existing Modelid2 (%u), setting it to 0", cInfo->Entry, cInfo->Modelid2);
        const_cast<CreatureTemplate*>(cInfo)->Modelid2 = 0;
    }
    if(cInfo->Modelid3 && !GetCreatureModelInfo(cInfo->Modelid3))
    {
        TC_LOG_ERROR("sql.sql","Creature (Entry: %u) has non-existing Modelid3 (%u), setting it to 0", cInfo->Entry, cInfo->Modelid3);
        const_cast<CreatureTemplate*>(cInfo)->Modelid3 = 0;
    }
    if(cInfo->Modelid4 && !GetCreatureModelInfo(cInfo->Modelid4))
    {
        TC_LOG_ERROR("sql.sql","Creature (Entry: %u) has non-existing Modelid4 (%u), setting it to 0", cInfo->Entry, cInfo->Modelid4);
        const_cast<CreatureTemplate*>(cInfo)->Modelid4 = 0;
    }

    if(cInfo->dmgschool >= MAX_SPELL_SCHOOL)
    {
        TC_LOG_ERROR("sql.sql","Creature (Entry: %u) has invalid spell school value (%u) in `dmgschool`",cInfo->Entry,cInfo->dmgschool);
        const_cast<CreatureTemplate*>(cInfo)->dmgschool = SPELL_SCHOOL_NORMAL;
    }

    if(cInfo->baseattacktime == 0)
        const_cast<CreatureTemplate*>(cInfo)->baseattacktime  = BASE_ATTACK_TIME;

    if(cInfo->rangeattacktime == 0)
        const_cast<CreatureTemplate*>(cInfo)->rangeattacktime = BASE_ATTACK_TIME;

    if((cInfo->npcflag & UNIT_NPC_FLAG_TRAINER) && cInfo->trainer_type >= MAX_TRAINER_TYPE)
        TC_LOG_ERROR("sql.sql","Creature (Entry: %u) has wrong trainer type %u",cInfo->Entry,cInfo->trainer_type);

    if (cInfo->speed_walk == 0.0f)
    {
        TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has wrong value (%f) in speed_walk, set to 1.", cInfo->Entry, cInfo->speed_walk);
        const_cast<CreatureTemplate*>(cInfo)->speed_walk = 1.0f;
    }

    if (cInfo->speed_run == 0.0f)
    {
        TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has wrong value (%f) in speed_run, set to 1.14286.", cInfo->Entry, cInfo->speed_run);
        const_cast<CreatureTemplate*>(cInfo)->speed_run = 1.14286f;
    }

    CheckCreatureMovement("creature_template_movement", cInfo->Entry, const_cast<CreatureTemplate*>(cInfo)->Movement);

    if(cInfo->PetSpellDataId)
    {
        CreatureSpellDataEntry const* spellDataId = sCreatureSpellDataStore.LookupEntry(cInfo->PetSpellDataId);
        if(!spellDataId)
            TC_LOG_ERROR("sql.sql","Creature (Entry: %u) has non-existing PetSpellDataId (%u)", cInfo->Entry, cInfo->PetSpellDataId);
    }


    for (uint8 j = 0; j < MAX_CREATURE_SPELLS; ++j)
    {
        if (cInfo->spells[j] && !sSpellMgr->GetSpellInfo(cInfo->spells[j]))
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has non-existing Spell%d (%u), set to 0.", cInfo->Entry, j + 1, cInfo->spells[j]);
            const_cast<CreatureTemplate*>(cInfo)->spells[j] = 0;
        }
    }

    if(cInfo->MovementType >= MAX_DB_MOTION_TYPE)
    {
        TC_LOG_ERROR("sql.sql","Creature (Entry: %u) has wrong movement generator type (%u), ignore and set to IDLE.",cInfo->Entry,cInfo->MovementType);
        const_cast<CreatureTemplate*>(cInfo)->MovementType = IDLE_MOTION_TYPE;
    }

    /// if not set custom creature scale then load scale from CreatureDisplayInfo.dbc
    if(cInfo->scale <= 0.0f)
    {
        uint32 modelid = cInfo->GetFirstValidModelId();
        CreatureDisplayInfoEntry const* ScaleEntry = sCreatureDisplayInfoStore.LookupEntry(modelid);
        const_cast<CreatureTemplate*>(cInfo)->scale = ScaleEntry ? ScaleEntry->scale : 1.0f;
    }

    if (cInfo->expansion > (MAX_EXPANSIONS - 1))
    {
        TC_LOG_ERROR("sql.sql", "Table `creature_template` lists creature (Entry: %u) with expansion %u. Ignored and set to 0.", cInfo->Entry, cInfo->expansion);
        const_cast<CreatureTemplate*>(cInfo)->expansion = 0;
    }

    if (uint32 badFlags = (cInfo->flags_extra & ~CREATURE_FLAG_EXTRA_DB_ALLOWED))
    {
        TC_LOG_ERROR("sql.sql", "Table `creature_template` lists creature (Entry: %u) with disallowed `flags_extra` %u, removing incorrect flag.", cInfo->Entry, badFlags);
        const_cast<CreatureTemplate*>(cInfo)->flags_extra &= CREATURE_FLAG_EXTRA_DB_ALLOWED;
    }
}

void ObjectMgr::CheckCreatureMovement(char const* table, uint64 id, CreatureMovementData& creatureMovement)
{
    if (creatureMovement.Ground >= CreatureGroundMovementType::Max)
    {
        TC_LOG_ERROR("sql.sql", "`%s`.`Ground` wrong value (%u) for Id " UI64FMTD ", setting to Run.",
            table, uint32(creatureMovement.Ground), id);
        creatureMovement.Ground = CreatureGroundMovementType::Run;
    }

    if (creatureMovement.Flight >= CreatureFlightMovementType::Max)
    {
        TC_LOG_ERROR("sql.sql", "`%s`.`Flight` wrong value (%u) for Id " UI64FMTD ", setting to None.",
            table, uint32(creatureMovement.Flight), id);
        creatureMovement.Flight = CreatureFlightMovementType::None;
    }
}

void ObjectMgr::LoadCreatureAddons()
{
    uint32 oldMSTime = GetMSTime();

    //                                                0       1        2      3       4       5      6          7
    QueryResult result = WorldDatabase.PQuery("SELECT spawnID, path_id, mount, bytes1, bytes2, emote, moveflags, auras "
                                              "FROM creature_addon t1 WHERE patch=(SELECT max(patch) FROM creature_addon t2 WHERE t1.spawnID = t2.spawnID && patch <= %u)", sWorld->GetWowPatch());

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 creature addon definitions. DB table `creature_addon` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        ObjectGuid::LowType guid = fields[0].GetUInt32();

        CreatureData const* creData = GetCreatureData(guid);
        if (!creData)
        {
            TC_LOG_ERROR("sql.sql", "Creature (SpawnId: %u) does not exist but has a record in `creature_addon`", guid);
            continue;
        }

        CreatureAddon& creatureAddon = _creatureAddonStore[guid];

        creatureAddon.path_id = fields[1].GetUInt32();
        if (creData->movementType == WAYPOINT_MOTION_TYPE && !creatureAddon.path_id)
        {
            const_cast<CreatureData*>(creData)->movementType = IDLE_MOTION_TYPE;
            TC_LOG_ERROR("sql.sql", "Creature (SpawnId %u) has movement type set to WAYPOINT_MOTION_TYPE but no path assigned", guid);
        }

        creatureAddon.mount   = fields[2].GetUInt32();
        creatureAddon.bytes1  = fields[3].GetUInt32();
        creatureAddon.bytes2  = fields[4].GetUInt32();
        creatureAddon.emote   = fields[5].GetUInt32();
        creatureAddon.move_flags = fields[6].GetUInt32();

        Tokenizer tokens(fields[7].GetString(), ' ');
        uint8 i = 0;
        creatureAddon.auras.resize(tokens.size());
        for (auto token : tokens)
        {
            SpellInfo const* AdditionalSpellInfo = sSpellMgr->GetSpellInfo(uint32(atol(token)));
            if (!AdditionalSpellInfo)
            {
                TC_LOG_ERROR("sql.sql", "Creature (SpawnId: %u) has wrong spell %u defined in `auras` field in `creature_addon`.", guid, uint32(atol(token)));
                continue;
            }
#ifdef LICH_KING
            if (AdditionalSpellInfo->HasAuraEffect(SPELL_AURA_CONTROL_VEHICLE))
                TC_LOG_ERROR("sql.sql", "Creature (SpawnId: %u) has SPELL_AURA_CONTROL_VEHICLE aura %u defined in `auras` field in `creature_addon`.", guid, uint32(atol(*itr)));
#endif
            creatureAddon.auras[i++] = uint32(atol(token));
        }

        if (creatureAddon.mount)
        {
            if (!sCreatureDisplayInfoStore.LookupEntry(creatureAddon.mount))
            {
                TC_LOG_ERROR("sql.sql", "Creature (SpawnId: %u) has invalid displayInfoId (%u) for mount defined in `creature_addon`", guid, creatureAddon.mount);
                creatureAddon.mount = 0;
            }
        }

        if (!sEmotesStore.LookupEntry(creatureAddon.emote))
        {
            TC_LOG_ERROR("sql.sql", "Creature (SpawnId: %u) has invalid emote (%u) defined in `creature_addon`.", guid, creatureAddon.emote);
            creatureAddon.emote = 0;
        }

        if(_creatureDataStore.find(guid)==_creatureDataStore.end())
            TC_LOG_ERROR("sql.sql","Creature (SpawnId: %u) does not exist but has a record in `creature_addon`",guid);

        //sun: check move flags
        if (creatureAddon.move_flags & MOVEMENTFLAG_SPLINE_ENABLED)
        {
            TC_LOG_ERROR("sql.sql", "Creature (SpawnId: %u) have invalid flag MOVEMENTFLAG_SPLINE_ENABLED in moveflags, removed", guid);
            creatureAddon.move_flags = creatureAddon.move_flags & ~MOVEMENTFLAG_SPLINE_ENABLED;
        }

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u creature addons in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

CreatureAddon const* ObjectMgr::GetCreatureAddon(ObjectGuid::LowType lowguid) const
{
    return Trinity::Containers::MapGetValuePtr(_creatureAddonStore, lowguid);
}

void ObjectMgr::LoadCreatureTemplateAddons()
{
    uint32 oldMSTime = GetMSTime();

    //                                                0       1       2      3       4       5        6         7
    QueryResult result = WorldDatabase.PQuery("SELECT entry, path_id, mount, bytes1, bytes2, emote, moveflags, auras "
                                              "FROM creature_template_addon t1 WHERE patch=(SELECT max(patch) FROM creature_template_addon t2 WHERE t1.entry = t2.entry && patch <= %u)", sWorld->GetWowPatch());

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 creature template addon definitions. DB table `creature_template_addon` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        if (!sObjectMgr->GetCreatureTemplate(entry))
        {
            TC_LOG_ERROR("sql.sql", "Creature template (Entry: %u) does not exist but has a record in `creature_template_addon`", entry);
            continue;
        }

        CreatureAddon& creatureAddon = _creatureTemplateAddonStore[entry];

        creatureAddon.path_id = fields[1].GetUInt32();
        creatureAddon.mount   = fields[2].GetUInt32();
        creatureAddon.bytes1  = fields[3].GetUInt32();
        creatureAddon.bytes2  = fields[4].GetUInt32();
        creatureAddon.emote   = fields[5].GetUInt32();
        creatureAddon.move_flags = fields[6].GetUInt32();

        Tokenizer tokens(fields[7].GetString(), ' ');
        uint8 i = 0;
        creatureAddon.auras.resize(tokens.size());
        for (auto token : tokens)
        {
            SpellInfo const* AdditionalSpellInfo = sSpellMgr->GetSpellInfo(uint32(atol(token)));
            if (!AdditionalSpellInfo)
            {
                TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has wrong spell %u defined in `auras` field in `creature_template_addon`.", entry, uint32(atol(token)));
                continue;
            }
#ifdef LICH_KING
            if (AdditionalSpellInfo->HasAuraEffect(SPELL_AURA_CONTROL_VEHICLE))
                TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has SPELL_AURA_CONTROL_VEHICLE aura %u defined in `auras` field in `creature_template_addon`.", entry, uint32(atol(*itr)));
#endif
            creatureAddon.auras[i++] = uint32(atol(token));
        }

        if (creatureAddon.mount)
        {
            if (!sCreatureDisplayInfoStore.LookupEntry(creatureAddon.mount))
            {
                TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has invalid displayInfoId (%u) for mount defined in `creature_template_addon`", entry, creatureAddon.mount);
                creatureAddon.mount = 0;
            }
        }

        if (!sEmotesStore.LookupEntry(creatureAddon.emote))
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) has invalid emote (%u) defined in `creature_template_addon`.", entry, creatureAddon.emote);
            creatureAddon.emote = 0;
        }

        //sun: check move flags
        if (creatureAddon.move_flags & MOVEMENTFLAG_SPLINE_ENABLED)
        {
            TC_LOG_ERROR("sql.sql", "Creature (Entry: %u) have invalid flag MOVEMENTFLAG_SPLINE_ENABLED in moveflags, removed", entry);
            creatureAddon.move_flags = creatureAddon.move_flags & ~MOVEMENTFLAG_SPLINE_ENABLED;
        }

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u creature template addons in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

CreatureAddon const* ObjectMgr::GetCreatureTemplateAddon(uint32 entry) const
{
    return Trinity::Containers::MapGetValuePtr(_creatureTemplateAddonStore, entry);
}

CreatureMovementData const* ObjectMgr::GetCreatureMovementOverride(ObjectGuid::LowType spawnId) const
{
    return Trinity::Containers::MapGetValuePtr(_creatureMovementOverrides, spawnId);
}

EquipmentInfo const* ObjectMgr::GetEquipmentInfo(uint32 entry, int8& id) const
{
    EquipmentInfoContainer::const_iterator itr = _equipmentInfoStore.find(entry);
    if (itr == _equipmentInfoStore.end())
        return nullptr;

    if (itr->second.empty())
        return NULL;

    if (id == -1) // select a random element
    {
        EquipmentInfoContainerInternal::const_iterator ritr = itr->second.begin();
        std::advance(ritr, urand(0u, itr->second.size() - 1));
        id = std::distance(itr->second.begin(), ritr) + 1;
        return &ritr->second;
    }
    else
    {
        EquipmentInfoContainerInternal::const_iterator itr2 = itr->second.find(id);
        if (itr2 != itr->second.end())
            return &itr2->second;
    }

    return nullptr;
}

void ObjectMgr::LoadEquipmentTemplates()
{
    uint32 oldMSTime = GetMSTime();

    //                                                    0      1        2           3            4           5           6            7          8           9         10
    QueryResult result = WorldDatabase.PQuery("SELECT creatureID, id, equipmodel1, equipmodel2, equipmodel3, equipinfo1, equipinfo2, equipinfo3, equipslot1, equipslot2, equipslot3 "
                                              "FROM creature_equip_template t1 WHERE patch=(SELECT max(patch) FROM creature_equip_template t2 WHERE t1.creatureID = t2.creatureID && patch <= %u)", sWorld->GetWowPatch());

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 creature equipment templates. DB table `creature_equip_template` is empty!");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();
        if (!sObjectMgr->GetCreatureTemplate(entry))
        {
            TC_LOG_ERROR("sql.sql", "Creature template (Entry: %u) does not exist but has a record in `creature_equip_template`", entry);
            continue;
        }
        uint8 id = fields[1].GetUInt8();
        
        EquipmentInfo& equipmentInfo = _equipmentInfoStore[entry][id];

        equipmentInfo.equipmodel[0] = fields[2].GetUInt32();
        equipmentInfo.equipmodel[1] = fields[3].GetUInt32();
        equipmentInfo.equipmodel[2] = fields[4].GetUInt32();
        equipmentInfo.equipinfo[0] = fields[5].GetUInt32();
        equipmentInfo.equipinfo[1] = fields[6].GetUInt32();
        equipmentInfo.equipinfo[2] = fields[7].GetUInt32();
        equipmentInfo.equipslot[0] = fields[8].GetUInt32();
        equipmentInfo.equipslot[1] = fields[9].GetUInt32();
        equipmentInfo.equipslot[2] = fields[10].GetUInt32();

#ifdef LICH_KING
        for (uint8 i = 0; i < MAX_EQUIPMENT_ITEMS; ++i)
        {
            if (!equipmentInfo.ItemEntry[i])
                continue;

            ItemEntry const* dbcItem = sItemStore.LookupEntry(equipmentInfo.ItemEntry[i]);

            if (!dbcItem)
            {
                TC_LOG_ERROR("sql.sql", "Unknown item (entry=%u) in creature_equip_template.itemEntry%u for entry = %u and id=%u, forced to 0.",
                    equipmentInfo.ItemEntry[i], i+1, entry, id);
                equipmentInfo.ItemEntry[i] = 0;
                continue;
            }

            if (dbcItem->InventoryType != INVTYPE_WEAPON &&
                dbcItem->InventoryType != INVTYPE_SHIELD &&
                dbcItem->InventoryType != INVTYPE_RANGED &&
                dbcItem->InventoryType != INVTYPE_2HWEAPON &&
                dbcItem->InventoryType != INVTYPE_WEAPONMAINHAND &&
                dbcItem->InventoryType != INVTYPE_WEAPONOFFHAND &&
                dbcItem->InventoryType != INVTYPE_HOLDABLE &&
                dbcItem->InventoryType != INVTYPE_THROWN &&
                dbcItem->InventoryType != INVTYPE_RANGEDRIGHT)
            {
                TC_LOG_ERROR("sql.sql", "Item (entry=%u) in creature_equip_template.itemEntry%u for entry = %u and id = %u is not equipable in a hand, forced to 0.",
                    equipmentInfo.ItemEntry[i], i+1, entry, id);
                equipmentInfo.ItemEntry[i] = 0;
            }
        }
#endif

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u equipment templates in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

ItemTemplate const* ObjectMgr::GetItemTemplate(uint32 entry)
{
    return Trinity::Containers::MapGetValuePtr(_itemTemplateStore, entry);
}

void ObjectMgr::LoadCreatureMovementOverrides()
{
    uint32 oldMSTime = GetMSTime();

    _creatureMovementOverrides.clear();

    QueryResult result = WorldDatabase.Query("SELECT SpawnId, Ground, Swim, Flight, Rooted from creature_movement_override");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 creature movement overrides. DB table `creature_movement_override` is empty!");
        return;
    }

    do
    {
        Field* fields = result->Fetch();
        ObjectGuid::LowType spawnId = fields[0].GetUInt32();
        if (!GetCreatureData(spawnId))
        {
            TC_LOG_ERROR("sql.sql", "Creature (GUID: %u) does not exist but has a record in `creature_movement_override`", spawnId);
            continue;
        }

        CreatureMovementData& movement = _creatureMovementOverrides[spawnId];
        movement.Ground = static_cast<CreatureGroundMovementType>(fields[1].GetUInt8());
        movement.Swim = fields[2].GetBool();
        movement.Flight = static_cast<CreatureFlightMovementType>(fields[3].GetUInt8());
        movement.Rooted = fields[4].GetBool();

        CheckCreatureMovement("creature_movement_override", spawnId, movement);
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded " SZFMTD " movement overrides in %u ms", _creatureMovementOverrides.size(), GetMSTimeDiffToNow(oldMSTime));
}

CreatureModelInfo const* ObjectMgr::GetCreatureModelInfo(uint32 modelId)
{
    return Trinity::Containers::MapGetValuePtr(_creatureModelStore, modelId);
}

uint32 ObjectMgr::ChooseDisplayId(const CreatureTemplate *cinfo, const CreatureData *data)
{
    // Load creature model (display id)
    if (data && data->displayid)
        return data->displayid;

    if (!(cinfo->flags_extra & CREATURE_FLAG_EXTRA_TRIGGER))
        return cinfo->GetRandomValidModelId();

    // Triggers by default receive the invisible model
    return cinfo->GetFirstInvisibleModel();
}

void ObjectMgr::ChooseCreatureFlags(CreatureTemplate const* cinfo, uint32& npcflag, uint32& unit_flags, uint32& dynamicflags, CreatureData const* data /*= nullptr*/)
{
    npcflag = cinfo->npcflag;
    unit_flags = cinfo->unit_flags;
    dynamicflags = cinfo->dynamicflags;

    if (data)
    {
        if (data->unit_flags)
            unit_flags = data->unit_flags;

        /*TC
        if (data->npcflag)
            npcflag = data->npcflag;

        if (data->dynamicflags)
            dynamicflags = data->dynamicflags;
            */
    }
}

CreatureModelInfo const* ObjectMgr::GetCreatureModelRandomGender(uint32& displayID)
{
    CreatureModelInfo const* modelInfo = GetCreatureModelInfo(displayID);
    if (!modelInfo)
        return nullptr;

    // If a model for another gender exists, 50% chance to use it
    if (modelInfo->modelid_other_gender != 0 && urand(0, 1) == 0)
    {
        CreatureModelInfo const* minfo_tmp = GetCreatureModelInfo(modelInfo->modelid_other_gender);
        if (!minfo_tmp)
            TC_LOG_ERROR("sql.sql", "Model (Entry: %u) has modelid_other_gender %u not found in table `creature_model_info`. ", displayID, modelInfo->modelid_other_gender);
        else
        {
            // Model ID changed
            displayID = modelInfo->modelid_other_gender;
            return minfo_tmp;
        }
    }

    return modelInfo;
}

CreatureModelInfo const* ObjectMgr::GetCreatureModelSameGenderAndRaceAs(uint32& displayID, uint32 baseDisplayId)
{
    CreatureModelInfo const* modelInfo = GetCreatureModelInfo(displayID);
    if (!modelInfo)
        return nullptr;

    CreatureModelInfo const* baseModelInfo = GetCreatureModelInfo(baseDisplayId);
    if (!baseModelInfo)
        return modelInfo;

    uint8 baseGender = baseModelInfo->gender;
    //uint8 baseRace = modelInfo->race;

    if (modelInfo->gender == baseGender)
        return modelInfo;

    if (modelInfo->modelid_other_gender)
    {
        CreatureModelInfo const* modelInfoOtherGender = GetCreatureModelInfo(modelInfo->modelid_other_gender);
        if (modelInfoOtherGender)
        {
            if (modelInfoOtherGender->gender == baseGender)
                return modelInfoOtherGender;
        } else {
            TC_LOG_ERROR("sql.sql", "Model (Entry: %u) has modelid_other_gender %u not found in table `creature_model_info`. ", displayID, modelInfo->modelid_other_gender);
        }
    }

    //nothing found, return standard modelInfo
    return modelInfo;
}

void ObjectMgr::LoadCreatureModelInfo()
{
    _creatureModelStore.clear();

    uint32 oldMSTime = GetMSTime();

    QueryResult result = WorldDatabase.Query("SELECT modelid, bounding_radius, combat_reach, gender, modelid_other_gender FROM creature_model_info");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 creature model definitions. DB table `creature_model_info` is empty.");
        return;
    }

    _creatureModelStore.rehash(result->GetRowCount());
    uint32 count = 0;

    // List of model FileDataIDs that the client treats as invisible stalker
    uint32 trigggerCreatureModelFileID[5] = { 124640, 124641, 124642, 343863, 439302 };

    do
    {
        Field* fields = result->Fetch();

        uint32 displayId = fields[0].GetUInt32();

        CreatureDisplayInfoEntry const* creatureDisplay = sCreatureDisplayInfoStore.LookupEntry(displayId);
        if (!creatureDisplay)
        {
            TC_LOG_ERROR("sql.sql", "Table `creature_model_info` has a non-existent DisplayID (ID: %u). Skipped.", displayId);
            continue;
        }

        CreatureModelInfo& modelInfo = _creatureModelStore[displayId];

        modelInfo.bounding_radius        = fields[1].GetFloat();
        modelInfo.combat_reach           = fields[2].GetFloat();
        modelInfo.gender                 = fields[3].GetUInt8();
        modelInfo.modelid_other_gender   = fields[4].GetUInt32();
        modelInfo.is_trigger             = false;

        // Checks

        if (modelInfo.gender > GENDER_NONE)
        {
            TC_LOG_ERROR("sql.sql", "Table `creature_model_info` has wrong gender (%u) for display id (%u).", uint32(modelInfo.gender), displayId);
            modelInfo.gender = GENDER_MALE;
        }

        if (modelInfo.modelid_other_gender && !sCreatureDisplayInfoStore.LookupEntry(modelInfo.modelid_other_gender))
        {
            TC_LOG_ERROR("sql.sql", "Table `creature_model_info` has not existed alt.gender model (%u) for existed display id (%u).", modelInfo.modelid_other_gender, displayId);
            modelInfo.modelid_other_gender = 0;
        }

        if (modelInfo.combat_reach < 0.1f)
            modelInfo.combat_reach = DEFAULT_PLAYER_COMBAT_REACH;

        if (CreatureModelDataEntry const* modelData = sCreatureModelDataStore.LookupEntry(creatureDisplay->ModelId))
            modelInfo.is_trigger = strstr(modelData->ModelPath, "InvisibleStalker") != nullptr;

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u creature model based info in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadTempSummons()
{
    uint32 oldMSTime = GetMSTime();

    _tempSummonDataStore.clear();   // needed for reload case

                                    //                                               0           1             2        3      4           5           6           7            8           9
    QueryResult result = WorldDatabase.Query("SELECT summonerId, summonerType, groupId, entry, position_x, position_y, position_z, orientation, summonType, summonTime FROM creature_summon_groups");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 temp summons. DB table `creature_summon_groups` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 summonerId = fields[0].GetUInt32();
        SummonerType summonerType = SummonerType(fields[1].GetUInt8());
        uint8 group = fields[2].GetUInt8();

        switch (summonerType)
        {
        case SUMMONER_TYPE_CREATURE:
            if (!GetCreatureTemplate(summonerId))
            {
                TC_LOG_ERROR("sql.sql", "Table `creature_summon_groups` has summoner with non existing entry %u for creature summoner type, skipped.", summonerId);
                continue;
            }
            break;
        case SUMMONER_TYPE_GAMEOBJECT:
            if (!GetGameObjectTemplate(summonerId))
            {
                TC_LOG_ERROR("sql.sql", "Table `creature_summon_groups` has summoner with non existing entry %u for gameobject summoner type, skipped.", summonerId);
                continue;
            }
            break;
        case SUMMONER_TYPE_MAP:
            if (!sMapStore.LookupEntry(summonerId))
            {
                TC_LOG_ERROR("sql.sql", "Table `creature_summon_groups` has summoner with non existing entry %u for map summoner type, skipped.", summonerId);
                continue;
            }
            break;
        default:
            TC_LOG_ERROR("sql.sql", "Table `creature_summon_groups` has unhandled summoner type %u for summoner %u, skipped.", summonerType, summonerId);
            continue;
        }

        TempSummonData data;
        data.entry = fields[3].GetUInt32();

        if (!GetCreatureTemplate(data.entry))
        {
            TC_LOG_ERROR("sql.sql", "Table `creature_summon_groups` has creature in group [Summoner ID: %u, Summoner Type: %u, Group ID: %u] with non existing creature entry %u, skipped.", summonerId, summonerType, group, data.entry);
            continue;
        }

        float posX = fields[4].GetFloat();
        float posY = fields[5].GetFloat();
        float posZ = fields[6].GetFloat();
        float orientation = fields[7].GetFloat();

        data.pos.Relocate(posX, posY, posZ, orientation);

        data.type = TempSummonType(fields[8].GetUInt8());

        if (data.type > TEMPSUMMON_MANUAL_DESPAWN)
        {
            TC_LOG_ERROR("sql.sql", "Table `creature_summon_groups` has unhandled temp summon type %u in group [Summoner ID: %u, Summoner Type: %u, Group ID: %u] for creature entry %u, skipped.", data.type, summonerId, summonerType, group, data.entry);
            continue;
        }

        data.time = fields[9].GetUInt32();

        TempSummonGroupKey key(summonerId, summonerType, group);
        _tempSummonDataStore[key].push_back(std::move(data));

        ++count;

    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u temp summons in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadCreatures()
{
    _creatureDataStore.clear();

    uint32 count = 0;
    //                                                0                1    2          3
    QueryResult result = WorldDatabase.Query("SELECT creature.spawnID, map, spawnMask, modelid, "
    //   4           5           6           7            8              9         10
        "position_x, position_y, position_z, orientation, spawntimesecs, spawndist, currentwaypoint, "
    //   11         12       13            14          15                   16     17       18          19         20
        "curhealth, curmana, MovementType, unit_flags, creature.ScriptName, event, pool_id, pool_entry, patch_min, patch_max "
        "FROM creature "
        "LEFT OUTER JOIN game_event_creature ON creature.SpawnID = game_event_creature.guid "
        "LEFT OUTER JOIN pool_creature ON creature.SpawnID = pool_creature.guid "
        );

    if(!result)
    {
        TC_LOG_ERROR("server.loading",">> Loaded 0 creature. DB table `creature` is empty.");
        return;
    }

    QueryResult result2 = WorldDatabase.Query("SELECT spawnID, entry, equipment_id FROM creature_entry");
    if (!result2)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 creature entries. DB table `creature_entry` is empty.");
        return;
    }

    // build single time for check creature data
    std::map<uint32, uint32> spawnMasks;
    for (uint32 i = 0; i < sMapStore.GetNumRows(); ++i)
        if (sMapStore.LookupEntry(i))
            for (uint8 k = 0; k < MAX_DIFFICULTY; ++k)
                if (GetMapDifficultyData(i, Difficulty(k)))
                    spawnMasks[i] |= (1 << k);

    std::unordered_map<uint32 /*spawnId*/, CreatureData::SpawnDataIds> spawnEntries;

    do
    {
        Field* fields = result2->Fetch();
        uint32 spawnId = fields[0].GetUInt32();
        uint32 templateId = fields[1].GetUInt32();
        int8 equipmentId = fields[2].GetInt8();

        CreatureTemplate const* cInfo = GetCreatureTemplate(templateId);
        if (!cInfo)
        {
            TC_LOG_ERROR("sql.sql", "Table `creature_entry` has creature (SpawnId: %u) with not existing creature entry %u, skipped.", spawnId, templateId);
            continue;
        }

        // -1 random, 0 no equipment,
        if (equipmentId != 0 && equipmentId != -1)
        {
            if (!GetEquipmentInfo(templateId, equipmentId))
            {
                TC_LOG_ERROR("sql.sql", "Table `creature_entry` has creature (SpawnId: %u) with equipment_id %u not found in table `creature_equip_template`, reverting to no equipment.", spawnId, equipmentId);
                equipmentId = -1;
            }
        }
        CreatureData::SpawnDataIds& data = spawnEntries[spawnId];
        data.emplace_back(templateId, equipmentId);
    } while (result2->NextRow());

    do
    {
        Field* fields = result->Fetch();

        uint32 spawnId = fields[0].GetUInt32();

        // we create a _creatureDataStore entry in creature_entry loading
        auto spawnEntryItr = spawnEntries.find(spawnId);
        if (spawnEntryItr == spawnEntries.end())
        {
            TC_LOG_ERROR("sql.sql", "Table `creature` has creature (SpawnId: %u) with no listed creature id in table creature_entry, skipped.", spawnId);
            continue;
        }

        uint32 mapId = fields[1].GetUInt16();
        MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
        if (!mapEntry)
        {
            TC_LOG_ERROR("sql.sql", "Table `creature` has creature (SpawnId: %u) spawned on non existent map %u, skipped.", spawnId, mapId);
            continue;
        }
        CreatureData& data = _creatureDataStore[spawnId];

        data.ids = spawnEntryItr->second;
        data.spawnMask       = fields[2].GetUInt8();

        data.spawnPoint.WorldRelocate(mapId, fields[4].GetFloat(), fields[5].GetFloat(), fields[6].GetFloat(), fields[7].GetFloat());
        data.displayid       = fields[ 3].GetUInt32();
        data.spawntimesecs   = fields[ 8].GetUInt32();
        data.spawndist       = fields[ 9].GetFloat();
        data.currentwaypoint = fields[10].GetUInt32();
        data.curhealth       = fields[11].GetUInt32();
        data.curmana         = fields[12].GetUInt32();
        data.movementType    = fields[13].GetUInt8();
        data.unit_flags      = fields[14].GetUInt32();
        data.scriptId        = GetScriptId(fields[15].GetString());
        int32 gameEvent      = fields[16].GetInt32();
        data.poolId          = fields[17].GetUInt32(); //Old WR pool system
        //sun: use legacy group by default for instances, else it would break a lot of existing scripts. This will be overriden by any entry in spawn_group table.
        if(mapEntry->Instanceable())
            data.spawnGroupData = &_spawnGroupDataStore[1]; //Legacy group
        else
            data.spawnGroupData = &_spawnGroupDataStore[0]; //Default group

        uint32 poolId = fields[18].GetUInt32();

        uint8 patch_min = fields[19].GetInt8();
        uint8 patch_max = fields[20].GetInt8();

        bool existsInPatch = true;

        if ((patch_min > patch_max) || (patch_max > WOW_PATCH_MAX))
        {
            TC_LOG_ERROR("sql.sql", "Table `creature` spawnId %u has invalid values patch_min=%u, patch_max=%u.", spawnId, patch_min, patch_max);
            patch_min = WOW_PATCH_MIN;
            patch_max = WOW_PATCH_MAX;
        }

        if (!((sWorld->GetWowPatch() >= patch_min) && (sWorld->GetWowPatch() <= patch_max)))
            existsInPatch = false;

        // Skip spawnMask check for transport maps
        if (!IsTransportMap(data.spawnPoint.GetMapId()))
        {
            if (data.spawnMask & ~spawnMasks[data.spawnPoint.GetMapId()])
                TC_LOG_ERROR("sql.sql", "Table `creature` has creature (GUID: %u) that have wrong spawn mask %u including unsupported difficulty modes for map (Id: %u).", spawnId, data.spawnMask, data.spawnPoint.GetMapId());
        } else
            data.spawnGroupData = &_spawnGroupDataStore[1]; // force compatibility group for transport spawns

        if (data.displayid != 0)
        {
            if (!sCreatureDisplayInfoStore.LookupEntry(data.displayid))
            {
                TC_LOG_ERROR("sql.sql", "Table `creature` have creature (SpawnId: %u) with invalid displayid %u, set to 0.", spawnId, data.displayid);
                data.displayid = 0;
            }
        }

        if (data.movementType >= MAX_DB_MOTION_TYPE)
        {
            TC_LOG_ERROR("sql.sql", "Table `creature` has creature (SpawnId: %u) with wrong movement generator type (%u), ignored and set to IDLE.", spawnId, data.movementType);
            data.movementType = IDLE_MOTION_TYPE;
        }

        if(data.spawndist < 0.0f)
        {
            TC_LOG_ERROR("sql.sql","Table `creature` have creature (SpawnId: %u) with `spawndist`< 0, set to 0.", spawnId);
            data.spawndist = 0.0f;
        }
        else if(data.movementType == RANDOM_MOTION_TYPE)
        {
            if(data.spawndist == 0.0f)
            {
                TC_LOG_ERROR("sql.sql","Table `creature` have creature (SpawnId: %u) with `MovementType`=1 (random movement) but with `spawndist`=0, replace by idle movement type (0).", spawnId);
                data.movementType = IDLE_MOTION_TYPE;
            }
        }
        else if(data.movementType == IDLE_MOTION_TYPE)
        {
            if(data.spawndist != 0.0f)
            {
                TC_LOG_ERROR("sql.sql","Table `creature` have creature (SpawnId: %u) with `MovementType`=0 (idle) have `spawndist`<>0, set to 0.", spawnId);
                data.spawndist = 0.0f;
            }
        }

        // Add to grid if not managed by the game event, pool system or patch version
        if (gameEvent == 0 && poolId == 0 && existsInPatch)
            AddCreatureToGrid(spawnId, &data);
        ++count;

    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded " UI64FMTD " creatures", _creatureDataStore.size());
}

void ObjectMgr::DeleteCreatureData(ObjectGuid::LowType spawnId)
{
    // remove mapid*cellid -> guid_set map
    CreatureData const* data = GetCreatureData(spawnId);
    if (data)
    {
        RemoveCreatureFromGrid(spawnId, data);
        OnDeleteSpawnData(data);
    }

    _creatureDataStore.erase(spawnId);
}

bool ObjectMgr::AddCreatureToGrid(ObjectGuid::LowType spawnId, CreatureData const* data)
{
    uint8 mask = data->spawnMask;
    bool inserted = false;
    for(uint8 i = 0; mask != 0; i++, mask >>= 1)
    {
        if(mask & 1)
        {
            CellCoord cell_pair = Trinity::ComputeCellCoord(data->spawnPoint.GetPositionX(), data->spawnPoint.GetPositionY());
            uint32 cell_id = (cell_pair.y_coord*TOTAL_NUMBER_OF_CELLS_PER_MAP) + cell_pair.x_coord;

            CellObjectGuids& cell_guids = _mapObjectGuidsStore[MAKE_PAIR32(data->spawnPoint.GetMapId(), i)][cell_id];
            if (cell_guids.creatures.find(spawnId) == cell_guids.creatures.end()) //sun: check existence. Currently needed by GameEventMgr.
            {
                cell_guids.creatures.insert(spawnId);
                inserted = true;
            }
        }
    }
    return inserted;
}

void ObjectMgr::RemoveCreatureFromGrid(ObjectGuid::LowType spawnId, CreatureData const* data)
{
    uint8 mask = data->spawnMask;
    for(uint8 i = 0; mask != 0; i++, mask >>= 1)
    {
        if(mask & 1)
        {
            CellCoord cell_pair = Trinity::ComputeCellCoord(data->spawnPoint.GetPositionX(), data->spawnPoint.GetPositionY());
            uint32 cell_id = (cell_pair.y_coord*TOTAL_NUMBER_OF_CELLS_PER_MAP) + cell_pair.x_coord;

            CellObjectGuids& cell_guids = _mapObjectGuidsStore[MAKE_PAIR32(data->spawnPoint.GetMapId(),i)][cell_id];
            cell_guids.creatures.erase(spawnId);
        }
    }
}

void ObjectMgr::LoadGameObjects()
{
    uint32 count = 0;

    //                                                0                1   2    3           4           5           6
    QueryResult result = WorldDatabase.Query("SELECT gameobject.guid, id, map, position_x, position_y, position_z, orientation,"
    //   7          8          9          10         11             12            13     14         15         16         17       18         19
        "rotation0, rotation1, rotation2, rotation3, spawntimesecs, animprogress, state, spawnMask, event, ScriptName, pool_entry, patch_min, patch_max "
        "FROM gameobject "
        "LEFT OUTER JOIN game_event_gameobject ON gameobject.guid = game_event_gameobject.guid "
        "LEFT OUTER JOIN pool_gameobject ON gameobject.guid = pool_gameobject.guid "
    );

    if(!result)
    {
        TC_LOG_ERROR("server.loading",">> Loaded 0 gameobjects. DB table `gameobject` is empty.");
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        ObjectGuid::LowType guid         = fields[0].GetUInt32();
        uint32 entry        = fields[1].GetUInt32();

        uint8 patch_min = fields[18].GetInt8();
        uint8 patch_max = fields[19].GetInt8();
        bool existsInPatch = true;

        if ((patch_min > patch_max) || (patch_max > WOW_PATCH_MAX))
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` GUID %u (entry %u) has invalid values patch_min=%u, patch_max=%u.", guid, entry, patch_min, patch_max);
            patch_min = WOW_PATCH_MIN;
            patch_max = WOW_PATCH_MAX;
        }

        if (!((sWorld->GetWowPatch() >= patch_min) && (sWorld->GetWowPatch() <= patch_max)))
            existsInPatch = false;

        GameObjectTemplate const* gInfo = GetGameObjectTemplate(entry);
        if (!gInfo)
        {
            if (existsInPatch) // don't print error when it is not loaded for the current patch
                TC_LOG_ERROR("sql.sql", "Table `gameobject` have gameobject (SpawnId: %u) with not existed gameobject entry %u, skipped.", guid, entry);

            continue;
        }

        if (!gInfo->displayId)
        {
            switch (gInfo->type)
            {
            case GAMEOBJECT_TYPE_TRAP:
            case GAMEOBJECT_TYPE_SPELL_FOCUS:
                break;
            default:
                TC_LOG_ERROR("sql.sql", "Gameobject (SpawnId: %u Entry %u GoType: %u) doesn't have a displayId (%u), not loaded.", guid, entry, gInfo->type, gInfo->displayId);
                break;
            }
        }

        if (gInfo->displayId && !sGameObjectDisplayInfoStore.LookupEntry(gInfo->displayId))
        {
            TC_LOG_ERROR("sql.sql", "Gameobject (SpawnId: %u Entry %u GoType: %u) has an invalid displayId (%u), not loaded.", guid, entry, gInfo->type, gInfo->displayId);
            continue;
        }

        GameObjectData& data = _gameObjectDataStore[guid];

        data.id             = entry;
        uint32 mapId = fields[2].GetUInt16();
        MapEntry const* mapEntry = sMapStore.LookupEntry(mapId);
        if (!mapEntry)
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (SpawnId: %u Entry: %u) spawned on a non-existed map (Id: %u), skip", guid, data.id, data.spawnPoint.GetMapId());
            continue;
        }
        data.spawnPoint.WorldRelocate(mapId, fields[3].GetFloat(), fields[4].GetFloat(), fields[5].GetFloat(), fields[6].GetFloat());
        data.rotation       = G3D::Quat(fields[ 7].GetFloat(), fields[ 8].GetFloat(), fields[ 9].GetFloat(), fields[10].GetFloat());
        data.spawntimesecs  = fields[11].GetInt32();
        data.animprogress   = fields[12].GetUInt8();
        data.go_state       = fields[13].GetUInt8();
        data.ArtKit         = 0;
        data.spawnMask      = fields[14].GetUInt8();
        int16 gameEvent     = fields[15].GetInt16();
        data.ScriptId = GetScriptId(fields[16].GetString());
        //sun: use legacy group by default for instances, else it would break a lot of existing scripts. This will be overriden by any entry in spawn_group table.
        if (mapEntry->Instanceable())
            data.spawnGroupData = &_spawnGroupDataStore[1]; //Legacy group
        else
            data.spawnGroupData = &_spawnGroupDataStore[0]; //Default group
        uint32 PoolId = fields[17].GetUInt32();

        if (data.spawntimesecs == 0 && gInfo->IsDespawnAtAction())
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (SpawnId: %u Entry: %u) with `spawntimesecs` (0) value, but the gameobject is marked as despawnable at action.", guid, data.id);
        }

        if (data.go_state >= MAX_GO_STATE)
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (SpawnId: %u Entry: %u) with invalid `state` (%u) value, skip", guid, data.id, data.go_state);
            continue;
        }

        if (std::abs(data.spawnPoint.GetOrientation()) > 2 * float(M_PI))
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (SpawnId: %u Entry: %u) with abs(`orientation`) > 2*PI (orientation is expressed in radians), normalized.", guid, data.id);
            data.spawnPoint.m_orientation = Position::NormalizeOrientation(data.spawnPoint.GetOrientation());
        }

        if (data.rotation.x < -1.0f || data.rotation.x > 1.0f)
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (SpawnId: %u Entry: %u) with invalid rotationX (%f) value, skip", guid, data.id, data.rotation.x);
            continue;
        }

        if (data.rotation.y < -1.0f || data.rotation.y > 1.0f)
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (SpawnId: %u Entry: %u) with invalid rotationY (%f) value, skip", guid, data.id, data.rotation.y);
            continue;
        }

        if (data.rotation.z < -1.0f || data.rotation.z > 1.0f)
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (SpawnId: %u Entry: %u) with invalid rotationZ (%f) value, skip", guid, data.id, data.rotation.z);
            continue;
        }

        if (data.rotation.w < -1.0f || data.rotation.w > 1.0f)
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (SpawnId: %u Entry: %u) with invalid rotationW (%f) value, skip", guid, data.id, data.rotation.w);
            continue;
        }

#ifdef LICH_KING
        if (data.phaseMask == 0)
        {
            TC_LOG_ERROR("sql.sql", "Table `gameobject` has gameobject (SpawnId: %u Entry: %u) with `phaseMask`=0 (not visible for anyone), set to 1.", guid, data.id);
            data.phaseMask = 1;
        }
#endif

        if (gameEvent == 0 && PoolId == 0 && existsInPatch)                      // if not this is to be managed by GameEvent System, Pool system or Wow Patch
            AddGameobjectToGrid(guid, &data);
        ++count;

    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded " UI64FMTD " gameobjects", _gameObjectDataStore.size());
}

void ObjectMgr::LoadSpawnGroupTemplates()
{
    uint32 oldMSTime = GetMSTime();

    //                                               0        1          2
    QueryResult result = WorldDatabase.Query("SELECT groupId, groupName, groupFlags FROM spawn_group_template");

    if (result)
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 groupId = fields[0].GetUInt32();
            SpawnGroupTemplateData& group = _spawnGroupDataStore[groupId];
            group.groupId = groupId;
            group.name = fields[1].GetString();
            group.mapId = SPAWNGROUP_MAP_UNSET;
            uint32 flags = fields[2].GetUInt32();
            if (flags & ~SPAWNGROUP_FLAGS_ALL)
            {
                flags &= SPAWNGROUP_FLAGS_ALL;
                TC_LOG_ERROR("sql.sql", "Invalid spawn group flag %u on group ID %u (%s), reduced to valid flag %u.", flags, groupId, group.name.c_str(), uint32(group.flags));
            }
            if (flags & SPAWNGROUP_FLAG_SYSTEM && flags & SPAWNGROUP_FLAG_MANUAL_SPAWN)
            {
                flags &= ~SPAWNGROUP_FLAG_MANUAL_SPAWN;
                TC_LOG_ERROR("sql.sql", "System spawn group %u (%s) has invalid manual spawn flag. Ignored.", groupId, group.name.c_str());
            }
            group.flags = SpawnGroupFlags(flags);
        } while (result->NextRow());
    }

    if (_spawnGroupDataStore.find(0) == _spawnGroupDataStore.end())
    {
        TC_LOG_ERROR("sql.sql", "Default spawn group (index 0) is missing from DB! Manually inserted.");
        SpawnGroupTemplateData& data = _spawnGroupDataStore[0];
        data.groupId = 0;
        data.name = "Default Group";
        data.mapId = 0;
        data.flags = SPAWNGROUP_FLAG_SYSTEM;
    }
    if (_spawnGroupDataStore.find(1) == _spawnGroupDataStore.end())
    {
        TC_LOG_ERROR("sql.sql", "Default legacy spawn group (index 1) is missing from DB! Manually inserted.");
        SpawnGroupTemplateData&data = _spawnGroupDataStore[1];
        data.groupId = 1;
        data.name = "Legacy Group";
        data.mapId = 0;
        data.flags = SpawnGroupFlags(SPAWNGROUP_FLAG_SYSTEM | SPAWNGROUP_FLAG_COMPATIBILITY_MODE);
    }

    if (result)
        TC_LOG_INFO("server.loading", ">> Loaded " SZFMTD " spawn group templates in %u ms", _spawnGroupDataStore.size(), GetMSTimeDiffToNow(oldMSTime));
    else
        TC_LOG_INFO("server.loading", ">> Loaded 0 spawn group templates. DB table `spawn_group_template` is empty.");

    return;
}

void ObjectMgr::LoadSpawnGroups()
{
    uint32 oldMSTime = GetMSTime();

    //                                               0        1          2
    QueryResult result = WorldDatabase.Query("SELECT groupId, spawnType, spawnId FROM spawn_group");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 spawn group members. DB table `spawn_group` is empty.");
        return;
    }

    uint32 numMembers = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 groupId = fields[0].GetUInt32();
        SpawnObjectType spawnType;
        {
            uint32 type = fields[1].GetUInt8();
            if (type >= SPAWN_TYPE_MAX)
            {
                TC_LOG_ERROR("sql.sql", "Spawn data with invalid type %u listed for spawn group %u. Skipped.", type, groupId);
                continue;
            }
            spawnType = SpawnObjectType(type);
        }
        ObjectGuid::LowType spawnId = fields[2].GetUInt32();

        SpawnData const* data = GetSpawnData(spawnType, spawnId);
        if (!data)
        {
            TC_LOG_ERROR("sql.sql", "Spawn data with ID (%u,%u) not found, but is listed as a member of spawn group %u!", uint32(spawnType), spawnId, groupId);
            continue;
        }
        /* sun: Disabled, we now use groupId 1 as default for some creature
        else if (data->spawnGroupData->groupId)
        {
            TC_LOG_ERROR("sql.sql", "Spawn with ID (%u,%u) is listed as a member of spawn group %u, but is already a member of spawn group %u. Skipping.", uint32(spawnType), spawnId, groupId, data->spawnGroupData->groupId);
            continue;
        }*/
        auto it = _spawnGroupDataStore.find(groupId);
        if (it == _spawnGroupDataStore.end())
        {
            TC_LOG_ERROR("sql.sql", "Spawn group %u assigned to spawn ID (%u,%u), but group is found!", groupId, uint32(spawnType), spawnId);
            continue;
        }
        else
        {
            SpawnGroupTemplateData& groupTemplate = it->second;
            if (groupTemplate.mapId == SPAWNGROUP_MAP_UNSET)
                groupTemplate.mapId = data->spawnPoint.GetMapId();
            else if (groupTemplate.mapId != data->spawnPoint.GetMapId() && !(groupTemplate.flags & SPAWNGROUP_FLAG_SYSTEM))
            {
                TC_LOG_ERROR("sql.sql", "Spawn group %u has map ID %u, but spawn (%u,%u) has map id %u - spawn NOT added to group!", groupId, groupTemplate.mapId, uint32(spawnType), spawnId, data->spawnPoint.GetMapId());
                continue;
            }
            const_cast<SpawnData*>(data)->spawnGroupData = &groupTemplate;
            if (!(groupTemplate.flags & SPAWNGROUP_FLAG_SYSTEM))
                _spawnGroupMapStore.emplace(groupId, data);
            ++numMembers;
        }
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u spawn group members in %u ms", numMembers, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadInstanceSpawnGroups()
{
    uint32 oldMSTime = GetMSTime();

    //                                               0              1            2           3             4
    QueryResult result = WorldDatabase.Query("SELECT instanceMapId, bossStateId, bossStates, spawnGroupId, flags FROM instance_spawn_groups");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 instance spawn groups. DB table `instance_spawn_groups` is empty.");
        return;
    }

    uint32 n = 0;
    do
    {
        Field* fields = result->Fetch();
        uint32 const spawnGroupId = fields[3].GetUInt32();
        auto it = _spawnGroupDataStore.find(spawnGroupId);
        if (it == _spawnGroupDataStore.end() || (it->second.flags & SPAWNGROUP_FLAG_SYSTEM))
        {
            TC_LOG_ERROR("sql.sql", "Invalid spawn group %u specified for instance %u. Skipped.", spawnGroupId, fields[0].GetUInt16());
            continue;
        }

        uint16 const instanceMapId = fields[0].GetUInt16();
        auto& vector = _instanceSpawnGroupStore[instanceMapId];
        vector.emplace_back();
        InstanceSpawnGroupInfo& info = vector.back();
        info.SpawnGroupId = spawnGroupId;
        info.BossStateId = fields[1].GetUInt8();

        uint8 const ALL_STATES = (1 << TO_BE_DECIDED) - 1;
        uint8 const states = fields[2].GetUInt8();
        if (states & ~ALL_STATES)
        {
            info.BossStates = states & ALL_STATES;
            TC_LOG_ERROR("sql.sql", "Instance spawn group (%u,%u) had invalid boss state mask %u - truncated to %u.", instanceMapId, spawnGroupId, states, info.BossStates);
        }
        else
            info.BossStates = states;

        uint8 const flags = fields[4].GetUInt8();
        if (flags & ~InstanceSpawnGroupInfo::FLAG_ALL)
        {
            info.Flags = flags & InstanceSpawnGroupInfo::FLAG_ALL;
            TC_LOG_ERROR("sql.sql", "Instance spawn group (%u,%u) had invalid flags %u - truncated to %u.", instanceMapId, spawnGroupId, flags, info.Flags);
        }
        else
            info.Flags = flags;

        ++n;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u instance spawn groups in %u ms", n, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::OnDeleteSpawnData(SpawnData const* data)
{
    auto templateIt = _spawnGroupDataStore.find(data->spawnGroupData->groupId);
    ASSERT(templateIt != _spawnGroupDataStore.end(), "Creature data for (%u,%u) is being deleted and has invalid spawn group index %u!", uint32(data->type), data->spawnId, data->spawnGroupData->groupId);
    if (templateIt->second.flags & SPAWNGROUP_FLAG_SYSTEM) // system groups don't store their members in the map
        return;

    auto pair = _spawnGroupMapStore.equal_range(data->spawnGroupData->groupId);
    for (auto it = pair.first; it != pair.second; ++it)
    {
        if (it->second != data)
            continue;
        _spawnGroupMapStore.erase(it);
        return;
    }
    ASSERT(false, "Spawn data (%u,%u) being removed is member of spawn group %u, but not actually listed in the lookup table for that group!", uint32(data->type), data->spawnId, data->spawnGroupData->groupId);
}

bool ObjectMgr::AddGameobjectToGrid(ObjectGuid::LowType spawnId, GameObjectData const* data)
{
    assert(data);

    uint8 mask = data->spawnMask;
    bool inserted = false;
    for(uint8 i = 0; mask != 0; i++, mask >>= 1)
    {
        if(mask & 1)
        {
            CellCoord cell_pair = Trinity::ComputeCellCoord(data->spawnPoint.GetPositionX(), data->spawnPoint.GetPositionY());
            uint32 cell_id = (cell_pair.y_coord*TOTAL_NUMBER_OF_CELLS_PER_MAP) + cell_pair.x_coord;

            CellObjectGuids& cell_guids = _mapObjectGuidsStore[MAKE_PAIR32(data->spawnPoint.GetMapId(), i)][cell_id];
            if (cell_guids.gameobjects.find(spawnId) == cell_guids.gameobjects.end()) //sun: check existence. Currently needed by GameEventMgr.
            {
                cell_guids.gameobjects.insert(spawnId);
                inserted = true;
            }
        }
    }
    return inserted;
}

void ObjectMgr::RemoveGameobjectFromGrid(ObjectGuid::LowType spawnId, GameObjectData const* data)
{
    uint8 mask = data->spawnMask;
    for(uint8 i = 0; mask != 0; i++, mask >>= 1)
    {
        if(mask & 1)
        {
            CellCoord cell_pair = Trinity::ComputeCellCoord(data->spawnPoint.GetPositionX(), data->spawnPoint.GetPositionY());
            uint32 cell_id = (cell_pair.y_coord*TOTAL_NUMBER_OF_CELLS_PER_MAP) + cell_pair.x_coord;

            CellObjectGuids& cell_guids = _mapObjectGuidsStore[MAKE_PAIR32(data->spawnPoint.GetMapId(),i)][cell_id];
            cell_guids.gameobjects.erase(spawnId);
        }
    }
}

ObjectGuid::LowType ObjectMgr::AddGameObjectData(uint32 entry, uint32 mapId, float x, float y, float z, float o, uint32 spawntimedelay, float rotation0, float rotation1, float rotation2, float rotation3)
{
    GameObjectTemplate const* goinfo = GetGameObjectTemplate(entry);
    if (!goinfo)
        return 0;

    Map* map = sMapMgr->CreateBaseMap(mapId);
    if (!map)
        return 0;

    ObjectGuid::LowType guid = GenerateGameObjectSpawnId();

    GameObjectData& data = NewOrExistGameObjectData(guid);
    data.id = entry;
    data.spawnPoint.WorldRelocate(mapId, x, y, z, o);
    data.rotation = G3D::Quat(rotation0, rotation1, rotation2, rotation3);
    data.spawntimesecs = spawntimedelay;
    data.animprogress = 100;
    data.spawnMask = 1;
    data.go_state = GO_STATE_READY;
    //data.phaseMask = PHASEMASK_NORMAL;
    //data.artKit = goinfo->type == GAMEOBJECT_TYPE_CAPTURE_POINT ? 21 : 0;
    //data.dbData = false;
    data.spawnGroupData = GetLegacySpawnGroup();

    AddGameobjectToGrid(guid, &data);

    // Spawn if necessary (loaded grids only)
    // We use spawn coords to spawn
    if (!map->Instanceable() && map->IsGridLoaded(x, y))
    {
        GameObject* go = new GameObject;
        if (!go->LoadFromDB(guid, map, true))
        {
            TC_LOG_ERROR("misc", "AddGameObjectData: cannot add gameobject entry %u to map", entry);
            delete go;
            return 0;
        }
    }

    TC_LOG_DEBUG("maps", "AddGameObjectData: dbguid %u entry %u map %u x %f y %f z %f o %f", guid, entry, mapId, x, y, z, o);

    return guid;
}



ObjectGuid::LowType ObjectMgr::AddCreatureData(uint32 entry, uint32 mapId, float x, float y, float z, float o, uint32 spawntimedelay /*= 0*/)
{
    CreatureTemplate const* cInfo = GetCreatureTemplate(entry);
    if (!cInfo)
        return 0;

    uint32 level = cInfo->minlevel == cInfo->maxlevel ? cInfo->minlevel : urand(cInfo->minlevel, cInfo->maxlevel); // Only used for extracting creature base stats
    CreatureBaseStats const* stats = GetCreatureBaseStats(level, cInfo->unit_class);
    Map* map = sMapMgr->CreateBaseMap(mapId);
    if (!map)
        return 0;

    ObjectGuid::LowType spawnId = GenerateCreatureSpawnId();

    CreatureData& data = NewOrExistCreatureData(spawnId);
    data.ids.emplace_back(entry);
    data.displayid = 0;
    data.spawnPoint.WorldRelocate(mapId, x, y, z, o);
    data.spawntimesecs = spawntimedelay;
    data.spawndist = 0;
    data.currentwaypoint = 0;
    data.curhealth = stats->GenerateHealth(cInfo);
    data.curmana = stats->GenerateMana(cInfo);
    data.movementType = cInfo->MovementType;
    data.spawnMask = PHASEMASK_NORMAL;
    //data.dbData = false;
    //data.npcflag = cInfo->npcflag;
    //data.unit_flags = cInfo->unit_flags;
    //data.dynamicflags = cInfo->dynamicflags;
    data.spawnGroupData = GetLegacySpawnGroup();

    AddCreatureToGrid(spawnId, &data);

    // We use spawn coords to spawn
    if (!map->Instanceable() && !map->IsRemovalGrid(data.spawnPoint))
    {
        Creature* creature = new Creature();
        if (!creature->LoadFromDB(spawnId, map, true, true))
        {
            TC_LOG_ERROR("misc", "AddCreature: Cannot add creature entry %u to map", entry);
            delete creature;
            return 0;
        }
    }

    return spawnId;
}

void ObjectMgr::LoadItemLocales()
{
    _itemLocaleStore.clear();                                 // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT entry,name_loc1,description_loc1,name_loc2,description_loc2,name_loc3,description_loc3,name_loc4,description_loc4,name_loc5,description_loc5,name_loc6,description_loc6,name_loc7,description_loc7,name_loc8,description_loc8 FROM locales_item");

    if(!result)
    {
        TC_LOG_INFO("server.loading",">> Loaded 0 Item locale strings. DB table `locales_item` is empty.");
        return;
    }
    do
    {
        Field *fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        ItemLocale& data = _itemLocaleStore[entry];

        for (uint8 i = TOTAL_LOCALES - 1; i > 0; --i)
        {
            LocaleConstant locale = (LocaleConstant) i;
            AddLocaleString(fields[1 + 2 * (i - 1)].GetString(), locale, data.Name);
            AddLocaleString(fields[1 + 2 * (i - 1) + 1].GetString(), locale, data.Description);
        }
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded " UI64FMTD " Item locale strings", _itemLocaleStore.size());
}
void ObjectMgr::LoadItemTemplates()
{
    uint32 oldMSTime = GetMSTime();

    //                                                 0      1       2               3              4        5        6       7       8            9        10        11
    QueryResult result = WorldDatabase.PQuery("SELECT entry, class, subclass, SoundOverrideSubclass, name, displayid, Quality, Flags, BuyCount, BuyPrice, SellPrice, InventoryType, "
    //                                              12                                                                                                        19
                                             "AllowableClass, AllowableRace, ItemLevel, RequiredLevel, RequiredSkill, RequiredSkillRank, requiredspell, requiredhonorrank, "
    //                                              20
                                             "RequiredCityRank, RequiredReputationFaction, RequiredReputationRank, maxcount, stackable, ContainerSlots, stat_type1, "
    //                                             27
                                             "stat_value1, stat_type2, stat_value2, stat_type3, stat_value3, stat_type4, stat_value4, stat_type5, stat_value5, stat_type6, "
    //                                          37
                                             "stat_value6, stat_type7, stat_value7, stat_type8, stat_value8, stat_type9, stat_value9, stat_type10, stat_value10, "
    //                                           46
                                             "dmg_min1, dmg_max1, dmg_type1, dmg_min2, dmg_max2, dmg_type2,  dmg_min3, dmg_max3, dmg_type3, dmg_min4, dmg_max4, dmg_type4, dmg_min5, dmg_max5, dmg_type5,"
    //                                          61
                                             "armor, holy_res, fire_res, "
    //                                            64
                                             "nature_res, frost_res, shadow_res, arcane_res, delay, ammo_type, RangedModRange, spellid_1, spelltrigger_1, spellcharges_1, "
    //                                             74
                                             "spellppmRate_1, spellcooldown_1, spellcategory_1, spellcategorycooldown_1, spellid_2, spelltrigger_2, spellcharges_2, "
    //                                             81
                                             "spellppmRate_2, spellcooldown_2, spellcategory_2, spellcategorycooldown_2, spellid_3, spelltrigger_3, spellcharges_3, "
    //                                             88
                                             "spellppmRate_3, spellcooldown_3, spellcategory_3, spellcategorycooldown_3, spellid_4, spelltrigger_4, spellcharges_4, "
    //                                              95
                                             "spellppmRate_4, spellcooldown_4, spellcategory_4, spellcategorycooldown_4, spellid_5, spelltrigger_5, spellcharges_5, "
    //                                             102
                                             "spellppmRate_5, spellcooldown_5, spellcategory_5, spellcategorycooldown_5, bonding, description, PageText, LanguageID, PageMaterial, "
    //                                            111
                                             "startquest, lockid, Material, sheath, RandomProperty, RandomSuffix, block, itemset, MaxDurability, zone, Map, BagFamily, "
    //                                            123                          125                             127                               129               130
                                             "TotemCategory, socketColor_1, socketContent_1, socketColor_2, socketContent_2, socketColor_3, socketContent_3, socketBonus, "
    //                                            131                   132                 133                134        135
                                             "GemProperties, RequiredDisenchantSkill, ArmorDamageModifier, ScriptName, DisenchantID, "
    //                                           136         137         138         139 
                                             "FoodType, minMoneyLoot, maxMoneyLoot, Duration "
    //
                                             "FROM item_template t1 WHERE patch=(SELECT max(patch) FROM item_template t2 WHERE t1.entry=t2.entry && patch <= %u)", sWorld->GetWowPatch());

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 item templates. DB table `item_template` is empty.");
        return;
    }

    _itemTemplateStore.reserve(result->GetRowCount());
    bool enforceDBCAttributes = false; //sWorld->getBoolConfig(CONFIG_DBC_ENFORCE_ITEM_ATTRIBUTES);

    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        ItemTemplate& itemTemplate = _itemTemplateStore[entry];

        itemTemplate.ItemId                    = entry;
        itemTemplate.Class                     = uint32(fields[1].GetUInt8());
        itemTemplate.SubClass                  = uint32(fields[2].GetUInt8());
        itemTemplate.SoundOverrideSubclass     = int32(fields[3].GetInt32());
        itemTemplate.Name1                     = fields[4].GetString();
        itemTemplate.DisplayInfoID             = fields[5].GetUInt32();
        itemTemplate.Quality                   = uint32(fields[6].GetUInt8());
        itemTemplate.Flags                     = fields[7].GetUInt32();
        itemTemplate.BuyCount                  = uint32(fields[8].GetUInt8());
        itemTemplate.BuyPrice                  = int32(fields[9].GetUInt32());
        itemTemplate.SellPrice                 = fields[10].GetUInt32();
        itemTemplate.InventoryType             = uint32(fields[11].GetUInt8());
        itemTemplate.AllowableClass            = fields[12].GetUInt32();
        itemTemplate.AllowableRace             = fields[13].GetUInt32();
        itemTemplate.ItemLevel                 = uint32(fields[14].GetUInt8());
        itemTemplate.RequiredLevel             = uint32(fields[15].GetUInt8());
        itemTemplate.RequiredSkill             = uint32(fields[16].GetUInt16());
        itemTemplate.RequiredSkillRank         = uint32(fields[17].GetUInt16());
        itemTemplate.RequiredSpell             = fields[18].GetUInt32();
        itemTemplate.RequiredHonorRank         = fields[19].GetUInt32();
        itemTemplate.RequiredCityRank          = fields[20].GetUInt32();
        itemTemplate.RequiredReputationFaction = uint32(fields[21].GetUInt16());
        itemTemplate.RequiredReputationRank    = uint32(fields[22].GetUInt16());
        itemTemplate.MaxCount                  = fields[23].GetUInt16();
        itemTemplate.Stackable                 = fields[24].GetUInt16();
        itemTemplate.ContainerSlots            = uint32(fields[25].GetUInt8());
#ifdef LICH_KING
        for (uint8 i = 0; i < itemTemplate.StatsCount; ++i)
        {
            itemTemplate.ItemStat[i].ItemStatType  = uint32(fields[26 + i*2].GetUInt8());
            itemTemplate.ItemStat[i].ItemStatValue = int32(fields[27 + i*2].GetInt16());
        }
        itemTemplate.ScalingStatDistribution = uint32(fields[48].GetUInt16());
        itemTemplate.ScalingStatValue        = fields[49].GetInt32();
#else
        for (uint8 i = 0; i < 10; ++i)
        {
            itemTemplate.ItemStat[i].ItemStatType  = uint32(fields[26 + i*2].GetUInt8());
            itemTemplate.ItemStat[i].ItemStatValue = int32(fields[27 + i*2].GetInt16());
        }
#endif

        for (uint8 i = 0; i < MAX_ITEM_PROTO_DAMAGES; ++i)
        {
            itemTemplate.Damage[i].DamageMin  = fields[46 + i*3].GetFloat();
            itemTemplate.Damage[i].DamageMax  = fields[47 + i*3].GetFloat();
            itemTemplate.Damage[i].DamageType = uint32(fields[48 + i*3].GetUInt8());
        }

        itemTemplate.Armor          = uint32(fields[61].GetUInt16());
        itemTemplate.HolyRes        = uint32(fields[62].GetUInt8());
        itemTemplate.FireRes        = uint32(fields[63].GetUInt8());
        itemTemplate.NatureRes      = uint32(fields[64].GetUInt8());
        itemTemplate.FrostRes       = uint32(fields[65].GetUInt8());
        itemTemplate.ShadowRes      = uint32(fields[66].GetUInt8());
        itemTemplate.ArcaneRes      = uint32(fields[67].GetUInt8());
        itemTemplate.Delay          = uint32(fields[68].GetUInt16());
        itemTemplate.AmmoType       = uint32(fields[69].GetUInt8());
        itemTemplate.RangedModRange = fields[70].GetFloat();

        for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
        {
            itemTemplate.Spells[i].SpellId               = fields[71 + i*7  ].GetInt32();
            itemTemplate.Spells[i].SpellTrigger          = uint32(fields[72 + i*7].GetUInt8());
            itemTemplate.Spells[i].SpellCharges          = int32(fields[73 + i*7].GetInt8());
            itemTemplate.Spells[i].SpellPPMRate          = fields[74 + i*7].GetFloat();
            itemTemplate.Spells[i].SpellCooldown         = fields[75 + i*7].GetInt32();
            itemTemplate.Spells[i].SpellCategory         = uint32(fields[76 + i*7].GetUInt16());
            itemTemplate.Spells[i].SpellCategoryCooldown = fields[77 + i*7].GetInt32();
        }

        itemTemplate.Bonding        = uint32(fields[106].GetUInt8());
        itemTemplate.Description    = fields[107].GetString();
        itemTemplate.PageText       = fields[108].GetUInt32();
        itemTemplate.LanguageID     = uint32(fields[109].GetUInt8());
        itemTemplate.PageMaterial   = uint32(fields[110].GetUInt8());
        itemTemplate.StartQuest     = fields[111].GetUInt32();
        itemTemplate.LockID         = fields[112].GetUInt32();
        itemTemplate.Material       = int32(fields[113].GetInt8());
        itemTemplate.Sheath         = uint32(fields[114].GetUInt8());
        itemTemplate.RandomProperty = fields[115].GetUInt32();
        itemTemplate.RandomSuffix   = fields[116].GetInt32();
        itemTemplate.Block          = fields[117].GetUInt32();
        itemTemplate.ItemSet        = fields[118].GetUInt32();
        itemTemplate.MaxDurability  = uint32(fields[119].GetUInt16());
        itemTemplate.Area           = fields[120].GetUInt32();
        itemTemplate.Map            = uint32(fields[121].GetUInt16());
        itemTemplate.BagFamily      = fields[122].GetUInt32();
        itemTemplate.TotemCategory  = fields[123].GetUInt8();

        for (uint8 i = 0; i < MAX_ITEM_PROTO_SOCKETS; ++i)
        {
            itemTemplate.Socket[i].Color   = uint32(fields[124 + i*2].GetUInt8());
            itemTemplate.Socket[i].Content = fields[125 + i*2].GetUInt32();
        }

        itemTemplate.socketBonus             = fields[130].GetUInt32();
        itemTemplate.GemProperties           = fields[131].GetUInt32();
        itemTemplate.RequiredDisenchantSkill = uint32(fields[132].GetInt16());
        itemTemplate.ArmorDamageModifier     = fields[133].GetFloat();
        itemTemplate.ScriptId                = sObjectMgr->GetScriptId(fields[134].GetCString());
        itemTemplate.DisenchantID            = fields[135].GetUInt32();
        itemTemplate.FoodType                = uint32(fields[136].GetUInt8());
        itemTemplate.MinMoneyLoot            = fields[137].GetUInt32();
        itemTemplate.MaxMoneyLoot            = fields[138].GetUInt32();
        itemTemplate.Duration                = fields[139].GetUInt32();

        // Checks

        ItemEntry const* dbcitem = sItemStore.LookupEntry(entry);

        if (dbcitem)
        {
#ifdef LICH_KING
            if (itemTemplate.Class != dbcitem->Class)
            {
                TC_LOG_ERROR("sql.sql", "Item (Entry: %u) does not have a correct class %u, must be %u .", entry, itemTemplate.Class, dbcitem->Class);
                if (enforceDBCAttributes)
                    itemTemplate.Class = dbcitem->Class;
            }

            if (itemTemplate.SoundOverrideSubclass != dbcitem->SoundOverrideSubclass)
            {
                TC_LOG_ERROR("sql.sql", "Item (Entry: %u) does not have a correct SoundOverrideSubclass (%i), must be %i .", entry, itemTemplate.SoundOverrideSubclass, dbcitem->SoundOverrideSubclass);
                if (enforceDBCAttributes)
                    itemTemplate.SoundOverrideSubclass = dbcitem->SoundOverrideSubclass;
            }
            if (itemTemplate.Material != dbcitem->Material)
            {
                TC_LOG_ERROR("sql.sql", "Item (Entry: %u) does not have a correct material (%i), must be %i .", entry, itemTemplate.Material, dbcitem->Material);
                if (enforceDBCAttributes)
                    itemTemplate.Material = dbcitem->Material;
            }
#endif
            if (itemTemplate.InventoryType != dbcitem->InventoryType)
            {
                TC_LOG_ERROR("sql.sql", "Item (Entry: %u) does not have a correct inventory type (%u), must be %u .", entry, itemTemplate.InventoryType, dbcitem->InventoryType);
                if (enforceDBCAttributes)
                    itemTemplate.InventoryType = dbcitem->InventoryType;
            }
            if (itemTemplate.DisplayInfoID != dbcitem->DisplayId)
            {
                TC_LOG_ERROR("sql.sql", "Item (Entry: %u) does not have a correct display id (%u), must be %u .", entry, itemTemplate.DisplayInfoID, dbcitem->DisplayId);
                if (enforceDBCAttributes)
                    itemTemplate.DisplayInfoID = dbcitem->DisplayId;
            }
            if (itemTemplate.Sheath != dbcitem->Sheath)
            {
                TC_LOG_ERROR("sql.sql", "Item (Entry: %u) does not have a correct sheathid (%u), must be %u .", entry, itemTemplate.Sheath, dbcitem->Sheath);
                if (enforceDBCAttributes)
                    itemTemplate.Sheath = dbcitem->Sheath;
            }

        }
        else
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) does not exist in item.dbc! (not correct id?).", entry);

        if (itemTemplate.Class >= MAX_ITEM_CLASS)
        {
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong Class value (%u)", entry, itemTemplate.Class);
            itemTemplate.Class = ITEM_CLASS_MISC;
        }

        if (itemTemplate.SubClass >= MaxItemSubclassValues[itemTemplate.Class])
        {
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong Subclass value (%u) for class %u", entry, itemTemplate.SubClass, itemTemplate.Class);
            itemTemplate.SubClass = 0;// exist for all item classes
        }

        if (itemTemplate.Quality >= MAX_ITEM_QUALITY)
        {
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong Quality value (%u)", entry, itemTemplate.Quality);
            itemTemplate.Quality = ITEM_QUALITY_NORMAL;
        }

#ifdef LICH_KING
        if (itemTemplate.Flags2 & ITEM_FLAG_EXTRA_HORDE_ONLY)
        {
            if (FactionEntry const* faction = sFactionStore.LookupEntry(HORDE))
                if ((itemTemplate.AllowableRace & faction->BaseRepRaceMask[0]) == 0)
                    TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has value (%u) in `AllowableRace` races, not compatible with ITEM_FLAG_EXTRA_HORDE_ONLY (%u) in Flags field, item cannot be equipped or used by these races.",
                        entry, itemTemplate.AllowableRace, ITEM_FLAG_EXTRA_HORDE_ONLY);

            if (itemTemplate.Flags2 & ITEM_FLAG_EXTRA_ALLIANCE_ONLY)
                TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has value (%u) in `Flags2` flags (ITEM_FLAG_EXTRA_ALLIANCE_ONLY) and ITEM_FLAG_EXTRA_HORDE_ONLY (%u) in Flags field, this is a wrong combination.",
                    entry, ITEM_FLAG_EXTRA_ALLIANCE_ONLY, ITEM_FLAG_EXTRA_HORDE_ONLY);
        }
        else if (itemTemplate.Flags2 & ITEM_FLAG_EXTRA_ALLIANCE_ONLY)
        {
            if (FactionEntry const* faction = sFactionStore.LookupEntry(ALLIANCE))
                if ((itemTemplate.AllowableRace & faction->BaseRepRaceMask[0]) == 0)
                    TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has value (%u) in `AllowableRace` races, not compatible with ITEM_FLAG_EXTRA_ALLIANCE_ONLY (%u) in Flags field, item cannot be equipped or used by these races.",
                        entry, itemTemplate.AllowableRace, ITEM_FLAG_EXTRA_ALLIANCE_ONLY);
        }
#endif

        if (itemTemplate.BuyCount <= 0)
        {
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong BuyCount value (%u), set to default(1).", entry, itemTemplate.BuyCount);
            itemTemplate.BuyCount = 1;
        }

        if (itemTemplate.InventoryType >= MAX_INVTYPE)
        {
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong InventoryType value (%u)", entry, itemTemplate.InventoryType);
            itemTemplate.InventoryType = INVTYPE_NON_EQUIP;
        }

        if (itemTemplate.RequiredSkill >= MAX_SKILL_TYPE)
        {
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong RequiredSkill value (%u)", entry, itemTemplate.RequiredSkill);
            itemTemplate.RequiredSkill = 0;
        }

        {
            // can be used in equip slot, as page read use in inventory, or spell casting at use
            bool req = itemTemplate.InventoryType != INVTYPE_NON_EQUIP || itemTemplate.PageText;
            if (!req)
                for (auto & Spell : itemTemplate.Spells)
                {
                    if (Spell.SpellId)
                    {
                        req = true;
                        break;
                    }
                }

            if (req)
            {
                if (!(itemTemplate.AllowableClass & CLASSMASK_ALL_PLAYABLE))
                    TC_LOG_ERROR("sql.sql", "Item (Entry: %u) does not have any playable classes (%u) in `AllowableClass` and can't be equipped or used.", entry, itemTemplate.AllowableClass);

                if (!(itemTemplate.AllowableRace & RACEMASK_ALL_PLAYABLE))
                    TC_LOG_ERROR("sql.sql", "Item (Entry: %u) does not have any playable races (%u) in `AllowableRace` and can't be equipped or used.", entry, itemTemplate.AllowableRace);
            }
        }

        if (itemTemplate.RequiredSpell && !sSpellMgr->GetSpellInfo(itemTemplate.RequiredSpell))
        {
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has a wrong (non-existing) spell in RequiredSpell (%u)", entry, itemTemplate.RequiredSpell);
            itemTemplate.RequiredSpell = 0;
        }

        if (itemTemplate.RequiredReputationRank >= MAX_REPUTATION_RANK)
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong reputation rank in RequiredReputationRank (%u), item can't be used.", entry, itemTemplate.RequiredReputationRank);

        if (itemTemplate.RequiredReputationFaction)
        {
            if (!sFactionStore.LookupEntry(itemTemplate.RequiredReputationFaction))
            {
                TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong (not existing) faction in RequiredReputationFaction (%u)", entry, itemTemplate.RequiredReputationFaction);
                itemTemplate.RequiredReputationFaction = 0;
            }

            if (itemTemplate.RequiredReputationRank == MIN_REPUTATION_RANK)
                TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has min. reputation rank in RequiredReputationRank (0) but RequiredReputationFaction > 0, faction setting is useless.", entry);
        }

        if (itemTemplate.ContainerSlots > MAX_BAG_SIZE)
        {
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has too large value in ContainerSlots (%u), replace by hardcoded limit (%u).", entry, itemTemplate.ContainerSlots, MAX_BAG_SIZE);
            itemTemplate.ContainerSlots = MAX_BAG_SIZE;
        }
#ifdef LICH_KING
        if (itemTemplate.StatsCount > MAX_ITEM_PROTO_STATS)
        {
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has too large value in statscount (%u), replace by hardcoded limit (%u).", entry, itemTemplate.StatsCount, MAX_ITEM_PROTO_STATS);
            itemTemplate.StatsCount = MAX_ITEM_PROTO_STATS;
        }
        for (uint8 j = 0; j < itemTemplate.StatsCount; ++j)
#else
        for (uint8 j = 0; j < MAX_ITEM_PROTO_STATS; ++j)
#endif
        {
            // for ItemStatValue != 0
            if (itemTemplate.ItemStat[j].ItemStatValue && itemTemplate.ItemStat[j].ItemStatType >= MAX_ITEM_MOD)
            {
                TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong (non-existing?) stat_type%d (%u)", entry, j+1, itemTemplate.ItemStat[j].ItemStatType);
                itemTemplate.ItemStat[j].ItemStatType = 0;
            }

            switch (itemTemplate.ItemStat[j].ItemStatType)
            {
                case ITEM_MOD_SPELL_HEALING_DONE:
                case ITEM_MOD_SPELL_DAMAGE_DONE:
                    TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has deprecated stat_type%d (%u)", entry, j+1, itemTemplate.ItemStat[j].ItemStatType);
                    break;
                default:
                    break;
            }
        }

        for (uint8 j = 0; j < MAX_ITEM_PROTO_DAMAGES; ++j)
        {
            if (itemTemplate.Damage[j].DamageType >= MAX_SPELL_SCHOOL)
            {
                TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong dmg_type%d (%u)", entry, j+1, itemTemplate.Damage[j].DamageType);
                itemTemplate.Damage[j].DamageType = 0;
            }
        }

        // special format
        if ((itemTemplate.Spells[0].SpellId == 483) || (itemTemplate.Spells[0].SpellId == 55884))
        {
            // spell_1
            if (itemTemplate.Spells[0].SpellTrigger != ITEM_SPELLTRIGGER_ON_USE)
            {
                TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong item spell trigger value in spelltrigger_%d (%u) for special learning format", entry, 0+1, itemTemplate.Spells[0].SpellTrigger);
                itemTemplate.Spells[0].SpellId = 0;
                itemTemplate.Spells[0].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                itemTemplate.Spells[1].SpellId = 0;
                itemTemplate.Spells[1].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
            }

            // spell_2 have learning spell
            if (itemTemplate.Spells[1].SpellTrigger != ITEM_SPELLTRIGGER_LEARN_SPELL_ID)
            {
                TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong item spell trigger value in spelltrigger_%d (%u) for special learning format.", entry, 1+1, itemTemplate.Spells[1].SpellTrigger);
                itemTemplate.Spells[0].SpellId = 0;
                itemTemplate.Spells[1].SpellId = 0;
                itemTemplate.Spells[1].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
            }
            else if (!itemTemplate.Spells[1].SpellId)
            {
                TC_LOG_ERROR("sql.sql", "Item (Entry: %u) does not have an expected spell in spellid_%d in special learning format.", entry, 1+1);
                itemTemplate.Spells[0].SpellId = 0;
                itemTemplate.Spells[1].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
            }
            else if (itemTemplate.Spells[1].SpellId != -1)
            {
                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(itemTemplate.Spells[1].SpellId);
                if (!spellInfo /*&& !DisableMgr::IsDisabledFor(DISABLE_TYPE_SPELL, itemTemplate.Spells[1].SpellId, NULL) */)
                {
                    TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong (not existing) spell in spellid_%d (%d)", entry, 1+1, itemTemplate.Spells[1].SpellId);
                    itemTemplate.Spells[0].SpellId = 0;
                    itemTemplate.Spells[1].SpellId = 0;
                    itemTemplate.Spells[1].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                }
                // allowed only in special format
                else if ((itemTemplate.Spells[1].SpellId == 483) || (itemTemplate.Spells[1].SpellId == 55884))
                {
                    TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has broken spell in spellid_%d (%d)", entry, 1+1, itemTemplate.Spells[1].SpellId);
                    itemTemplate.Spells[0].SpellId = 0;
                    itemTemplate.Spells[1].SpellId = 0;
                    itemTemplate.Spells[1].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                }
            }

            // spell_3*, spell_4*, spell_5* is empty
            for (uint8 j = 2; j < MAX_ITEM_PROTO_SPELLS; ++j)
            {
                if (itemTemplate.Spells[j].SpellTrigger != ITEM_SPELLTRIGGER_ON_USE)
                {
                    TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong item spell trigger value in spelltrigger_%d (%u)", entry, j+1, itemTemplate.Spells[j].SpellTrigger);
                    itemTemplate.Spells[j].SpellId = 0;
                    itemTemplate.Spells[j].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                }
                else if (itemTemplate.Spells[j].SpellId != 0)
                {
                    TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong spell in spellid_%d (%d) for learning special format", entry, j+1, itemTemplate.Spells[j].SpellId);
                    itemTemplate.Spells[j].SpellId = 0;
                }
            }
        }
        // normal spell list
        else
        {
            for (uint8 j = 0; j < MAX_ITEM_PROTO_SPELLS; ++j)
            {
                if (itemTemplate.Spells[j].SpellTrigger >= MAX_ITEM_SPELLTRIGGER || itemTemplate.Spells[j].SpellTrigger == ITEM_SPELLTRIGGER_LEARN_SPELL_ID)
                {
                    TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong item spell trigger value in spelltrigger_%d (%u)", entry, j+1, itemTemplate.Spells[j].SpellTrigger);
                    itemTemplate.Spells[j].SpellId = 0;
                    itemTemplate.Spells[j].SpellTrigger = ITEM_SPELLTRIGGER_ON_USE;
                }

                if (itemTemplate.Spells[j].SpellId && itemTemplate.Spells[j].SpellId != -1)
                {
                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(itemTemplate.Spells[j].SpellId);
                    if (!spellInfo /* && !DisableMgr::IsDisabledFor(DISABLE_TYPE_SPELL, itemTemplate.Spells[j].SpellId, NULL) */)
                    {
                        TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong (not existing) spell in spellid_%d (%d)", entry, j+1, itemTemplate.Spells[j].SpellId);
                        itemTemplate.Spells[j].SpellId = 0;
                    }
                    // allowed only in special format
                    else if ((itemTemplate.Spells[j].SpellId == 483) || (itemTemplate.Spells[j].SpellId == 55884))
                    {
                        TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has broken spell in spellid_%d (%d)", entry, j+1, itemTemplate.Spells[j].SpellId);
                        itemTemplate.Spells[j].SpellId = 0;
                    }
                }
            }
        }

        if (itemTemplate.Bonding >= MAX_BIND_TYPE)
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong Bonding value (%u)", entry, itemTemplate.Bonding);

        if (itemTemplate.PageText && !GetPageText(itemTemplate.PageText))
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has non existing first page (Id:%u)", entry, itemTemplate.PageText);

        if (itemTemplate.LockID && !sLockStore.LookupEntry(itemTemplate.LockID))
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong LockID (%u)", entry, itemTemplate.LockID);

        if (itemTemplate.Sheath >= MAX_SHEATHETYPE)
        {
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong Sheath (%u)", entry, itemTemplate.Sheath);
            itemTemplate.Sheath = SHEATHETYPE_NONE;
        }

        if (itemTemplate.RandomProperty)
        {
            // To be implemented later
            if (itemTemplate.RandomProperty == -1)
                itemTemplate.RandomProperty = 0;

            else if (!sItemRandomPropertiesStore.LookupEntry(GetItemEnchantMod(itemTemplate.RandomProperty)))
            {
                TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has unknown (wrong or not listed in `item_enchantment_template`) RandomProperty (%u)", entry, itemTemplate.RandomProperty);
                itemTemplate.RandomProperty = 0;
            }
        }

        if (itemTemplate.RandomSuffix && !sItemRandomSuffixStore.LookupEntry(GetItemEnchantMod(itemTemplate.RandomSuffix)))
        {
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong RandomSuffix (%u)", entry, itemTemplate.RandomSuffix);
            itemTemplate.RandomSuffix = 0;
        }

        if (itemTemplate.ItemSet && !sItemSetStore.LookupEntry(itemTemplate.ItemSet))
        {
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) have wrong ItemSet (%u)", entry, itemTemplate.ItemSet);
            itemTemplate.ItemSet = 0;
        }

        if (itemTemplate.Area && !sAreaTableStore.LookupEntry(itemTemplate.Area))
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong Zone (%u)", entry, itemTemplate.Area);

        if (itemTemplate.Map && !sMapStore.LookupEntry(itemTemplate.Map))
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong Map (%u)", entry, itemTemplate.Map);

        if (itemTemplate.BagFamily)
        {
            // check bits
            /*
            for (uint32 j = 0; j < sizeof(itemTemplate.BagFamily)*8; ++j)
            {
                uint32 mask = 1 << j;
                if ((itemTemplate.BagFamily & mask) == 0)
                    continue;

                ItemBagFamilyEntry const* bf = sItemBagFamilyStore.LookupEntry(j+1);
                if (!bf)
                {
                    TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has bag family bit set not listed in ItemBagFamily.dbc, remove bit", entry);
                    itemTemplate.BagFamily &= ~mask;
                    continue;
                }

                if (BAG_FAMILY_MASK_CURRENCY_TOKENS & mask)
                {
                    CurrencyTypesEntry const* ctEntry = sCurrencyTypesStore.LookupEntry(itemTemplate.ItemId);
                    if (!ctEntry)
                    {
                        TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has currency bag family bit set in BagFamily but not listed in CurrencyTypes.dbc, remove bit", entry);
                        itemTemplate.BagFamily &= ~mask;
                    }
                }
            }*/
        }

        if (itemTemplate.TotemCategory && !sTotemCategoryStore.LookupEntry(itemTemplate.TotemCategory))
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong TotemCategory (%u)", entry, itemTemplate.TotemCategory);

        for (uint8 j = 0; j < MAX_ITEM_PROTO_SOCKETS; ++j)
        {
            if (itemTemplate.Socket[j].Color && (itemTemplate.Socket[j].Color & SOCKET_COLOR_ALL) != itemTemplate.Socket[j].Color)
            {
                TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong socketColor_%d (%u)", entry, j+1, itemTemplate.Socket[j].Color);
                itemTemplate.Socket[j].Color = 0;
            }
        }

        if (itemTemplate.GemProperties && !sGemPropertiesStore.LookupEntry(itemTemplate.GemProperties))
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong GemProperties (%u)", entry, itemTemplate.GemProperties);

        if (itemTemplate.FoodType >= MAX_PET_DIET)
        {
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong FoodType value (%u)", entry, itemTemplate.FoodType);
            itemTemplate.FoodType = 0;
        }
        /*
        if (itemTemplate.ItemLimitCategory && !sItemLimitCategoryStore.LookupEntry(itemTemplate.ItemLimitCategory))
        {
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong LimitCategory value (%u)", entry, itemTemplate.ItemLimitCategory);
            itemTemplate.ItemLimitCategory = 0;
        }

        if (itemTemplate.HolidayId && !sHolidaysStore.LookupEntry(itemTemplate.HolidayId))
        {
            TC_LOG_ERROR("sql.sql", "Item (Entry: %u) has wrong HolidayId value (%u)", entry, itemTemplate.HolidayId);
            itemTemplate.HolidayId = 0;
        }

        if (itemTemplate.FlagsCu & ITEM_FLAG_CU_DURATION_REAL_TIME && !itemTemplate.Duration)
        {
            TC_LOG_ERROR("sql.sql", "Item (Entry %u) has flag ITEM_FLAG_CU_DURATION_REAL_TIME but it does not have duration limit", entry);
            itemTemplate.FlagsCu &= ~ITEM_FLAG_CU_DURATION_REAL_TIME;
        }*/
    } while (result->NextRow());

    // Check if item templates for DBC referenced character start outfit are present
    std::set<uint32> notFoundOutfit;
    for (uint32 i = 1; i < sCharStartOutfitStore.GetNumRows(); ++i)
    {
        CharStartOutfitEntry const* entry = sCharStartOutfitStore.LookupEntry(i);
        if (!entry)
            continue;

        for (int j : entry->ItemId)
        {
            if (j <= 0)
                continue;

            uint32 item_id = j;

            if (!GetItemTemplate(item_id))
                notFoundOutfit.insert(item_id);
        }
    }

    for (uint32 itr : notFoundOutfit)
    {
        TC_LOG_ERROR("sql.sql", "Item (Entry: %u) does not exist in `item_template` but is referenced in `CharStartOutfit.dbc`", itr);
    }

    TC_LOG_INFO("server.loading", ">> Loaded " SZFMTD " item templates in %u ms", _itemTemplateStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadPetLevelInfo()
{
    // Loading levels data
    {
        //                                                0               1      2   3     4    5    6    7     8    9      10       11
        QueryResult result  = WorldDatabase.Query("SELECT creature_entry, level, hp, mana, str, agi, sta, inte, spi, armor, min_dmg, max_dmg FROM pet_levelstats");

        uint32 count = 0;

        if (!result)
        {
            TC_LOG_ERROR("server.loading", "Error loading `pet_levelstats` table or empty table.");
            return;
        }
        do
        {
            Field* fields = result->Fetch();

            uint32 creature_id = fields[0].GetUInt32();
            if(!sObjectMgr->GetCreatureTemplate(creature_id))
            {
                TC_LOG_ERROR("sql.sql","Wrong creature id %u in `pet_levelstats` table, ignoring.",creature_id);
                continue;
            }

            uint32 current_level = fields[1].GetUInt8();
            if(current_level > sWorld->getConfig(CONFIG_MAX_PLAYER_LEVEL))
            {
                if(current_level > STRONG_MAX_LEVEL)        // hardcoded level maximum
                    TC_LOG_ERROR("sql.sql","Wrong (> %u) level %u in `pet_levelstats` table, ignoring.",STRONG_MAX_LEVEL,current_level);
                else
                    TC_LOG_DEBUG("misc","Unused (> MaxPlayerLevel in worldserver.conf) level %u in `pet_levelstats` table, ignoring.",current_level);
                continue;
            }
            else if(current_level < 1)
            {
                TC_LOG_ERROR("sql.sql","Wrong (<1) level %u in `pet_levelstats` table, ignoring.",current_level);
                continue;
            }

            auto& pInfoMapEntry = _petInfoStore[creature_id];

            if(pInfoMapEntry==nullptr)
                pInfoMapEntry = Trinity::make_unique<PetLevelInfo[]>(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL));

            // data for level 1 stored in [0] array element, ...
            PetLevelInfo* pLevelInfo = &pInfoMapEntry[current_level-1];

            pLevelInfo->health = fields[2].GetUInt16();
            pLevelInfo->mana   = fields[3].GetUInt16();
            pLevelInfo->armor  = fields[9].GetUInt32();
            pLevelInfo->minDamage = fields[10].GetUInt16();
            pLevelInfo->maxDamage = fields[11].GetUInt16();

            for (uint8 i = 0; i < MAX_STATS; i++)
                pLevelInfo->stats[i] = fields[i+4].GetUInt16();
            
            ++count;
        }
        while (result->NextRow());

        TC_LOG_INFO("server.loading", ">> Loaded %u level pet stats definitions", count );
    }

    // Fill gaps and check integrity
    for (auto itr = _petInfoStore.begin(); itr != _petInfoStore.end(); ++itr)
    {
        auto& pInfo = itr->second;

        // fatal error if no level 1 data
        if(!pInfo || pInfo[0].health == 0 )
        {
            TC_LOG_ERROR("sql.sql","Creature %u does not have pet stats data for Level 1!",itr->first);
            ABORT();
        }

        // fill level gaps
        for (uint32 level = 1; level < sWorld->getConfig(CONFIG_MAX_PLAYER_LEVEL); ++level)
        {
            if(pInfo[level].health == 0)
            {
                TC_LOG_ERROR("sql.sql","Creature %u has no data for Level %i pet stats data, using data of Level %i.",itr->first,level+1, level);
                pInfo[level] = pInfo[level-1];
            }
        }
    }
}

PetLevelInfo const* ObjectMgr::GetPetLevelInfo(uint32 creature_id, uint32 level) const
{
    if(level > sWorld->getConfig(CONFIG_MAX_PLAYER_LEVEL))
        level = sWorld->getConfig(CONFIG_MAX_PLAYER_LEVEL);

    auto itr = _petInfoStore.find(creature_id);
    if(itr == _petInfoStore.end())
        return nullptr;

    return &itr->second[level-1];                           // data for level 1 stored in [0] array element, ...
}

void ObjectMgr::LoadPlayerInfo()
{
    // Load playercreate
    {
        //                                               0     1      2    3     4           5           6           7
        QueryResult result = WorldDatabase.Query("SELECT race, class, map, zone, position_x, position_y, position_z, position_o FROM playercreateinfo");

        uint32 count = 0;

        if (!result)
        {
            TC_LOG_ERROR("server.loading", ">> Loaded 0 player create definitions. DB table `playercreateinfo` is empty.");
            ABORT();
        }

        do
        {
            Field* fields = result->Fetch();

            uint32 current_race  = fields[0].GetUInt8();
            uint32 current_class = fields[1].GetUInt8();
            uint32 mapId         = fields[2].GetUInt16();
            uint32 areaId        = fields[3].GetUInt32();
            float positionX      = fields[4].GetFloat();
            float positionY      = fields[5].GetFloat();
            float positionZ      = fields[6].GetFloat();
            float positionO      = fields[7].GetFloat();

            if (current_race >= MAX_RACES)
            {
                TC_LOG_ERROR("sql.sql","Wrong race %u in `playercreateinfo` table, ignoring.", current_race);
                continue;
            }

            ChrRacesEntry const* rEntry = sChrRacesStore.LookupEntry(current_race);
            if (!rEntry)
            {
                TC_LOG_ERROR("sql.sql","Wrong race %u in `playercreateinfo` table, ignoring.", current_race);
                continue;
            }

            if (current_class >= MAX_CLASSES)
            {
                TC_LOG_ERROR("sql.sql","Wrong class %u in `playercreateinfo` table, ignoring.", current_class);
                continue;
            }

            if (!sChrClassesStore.LookupEntry(current_class))
            {
                TC_LOG_ERROR("sql.sql","Wrong class %u in `playercreateinfo` table, ignoring.", current_class);
                continue;
            }

            // accept DB data only for valid position (and non instanceable)
            if (!MapManager::IsValidMapCoord(mapId, positionX, positionY, positionZ, positionO))
            {
                TC_LOG_ERROR("sql.sql","Wrong home position for class %u race %u pair in `playercreateinfo` table, ignoring.", current_class, current_race);
                continue;
            }

            if (sMapStore.LookupEntry(mapId)->Instanceable())
            {
                TC_LOG_ERROR("sql.sql","Home position in instanceable map for class %u race %u pair in `playercreateinfo` table, ignoring.", current_class, current_race);
                continue;
            }

            std::unique_ptr<PlayerInfo> info = Trinity::make_unique<PlayerInfo>();

            info->mapId     = mapId;
            info->areaId    = areaId;
            info->positionX = positionX;
            info->positionY = positionY;
            info->positionZ = positionZ;
            info->positionO = positionO;
            info->displayId_m = rEntry->model_m;
            info->displayId_f = rEntry->model_f;

            _playerInfo[current_race][current_class] = std::move(info);

            ++count;
        }
        while (result->NextRow());

        TC_LOG_INFO("server.loading", ">> Loaded %u player create definitions", count);
    }

    // Load playercreate items
    TC_LOG_INFO("server.loading", "Loading Player Create Items Data...");
    {
        //                                                0     1      2       3
        QueryResult result = WorldDatabase.Query("SELECT race, class, itemid, amount FROM playercreateinfo_item");

        uint32 count = 0;

        if (!result)
        {
            TC_LOG_INFO( "server.loading",">> Loaded %u custom player create items", count );
        }
        else
        {
            do
            {
                Field* fields = result->Fetch();

                uint32 current_race = fields[0].GetUInt32();
                if(current_race >= MAX_RACES)
                {
                    TC_LOG_ERROR("sql.sql","Wrong race %u in `playercreateinfo_item` table, ignoring.",current_race);
                    continue;
                }

                uint32 current_class = fields[1].GetUInt32();
                if(current_class >= MAX_CLASSES)
                {
                    TC_LOG_ERROR("sql.sql","Wrong class %u in `playercreateinfo_item` table, ignoring.",current_class);
                    continue;
                }

                auto& pInfo = _playerInfo[current_race][current_class];

                uint32 item_id = fields[2].GetUInt32();

                if(!GetItemTemplate(item_id))
                {
                    TC_LOG_ERROR("sql.sql","Item id %u (race %u class %u) in `playercreateinfo_item` table but not listed in `item_template`, ignoring.",item_id,current_race,current_class);
                    continue;
                }

                uint32 amount  = fields[3].GetUInt32();

                if(!amount)
                {
                    TC_LOG_ERROR("sql.sql","Item id %u (class %u race %u) have amount==0 in `playercreateinfo_item` table, ignoring.",item_id,current_race,current_class);
                    continue;
                }

                pInfo->item.push_back(PlayerCreateInfoItem( item_id, amount));

                ++count;
            }
            while(result->NextRow());

            TC_LOG_INFO("server.loading", ">> Loaded %u custom player create items", count );
        }
    }

    TC_LOG_INFO("server.loading", "Loading Player Create Skill Data...");
    {
        uint32 oldMSTime = GetMSTime();

        QueryResult result = WorldDatabase.PQuery("SELECT raceMask, classMask, skill, `rank` FROM playercreateinfo_skills");

        if (!result)
        {
            TC_LOG_INFO("server.loading", ">> Loaded 0 player create skills. DB table `playercreateinfo_skills` is empty.");
        }
        else
        {
            uint32 count = 0;

            do
            {
                Field* fields = result->Fetch();
                uint32 raceMask = fields[0].GetUInt32();
                uint32 classMask = fields[1].GetUInt32();
                PlayerCreateInfoSkill skill;
                skill.SkillId = fields[2].GetUInt16();
                skill.Rank = fields[3].GetUInt16();

                if (skill.Rank >= MAX_SKILL_STEP)
                {
                    TC_LOG_ERROR("sql.sql", "Skill rank value %hu set for skill %hu raceMask %u classMask %u is too high, max allowed value is %d", skill.Rank, skill.SkillId, raceMask, classMask, MAX_SKILL_STEP);
                    continue;
                }

                if (raceMask != 0 && !(raceMask & RACEMASK_ALL_PLAYABLE))
                {
                    TC_LOG_ERROR("sql.sql", "Wrong race mask %u in `playercreateinfo_skills` table, ignoring.", raceMask);
                    continue;
                }

                if (classMask != 0 && !(classMask & CLASSMASK_ALL_PLAYABLE))
                {
                    TC_LOG_ERROR("sql.sql", "Wrong class mask %u in `playercreateinfo_skills` table, ignoring.", classMask);
                    continue;
                }

                if (!sSkillLineStore.LookupEntry(skill.SkillId))
                {
                    TC_LOG_ERROR("sql.sql", "Wrong skill id %u in `playercreateinfo_skills` table, ignoring.", skill.SkillId);
                    continue;
                }

                for (uint32 raceIndex = RACE_HUMAN; raceIndex < MAX_RACES; ++raceIndex)
                {
                    if (raceMask == 0 || ((1 << (raceIndex - 1)) & raceMask))
                    {
                        for (uint32 classIndex = CLASS_WARRIOR; classIndex < MAX_CLASSES; ++classIndex)
                        {
                            if (classMask == 0 || ((1 << (classIndex - 1)) & classMask))
                            {
                                if (!GetSkillRaceClassInfo(skill.SkillId, raceIndex, classIndex))
                                    continue;

                                if (auto& info = _playerInfo[raceIndex][classIndex])
                                {
                                    info->skills.push_back(skill);
                                    ++count;
                                }
                            }
                        }
                    }
                }
            } while (result->NextRow());

            TC_LOG_INFO("server.loading", ">> Loaded %u player create skills in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
        }
    }

    // Load playercreate spells
    {

        QueryResult result = WorldDatabase.Query("SELECT race, class, Spell, Active FROM playercreateinfo_spell");

        uint32 count = 0;

        if (!result)
        {
            TC_LOG_ERROR("sql.sql", "Error loading player starting spells or empty table.");
        }
        else
        {
            do
            {
                Field* fields = result->Fetch();

                uint32 current_race = fields[0].GetUInt8();
                if(current_race >= MAX_RACES)
                {
                    TC_LOG_ERROR("sql.sql","Wrong race %u in `playercreateinfo_spell` table, ignoring.",current_race);
                    continue;
                }

                uint32 current_class = fields[1].GetUInt8();
                if(current_class >= MAX_CLASSES)
                {
                    TC_LOG_ERROR("sql.sql","Wrong class %u in `playercreateinfo_spell` table, ignoring.",current_class);
                    continue;
                }

                auto& pInfo = _playerInfo[current_race][current_class];
                uint32 spell = fields[2].GetUInt32();
                DEBUG_ASSERT(spell < std::numeric_limits<uint16>::max());
                pInfo->spell.push_back(CreateSpellPair(uint16(fields[2].GetUInt32()), fields[3].GetUInt8()));

                ++count;
            }
            while( result->NextRow() );

            TC_LOG_INFO( "server.loading",">> Loaded %u player create spells", count );
        }
    }

    // Load playercreate actions
    {
        //                                                0     1      2       3       4     5
        QueryResult result = WorldDatabase.Query("SELECT race, class, button, action, type, misc FROM playercreateinfo_action");

        uint32 count = 0;

        if (!result)
        {
            TC_LOG_ERROR( "server.loading","Error loading `playercreateinfo_action` table or empty table.");
        }
        else
        {
            do
            {
                Field* fields = result->Fetch();

                uint32 current_race = fields[0].GetUInt8();
                if(current_race >= MAX_RACES)
                {
                    TC_LOG_ERROR("sql.sql","Wrong race %u in `playercreateinfo_action` table, ignoring.",current_race);
                    continue;
                }

                uint32 current_class = fields[1].GetUInt8();
                if(current_class >= MAX_CLASSES)
                {
                    TC_LOG_ERROR("sql.sql","Wrong class %u in `playercreateinfo_action` table, ignoring.",current_class);
                    continue;
                }

                auto& pInfo = _playerInfo[current_race][current_class];
                pInfo->action[0].push_back(fields[2].GetUInt16());
                pInfo->action[1].push_back(fields[3].GetUInt16());
                pInfo->action[2].push_back(fields[4].GetUInt16());
                pInfo->action[3].push_back(fields[5].GetUInt16());

                ++count;
            }
            while( result->NextRow() );

            TC_LOG_INFO("server.loading", ">> Loaded %u player create actions", count );
        }
    }

    // Loading levels data (class only dependent)
    {
        //                                                 0      1      2       3
        QueryResult result  = WorldDatabase.Query("SELECT class, level, basehp, basemana FROM player_classlevelstats");

        uint32 count = 0;

        if (!result)
        {
            TC_LOG_ERROR( "server.loading", "Error loading `player_classlevelstats` table or empty table.");
            ABORT();
        }

        do
        {
            Field* fields = result->Fetch();

            uint32 current_class = fields[0].GetUInt8();
            if(current_class >= MAX_CLASSES)
            {
                TC_LOG_ERROR("sql.sql","Wrong class %u in `player_classlevelstats` table, ignoring.",current_class);
                continue;
            }

            uint32 current_level = fields[1].GetUInt8();
            if(current_level > sWorld->getConfig(CONFIG_MAX_PLAYER_LEVEL))
            {
                if(current_level > STRONG_MAX_LEVEL)        // hardcoded level maximum
                    TC_LOG_ERROR("sql.sql","Wrong (> %u) level %u in `player_classlevelstats` table, ignoring.", STRONG_MAX_LEVEL,current_level);
                else
                    TC_LOG_DEBUG("sql.sql","Unused (> MaxPlayerLevel in worldserver.conf) level %u in `player_classlevelstats` table, ignoring.",current_level);
                continue;
            }
            else if (current_level == 0) {
                TC_LOG_ERROR("sql.sql", "Wrong level 0 in `player_classlevelstats` table, ignoring.");
                continue;
            }

            auto& info = _playerClassInfo[current_class];
            if (!info)
            {
                info = Trinity::make_unique<PlayerClassInfo>();
                info->levelInfo = Trinity::make_unique<PlayerClassLevelInfo[]>(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL));
                _playerClassInfo[current_class] = std::move(info);
            }


            PlayerClassLevelInfo& levelInfo = info->levelInfo[current_level - 1];

            levelInfo.basehealth = fields[2].GetUInt16();
            levelInfo.basemana   = fields[3].GetUInt16();

            ++count;
        }
        while (result->NextRow());

        TC_LOG_INFO( "server.loading",">> Loaded %u level health/mana definitions", count );
    }

    // Fill gaps and check integrity
    for (int class_ = 0; class_ < MAX_CLASSES; ++class_)
    {
        // skip non existed classes
        if(!sChrClassesStore.LookupEntry(class_))
            continue;

        auto& pClassInfo = _playerClassInfo[class_];

        // fatal error if no level 1 data
        if(!pClassInfo->levelInfo || pClassInfo->levelInfo[0].basehealth == 0 )
        {
            TC_LOG_ERROR("FIXME","Class %i Level 1 does not have health/mana data!",class_);
            ABORT();
        }

        // fill level gaps
        for (uint32 level = 1; level < sWorld->getConfig(CONFIG_MAX_PLAYER_LEVEL); ++level)
        {
            if(pClassInfo->levelInfo[level].basehealth == 0)
            {
                TC_LOG_ERROR("FIXME","Class %i Level %i does not have health/mana data. Using stats data of level %i.",class_,level+1, level);
                pClassInfo->levelInfo[level] = pClassInfo->levelInfo[level-1];
            }
        }
    }

    // Loading levels data (class/race dependent)
    {
        //                                                 0     1      2      3    4    5    6    7
        QueryResult result  = WorldDatabase.Query("SELECT race, class, level, str, agi, sta, inte, spi FROM player_levelstats");

        uint32 count = 0;

        if (!result)
        {
            TC_LOG_ERROR( "player.loading", "Error loading `player_levelstats` table or empty table.");
            ABORT();
        }

        do
        {
            Field* fields = result->Fetch();

            uint32 current_race = fields[0].GetUInt8();
            if(current_race >= MAX_RACES)
            {
                TC_LOG_ERROR("sql.sql","Wrong race %u in `player_levelstats` table, ignoring.",current_race);
                continue;
            }

            uint32 current_class = fields[1].GetUInt8();
            if(current_class >= MAX_CLASSES)
            {
                TC_LOG_ERROR("sql.sql","Wrong class %u in `player_levelstats` table, ignoring.",current_class);
                continue;
            }

            uint32 current_level = fields[2].GetUInt8();
            if(current_level > sWorld->getConfig(CONFIG_MAX_PLAYER_LEVEL))
            {
                if(current_level > STRONG_MAX_LEVEL)        // hardcoded level maximum
                    TC_LOG_ERROR("sql.sql","Wrong (> %u) level %u in `player_levelstats` table, ignoring.",STRONG_MAX_LEVEL,current_level);
                else
                    TC_LOG_DEBUG("sql.sql","Unused (> MaxPlayerLevel in worldserver.conf) level %u in `player_levelstats` table, ignoring.",current_level);
                continue;
            }

            auto& pInfo = _playerInfo[current_race][current_class];

            if(!pInfo->levelInfo)
                pInfo->levelInfo = Trinity::make_unique<PlayerLevelInfo[]>(sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL));

            PlayerLevelInfo* pLevelInfo = &pInfo->levelInfo[current_level-1];

            for (int i = 0; i < MAX_STATS; i++)
            {
                pLevelInfo->stats[i] = fields[i+3].GetUInt8();
            }

            ++count;
        }
        while (result->NextRow());

        TC_LOG_INFO("server.loading", ">> Loaded %u level stats definitions", count );
    }

    // Fill gaps and check integrity
    for (int race = 0; race < MAX_RACES; ++race)
    {
        // skip non existed races
        if(!sChrRacesStore.LookupEntry(race))
            continue;

        for (int class_ = 0; class_ < MAX_CLASSES; ++class_)
        {
            // skip non existed classes
            if(!sChrClassesStore.LookupEntry(class_))
                continue;

            auto& pInfo = _playerInfo[race][class_];
            if (!pInfo)
                continue;

            // skip non loaded combinations
            if(!pInfo->displayId_m || !pInfo->displayId_f)
                continue;

            // skip expansion races if not playing with expansion
            if (sWorld->getConfig(CONFIG_EXPANSION) < 1 && (race == RACE_BLOODELF || race == RACE_DRAENEI))
                continue;

#ifdef LICH_KING
            // skip expansion classes if not playing with expansion
            if (sWorld->getConfig(CONFIG_EXPANSION) < 2 && class_ == CLASS_DEATH_KNIGHT)
                continue;
#endif

            // fatal error if no level 1 data
            if(!pInfo->levelInfo || pInfo->levelInfo[0].stats[0] == 0 )
            {
                TC_LOG_ERROR("sql.sql","Race %i Class %i Level 1 does not have stats data!",race,class_);
                ABORT();
            }

            // fill level gaps
            for (uint32 level = 1; level < sWorld->getConfig(CONFIG_MAX_PLAYER_LEVEL); ++level)
            {
                if(pInfo->levelInfo[level].stats[0] == 0)
                {
                    TC_LOG_ERROR("sql.sql","Race %i Class %i Level %i does not have stats data. Using stats data of level %i.",race,class_,level+1, level);
                    pInfo->levelInfo[level] = pInfo->levelInfo[level-1];
                }
            }
        }
    }
}

void ObjectMgr::GetPlayerClassLevelInfo(uint32 class_, uint32 level, PlayerClassLevelInfo* info) const
{
    if(level < 1 || class_ >= MAX_CLASSES)
        return;

    auto& pInfo = _playerClassInfo[class_];

    if(level > sWorld->getConfig(CONFIG_MAX_PLAYER_LEVEL))
        level = sWorld->getConfig(CONFIG_MAX_PLAYER_LEVEL);

    *info = pInfo->levelInfo[level-1];
}

void ObjectMgr::GetPlayerLevelInfo(uint32 race, uint32 class_, uint32 level, PlayerLevelInfo* info) const
{
    if(level < 1 || race   >= MAX_RACES || class_ >= MAX_CLASSES)
        return;

    auto& pInfo = _playerInfo[race][class_];
    if(pInfo->displayId_m==0 || pInfo->displayId_f==0)
        return;

    if(level <= sWorld->getConfig(CONFIG_MAX_PLAYER_LEVEL))
        *info = pInfo->levelInfo[level-1];
    else
        BuildPlayerLevelInfo(race,class_,level,info);
}

PlayerInfo const* ObjectMgr::GetPlayerInfo(uint32 race, uint32 class_) const
{
    if (race >= MAX_RACES)   return nullptr;
    if (class_ >= MAX_CLASSES) return nullptr;
    auto& info = _playerInfo[race][class_];
    if (!info)
        return nullptr;
    if (info->displayId_m == 0 || info->displayId_f == 0) 
        return nullptr;
    return info.get();
}

void ObjectMgr::BuildPlayerLevelInfo(uint8 race, uint8 _class, uint8 level, PlayerLevelInfo* info) const
{
    // base data (last known level)
    *info = _playerInfo[race][_class]->levelInfo[sWorld->getConfig(CONFIG_MAX_PLAYER_LEVEL)-1];

    for(int lvl = sWorld->getConfig(CONFIG_MAX_PLAYER_LEVEL)-1; lvl < level; ++lvl)
    {
        switch(_class)
        {
            case CLASS_WARRIOR:
                info->stats[STAT_STRENGTH]  += (lvl > 23 ? 2: (lvl > 1  ? 1: 0));
                info->stats[STAT_STAMINA]   += (lvl > 23 ? 2: (lvl > 1  ? 1: 0));
                info->stats[STAT_AGILITY]   += (lvl > 36 ? 1: (lvl > 6 && (lvl%2) ? 1: 0));
                info->stats[STAT_INTELLECT] += (lvl > 9 && !(lvl%2) ? 1: 0);
                info->stats[STAT_SPIRIT]    += (lvl > 9 && !(lvl%2) ? 1: 0);
                break;
            case CLASS_PALADIN:
                info->stats[STAT_STRENGTH]  += (lvl > 3  ? 1: 0);
                info->stats[STAT_STAMINA]   += (lvl > 33 ? 2: (lvl > 1 ? 1: 0));
                info->stats[STAT_AGILITY]   += (lvl > 38 ? 1: (lvl > 7 && !(lvl%2) ? 1: 0));
                info->stats[STAT_INTELLECT] += (lvl > 6 && (lvl%2) ? 1: 0);
                info->stats[STAT_SPIRIT]    += (lvl > 7 ? 1: 0);
                break;
            case CLASS_HUNTER:
                info->stats[STAT_STRENGTH]  += (lvl > 4  ? 1: 0);
                info->stats[STAT_STAMINA]   += (lvl > 4  ? 1: 0);
                info->stats[STAT_AGILITY]   += (lvl > 33 ? 2: (lvl > 1 ? 1: 0));
                info->stats[STAT_INTELLECT] += (lvl > 8 && (lvl%2) ? 1: 0);
                info->stats[STAT_SPIRIT]    += (lvl > 38 ? 1: (lvl > 9 && !(lvl%2) ? 1: 0));
                break;
            case CLASS_ROGUE:
                info->stats[STAT_STRENGTH]  += (lvl > 5  ? 1: 0);
                info->stats[STAT_STAMINA]   += (lvl > 4  ? 1: 0);
                info->stats[STAT_AGILITY]   += (lvl > 16 ? 2: (lvl > 1 ? 1: 0));
                info->stats[STAT_INTELLECT] += (lvl > 8 && !(lvl%2) ? 1: 0);
                info->stats[STAT_SPIRIT]    += (lvl > 38 ? 1: (lvl > 9 && !(lvl%2) ? 1: 0));
                break;
            case CLASS_PRIEST:
                info->stats[STAT_STRENGTH]  += (lvl > 9 && !(lvl%2) ? 1: 0);
                info->stats[STAT_STAMINA]   += (lvl > 5  ? 1: 0);
                info->stats[STAT_AGILITY]   += (lvl > 38 ? 1: (lvl > 8 && (lvl%2) ? 1: 0));
                info->stats[STAT_INTELLECT] += (lvl > 22 ? 2: (lvl > 1 ? 1: 0));
                info->stats[STAT_SPIRIT]    += (lvl > 3  ? 1: 0);
                break;
            case CLASS_SHAMAN:
                info->stats[STAT_STRENGTH]  += (lvl > 34 ? 1: (lvl > 6 && (lvl%2) ? 1: 0));
                info->stats[STAT_STAMINA]   += (lvl > 4 ? 1: 0);
                info->stats[STAT_AGILITY]   += (lvl > 7 && !(lvl%2) ? 1: 0);
                info->stats[STAT_INTELLECT] += (lvl > 5 ? 1: 0);
                info->stats[STAT_SPIRIT]    += (lvl > 4 ? 1: 0);
                break;
            case CLASS_MAGE:
                info->stats[STAT_STRENGTH]  += (lvl > 9 && !(lvl%2) ? 1: 0);
                info->stats[STAT_STAMINA]   += (lvl > 5  ? 1: 0);
                info->stats[STAT_AGILITY]   += (lvl > 9 && !(lvl%2) ? 1: 0);
                info->stats[STAT_INTELLECT] += (lvl > 24 ? 2: (lvl > 1 ? 1: 0));
                info->stats[STAT_SPIRIT]    += (lvl > 33 ? 2: (lvl > 2 ? 1: 0));
                break;
            case CLASS_WARLOCK:
                info->stats[STAT_STRENGTH]  += (lvl > 9 && !(lvl%2) ? 1: 0);
                info->stats[STAT_STAMINA]   += (lvl > 38 ? 2: (lvl > 3 ? 1: 0));
                info->stats[STAT_AGILITY]   += (lvl > 9 && !(lvl%2) ? 1: 0);
                info->stats[STAT_INTELLECT] += (lvl > 33 ? 2: (lvl > 2 ? 1: 0));
                info->stats[STAT_SPIRIT]    += (lvl > 38 ? 2: (lvl > 3 ? 1: 0));
                break;
            case CLASS_DRUID:
                info->stats[STAT_STRENGTH]  += (lvl > 38 ? 2: (lvl > 6 && (lvl%2) ? 1: 0));
                info->stats[STAT_STAMINA]   += (lvl > 32 ? 2: (lvl > 4 ? 1: 0));
                info->stats[STAT_AGILITY]   += (lvl > 38 ? 2: (lvl > 8 && (lvl%2) ? 1: 0));
                info->stats[STAT_INTELLECT] += (lvl > 38 ? 3: (lvl > 4 ? 1: 0));
                info->stats[STAT_SPIRIT]    += (lvl > 38 ? 3: (lvl > 5 ? 1: 0));
        }
    }
}

void ObjectMgr::LoadQuests()
{
    uint32 oldMSTime = GetMSTime();

    // For reload case
    _questTemplates.clear();

    _exclusiveQuestGroups.clear();

    //                                               0                     1      
    QueryResult result = WorldDatabase.PQuery("SELECT entry, Method, ZoneOrSort, MinLevel, QuestLevel, Type, SuggestedPlayers, LimitTime, RequiredRaces, "
    //   
        "RepObjectiveFaction, RepObjectiveValue, "
    //   
        "QuestFlags, CharTitleId, NextQuestId, SrcItemId, "
    //
        "Title, Details, Objectives, EndText, ObjectiveText1, ObjectiveText2, ObjectiveText3, ObjectiveText4,"
        "ReqItemId1, ReqItemId2, ReqItemId3, ReqItemId4, ReqItemCount1, ReqItemCount2, ReqItemCount3, ReqItemCount4,"
        "ReqSourceId1, ReqSourceId2, ReqSourceId3, ReqSourceId4, ReqSourceCount1, ReqSourceCount2, ReqSourceCount3, ReqSourceCount4, ReqSourceRef1, ReqSourceRef2, ReqSourceRef3, ReqSourceRef4,"
        "ReqCreatureOrGOId1, ReqCreatureOrGOId2, ReqCreatureOrGOId3, ReqCreatureOrGOId4, ReqCreatureOrGOCount1, ReqCreatureOrGOCount2, ReqCreatureOrGOCount3, ReqCreatureOrGOCount4,"
        "ReqSpellCast1, ReqSpellCast2, ReqSpellCast3, ReqSpellCast4,"
        "RewChoiceItemId1, RewChoiceItemId2, RewChoiceItemId3, RewChoiceItemId4, RewChoiceItemId5, RewChoiceItemId6,"
        "RewChoiceItemCount1, RewChoiceItemCount2, RewChoiceItemCount3, RewChoiceItemCount4, RewChoiceItemCount5, RewChoiceItemCount6,"
        "RewItemId1, RewItemId2, RewItemId3, RewItemId4, RewItemCount1, RewItemCount2, RewItemCount3, RewItemCount4,"
        "RewRepFaction1, RewRepFaction2, RewRepFaction3, RewRepFaction4, RewRepFaction5, RewRepValue1, RewRepValue2, RewRepValue3, RewRepValue4, RewRepValue5,"
        "RewHonorableKills, RewOrReqMoney, RewMoneyMaxLevel, RewSpell, RewSpellCast, PointMapId, PointX, PointY, PointOpt,"
        "StartScript, CompleteScript "
        " FROM quest_template t1 WHERE patch=(SELECT max(patch) FROM quest_template t2 WHERE t1.entry = t2.entry && patch <= %u)", sWorld->GetWowPatch());

    if(result == nullptr)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 quests definitions" );
        TC_LOG_ERROR("server.loading","`quest_template` table is empty!");
        return;
    }
    
    // create multimap previous quest for each existed quest
    // some quests can have many previous maps set by NextQuestId in previous quest
    // for example set of race quests can lead to single not race specific quest
    do
    {
        Field *fields = result->Fetch();

        uint32 questId = fields[0].GetUInt32();
        _questTemplates.emplace(std::piecewise_construct, std::forward_as_tuple(questId), std::forward_as_tuple(fields));
    } while( result->NextRow() );

    std::unordered_map<uint32, uint32> usedMailTemplates;

    struct QuestLoaderHelper
    {
        typedef void(Quest::*QuestLoaderFunction)(Field* fields);

        char const* QueryFields;
        char const* TableName;
        char const* TableDesc;
        QuestLoaderFunction LoaderFunction;
    };

    static std::vector<QuestLoaderHelper> const QuestLoaderHelpers =
    {
        // 0   1       2       3       4       5            6            7            8
        { "ID, Emote1, Emote2, Emote3, Emote4, EmoteDelay1, EmoteDelay2, EmoteDelay3, EmoteDelay4",                       "quest_details",        "details",             &Quest::LoadQuestDetails },

            // 0   1                2                  3
        { "ID, EmoteOnComplete, EmoteOnIncomplete, CompletionText",                                                       "quest_request_items",  "request items",       &Quest::LoadQuestRequestItems },

        // 0   1       2       3       4       5            6            7            8            9
        { "ID, Emote1, Emote2, Emote3, Emote4, EmoteDelay1, EmoteDelay2, EmoteDelay3, EmoteDelay4, RewardText",           "quest_offer_reward",   "reward emotes",       &Quest::LoadQuestOfferReward },


        // 0   1         2                 3              4            5            6               7                     8
        { "ID, MaxLevel, AllowableClasses, SourceSpellID, PrevQuestID, NextQuestID, ExclusiveGroup, RewardMailTemplateID, RewardMailDelay,"
        // 9               10                   11                     12                     13                   14                   15                 16
        " RequiredSkillID, RequiredSkillPoints, RequiredMinRepFaction, RequiredMaxRepFaction, RequiredMinRepValue, RequiredMaxRepValue, ProvidedItemCount, SpecialFlags", "quest_template_addon", "template addons",     &Quest::LoadQuestTemplateAddon },

            // 0        1
        { "QuestId, RewardMailSenderEntry",                                                                               "quest_mail_sender",    "mail sender entries", &Quest::LoadQuestMailSender },
    };

    for (QuestLoaderHelper const& loader : QuestLoaderHelpers)
    {
        QueryResult result2 = WorldDatabase.PQuery("SELECT %s FROM %s", loader.QueryFields, loader.TableName);
        if (!result2)
            TC_LOG_INFO("server.loading", ">> Loaded 0 quest %s. DB table `%s` is empty.", loader.TableDesc, loader.TableName);
        else
        {
            do
            {
                Field* fields = result2->Fetch();
                uint32 questId = fields[0].GetUInt32();

                auto itr = _questTemplates.find(questId);
                if (itr != _questTemplates.end())
                    (itr->second.*loader.LoaderFunction)(fields);
                else
                    TC_LOG_ERROR("server.loading", "Table `%s` has data for quest %u but such quest does not exist", loader.TableName, questId);
            } while (result2->NextRow());
        }
    }


    // Post processing
    for (auto& questPair : _questTemplates)
    {
        /*TC // skip post-loading checks for disabled quests
        if (DisableMgr::IsDisabledFor(DISABLE_TYPE_QUEST, questPair.first, nullptr))
            continue;*/

        Quest* qinfo = &questPair.second;

        // additional quest integrity checks (GO, creature_template and item_template must be loaded already)

        if( qinfo->GetQuestMethod() >= 3 )
        {
            TC_LOG_ERROR("sql.sql","Quest %u has `Method` = %u, expected values are 0, 1 or 2.",qinfo->GetQuestId(),qinfo->GetQuestMethod());
        }

        if (qinfo->_specialFlags & ~QUEST_SPECIAL_FLAGS_DB_ALLOWED)
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `SpecialFlags` = %u > max allowed value. Correct `SpecialFlags` to value <= %u",
                qinfo->GetQuestId(), qinfo->_specialFlags, QUEST_SPECIAL_FLAGS_DB_ALLOWED);
            qinfo->_specialFlags &= QUEST_SPECIAL_FLAGS_DB_ALLOWED;
        }

#ifdef LICH_KING
        if (qinfo->_flags & QUEST_FLAGS_DAILY && qinfo->_flags & QUEST_FLAGS_WEEKLY)
        {
            TC_LOG_ERROR("sql.sql", "Weekly Quest %u is marked as daily quest in `Flags`, removed daily flag.", qinfo->GetQuestId());
            qinfo->_flags &= ~QUEST_FLAGS_DAILY;
        }
#endif

        if(qinfo->_flags & QUEST_FLAGS_DAILY)
        {
            if(!(qinfo->_flags & QUEST_SPECIAL_FLAGS_REPEATABLE))
            {
                TC_LOG_ERROR("sql.sql","Daily Quest %u not marked as repeatable in `SpecialFlags`, added.",qinfo->GetQuestId());
                qinfo->_flags |= QUEST_SPECIAL_FLAGS_REPEATABLE;
            }
        }

#ifdef LICH_KING

        if (qinfo->_flags & QUEST_FLAGS_WEEKLY)
        {
            if (!(qinfo->_specialFlags & QUEST_SPECIAL_FLAGS_REPEATABLE))
            {
                TC_LOG_ERROR("sql.sql", "Weekly Quest %u not marked as repeatable in `SpecialFlags`, added.", qinfo->GetQuestId());
                qinfo->_specialFlags |= QUEST_SPECIAL_FLAGS_REPEATABLE;
            }
        }

        if (qinfo->_specialFlags & QUEST_SPECIAL_FLAGS_MONTHLY)
        {
            if (!(qinfo->_specialFlags & QUEST_SPECIAL_FLAGS_REPEATABLE))
            {
                TC_LOG_ERROR("sql.sql", "Monthly quest %u not marked as repeatable in `SpecialFlags`, added.", qinfo->GetQuestId());
                qinfo->_specialFlags |= QUEST_SPECIAL_FLAGS_REPEATABLE;
            }
        }
#endif

        if(qinfo->_flags & QUEST_FLAGS_TRACKING)
        {
            // at auto-reward can be rewarded only RewardChoiceItemId[0]
            for(int j = 1; j < QUEST_REWARD_CHOICES_COUNT; ++j )
            {
                if(uint32 id = qinfo->RewardChoiceItemId[j])
                {
                    TC_LOG_ERROR("sql.sql","Quest %u has `RewardChoiceItemId%d` = %u but item from `RewardChoiceItemId%d` can't be rewarded with quest flag QUEST_FLAGS_TRACKING.",
                        qinfo->GetQuestId(), j + 1, id, j + 1);
                    // no changes, quest ignore this data
                }
            }
        }

        // client quest log visual (area case)
        if (qinfo->_zoneOrSort > 0)
        {
            if(!sAreaTableStore.LookupEntry(qinfo->_zoneOrSort))
            {
                TC_LOG_ERROR("sql.sql","Quest %u has `_zoneOrSort` = %u (zone case) but zone with this id does not exist.",
                    qinfo->GetQuestId(),qinfo->_zoneOrSort);
                // no changes, quest not dependent from this value but can have problems at client
            }
        }
        // client quest log visual (sort case)
        if (qinfo->_zoneOrSort < 0)
        {
            QuestSortEntry const* qSort = sQuestSortStore.LookupEntry(-int32(qinfo->_zoneOrSort));
            if (!qSort)
            {
                TC_LOG_ERROR("sql.sql","Quest %u has `_zoneOrSort` = %i (sort case) but quest sort with this id does not exist.",
                    qinfo->GetQuestId(),qinfo->_zoneOrSort);
                // no changes, quest not dependent from this value but can have problems at client (note some may be 0, we must allow this so no check)
            }
            //check for proper SkillOrClass value (skill case)
            if(int32 skill_id =  SkillByQuestSort(-int32(qinfo->_zoneOrSort)))
            {
                // skill is positive value in SkillOrClass
                if(qinfo->_requiredSkillId != skill_id )
                {
                    TC_LOG_ERROR("sql.sql","Quest %u has `_zoneOrSort` = %i (skill sort case) but `SkillOrClass` does not have a corresponding value (%i).",
                        qinfo->GetQuestId(),qinfo->_zoneOrSort,skill_id);
                    //override, and force proper value here?
                }
            }
        }

        // RequiredClasses, can be 0/CLASSMASK_ALL_PLAYABLE to allow any class
        if (qinfo->_requiredClasses)
        {
            if (!(qinfo->_requiredClasses & CLASSMASK_ALL_PLAYABLE))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u does not contain any playable classes in `RequiredClasses` (%u), value set to 0 (all classes).", qinfo->GetQuestId(), qinfo->_requiredClasses);
                qinfo->_requiredClasses = 0;
            }
        }

        // AllowableRaces, can be 0/RACEMASK_ALL_PLAYABLE to allow any race
        if (qinfo->_allowableRaces)
        {
            if (!(qinfo->_allowableRaces & RACEMASK_ALL_PLAYABLE))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u does not contain any playable races in `AllowableRaces` (%u), value set to 0 (all races).", qinfo->GetQuestId(), qinfo->_allowableRaces);
                qinfo->_allowableRaces = 0;
            }
        }

        // SkillOrClass (skill case)
        if (qinfo->_requiredSkillId)
        {
            if (!sSkillLineStore.LookupEntry(qinfo->_requiredSkillId))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredSkillId` = %u but this skill does not exist",
                    qinfo->GetQuestId(), qinfo->_requiredSkillId);
            }
        }

        if (qinfo->_requiredSkillPoints)
        {
            if (qinfo->_requiredSkillPoints > sWorld->GetConfigMaxSkillValue())
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredSkillPoints` = %u but max possible skill is %u, quest can't be done.",
                    qinfo->GetQuestId(), qinfo->_requiredSkillPoints, sWorld->GetConfigMaxSkillValue());
                // no changes, quest can't be done for this requirement
            }
        }
        // else Skill quests can have 0 skill level, this is ok

        if(qinfo->_requiredFactionId1 && !sFactionStore.LookupEntry(qinfo->_requiredFactionId1))
        {
            TC_LOG_ERROR("sql.sql","Quest %u has `_requiredFactionId1` = %u but faction template %u does not exist, quest can't be done.",
                qinfo->GetQuestId(),qinfo->_requiredFactionId1,qinfo->_requiredFactionId1);
            // no changes, quest can't be done for this requirement
        }

#ifdef LICH_KING
        if (qinfo->_requiredFactionId2 && !sFactionStore.LookupEntry(qinfo->_requiredFactionId2))
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredFactionId2` = %u but faction template %u does not exist, quest can't be done.",
                qinfo->GetQuestId(), qinfo->_requiredFactionId2, qinfo->_requiredFactionId2);
            // no changes, quest can't be done for this requirement
        }
#endif

        if (qinfo->_requiredMinRepFaction && !sFactionStore.LookupEntry(qinfo->_requiredMinRepFaction))
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredMinRepFaction` = %u but faction template %u does not exist, quest can't be done.",
                qinfo->GetQuestId(), qinfo->_requiredMinRepFaction, qinfo->_requiredMinRepFaction);
            // no changes, quest can't be done for this requirement
        }

        if (qinfo->_requiredMaxRepFaction && !sFactionStore.LookupEntry(qinfo->_requiredMaxRepFaction))
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredMaxRepFaction` = %u but faction template %u does not exist, quest can't be done.",
                qinfo->GetQuestId(), qinfo->_requiredMaxRepFaction, qinfo->_requiredMaxRepFaction);
            // no changes, quest can't be done for this requirement
        }

        if (qinfo->_requiredMinRepValue && qinfo->_requiredMinRepValue > ReputationMgr::Reputation_Cap)
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredMinRepValue` = %d but max reputation is %u, quest can't be done.",
                qinfo->GetQuestId(), qinfo->_requiredMinRepValue, ReputationMgr::Reputation_Cap);
            // no changes, quest can't be done for this requirement
        }

        if (qinfo->_requiredMinRepValue && qinfo->_requiredMaxRepValue && qinfo->_requiredMaxRepValue <= qinfo->_requiredMinRepValue)
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredMaxRepValue` = %d and `RequiredMinRepValue` = %d, quest can't be done.",
                qinfo->GetQuestId(), qinfo->_requiredMaxRepValue, qinfo->_requiredMinRepValue);
            // no changes, quest can't be done for this requirement
        }

        if (!qinfo->_requiredFactionId1 && qinfo->_requiredFactionValue1 != 0)
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredFactionValue1` = %d but `RequiredFactionId1` is 0, value has no effect",
                qinfo->GetQuestId(), qinfo->_requiredFactionValue1);
            // warning
        }

#ifdef LICH_KING
        if (!qinfo->_requiredFactionId2 && qinfo->_requiredFactionValue2 != 0)
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredFactionValue2` = %d but `RequiredFactionId2` is 0, value has no effect",
                qinfo->GetQuestId(), qinfo->_requiredFactionValue2);
            // warning
        }
#endif

        if (!qinfo->_requiredMinRepFaction && qinfo->_requiredMinRepValue != 0)
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredMinRepValue` = %d but `RequiredMinRepFaction` is 0, value has no effect",
                qinfo->GetQuestId(), qinfo->_requiredMinRepValue);
            // warning
        }

        if (!qinfo->_requiredMaxRepFaction && qinfo->_requiredMaxRepValue != 0)
        {
            TC_LOG_ERROR("sql.sql", "Quest %u has `RequiredMaxRepValue` = %d but `RequiredMaxRepFaction` is 0, value has no effect",
                qinfo->GetQuestId(), qinfo->_requiredMaxRepValue);
            // warning
        }

        if(qinfo->_rewardTitleId && !sCharTitlesStore.LookupEntry(qinfo->_rewardTitleId))
        {
            TC_LOG_ERROR("sql.sql","Quest %u has `_rewardTitleId` = %u but CharTitle Id %u does not exist, quest can't be rewarded with title.",
                qinfo->GetQuestId(),qinfo->GetCharTitleId(),qinfo->GetCharTitleId());
            qinfo->_rewardTitleId = 0;
            // quest can't reward this title
        }

        if(qinfo->_startItem)
        {
            if(!sObjectMgr->GetItemTemplate(qinfo->_startItem))
            {
                TC_LOG_ERROR("sql.sql","Quest %u has `_startItem` = %u but item with entry %u does not exist, quest can't be done.",
                    qinfo->GetQuestId(),qinfo->_startItem,qinfo->_startItem);
                qinfo->_startItem = 0;                       // quest can't be done for this requirement
            }
            else if (qinfo->_startItemCount == 0)
            {
                TC_LOG_ERROR("sql.sql","Quest %u has `_startItem` = %u but `SrcItemCount` = 0, set to 1 but need fix in DB.",
                    qinfo->GetQuestId(),qinfo->_startItem);
                qinfo->_startItemCount = 1;                    // update to 1 for allow quest work for backward compatibility with DB
            }
        }
        else if (qinfo->_startItemCount > 0)
        {
            TC_LOG_ERROR("sql.sql","Quest %u has `_startItem` = 0 but `SrcItemCount` = %u, useless value.",
                qinfo->GetQuestId(),qinfo->_startItemCount);
            qinfo->_startItemCount =0;                          // no quest work changes in fact
        }

        if (qinfo->_sourceSpellid)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(qinfo->_sourceSpellid);
            if (!spellInfo)
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `SourceSpellid` = %u but spell %u doesn't exist, quest can't be done.",
                    qinfo->GetQuestId(), qinfo->_sourceSpellid, qinfo->_sourceSpellid);
                qinfo->_sourceSpellid = 0;                        // quest can't be done for this requirement
            }
            else if (!SpellMgr::IsSpellValid(spellInfo))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `SourceSpellid` = %u but spell %u is broken, quest can't be done.",
                    qinfo->GetQuestId(), qinfo->_sourceSpellid, qinfo->_sourceSpellid);
                qinfo->_sourceSpellid = 0;                        // quest can't be done for this requirement
            }
        }

        for(int j = 0; j < QUEST_ITEM_OBJECTIVES_COUNT; ++j )
        {
            uint32 id = qinfo->RequiredItemId[j];
            if(id)
            {
                if(qinfo->RequiredItemCount[j]==0)
                {
                    TC_LOG_ERROR("sql.sql","Quest %u has `RequiredItemId%d` = %u but `RequiredItemCount%d` = 0, quest can't be done.",
                        qinfo->GetQuestId(),j+1,id,j+1);
                    // no changes, quest can't be done for this requirement
                }

                qinfo->SetSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER);

                if(!sObjectMgr->GetItemTemplate(id))
                {
                    TC_LOG_ERROR("sql.sql","Quest %u has `RequiredItemId%d` = %u but item with entry %u does not exist, quest can't be done.",
                        qinfo->GetQuestId(), j + 1, id, id);
                    qinfo->RequiredItemCount[j] = 0;             // prevent incorrect work of quest
                }
            }
            else if (qinfo->RequiredItemCount[j] > 0)
            {
                TC_LOG_ERROR("sql.sql","Quest %u has `RequiredItemId%d` = 0 but `RequiredItemCount%d` = %u, quest can't be done.",
                    qinfo->GetQuestId(),j+1,j+1,qinfo->RequiredItemCount[j]);
                qinfo->RequiredItemCount[j] = 0;                 // prevent incorrect work of quest
            }
        }

        for(int j = 0; j < QUEST_SOURCE_ITEM_IDS_COUNT; ++j )
        {
            uint32 id = qinfo->RequiredSourceItemId[j];
            if(id)
            {
                if(!sObjectMgr->GetItemTemplate(id))
                {
                    TC_LOG_ERROR("sql.sql","Quest %u has `RequiredSourceItemId%d` = %u but item with entry %u does not exist, quest can't be done.",
                        qinfo->GetQuestId(),j+1,id,id);
                    // no changes, quest can't be done for this requirement
                }

                if(!qinfo->RequiredSourceItemCount[j])
                {
                    TC_LOG_ERROR("sql.sql","Quest %u has `RequiredSourceItemId%d` = %u but `RequiredSourceItemCount%d` = 0, quest can't be done.",
                        qinfo->GetQuestId(),j+1,id,j+1);
                    qinfo->RequiredSourceItemId[j] = 0;              // prevent incorrect work of quest
                }

                if(!qinfo->ReqSourceRef[j])
                {
                    TC_LOG_ERROR("sql.sql","Quest %u has `RequiredSourceItemId%d` = %u but `ReqSourceRef%d` = 0, quest can't be done.",
                        qinfo->GetQuestId(),j+1,id,j+1);
                    qinfo->RequiredSourceItemId[j] = 0;              // prevent incorrect work of quest
                }
            }
            else
            {
                if(qinfo->RequiredSourceItemCount[j]>0)
                {
                    TC_LOG_ERROR("sql.sql","Quest %u has `RequiredSourceItemId%d` = 0 but `RequiredSourceItemCount%d` = %u.",
                        qinfo->GetQuestId(),j+1,j+1,qinfo->RequiredSourceItemCount[j]);
                    // no changes, quest ignore this data
                }

                if(qinfo->ReqSourceRef[j]>0)
                {
                    TC_LOG_ERROR("sql.sql","Quest %u has `RequiredSourceItemId%d` = 0 but `ReqSourceRef%d` = %u.",
                        qinfo->GetQuestId(),j+1,j+1,qinfo->ReqSourceRef[j]);
                    // no changes, quest ignore this data
                }
            }
        }

        for(int j = 0; j < QUEST_SOURCE_ITEM_IDS_COUNT; ++j )
        {
            uint32 ref = qinfo->ReqSourceRef[j];
            if(ref)
            {
                if(ref > QUEST_OBJECTIVES_COUNT)
                {
                    TC_LOG_ERROR("sql.sql","Quest %u has `ReqSourceRef%d` = %u but max value in `ReqSourceRef%d` is %u, quest can't be done.",
                        qinfo->GetQuestId(),j+1,ref,j+1,QUEST_OBJECTIVES_COUNT);
                    // no changes, quest can't be done for this requirement
                }
                else
                if(!qinfo->RequiredItemId[ref-1] && !qinfo->ReqSpell[ref-1])
                {
                    TC_LOG_ERROR("sql.sql","Quest %u has `ReqSourceRef%d` = %u but `RequiredItemId%u` = 0 and `ReqSpellCast%u` = 0, quest can't be done.",
                        qinfo->GetQuestId(),j+1,ref,ref,ref);
                    // no changes, quest can't be done for this requirement
                }
                else if(qinfo->RequiredItemId[ref-1] && qinfo->ReqSpell[ref-1])
                {
                    TC_LOG_ERROR("sql.sql","Quest %u has `RequiredItemId%u` = %u and `ReqSpellCast%u` = %u, quest can't have both fields <> 0, then can't be done.",
                        qinfo->GetQuestId(),ref,qinfo->RequiredItemId[ref-1],ref,qinfo->ReqSpell[ref-1]);
                    // no changes, quest can't be done for this requirement
                    qinfo->RequiredSourceItemId[j] = 0;              // prevent incorrect work of quest
                }
            }
        }

        for(int j = 0; j < QUEST_OBJECTIVES_COUNT; ++j )
        {
            uint32 id = qinfo->ReqSpell[j];
            if(id)
            {
                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(id);
                if(!spellInfo)
                {
                    TC_LOG_ERROR("sql.sql","Quest %u has `ReqSpellCast%d` = %u but spell %u does not exist, quest can't be done.",
                        qinfo->GetQuestId(),j+1,id,id);
                    // no changes, quest can't be done for this requirement
                }

                if(!qinfo->RequiredNpcOrGo[j])
                {
                    bool found = false;
                    for(const auto & Effect : spellInfo->Effects)
                    {
                        if ((Effect.Effect == SPELL_EFFECT_QUEST_COMPLETE && uint32(Effect.MiscValue) == qinfo->_id)
                            || Effect.Effect == SPELL_EFFECT_SEND_EVENT)
                        {
                            found = true;
                            break;
                        }
                    }

                    if(found)
                    {
                        if(!qinfo->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT))
                        {
                            TC_LOG_ERROR("sql.sql","Spell (id: %u) have SPELL_EFFECT_QUEST_COMPLETE or SPELL_EFFECT_SEND_EVENT for quest %u and RequiredNpcOrGo%d = 0, but quest not have flag QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT. Quest flags or RequiredNpcOrGo%d must be fixed, quest modified to enable objective.",spellInfo->Id,qinfo->_id,j+1,j+1);

                            // this will prevent quest completing without objective
                            const_cast<Quest*>(qinfo)->SetSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT);
                        }
                    }
                    else
                    {
                        TC_LOG_ERROR("sql.sql","Quest %u has `ReqSpellCast%d` = %u and RequiredNpcOrGo%d = 0 but spell %u does not have SPELL_EFFECT_QUEST_COMPLETE or SPELL_EFFECT_SEND_EVENT effect for this quest, quest can't be done.",
                            qinfo->GetQuestId(),j+1,id,j+1,id);
                        // no changes, quest can't be done for this requirement
                    }
                }
            }
        }

        for(int j = 0; j < QUEST_OBJECTIVES_COUNT; ++j )
        {
            int32 id = qinfo->RequiredNpcOrGo[j];
            if(id < 0 && !GetGameObjectTemplate(-id))
            {
                TC_LOG_ERROR("sql.sql","Quest %u has `RequiredNpcOrGo%d` = %i but gameobject %u does not exist, quest can't be done.",
                    qinfo->GetQuestId(),j+1,id,uint32(-id));
                qinfo->RequiredNpcOrGo[j] = 0;            // quest can't be done for this requirement
            }

            if(id > 0 && !sObjectMgr->GetCreatureTemplate(id))
            {
                TC_LOG_ERROR("sql.sql","Quest %u has `RequiredNpcOrGo%d` = %i but creature with entry %u does not exist, quest can't be done.",
                    qinfo->GetQuestId(),j+1,id,uint32(id));
                qinfo->RequiredNpcOrGo[j] = 0;            // quest can't be done for this requirement
            }

            if(id)
            {
                qinfo->SetSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST | QUEST_SPECIAL_FLAGS_SPEAKTO);

                if(!qinfo->RequiredNpcOrGoCount[j])
                {
                    TC_LOG_ERROR("sql.sql","Quest %u has `RequiredNpcOrGo%d` = %u but `RequiredNpcOrGoCount%d` = 0, quest can't be done.",
                        qinfo->GetQuestId(), j + 1, id, j + 1);
                    // no changes, quest can be incorrectly done, but we already report this
                }
            }
            else if (qinfo->RequiredNpcOrGoCount[j] > 0)
            {
                TC_LOG_ERROR("sql.sql","Quest %u has `RequiredNpcOrGo%d` = 0 but `RequiredNpcOrGoCount%d` = %u.",
                    qinfo->GetQuestId(),j+1,j+1,qinfo->RequiredNpcOrGoCount[j]);
                // no changes, quest ignore this data
            }
        }

        for (int j = 0; j < QUEST_REWARD_CHOICES_COUNT; ++j)
        {
            uint32 id = qinfo->RewardChoiceItemId[j];
            if(id)
            {
                if(!sObjectMgr->GetItemTemplate(id))
                {
                    TC_LOG_ERROR("sql.sql","Quest %u has `RewardChoiceItemId%d` = %u but item with entry %u does not exist, quest will not reward this item.",
                        qinfo->GetQuestId(),j+1,id,id);
                    qinfo->RewardChoiceItemId[j] = 0;          // no changes, quest will not reward this
                }

                if(!qinfo->RewardChoiceItemCount[j])
                {
                    TC_LOG_ERROR("sql.sql","Quest %u has `RewardChoiceItemId%d` = %u but `RewardChoiceItemCount%d` = 0, quest can't be done.",
                        qinfo->GetQuestId(),j+1,id,j+1);
                    // no changes, quest can't be done
                }
            }
            else if(qinfo->RewardChoiceItemCount[j]>0)
            {
                TC_LOG_ERROR("sql.sql","Quest %u has `RewardChoiceItemId%d` = 0 but `RewardChoiceItemCount%d` = %u.",
                    qinfo->GetQuestId(),j+1,j+1,qinfo->RewardChoiceItemCount[j]);
                // no changes, quest ignore this data
            }
        }

        for(int j = 0; j < QUEST_REWARDS_COUNT; ++j )
        {
            uint32 id = qinfo->RewardItemId[j];
            if(id)
            {
                if(!sObjectMgr->GetItemTemplate(id))
                {
                    TC_LOG_ERROR("sql.sql","Quest %u has `RewardItemId%d` = %u but item with entry %u does not exist, quest will not reward this item.",
                        qinfo->GetQuestId(),j+1,id,id);
                    qinfo->RewardItemId[j] = 0;                // no changes, quest will not reward this item
                }

                if(!qinfo->RewardItemIdCount[j])
                {
                    TC_LOG_ERROR("sql.sql","Quest %u has `RewardItemId%d` = %u but `RewardItemIdCount%d` = 0, quest will not reward this item.",
                        qinfo->GetQuestId(),j+1,id,j+1);
                    // no changes
                }
            }
            else if(qinfo->RewardItemIdCount[j]>0)
            {
                TC_LOG_ERROR("sql.sql","Quest %u has `RewardItemId%d` = 0 but `RewardItemIdCount%d` = %u.",
                    qinfo->GetQuestId(),j+1,j+1,qinfo->RewardItemIdCount[j]);
                // no changes, quest ignore this data
            }
        }

        for(int j = 0; j < QUEST_REPUTATIONS_COUNT; ++j)
        {
            if(qinfo->RewardFactionId[j])
            {
                if(!qinfo->RewardRepValue[j])
                {
                    TC_LOG_ERROR("sql.sql","Quest %u has `RewardFactionId%d` = %u but `RewardRepValue%d` = 0, quest will not reward this reputation.",
                        qinfo->GetQuestId(),j+1,qinfo->RewardRepValue[j],j+1);
                    // no changes
                }

                if(!sFactionStore.LookupEntry(qinfo->RewardFactionId[j]))
                {
                    TC_LOG_ERROR("sql.sql","Quest %u has `RewardFactionId%d` = %u but raw faction (faction.dbc) %u does not exist, quest will not reward reputation for this faction.",
                        qinfo->GetQuestId(),j+1,qinfo->RewardFactionId[j] ,qinfo->RewardFactionId[j] );
                    qinfo->RewardFactionId[j] = 0;            // quest will not reward this
                }
            }
            else if(qinfo->RewardRepValue[j]!=0)
            {
                TC_LOG_ERROR("sql.sql","Quest %u has `RewardFactionId%d` = 0 but `RewardRepValue%d` = %u.",
                    qinfo->GetQuestId(),j+1,j+1,qinfo->RewardRepValue[j]);
                // no changes, quest ignore this data
            }
        }

        if(qinfo->_rewardDisplaySpell)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(qinfo->_rewardDisplaySpell);

            if(!spellInfo)
            {
                TC_LOG_ERROR("sql.sql","Quest %u has `_rewardDisplaySpell` = %u but spell %u does not exist, spell removed as display reward.",
                    qinfo->GetQuestId(),qinfo->_rewardDisplaySpell,qinfo->_rewardDisplaySpell);
                qinfo->_rewardDisplaySpell = 0;                        // no spell reward will display for this quest
            }

            else if(!SpellMgr::IsSpellValid(spellInfo))
            {
                TC_LOG_ERROR("sql.sql","Quest %u has `_rewardDisplaySpell` = %u but spell %u is broken, quest can't be done.",
                    qinfo->GetQuestId(),qinfo->_rewardDisplaySpell,qinfo->_rewardDisplaySpell);
                qinfo->_rewardDisplaySpell = 0;                        // no spell reward will display for this quest
            }

        }

        if(qinfo->_rewardSpell)
        {
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(qinfo->_rewardSpell);

            if(!spellInfo)
            {
                TC_LOG_ERROR("sql.sql","Quest %u has `_rewardSpell` = %u but spell %u does not exist, quest will not have a spell reward.",
                    qinfo->GetQuestId(),qinfo->_rewardSpell,qinfo->_rewardSpell);
                qinfo->_rewardSpell = 0;                    // no spell will be casted on player
            }

            else if(!SpellMgr::IsSpellValid(spellInfo))
            {
                TC_LOG_ERROR("sql.sql","Quest %u has `_rewardSpell` = %u but spell %u is broken, quest can't be done.",
                    qinfo->GetQuestId(),qinfo->_rewardSpell,qinfo->_rewardSpell);
                qinfo->_rewardSpell = 0;                    // no spell will be casted on player
            }

        }

        if(qinfo->_rewardMailTemplateId)
        {
            if(!sMailTemplateStore.LookupEntry(qinfo->_rewardMailTemplateId))
            {
                TC_LOG_ERROR("sql.sql","Quest %u has `RewardMailTemplateId` = %u but mail template  %u does not exist, quest will not have a mail reward.",
                    qinfo->GetQuestId(), qinfo->_rewardMailTemplateId, qinfo->_rewardMailTemplateId);
                qinfo->_rewardMailTemplateId = 0;               // no mail will send to player
                qinfo->_rewardMailDelay = 0;                // no mail will send to player
                qinfo->_rewardMailSenderEntry = 0;
            }
            else if (usedMailTemplates.find(qinfo->_rewardMailTemplateId) != usedMailTemplates.end())
            {
                auto used_mt_itr = usedMailTemplates.find(qinfo->_rewardMailTemplateId);
                TC_LOG_ERROR("sql.sql", "Quest %u has `RewardMailTemplateId` = %u but mail template  %u already used for quest %u, quest will not have a mail reward.",
                    qinfo->GetQuestId(), qinfo->_rewardMailTemplateId, qinfo->_rewardMailTemplateId, used_mt_itr->second);
                qinfo->_rewardMailTemplateId = 0;               // no mail will send to player
                qinfo->_rewardMailDelay = 0;                // no mail will send to player
                qinfo->_rewardMailSenderEntry = 0;
            }
            else
                usedMailTemplates.emplace(qinfo->_rewardMailTemplateId, qinfo->GetQuestId());
        }

        if (uint32 rewardNextQuest = qinfo->_rewardNextQuest)
        {
            if (!_questTemplates.count(rewardNextQuest))
            {
                TC_LOG_ERROR("sql.sql", "Quest %u has `RewardNextQuest` = %u but quest %u does not exist, quest chain will not work.",
                    qinfo->GetQuestId(), qinfo->_rewardNextQuest, qinfo->_rewardNextQuest);
                qinfo->_rewardNextQuest = 0;
            }
        }

        // fill additional data stores
        if (uint32 prevQuestId = std::abs(qinfo->_prevQuestId))
        {
            if (!_questTemplates.count(prevQuestId))
                TC_LOG_ERROR("sql.sql", "Quest %u has PrevQuestId %i, but no such quest", qinfo->GetQuestId(), qinfo->_prevQuestId);
        }

        if (uint32 nextQuestId = qinfo->_nextQuestId)
        {
            auto nextQuestItr = _questTemplates.find(nextQuestId);
            if (nextQuestItr == _questTemplates.end())
                TC_LOG_ERROR("sql.sql", "Quest %u has NextQuestId %u, but no such quest", qinfo->GetQuestId(), qinfo->_nextQuestId);
            else
                nextQuestItr->second.DependentPreviousQuests.push_back(qinfo->GetQuestId());
        }

        if (qinfo->_exclusiveGroup)
            _exclusiveQuestGroups.emplace(qinfo->_exclusiveGroup, qinfo->GetQuestId());
        if(qinfo->_timeAllowed)
            qinfo->SetFlag(QUEST_SPECIAL_FLAGS_TIMED);
#ifdef LICH_KING
        if (qinfo->_requiredPlayerKills)
            qinfo->SetSpecialFlag(QUEST_SPECIAL_FLAGS_PLAYER_KILL);
#endif

        // Special flag to determine if quest is completed from the start, used to determine if we can fail timed quest if it is completed
        if (!qinfo->HasSpecialFlag(QUEST_SPECIAL_FLAGS_KILL | QUEST_SPECIAL_FLAGS_CAST | QUEST_SPECIAL_FLAGS_SPEAKTO | QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT))
        {
            bool addFlag = true;
            if (qinfo->HasSpecialFlag(QUEST_SPECIAL_FLAGS_DELIVER))
            {
                for (uint8 j = 0; j < QUEST_ITEM_OBJECTIVES_COUNT; ++j)
                {
                    if (qinfo->RequiredItemId[j] != 0 && (qinfo->RequiredItemId[j] != qinfo->GetSrcItemId() || qinfo->RequiredItemCount[j] > qinfo->GetSrcItemCount()))
                    {
                        addFlag = false;
                        break;
                    }
                }
            }

            if (addFlag)
                qinfo->SetSpecialFlag(QUEST_SPECIAL_FLAGS_COMPLETED_AT_START);
        }
    }

    // check QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT for spell with SPELL_EFFECT_QUEST_COMPLETE
    //for (uint32 i = 0; i < sSpellStore.GetNumRows(); ++i)
    for (auto itr = sObjectMgr->GetSpellStore().begin(); itr != sObjectMgr->GetSpellStore().end(); ++itr)
    {
        uint32 i = itr->first;
        SpellInfo const *spellInfo = sSpellMgr->GetSpellInfo(i);
        if(!spellInfo)
            continue;

        for(const auto & Effect : spellInfo->Effects)
        {
            if(Effect.Effect != SPELL_EFFECT_QUEST_COMPLETE)
                continue;

            uint32 quest_id = Effect.MiscValue;

            Quest const* quest = GetQuestTemplate(quest_id);

            // some quest referenced in spells not exist (outdated spells)
            if(!quest)
                continue;

            if (!quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT))
            {
                TC_LOG_ERROR("sql.sql", "Spell (id: %u) have SPELL_EFFECT_QUEST_COMPLETE for quest %u, but quest not have flag QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT. Quest flags must be fixed, quest modified to enable objective.", spellInfo->Id, quest_id);

                // this will prevent quest completing without objective
                const_cast<Quest*>(quest)->SetSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT);
            }
        }
    }

    TC_LOG_INFO("server.loading", ">> Loaded " SZFMTD " quests definitions in %u ms", _questTemplates.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadQuestLocales()
{
    _questLocaleStore.clear();                                // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT entry,"
        "Title_loc1,Details_loc1,Objectives_loc1,OfferRewardText_loc1,RequestItemsText_loc1,EndText_loc1,ObjectiveText1_loc1,ObjectiveText2_loc1,ObjectiveText3_loc1,ObjectiveText4_loc1,"
        "Title_loc2,Details_loc2,Objectives_loc2,OfferRewardText_loc2,RequestItemsText_loc2,EndText_loc2,ObjectiveText1_loc2,ObjectiveText2_loc2,ObjectiveText3_loc2,ObjectiveText4_loc2,"
        "Title_loc3,Details_loc3,Objectives_loc3,OfferRewardText_loc3,RequestItemsText_loc3,EndText_loc3,ObjectiveText1_loc3,ObjectiveText2_loc3,ObjectiveText3_loc3,ObjectiveText4_loc3,"
        "Title_loc4,Details_loc4,Objectives_loc4,OfferRewardText_loc4,RequestItemsText_loc4,EndText_loc4,ObjectiveText1_loc4,ObjectiveText2_loc4,ObjectiveText3_loc4,ObjectiveText4_loc4,"
        "Title_loc5,Details_loc5,Objectives_loc5,OfferRewardText_loc5,RequestItemsText_loc5,EndText_loc5,ObjectiveText1_loc5,ObjectiveText2_loc5,ObjectiveText3_loc5,ObjectiveText4_loc5,"
        "Title_loc6,Details_loc6,Objectives_loc6,OfferRewardText_loc6,RequestItemsText_loc6,EndText_loc6,ObjectiveText1_loc6,ObjectiveText2_loc6,ObjectiveText3_loc6,ObjectiveText4_loc6,"
        "Title_loc7,Details_loc7,Objectives_loc7,OfferRewardText_loc7,RequestItemsText_loc7,EndText_loc7,ObjectiveText1_loc7,ObjectiveText2_loc7,ObjectiveText3_loc7,ObjectiveText4_loc7,"
        "Title_loc8,Details_loc8,Objectives_loc8,OfferRewardText_loc8,RequestItemsText_loc8,EndText_loc8,ObjectiveText1_loc8,ObjectiveText2_loc8,ObjectiveText3_loc8,ObjectiveText4_loc8"
        " FROM locales_quest"
        );

    if(!result)
    {
        TC_LOG_INFO("server.loading",">> Loaded 0 Quest locale strings. DB table `locales_quest` is empty.");
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        QuestLocale& data = _questLocaleStore[entry];

        for (uint8 i = MAX_LOCALE; i > 0; --i)
        {
            LocaleConstant locale = (LocaleConstant) i;

            AddLocaleString(fields[1 + 10 * (i - 1)].GetString(), locale, data.Title);
            AddLocaleString(fields[1 + 10 * (i - 1) + 1].GetString(), locale, data.Details);
            AddLocaleString(fields[1 + 10 * (i - 1) + 2].GetString(), locale, data.Objectives);
            AddLocaleString(fields[1 + 10 * (i - 1) + 3].GetString(), locale, data._offerRewardText);
            AddLocaleString(fields[1 + 10 * (i - 1) + 4].GetString(), locale, data._requestItemsText);
            AddLocaleString(fields[1 + 10 * (i - 1) + 5].GetString(), locale, data.AreaDescription);

            for (uint8 k = 0; k < 4; ++k)
                AddLocaleString(fields[1 + 10 * (i - 1) + 6 + k].GetString(), locale, data.ObjectiveText[k]);
        }
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded " UI64FMTD " Quest locale strings", _questLocaleStore.size() );
}

void ObjectMgr::LoadPetCreateSpells()
{
    QueryResult result = WorldDatabase.Query("SELECT entry, Spell1, Spell2, Spell3, Spell4 FROM petcreateinfo_spell");
    if(!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 pet create spells" );
        TC_LOG_ERROR("sql.sql","`petcreateinfo_spell` table is empty!");
        
        return;
    }

    uint32 count = 0;

    this->mPetCreateSpell.clear();

    do
    {
        Field *fields = result->Fetch();

        uint32 creature_id = fields[0].GetUInt32();

        if(!creature_id || !sObjectMgr->GetCreatureTemplate(creature_id))
            continue;

        PetCreateSpellEntry PetCreateSpell = { {0,0,0,0} }; // Initialize PetCreateSpell.spellid to all 0

        for(int i = 0; i < 4; i++)
        {
            PetCreateSpell.spellid[i] = fields[i + 1].GetUInt32();

            if(PetCreateSpell.spellid[i] && !sSpellMgr->GetSpellInfo(PetCreateSpell.spellid[i]))
                TC_LOG_ERROR("sql.sql","Spell %u listed in `petcreateinfo_spell` does not exist",PetCreateSpell.spellid[i]);
        }

        this->mPetCreateSpell[creature_id] = PetCreateSpell;

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u pet create spells", count );
}

void ObjectMgr::LoadScripts(ScriptMapMap& scripts, char const* tablename)
{
    if(sMapMgr->IsScriptScheduled())                          // function don't must be called in time scripts use.
        return;

    TC_LOG_INFO("server.loading", "%s :", tablename);

    scripts.clear();                                        // need for reload support

    QueryResult result = WorldDatabase.PQuery( "SELECT id,delay,command,datalong,datalong2,dataint, x, y, z, o FROM %s", tablename );

    uint32 count = 0;

    if( !result )
    {
        TC_LOG_INFO("server.loading", ">> Loaded %u script definitions", count );
        
        return;
    }

    do
    {
        Field *fields = result->Fetch();
        ScriptInfo tmp;
        tmp.id        = fields[0].GetUInt32();
        tmp.delay     = fields[1].GetUInt32();
        tmp.command   = fields[2].GetUInt32();
        tmp.Raw.nData[0] = fields[3].GetUInt32();
        tmp.Raw.nData[1] = fields[4].GetUInt32();
        tmp.Raw.nData[2] = fields[5].GetInt32();
        tmp.Raw.fData[0] = fields[6].GetFloat();
        tmp.Raw.fData[1] = fields[7].GetFloat();
        tmp.Raw.fData[2] = fields[8].GetFloat();
        tmp.Raw.fData[3] = fields[9].GetFloat();

        // generic command args check
        switch(tmp.command)
        {
            case SCRIPT_COMMAND_TALK:
            {
                if (tmp.Talk.ChatType > CHAT_TYPE_WHISPER && tmp.Talk.ChatType != CHAT_MSG_RAID_BOSS_WHISPER)
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has invalid talk type (datalong = %u) in SCRIPT_COMMAND_TALK for script id %u",
                        tablename, tmp.Talk.ChatType, tmp.id);
                    continue;
                }
                if (!sObjectMgr->GetBroadcastText(uint32(tmp.Talk.TextID)))
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has invalid talk text id (dataint = %i) in SCRIPT_COMMAND_TALK for script id %u",
                        tablename, tmp.Talk.TextID, tmp.id);
                    continue;
                }

                break;
            }

            case SCRIPT_COMMAND_EMOTE:
            {
                if (!sEmotesStore.LookupEntry(tmp.Emote.EmoteID))
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has invalid emote id (datalong = %u) in SCRIPT_COMMAND_EMOTE for script id %u",
                        tablename, tmp.Emote.EmoteID, tmp.id);
                    continue;
                }
                break;
            }

            case SCRIPT_COMMAND_TELEPORT_TO:
            {
                if(!sMapStore.LookupEntry(tmp.TeleportTo.MapID))
                {
                    TC_LOG_ERROR("sql.sql","Table `%s` has invalid map (Id: %u) in SCRIPT_COMMAND_TELEPORT_TO for script id %u",tablename, tmp.TeleportTo.MapID, tmp.id);
                    continue;
                }

                if(!Trinity::IsValidMapCoord(tmp.TeleportTo.DestX, tmp.TeleportTo.DestY, tmp.TeleportTo.DestZ, tmp.TeleportTo.Orientation))
                {
                    TC_LOG_ERROR("sql.sql","Table `%s` has invalid coordinates (X: %f Y: %f Z: %f O: %f) in SCRIPT_COMMAND_TELEPORT_TO for script id %u", tablename, tmp.TeleportTo.DestX, tmp.TeleportTo.DestY, tmp.TeleportTo.DestZ, tmp.TeleportTo.Orientation, tmp.id);
                    continue;
                }
                break;
            }

            case SCRIPT_COMMAND_TEMP_SUMMON_CREATURE:
            {
                if (!Trinity::IsValidMapCoord(tmp.TempSummonCreature.PosX, tmp.TempSummonCreature.PosY, tmp.TempSummonCreature.PosZ, tmp.TempSummonCreature.Orientation))
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` has invalid coordinates (X: %f Y: %f Z: %f O: %f) in SCRIPT_COMMAND_TEMP_SUMMON_CREATURE for script id %u",
                        tablename, tmp.TempSummonCreature.PosX, tmp.TempSummonCreature.PosY, tmp.TempSummonCreature.PosZ, tmp.TempSummonCreature.Orientation, tmp.id);
                    continue;
                }

                if (!GetCreatureTemplate(tmp.TempSummonCreature.CreatureEntry))
                {

                    TC_LOG_ERROR("sql.sql", "Table `%s` has invalid creature (Entry: %u) in SCRIPT_COMMAND_TEMP_SUMMON_CREATURE for script id %u",
                        tablename, tmp.TempSummonCreature.CreatureEntry, tmp.id);
                    continue;
                }
                break;
            }

            case SCRIPT_COMMAND_RESPAWN_GAMEOBJECT:
            {
                GameObjectData const* data = GetGameObjectData(tmp.RespawnGameobject.GOGuid);
                if(!data)
                {
                    TC_LOG_ERROR("sql.sql","Table `%s` has invalid gameobject (SpawnId: %u) in SCRIPT_COMMAND_RESPAWN_GAMEOBJECT for script id %u", tablename, tmp.RespawnGameobject.GOGuid, tmp.id);
                    continue;
                }

                GameObjectTemplate const* info = GetGameObjectTemplate(data->id);
                if(!info)
                {
                    TC_LOG_ERROR("sql.sql","Table `%s` has gameobject with invalid entry (SpawnId: %u Entry: %u) in SCRIPT_COMMAND_RESPAWN_GAMEOBJECT for script id %u",tablename, tmp.RespawnGameobject.GOGuid, data->id, tmp.id);
                    continue;
                }

                if( info->type==GAMEOBJECT_TYPE_FISHINGNODE ||
                    info->type==GAMEOBJECT_TYPE_FISHINGHOLE ||
                    info->type==GAMEOBJECT_TYPE_DOOR        ||
                    info->type==GAMEOBJECT_TYPE_BUTTON      ||
                    info->type==GAMEOBJECT_TYPE_TRAP )
                {
                    TC_LOG_ERROR("sql.sql","Table `%s` have gameobject type (%u) unsupported by command SCRIPT_COMMAND_RESPAWN_GAMEOBJECT for script id %u",tablename,info->entry,tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_OPEN_DOOR:
            case SCRIPT_COMMAND_CLOSE_DOOR:
            {
                GameObjectData const* data = GetGameObjectData(tmp.ToggleDoor.GOGuid);
                if(!data)
                {
                    TC_LOG_ERROR("sql.sql","Table `%s` has invalid gameobject (SpawnId: %u) in %s for script id %u",tablename, tmp.ToggleDoor.GOGuid,(tmp.command==SCRIPT_COMMAND_OPEN_DOOR ? "SCRIPT_COMMAND_OPEN_DOOR" : "SCRIPT_COMMAND_CLOSE_DOOR"),tmp.id);
                    continue;
                }

                GameObjectTemplate const* info = GetGameObjectTemplate(data->id);
                if(!info)
                {
                    TC_LOG_ERROR("sql.sql","Table `%s` has gameobject with invalid entry (SpawnId: %u Entry: %u) in %s for script id %u",tablename, tmp.ToggleDoor.GOGuid,data->id,(tmp.command==SCRIPT_COMMAND_OPEN_DOOR ? "SCRIPT_COMMAND_OPEN_DOOR" : "SCRIPT_COMMAND_CLOSE_DOOR"),tmp.id);
                    continue;
                }

                if( info->type!=GAMEOBJECT_TYPE_DOOR)
                {
                    TC_LOG_ERROR("sql.sql","Table `%s` has gameobject type (%u) non supported by command %s for script id %u",tablename,info->entry,(tmp.command==SCRIPT_COMMAND_OPEN_DOOR ? "SCRIPT_COMMAND_OPEN_DOOR" : "SCRIPT_COMMAND_CLOSE_DOOR"),tmp.id);
                    continue;
                }

                break;
            }
            case SCRIPT_COMMAND_QUEST_EXPLORED:
            {
                Quest const* quest = GetQuestTemplate(tmp.QuestExplored.QuestID);
                if(!quest)
                {
                    TC_LOG_ERROR("sql.sql","Table `%s` has invalid quest (ID: %u) in SCRIPT_COMMAND_QUEST_EXPLORED in `datalong` for script id %u", tablename, tmp.QuestExplored.QuestID, tmp.id);
                    continue;
                }

                if (!quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT))
                {
                    TC_LOG_ERROR("sql.sql","Table `%s` has quest (ID: %u) in SCRIPT_COMMAND_QUEST_EXPLORED in `datalong` for script id %u, but quest not have flag QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT in quest flags. Script command or quest flags wrong. Quest modified to require objective.", tablename, tmp.QuestExplored.QuestID, tmp.id);

                    // this will prevent quest completing without objective
                    const_cast<Quest*>(quest)->SetSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT);

                    // continue; - quest objective requirement set and command can be allowed
                }

                if(float(tmp.QuestExplored.Distance) > DEFAULT_VISIBILITY_DISTANCE)
                {
                    TC_LOG_ERROR("sql.sql","Table `%s` has too large distance (%u) for exploring objective complete in `datalong2` in SCRIPT_COMMAND_QUEST_EXPLORED in `datalong` for script id %u",
                        tablename,tmp.QuestExplored.Distance,tmp.id);
                    continue;
                }

                if(tmp.QuestExplored.Distance && float(tmp.QuestExplored.Distance) > DEFAULT_VISIBILITY_DISTANCE)
                {
                    TC_LOG_ERROR("sql.sql","Table `%s` has too large distance (%u) for exploring objective complete in `datalong2` in SCRIPT_COMMAND_QUEST_EXPLORED in `datalong` for script id %u, max distance is %f or 0 for disable distance check",
                        tablename,tmp.QuestExplored.Distance,tmp.id,DEFAULT_VISIBILITY_DISTANCE);
                    continue;
                }

                if(tmp.QuestExplored.Distance && float(tmp.QuestExplored.Distance) < INTERACTION_DISTANCE)
                {
                    TC_LOG_ERROR("sql.sql","Table `%s` has too small distance (%u) for exploring objective complete in `datalong2` in SCRIPT_COMMAND_QUEST_EXPLORED in `datalong` for script id %u, min distance is %f or 0 for disable distance check",
                        tablename,tmp.QuestExplored.Distance,tmp.id,INTERACTION_DISTANCE);
                    continue;
                }

                break;
            }

            case SCRIPT_COMMAND_REMOVE_AURA:
            {
                if(!sSpellMgr->GetSpellInfo(tmp.RemoveAura.SpellID))
                {
                    TC_LOG_ERROR("sql.sql","Table `%s` using non-existent spell (id: %u) in SCRIPT_COMMAND_REMOVE_AURA or SCRIPT_COMMAND_CAST_SPELL for script id %u",
                        tablename, tmp.RemoveAura.SpellID, tmp.id);
                    continue;
                }
                if (tmp.RemoveAura.Flags & ~0x1)                    // 1 bits (0, 1)
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` using unknown flags in datalong2 (%u) in SCRIPT_COMMAND_REMOVE_AURA for script id %u",
                        tablename, tmp.RemoveAura.Flags, tmp.id);
                    continue;
                }
                break;
            }
            case SCRIPT_COMMAND_CAST_SPELL:
            {
                if (!sSpellMgr->GetSpellInfo(tmp.CastSpell.SpellID))
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` using non-existent spell (id: %u) in SCRIPT_COMMAND_CAST_SPELL for script id %u",
                        tablename, tmp.CastSpell.SpellID, tmp.id);
                    continue;
                }
                if (tmp.CastSpell.Flags > 4)                      // targeting type
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` using unknown target in datalong2 (%u) in SCRIPT_COMMAND_CAST_SPELL for script id %u",
                        tablename, tmp.CastSpell.Flags, tmp.id);
                    continue;
                }
                if (tmp.CastSpell.Flags != 4 && tmp.CastSpell.CreatureEntry & ~0x1)                      // 1 bit (0, 1)
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` using unknown flags in dataint (%u) in SCRIPT_COMMAND_CAST_SPELL for script id %u",
                        tablename, tmp.CastSpell.CreatureEntry, tmp.id);
                    continue;
                }
                else if (tmp.CastSpell.Flags == 4 && !GetCreatureTemplate(tmp.CastSpell.CreatureEntry))
                {
                    TC_LOG_ERROR("sql.sql", "Table `%s` using invalid creature entry in dataint (%u) in SCRIPT_COMMAND_CAST_SPELL for script id %u",
                        tablename, tmp.CastSpell.CreatureEntry, tmp.id);
                    continue;
                }
                break;
            }
        }

        if (scripts.find(tmp.id) == scripts.end())
        {
            ScriptMap emptyMap;
            scripts[tmp.id] = emptyMap;
        }
        scripts[tmp.id].insert(std::pair<uint32, ScriptInfo>(tmp.delay, tmp));

        ++count;
    } while( result->NextRow() );

    TC_LOG_INFO("server.loading", ">> Loaded %u script definitions", count );
    
}

void ObjectMgr::LoadGameObjectScripts()
{
    LoadScripts(sGameObjectScripts,    "gameobject_scripts");

    // check ids
    for(ScriptMapMap::const_iterator itr = sGameObjectScripts.begin(); itr != sGameObjectScripts.end(); ++itr)
    {
        if(!GetGameObjectData(itr->first))
            TC_LOG_ERROR("sql.sql","Table `gameobject_scripts` has not existing gameobject (SpawnId: %u) as script id",itr->first);
    }
}

void ObjectMgr::LoadQuestEndScripts()
{
    LoadScripts(sQuestEndScripts,  "quest_end_scripts");

    // check ids
    for(ScriptMapMap::const_iterator itr = sQuestEndScripts.begin(); itr != sQuestEndScripts.end(); ++itr)
    {
        if(!GetQuestTemplate(itr->first))
            TC_LOG_ERROR("sql.sql","Table `quest_end_scripts` has not existing quest (Id: %u) as script id",itr->first);
    }
}

void ObjectMgr::LoadQuestStartScripts()
{
    LoadScripts(sQuestStartScripts,"quest_start_scripts");

    // check ids
    for(ScriptMapMap::const_iterator itr = sQuestStartScripts.begin(); itr != sQuestStartScripts.end(); ++itr)
    {
        if(!GetQuestTemplate(itr->first))
            TC_LOG_ERROR("sql.sql","Table `quest_start_scripts` has not existing quest (Id: %u) as script id",itr->first);
    }
}

void ObjectMgr::LoadSpellScripts()
{
    LoadScripts(sSpellScripts, "spell_scripts");

    // check ids
    for(ScriptMapMap::const_iterator itr = sSpellScripts.begin(); itr != sSpellScripts.end(); ++itr)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(itr->first);

        if(!spellInfo)
        {
            TC_LOG_ERROR("sql.sql","Table `spell_scripts` has not existing spell (Id: %u) as script id",itr->first);
            continue;
        }

        //check for correct spellEffect
        bool found = false;
        for(const auto & Effect : spellInfo->Effects)
        {
            // skip empty effects
            if( !Effect.Effect )
                continue;

            if( Effect.Effect == SPELL_EFFECT_SCRIPT_EFFECT )
            {
                found =  true;
                break;
            }
        }

        if(!found)
            TC_LOG_ERROR("sql.sql","Table `spell_scripts` has unsupported spell (Id: %u) without SPELL_EFFECT_SCRIPT_EFFECT (%u) spell effect",itr->first,SPELL_EFFECT_SCRIPT_EFFECT);
    }
}

void ObjectMgr::LoadEventScripts()
{
    LoadScripts(sEventScripts, "event_scripts");

    std::set<uint32> evt_scripts;
    // Load all possible script entries from gameobjects
    GameObjectTemplateContainer const* gotc = sObjectMgr->GetGameObjectTemplateStore();
    for (const auto & itr : *gotc)
    {
        if(GameObjectTemplate const* goInfo = &(itr.second))
        {
            switch(goInfo->type)
            {
                case GAMEOBJECT_TYPE_GOOBER:
                    if(goInfo->goober.eventId)
                        evt_scripts.insert(goInfo->goober.eventId);
                    break;
                case GAMEOBJECT_TYPE_CHEST:
                    if(goInfo->chest.eventId)
                        evt_scripts.insert(goInfo->chest.eventId);
                    break;
                default:
                    break;
            }
        }
    }
    // Load all possible script entries from spells
    //for(uint32 i = 1; i < sSpellStore.GetNumRows(); ++i)
    for (auto itr = sObjectMgr->GetSpellStore().begin(); itr != sObjectMgr->GetSpellStore().end(); ++itr)
    {
        uint32 i = itr->first;
        SpellInfo const * spell = sSpellMgr->GetSpellInfo(i);
        if (spell)
        {
            for(const auto & Effect : spell->Effects)
            {
                if( Effect.Effect == SPELL_EFFECT_SEND_EVENT )
                {
                    if (Effect.MiscValue)
                        evt_scripts.insert(Effect.MiscValue);
                }
            }
        }
    }
    // Then check if all scripts are in above list of possible script entries
    for(ScriptMapMap::const_iterator itr = sEventScripts.begin(); itr != sEventScripts.end(); ++itr)
    {
        auto itr2 = evt_scripts.find(itr->first);
        if (itr2 == evt_scripts.end())
            TC_LOG_ERROR("sql.sql","Table `event_scripts` has script (Id: %u) not referring to any gameobject_template type 10 data2 field or type 3 data6 field or any spell effect %u", itr->first, SPELL_EFFECT_SEND_EVENT);
    }
}

//Load WP Scripts
void ObjectMgr::LoadWaypointScripts()
{
    LoadScripts(sWaypointScripts, "waypoint_scripts");

    for(ScriptMapMap::const_iterator itr = sWaypointScripts.begin(); itr != sWaypointScripts.end(); ++itr)
    {
        QueryResult query = WorldDatabase.PQuery("SELECT * FROM `waypoint_scripts` WHERE `id` = %u", itr->first);
        if(query)
        {
            if (query->GetRowCount()) 
                continue;
        } //no query or no row count
        TC_LOG_ERROR("sql.sql","There is no waypoint which links to the waypoint script %u", itr->first);
    }
}


void ObjectMgr::LoadSpellScriptNames()
{
    uint32 oldMSTime = GetMSTime();

    _spellScriptsStore.clear();                            // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT spell_id, ScriptName FROM spell_script_names");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 spell script names. DB table `spell_script_names` is empty!");
        return;
    }

    uint32 count = 0;

    do
    {

        Field* fields = result->Fetch();

        int32 spellId = fields[0].GetInt32();
        const char *scriptName = fields[1].GetCString();

        bool allRanks = false;
        if (spellId <= 0)
        {
            allRanks = true;
            spellId = -spellId;
        }

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (!spellInfo)
        {
            TC_LOG_ERROR("sql.sql", "Scriptname:`%s` spell (spell_id:%d) does not exist in `Spell.dbc`.", scriptName, fields[0].GetInt32());
            continue;
        }

        if (allRanks)
        {
            if (sSpellMgr->GetFirstSpellInChain(spellId) != uint32(spellId))
            {
                TC_LOG_ERROR("sql.sql", "Scriptname:`%s` spell (spell_id:%d) is not first rank of spell.", scriptName, fields[0].GetInt32());
                continue;
            }
            while (spellInfo)
            {
                _spellScriptsStore.insert(SpellScriptsContainer::value_type(spellInfo->Id, std::make_pair(GetScriptId(scriptName), true)));
                spellInfo = spellInfo->GetNextRankSpell();
            }
        }
        else
            _spellScriptsStore.insert(SpellScriptsContainer::value_type(spellInfo->Id, std::make_pair(GetScriptId(scriptName), true)));
        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u spell script names in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::ValidateSpellScripts()
{
    uint32 oldMSTime = GetMSTime();

    if (_spellScriptsStore.empty())
    {
        TC_LOG_INFO("server.loading", ">> Validated 0 scripts.");
        return;
    }

    uint32 count = 0;

    for (auto spell : _spellScriptsStore)
    {
        SpellInfo const* spellEntry = sSpellMgr->GetSpellInfo(spell.first);
        if (!spellEntry)
        {
            TC_LOG_ERROR("scripts", "ValidateSpellScripts:: Could not find spell id %u", spell.first);
            continue;
        }

        auto const bounds = sObjectMgr->GetSpellScriptsBounds(spell.first);

        for (auto itr = bounds.first; itr != bounds.second; ++itr)
        {
            if (SpellScriptLoader* spellScriptLoader = sScriptMgr->GetSpellScriptLoader(itr->second.first))
            {
                ++count;

                std::unique_ptr<SpellScript> spellScript(spellScriptLoader->GetSpellScript());
                std::unique_ptr<AuraScript> auraScript(spellScriptLoader->GetAuraScript());

                if (!spellScript&& !auraScript)
                {
                    TC_LOG_ERROR("scripts", "Functions GetSpellScript() and GetAuraScript() of script `%s` do not return objects - script skipped", GetScriptName(itr->second.first).c_str());

                    itr->second.second = false;
                    continue;
                }

                if (spellScript)
                {
                    spellScript->_Init(&spellScriptLoader->GetName(), spellEntry->Id);
                    spellScript->_Register();

                    if (!spellScript->_Validate(spellEntry))
                    {
                        itr->second.second = false;
                        continue;
                    }
                }

                if (auraScript)
                {
                    auraScript->_Init(&spellScriptLoader->GetName(), spellEntry->Id);
                    auraScript->_Register();

                    if (!auraScript->_Validate(spellEntry))
                    {
                        itr->second.second = false;
                        continue;
                    }
                }

                // Enable the script when all checks passed
                itr->second.second = true;
            }
            else
                itr->second.second = false;
        }
    }

    TC_LOG_INFO("server.loading", ">> Validated %u scripts in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadPageTexts()
{
    uint32 oldMSTime = GetMSTime();

    //                                                 0      1       2
    QueryResult result = WorldDatabase.Query("SELECT entry, text, next_page FROM page_text");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 page texts. DB table `page_text` is empty!");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        PageText& pageText = _pageTextStore[fields[0].GetUInt32()];

        pageText.Text     = fields[1].GetString();
        pageText.NextPage = fields[2].GetUInt32();

        ++count;
    }
    while (result->NextRow());

    for (PageTextContainer::const_iterator itr = _pageTextStore.begin(); itr != _pageTextStore.end(); ++itr)
    {
        if (itr->second.NextPage)
        {
            PageTextContainer::const_iterator itr2 = _pageTextStore.find(itr->second.NextPage);
            if (itr2 == _pageTextStore.end())
                TC_LOG_ERROR("sql.sql", "Page text (Id: %u) has not existing next page (Id: %u)", itr->first, itr->second.NextPage);

        }
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u page texts in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

PageText const* ObjectMgr::GetPageText(uint32 pageEntry)
{
    PageTextContainer::const_iterator itr = _pageTextStore.find(pageEntry);
    if (itr != _pageTextStore.end())
        return &(itr->second);

    return nullptr;
}

void ObjectMgr::LoadPageTextLocales()
{
    uint32 oldMSTime = GetMSTime();

    mPageTextLocaleMap.clear();                             // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT entry,text_loc1,text_loc2,text_loc3,text_loc4,text_loc5,text_loc6,text_loc7,text_loc8 FROM locales_page_text");

    if(!result)
    {
        TC_LOG_INFO("server.loading",">> Loaded 0 PageText locale strings. DB table `locales_page_text` is empty.");
        
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        PageTextLocale& data = mPageTextLocaleMap[entry];

        for (uint8 i = TOTAL_LOCALES - 1; i > 0; --i)
            AddLocaleString(fields[i].GetString(), LocaleConstant(i), data.Text);

    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded " UI64FMTD " PageText locale strings in %u ms", mPageTextLocaleMap.size(), GetMSTimeDiffToNow(oldMSTime));
    
}

void ObjectMgr::LoadInstanceTemplate()
{
    uint32 oldMSTime = GetMSTime();
    //                                                0     1         2           4            5          6         7             8         9       10          11
    QueryResult result = WorldDatabase.PQuery("SELECT map, parent, maxPlayers, reset_delay, access_id, startLocX, startLocY, startLocZ, startLocO, script, forceHeroicEnabled FROM instance_template t1 WHERE patch = (SELECT max(patch) FROM instance_template t2 WHERE t1.map = t2.map && patch <= %u)", sWorld->GetWowPatch());

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 instance templates. DB table `instance_template` is empty!");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint16 mapID = fields[0].GetUInt16();

        if (!MapManager::IsValidMAP(mapID, true))
        {
            TC_LOG_ERROR("sql.sql", "ObjectMgr::LoadInstanceTemplate: bad mapid %d for template!", mapID);
            continue;
        }

        InstanceTemplate instanceTemplate;

        instanceTemplate.parent = fields[1].GetUInt32();
        instanceTemplate.maxPlayers = fields[2].GetUInt8();
        instanceTemplate.reset_delay = fields[3].GetUInt32();
        instanceTemplate.access_id = fields[4].GetUInt64();
        instanceTemplate.startLocX = fields[5].GetFloat();
        instanceTemplate.startLocY = fields[6].GetFloat();
        instanceTemplate.startLocZ = fields[7].GetFloat();
        instanceTemplate.startLocO = fields[8].GetFloat();
        instanceTemplate.ScriptId  = sObjectMgr->GetScriptId(fields[9].GetCString());
        instanceTemplate.heroicForced = fields[10].GetBool();

        _instanceTemplateStore[mapID] = instanceTemplate;

        ++count;
    }
    while (result->NextRow());

#ifndef LICH_KING
    //fake sMapDifficultyMap data, this dbc does not exists for BC
    //non instance part is handled in LoadDBCStores
    for(auto i : _instanceTemplateStore)
    {
        InstanceTemplate* temp = &(i.second);
        const MapEntry* entry = sMapStore.LookupEntry(i.first);
        if(!entry)
            continue;  //should never happen if sMapStore isn't altered elswhere
        
        if (temp->reset_delay == 0)
        {
            // use defaults from the DBC
            if (entry->resetTimeHeroic)
            {
                temp->reset_delay = entry->resetTimeHeroic;
            }
            else if (entry->resetTimeRaid && entry->map_type == MAP_RAID)
            {
                temp->reset_delay = entry->resetTimeRaid;
            }
            //defaults
            else if (entry->IsRaid())
            {
                temp->reset_delay = 7 * DAY;
            }
            else
                temp->reset_delay = 1 * DAY;
        }

        sMapDifficultyMap[MAKE_PAIR32(i.first, REGULAR_DIFFICULTY)] = MapDifficulty(temp->reset_delay, temp->maxPlayers, false);
        if (MapEntry const* _entry = sMapStore.LookupEntry(i.first))
        {
            if (_entry->resetTimeHeroic || temp->heroicForced)
                sMapDifficultyMap[MAKE_PAIR32(_entry->MapID, DUNGEON_DIFFICULTY_HEROIC)] = MapDifficulty(_entry->resetTimeHeroic / DAY, temp->maxPlayers, false);;
        }
    }
#endif

    TC_LOG_INFO("server.loading", ">> Loaded " UI64FMTD " Instance Template definitions in %u ms", _instanceTemplateStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

InstanceTemplate const* ObjectMgr::GetInstanceTemplate(uint32 mapID)
{
    return Trinity::Containers::MapGetValuePtr(_instanceTemplateStore, uint16(mapID));
}

InstanceTemplateAddon const* ObjectMgr::GetInstanceTemplateAddon(uint32 mapID)
{
    return Trinity::Containers::MapGetValuePtr(_instanceTemplateAddonStore, uint16(mapID));
}

BattlegroundTypeId ObjectMgr::GetBattleMasterBG(uint32 entry) const
{
    auto itr = _battleMastersStore.find(entry);
    if (itr != _battleMastersStore.end())
        return itr->second;

    TC_LOG_WARN("misc", "ObjectMgr::GetGossipText could not found battleground type %u, defaulting to warsong gulch", entry);
    return BATTLEGROUND_WS;
}

GossipText const* ObjectMgr::GetGossipText(uint32 Text_ID)
{
    GossipTextContainer::const_iterator itr = _gossipTextStore.find(Text_ID);
    if (itr != _gossipTextStore.end())
        return &itr->second;
    return nullptr;
}

void ObjectMgr::LoadGossipText()
{
    uint32 oldMSTime = GetMSTime();

    QueryResult result = WorldDatabase.Query("SELECT ID, "
        "text0_0, text0_1, BroadcastTextID0, lang0, prob0, em0_0, em0_1, em0_2, em0_3, em0_4, em0_5, "
        "text1_0, text1_1, BroadcastTextID1, lang1, prob1, em1_0, em1_1, em1_2, em1_3, em1_4, em1_5, "
        "text2_0, text2_1, BroadcastTextID2, lang2, prob2, em2_0, em2_1, em2_2, em2_3, em2_4, em2_5, "
        "text3_0, text3_1, BroadcastTextID3, lang3, prob3, em3_0, em3_1, em3_2, em3_3, em3_4, em3_5, "
        "text4_0, text4_1, BroadcastTextID4, lang4, prob4, em4_0, em4_1, em4_2, em4_3, em4_4, em4_5, "
        "text5_0, text5_1, BroadcastTextID5, lang5, prob5, em5_0, em5_1, em5_2, em5_3, em5_4, em5_5, "
        "text6_0, text6_1, BroadcastTextID6, lang6, prob6, em6_0, em6_1, em6_2, em6_3, em6_4, em6_5, "
        "text7_0, text7_1, BroadcastTextID7, lang7, prob7, em7_0, em7_1, em7_2, em7_3, em7_4, em7_5 "
        "FROM gossip_text");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 gossip texts, table is empty!");
        return;
    }

    _gossipTextStore.rehash(result->GetRowCount());

    int count = 0;
    int cic;

    do
    {
        ++count;
        cic = 0;

        Field* fields = result->Fetch();

        uint32 id = fields[cic++].GetUInt32();
        if (!id)
        {
            TC_LOG_ERROR("sql.sql", "Table `gossip_text` has record with reserved id 0, ignore.");
            continue;
        }

        GossipText& gText = _gossipTextStore[id];

        for (uint8 i = 0; i < MAX_GOSSIP_TEXT_OPTIONS; ++i)
        {
            GossipTextOption& gOption = gText.Options[i];
            gOption.Text_0 = fields[cic++].GetString();
            gOption.Text_1 = fields[cic++].GetString();
            gOption.BroadcastTextID = fields[cic++].GetUInt32();
            gOption.Language = fields[cic++].GetUInt8();
            gOption.Probability = fields[cic++].GetFloat();

            for (uint8 j = 0; j < MAX_GOSSIP_TEXT_EMOTES; ++j)
            {
                gOption.Emotes[j]._Delay = fields[cic++].GetUInt16();
                gOption.Emotes[j]._Emote = fields[cic++].GetUInt16();
            }

            // check broadcast_text correctness
            if (gOption.BroadcastTextID)
            {
                if (BroadcastText const* bcText = sObjectMgr->GetBroadcastText(gOption.BroadcastTextID))
                {
                    if (bcText->MaleText[DEFAULT_LOCALE] != gOption.Text_0)
                        TC_LOG_ERROR("sql.sql", "Row %u in table `gossip_text` has mismatch between text%u_0 and the corresponding MaleText in `broadcast_text` row %u", id, i, gOption.BroadcastTextID);
                    if (bcText->FemaleText[DEFAULT_LOCALE] != gOption.Text_1)
                        TC_LOG_ERROR("sql.sql", "Row %u in table `gossip_text` has mismatch between text%u_1 and the corresponding FemaleText in `broadcast_text` row %u", id, i, gOption.BroadcastTextID);
                }
                else
                {
                    TC_LOG_ERROR("sql.sql", "GossipText (Id: %u) in table `gossip_text` has non-existing or incompatible BroadcastTextID%u %u.", id, i, gOption.BroadcastTextID);
                    gOption.BroadcastTextID = 0;
                }
            }
        }
    } while( result->NextRow() );

    TC_LOG_INFO("server.loading", ">> Loaded %u gossip texts in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    
}

void ObjectMgr::LoadGossipTextLocales()
{
    mGossipTextLocaleMap.clear();                              // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT ID,"
        "Text0_0_loc1,Text0_1_loc1,Text1_0_loc1,Text1_1_loc1,Text2_0_loc1,Text2_1_loc1,Text3_0_loc1,Text3_1_loc1,Text4_0_loc1,Text4_1_loc1,Text5_0_loc1,Text5_1_loc1,Text6_0_loc1,Text6_1_loc1,Text7_0_loc1,Text7_1_loc1,"
        "Text0_0_loc2,Text0_1_loc2,Text1_0_loc2,Text1_1_loc2,Text2_0_loc2,Text2_1_loc2,Text3_0_loc2,Text3_1_loc1,Text4_0_loc2,Text4_1_loc2,Text5_0_loc2,Text5_1_loc2,Text6_0_loc2,Text6_1_loc2,Text7_0_loc2,Text7_1_loc2,"
        "Text0_0_loc3,Text0_1_loc3,Text1_0_loc3,Text1_1_loc3,Text2_0_loc3,Text2_1_loc3,Text3_0_loc3,Text3_1_loc1,Text4_0_loc3,Text4_1_loc3,Text5_0_loc3,Text5_1_loc3,Text6_0_loc3,Text6_1_loc3,Text7_0_loc3,Text7_1_loc3,"
        "Text0_0_loc4,Text0_1_loc4,Text1_0_loc4,Text1_1_loc4,Text2_0_loc4,Text2_1_loc4,Text3_0_loc4,Text3_1_loc1,Text4_0_loc4,Text4_1_loc4,Text5_0_loc4,Text5_1_loc4,Text6_0_loc4,Text6_1_loc4,Text7_0_loc4,Text7_1_loc4,"
        "Text0_0_loc5,Text0_1_loc5,Text1_0_loc5,Text1_1_loc5,Text2_0_loc5,Text2_1_loc5,Text3_0_loc5,Text3_1_loc1,Text4_0_loc5,Text4_1_loc5,Text5_0_loc5,Text5_1_loc5,Text6_0_loc5,Text6_1_loc5,Text7_0_loc5,Text7_1_loc5,"
        "Text0_0_loc6,Text0_1_loc6,Text1_0_loc6,Text1_1_loc6,Text2_0_loc6,Text2_1_loc6,Text3_0_loc6,Text3_1_loc1,Text4_0_loc6,Text4_1_loc6,Text5_0_loc6,Text5_1_loc6,Text6_0_loc6,Text6_1_loc6,Text7_0_loc6,Text7_1_loc6,"
        "Text0_0_loc7,Text0_1_loc7,Text1_0_loc7,Text1_1_loc7,Text2_0_loc7,Text2_1_loc7,Text3_0_loc7,Text3_1_loc1,Text4_0_loc7,Text4_1_loc7,Text5_0_loc7,Text5_1_loc7,Text6_0_loc7,Text6_1_loc7,Text7_0_loc7,Text7_1_loc7, "
        "Text0_0_loc8,Text0_1_loc8,Text1_0_loc8,Text1_1_loc8,Text2_0_loc8,Text2_1_loc8,Text3_0_loc8,Text3_1_loc1,Text4_0_loc8,Text4_1_loc8,Text5_0_loc8,Text5_1_loc8,Text6_0_loc8,Text6_1_loc8,Text7_0_loc8,Text7_1_loc8 "
        " FROM locales_gossip_text");

    if(!result)
    {
        TC_LOG_INFO("server.loading",">> Loaded 0 gossip locale strings. DB table `locales_gossip_text` is empty.");
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        NpcTextLocale& data = mGossipTextLocaleMap[entry];

        for (uint8 i = MAX_LOCALE; i > 0; --i)
        {
            LocaleConstant locale = (LocaleConstant) i;
            for (uint8 j = 0; j < MAX_GOSSIP_TEXT_OPTIONS; ++j)
            {
                AddLocaleString(fields[1 + 8 * 2 * (i - 1) + 2 * j].GetString(), locale, data.Text_0[j]);
                AddLocaleString(fields[1 + 8 * 2 * (i - 1) + 2 * j + 1].GetString(), locale, data.Text_1[j]);
            }
        }
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded " UI64FMTD " gossip text locale strings", mGossipTextLocaleMap.size());
}

//not very fast function but it is called only once a day, or on starting-up
void ObjectMgr::ReturnOrDeleteOldMails(bool serverUp)
{
    uint32 oldMSTime = GetMSTime();

    time_t curTime = WorldGameTime::GetGameTime();
    tm lt;
    localtime_r(&curTime, &lt);
    uint64 basetime(curTime);
    TC_LOG_INFO("misc", "Returning mails current time: hour: %d, minute: %d, second: %d ", lt.tm_hour, lt.tm_min, lt.tm_sec);

    // Delete all old mails without item and without body immediately, if starting server
    if (!serverUp)
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_EMPTY_EXPIRED_MAIL);
        stmt->setUInt64(0, basetime);
        CharacterDatabase.Execute(stmt);
    }
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_EXPIRED_MAIL);
    stmt->setUInt64(0, basetime);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> No expired mails found.");
        return;                                             // any mails need to be returned or deleted
    }

    std::map<uint32 /*messageId*/, MailItemInfoVec> itemsCache;
    stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_EXPIRED_MAIL_ITEMS);
    stmt->setUInt32(0, (uint32)basetime);
    if (PreparedQueryResult items = CharacterDatabase.Query(stmt))
    {
        MailItemInfo item;
        do
        {
            Field* fields = items->Fetch();
            item.item_guid = fields[0].GetUInt32();
            item.item_template = fields[1].GetUInt32();
            uint32 mailId = fields[2].GetUInt32();
            itemsCache[mailId].push_back(item);
        } while (items->NextRow());
    }

    uint32 deletedCount = 0;
    uint32 returnedCount = 0;
    do
    {
        Field* fields = result->Fetch();
        Mail* m = new Mail;
        m->messageID = fields[0].GetUInt32();
        m->messageType = fields[1].GetUInt8();
        m->sender = fields[2].GetUInt32();
        m->receiver = fields[3].GetUInt32();
        bool has_items = fields[4].GetBool();
        m->expire_time = time_t(fields[5].GetUInt32());
        m->deliver_time = 0;
        m->COD = fields[6].GetUInt32();
        m->checked = fields[7].GetUInt8();
        m->mailTemplateId = fields[8].GetInt16();

        Player* player = nullptr;
        if (serverUp)
            player = ObjectAccessor::FindConnectedPlayer(ObjectGuid(HighGuid::Player, m->receiver));

        if (player && player->m_mailsLoaded)
        {                                                   // this code will run very improbably (the time is between 4 and 5 am, in game is online a player, who has old mail
                                                            // his in mailbox and he has already listed his mails)
            delete m;
            continue;
        }

        // Delete or return mail
        if (has_items)
        {
            // read items from cache
            m->items.swap(itemsCache[m->messageID]);

            // if it is mail from non-player, or if it's already return mail, it shouldn't be returned, but deleted
            if (m->messageType != MAIL_NORMAL || (m->checked & (MAIL_CHECK_MASK_COD_PAYMENT | MAIL_CHECK_MASK_RETURNED)))
            {
                // mail open and then not returned
                for (MailItemInfoVec::iterator itr2 = m->items.begin(); itr2 != m->items.end(); ++itr2)
                {
                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_ITEM_INSTANCE);
                    stmt->setUInt32(0, itr2->item_guid);
                    CharacterDatabase.Execute(stmt);
                }

                stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL_ITEM_BY_ID);
                stmt->setUInt32(0, m->messageID);
                CharacterDatabase.Execute(stmt);
            }
            else
            {
                // Mail will be returned
                stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_MAIL_RETURNED);
                stmt->setUInt32(0, m->receiver);
                stmt->setUInt32(1, m->sender);
                stmt->setUInt32(2, basetime + 30 * DAY);
                stmt->setUInt32(3, basetime);
                stmt->setUInt8(4, uint8(MAIL_CHECK_MASK_RETURNED));
                stmt->setUInt32(5, m->messageID);
                CharacterDatabase.Execute(stmt);
                for (MailItemInfoVec::iterator itr2 = m->items.begin(); itr2 != m->items.end(); ++itr2)
                {
                    // Update receiver in mail items for its proper delivery, and in instance_item for avoid lost item at sender delete
                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_MAIL_ITEM_RECEIVER);
                    stmt->setUInt32(0, m->sender);
                    stmt->setUInt32(1, itr2->item_guid);
                    CharacterDatabase.Execute(stmt);

                    stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_ITEM_OWNER);
                    stmt->setUInt32(0, m->sender);
                    stmt->setUInt32(1, itr2->item_guid);
                    CharacterDatabase.Execute(stmt);
                }
                delete m;
                ++returnedCount;
                continue;
            }
        }

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_MAIL_BY_ID);
        stmt->setUInt32(0, m->messageID);
        CharacterDatabase.Execute(stmt);
        delete m;
        ++deletedCount;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Processed %u expired mails: %u deleted and %u returned in %u ms", deletedCount + returnedCount, deletedCount, returnedCount, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadQuestAreaTriggers()
{
    _questAreaTriggerStore.clear();                           // need for reload case

    QueryResult result = WorldDatabase.Query( "SELECT id,quest FROM areatrigger_involvedrelation" );

    uint32 count = 0;

    if( !result )
    {
        TC_LOG_INFO("server.loading", ">> Loaded %u quest trigger points", count );
        
        return;
    }

    do
    {
        ++count;

        Field *fields = result->Fetch();

        uint32 trigger_ID = fields[0].GetUInt32();
        uint32 quest_ID   = fields[1].GetUInt32();

        AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(trigger_ID);
        if(!atEntry)
        {
            TC_LOG_ERROR("sql.sql","Area trigger (ID:%u) does not exist in `AreaTrigger.dbc`.",trigger_ID);
            continue;
        }

        Quest const* quest = GetQuestTemplate(quest_ID);

        if(!quest)
        {
            TC_LOG_ERROR("sql.sql","Table `areatrigger_involvedrelation` has record (id: %u) for not existing quest %u",trigger_ID,quest_ID);
            continue;
        }

        if(!quest->HasSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT))
        {
            TC_LOG_ERROR("sql.sql","Table `areatrigger_involvedrelation` has record (id: %u) for not quest %u, but quest not have flag QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT. Trigger or quest flags must be fixed, quest modified to require objective.",trigger_ID,quest_ID);

            // this will prevent quest completing without objective
            const_cast<Quest*>(quest)->SetSpecialFlag(QUEST_SPECIAL_FLAGS_EXPLORATION_OR_EVENT);

            // continue; - quest modified to required objective and trigger can be allowed.
        }

        _questAreaTriggerStore[trigger_ID] = quest_ID;

    } while( result->NextRow() );

    TC_LOG_INFO("server.loading", ">> Loaded %u quest trigger points", count );
    
}

QuestGreeting const* ObjectMgr::GetQuestGreeting(ObjectGuid guid) const
{
    auto itr = _questGreetingStore.find(guid.GetTypeId());
    if (itr == _questGreetingStore.end())
        return nullptr;

    auto questItr = itr->second.find(guid.GetEntry());
    if (questItr == itr->second.end())
        return nullptr;

    return &questItr->second;
}

void ObjectMgr::LoadQuestGreetings()
{
    uint32 oldMSTime = GetMSTime();

    _questGreetingStore.clear(); // need for reload case

                                 //                                                0   1          2                3             4
    QueryResult result = WorldDatabase.Query("SELECT ID, Type, GreetEmoteType, GreetEmoteDelay, Greeting FROM quest_greeting");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 quest greetings. DB table `quest_greeting` is empty.");
        return;
    }

    _questGreetingStore.rehash(result->GetRowCount());

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 id = fields[0].GetUInt32();
        uint8 type = fields[1].GetUInt8();
        // overwrite
        switch (type)
        {
        case 0: // Creature
            type = TYPEID_UNIT;
            if (!sObjectMgr->GetCreatureTemplate(id))
            {
                TC_LOG_ERROR("sql.sql", "Table `quest_greeting`: creature template (entry: %u) does not exist.", id);
                continue;
            }
            break;
        case 1: // GameObject
            type = TYPEID_GAMEOBJECT;
            if (!sObjectMgr->GetGameObjectTemplate(id))
            {
                TC_LOG_ERROR("sql.sql", "Table `quest_greeting`: gameobject template (entry: %u) does not exist.", id);
                continue;
            }
            break;
        default:
            TC_LOG_ERROR("sql.sql", "Table `quest_greeting`: unknown type = %u for entry = %u. Skipped.", type, id);
            continue;
        }

        uint16 greetEmoteType = fields[2].GetUInt16();

        if (greetEmoteType > 0 && !sEmotesStore.LookupEntry(greetEmoteType))
        {
            TC_LOG_DEBUG("sql.sql", "Table `quest_greeting`: entry %u has greetEmoteType = %u but emote does not exist. Set to 0.", id, greetEmoteType);
            greetEmoteType = 0;
        }

        uint32 greetEmoteDelay = fields[3].GetUInt32();
        std::string greeting = fields[4].GetString();

        _questGreetingStore[type][id] = QuestGreeting(greetEmoteType, greetEmoteDelay, greeting);

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u quest_greeting in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadQuestGreetingsLocales()
{
    uint32 oldMSTime = GetMSTime();

    _questGreetingLocaleStore.clear();                              // need for reload case

                                                                    //                                               0     1      2       3
    QueryResult result = WorldDatabase.Query("SELECT ID, Type, Locale, Greeting FROM quest_greeting_locale");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 quest_greeting locales. DB table `quest_greeting_locale` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 id = fields[0].GetUInt32();
        uint8 type = fields[1].GetUInt8();
        // overwrite
        switch (type)
        {
        case 0: // Creature
            type = TYPEID_UNIT;
            break;
        case 1: // GameObject
            type = TYPEID_GAMEOBJECT;
            break;
        default:
            break;
        }

        std::string localeName = fields[2].GetString();
        std::string greeting = fields[3].GetString();

        QuestGreetingLocale& data = _questGreetingLocaleStore[MAKE_PAIR32(id, type)];
        LocaleConstant locale = GetLocaleByName(localeName);
        if (locale == LOCALE_enUS)
            continue;

        AddLocaleString(greeting, locale, data.greeting);
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u quest greeting locale strings in %u ms", uint32(_questGreetingLocaleStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadTavernAreaTriggers()
{
    _tavernAreaTriggerStore.clear();                          // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT id FROM areatrigger_tavern");

    uint32 count = 0;

    if( !result )
    {
        TC_LOG_INFO("server.loading", ">> Loaded %u tavern triggers", count );
        
        return;
    }

    do
    {
        ++count;

        Field *fields = result->Fetch();

        uint32 Trigger_ID      = fields[0].GetUInt32();

        AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(Trigger_ID);
        if(!atEntry)
        {
            TC_LOG_ERROR("FIXME","Area trigger (ID:%u) does not exist in `AreaTrigger.dbc`.",Trigger_ID);
            continue;
        }

        _tavernAreaTriggerStore.insert(Trigger_ID);
    } while( result->NextRow() );

    TC_LOG_INFO("server.loading", ">> Loaded %u tavern triggers", count );
    
}

void ObjectMgr::LoadAreaTriggerScripts()
{
    _areaTriggerScriptStore.clear();                            // need for reload case
    QueryResult result = WorldDatabase.Query("SELECT entry, ScriptName FROM areatrigger_scripts");

    uint32 count = 0;

    if( !result )
    {
        TC_LOG_INFO("server.loading", ">> Loaded %u areatrigger scripts", count );
        
        return;
    }

    do
    {
        ++count;

        Field *fields = result->Fetch();

        uint32 Trigger_ID      = fields[0].GetUInt32();
        const char *scriptName = fields[1].GetCString();

        AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(Trigger_ID);
        if(!atEntry)
        {
            TC_LOG_ERROR("FIXME","Area trigger (ID:%u) does not exist in `AreaTrigger.dbc`.",Trigger_ID);
            continue;
        }
        _areaTriggerScriptStore[Trigger_ID] = GetScriptId(scriptName);
    } while( result->NextRow() );

    TC_LOG_INFO("server.loading", ">> Loaded %u areatrigger scripts", count );
    
}

uint32 ObjectMgr::GetNearestTaxiNode(float x, float y, float z, uint32 mapid, uint32 team)
{
    bool found = false;
    float dist = 10000;
    uint32 id = 0;

    for (uint32 i = 1; i < sTaxiNodesStore.GetNumRows(); ++i)
    {
        TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(i);

        if (!node || node->map_id != mapid || (!node->MountCreatureID[team == ALLIANCE ? 1 : 0] && node->MountCreatureID[0] != 32981)) // dk flight
            continue;

        uint8  field   = (uint8)((i - 1) / 32);
        uint32 submask = 1<<((i-1)%32);

        // skip not taxi network nodes
        if ((sTaxiNodesMask[field] & submask) == 0)
            continue;

        float dist2 = (node->x - x)*(node->x - x)+(node->y - y)*(node->y - y)+(node->z - z)*(node->z - z);
        if (found)
        {
            if (dist2 < dist)
            {
                dist = dist2;
                id = i;
            }
        }
        else
        {
            found = true;
            dist = dist2;
            id = i;
        }
    }

    return id;
}

void ObjectMgr::GetTaxiPath( uint32 source, uint32 destination, uint32 &path, uint32 &cost)
{
    auto src_i = sTaxiPathSetBySource.find(source);
    if(src_i==sTaxiPathSetBySource.end())
    {
        path = 0;
        cost = 0;
        return;
    }

    TaxiPathSetForSource& pathSet = src_i->second;

    auto dest_i = pathSet.find(destination);
    if(dest_i==pathSet.end())
    {
        path = 0;
        cost = 0;
        return;
    }

    cost = dest_i->second.price;
    path = dest_i->second.ID;
}

uint32 ObjectMgr::GetTaxiMountDisplayId(uint32 id, uint32 team, bool allowed_alt_team /* = false */)
{
    uint32 mount_id = 0;

    // select mount creature id
    TaxiNodesEntry const* node = sTaxiNodesStore.LookupEntry(id);
    if (node)
    {
        uint32 mount_entry = 0;
        if (team == ALLIANCE)
            mount_entry = node->MountCreatureID[1];
        else
            mount_entry = node->MountCreatureID[0];

        // Fix for Alliance not being able to use Acherus taxi
        // only one mount type for both sides
        if (mount_entry == 0 && allowed_alt_team)
        {
            // Simply reverse the selection. At least one team in theory should have a valid mount ID to choose.
            mount_entry = team == ALLIANCE ? node->MountCreatureID[0] : node->MountCreatureID[1];
        }

        CreatureTemplate const* mount_info = GetCreatureTemplate(mount_entry);
        if (mount_info)
        {
            mount_id = mount_info->GetRandomValidModelId();
            if (!mount_id)
            {
                TC_LOG_ERROR("sql.sql", "No displayid found for the taxi mount with the entry %u! Can't load it!", mount_entry);
                return 0;
            }
        }
    }

    // minfo is not actually used but the mount_id was updated
    GetCreatureModelRandomGender(mount_id);

    return mount_id;
}

Quest const* ObjectMgr::GetQuestTemplate(uint32 quest_id) const
{
    return Trinity::Containers::MapGetValuePtr(_questTemplates, quest_id);
}

void ObjectMgr::LoadGraveyardZones()
{
    mGraveYardMap.clear();                                  // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT id,ghost_zone,faction FROM game_graveyard_zone");

    uint32 count = 0;

    if( !result )
    {
        TC_LOG_INFO("server.loading", ">> Loaded %u graveyard-zone links", count );
        
        return;
    }

    do
    {
        ++count;

        Field *fields = result->Fetch();

        uint32 safeLocId = fields[0].GetUInt32();
        uint32 zoneId = fields[1].GetUInt32();
        uint32 team   = fields[2].GetUInt16();

        WorldSafeLocsEntry const* entry = sWorldSafeLocsStore.LookupEntry(safeLocId);
        if(!entry)
        {
            TC_LOG_ERROR("sql.sql","Table `game_graveyard_zone` has record for not existing graveyard (WorldSafeLocs.dbc id) %u, skipped.",safeLocId);
            continue;
        }

        AreaTableEntry const *areaEntry = sAreaTableStore.LookupEntry(zoneId);
        if(!areaEntry)
        {
            TC_LOG_ERROR("sql.sql","Table `game_graveyard_zone` has record for not existing zone id (%u), skipped.",zoneId);
            continue;
        }

        if(areaEntry->zone != 0)
        {
            TC_LOG_ERROR("sql.sql","Table `game_graveyard_zone` has record subzone/area id (%u) instead of zone, skipped.",zoneId);
            continue;
        }

        if(team!=0 && team!=HORDE && team!=ALLIANCE)
        {
            TC_LOG_ERROR("sql.sql","Table `game_graveyard_zone` has record for non player faction (%u), skipped.",team);
            continue;
        }

        if(!AddGraveYardLink(safeLocId,zoneId,team,false))
            TC_LOG_ERROR("sql.sql","Table `game_graveyard_zone` has a duplicate record for Graveyard (ID: %u) and Zone (ID: %u), skipped.",safeLocId,zoneId);
    } while( result->NextRow() );

    TC_LOG_INFO("server.loading", ">> Loaded %u graveyard-zone links", count );
    
}

WorldSafeLocsEntry const *ObjectMgr::GetClosestGraveYard(float x, float y, float z, uint32 MapId, uint32 team)
{
    // search for zone associated closest graveyard
    uint32 zoneId = sMapMgr->GetZoneId(MapId,x,y,z);

    // Simulate std. algorithm:
    //   found some graveyard associated to (ghost_zone,ghost_map)
    //
    //   if mapId == graveyard.mapId (ghost in plain zone or city or battleground) and search graveyard at same map
    //     then check faction
    //   if mapId != graveyard.mapId (ghost in instance) and search any graveyard associated
    //     then check faction
    GraveYardMap::const_iterator graveLow  = mGraveYardMap.lower_bound(zoneId);
    GraveYardMap::const_iterator graveUp   = mGraveYardMap.upper_bound(zoneId);
    if (graveLow == graveUp)
    {
        if(MapId != 13) //map 13 has no graveyard
            TC_LOG_ERROR("sql.sql", "Table `game_graveyard_zone` incomplete: Zone %u Team %u does not have a linked graveyard.",zoneId,team);
        return nullptr;
    }

    // at corpse map
    bool foundNear = false;
    float distNear = 0;
    WorldSafeLocsEntry const* entryNear = nullptr;

    // at entrance map for corpse map
    bool foundEntr = false;
    float distEntr = 0;
    WorldSafeLocsEntry const* entryEntr = nullptr;

    // some where other
    WorldSafeLocsEntry const* entryFar = nullptr;

    MapEntry const* mapEntry = sMapStore.LookupEntry(MapId);

    for(auto itr = graveLow; itr != graveUp; ++itr)
    {
        GraveYardData const& data = itr->second;

        WorldSafeLocsEntry const* entry = sWorldSafeLocsStore.LookupEntry(data.safeLocId);
        if(!entry)
        {
            TC_LOG_ERROR("sql.sql","Table `game_graveyard_zone` has record for not existing graveyard (WorldSafeLocs.dbc id) %u, skipped.", data.safeLocId);
            continue;
        }

        // skip enemy faction graveyard
        // team == 0 case can be at call from .neargrave
        if(data.team != 0 && team != 0 && data.team != team)
            continue;

        // find now nearest graveyard at other map
        if(MapId != entry->map_id)
        {
            // if find graveyard at different map from where entrance placed (or no entrance data), use any first
            if (!mapEntry || mapEntry->entrance_map != int32(entry->map_id) ||
                ((mapEntry->entrance_x == 0) && (mapEntry->entrance_y == 0)))
            {
                // not have any corrdinates for check distance anyway
                entryFar = entry;
                continue;
            }

            // at entrance map calculate distance (2D);
            float dist2 = (entry->x - mapEntry->entrance_x)*(entry->x - mapEntry->entrance_x)
                +(entry->y - mapEntry->entrance_y)*(entry->y - mapEntry->entrance_y);
            if(foundEntr)
            {
                if(dist2 < distEntr)
                {
                    distEntr = dist2;
                    entryEntr = entry;
                }
            }
            else
            {
                foundEntr = true;
                distEntr = dist2;
                entryEntr = entry;
            }
        }
        // find now nearest graveyard at same map
        else
        {
            float dist2 = (entry->x - x)*(entry->x - x)+(entry->y - y)*(entry->y - y)+(entry->z - z)*(entry->z - z);
            if(foundNear)
            {
                if(dist2 < distNear)
                {
                    distNear = dist2;
                    entryNear = entry;
                }
            }
            else
            {
                foundNear = true;
                distNear = dist2;
                entryNear = entry;
            }
        }
    }

    if(entryNear)
        return entryNear;

    if(entryEntr)
        return entryEntr;

    return entryFar;
}

GraveYardData const* ObjectMgr::FindGraveYardData(uint32 id, uint32 zoneId)
{
    GraveYardMap::const_iterator graveLow  = mGraveYardMap.lower_bound(zoneId);
    GraveYardMap::const_iterator graveUp   = mGraveYardMap.upper_bound(zoneId);

    for(auto itr = graveLow; itr != graveUp; ++itr)
    {
        if(itr->second.safeLocId==id)
            return &itr->second;
    }

    return nullptr;
}

bool ObjectMgr::AddGraveYardLink(uint32 id, uint32 zoneId, uint32 team, bool inDB)
{
    if(FindGraveYardData(id,zoneId))
        return false;

    // add link to loaded data
    GraveYardData data;
    data.safeLocId = id;
    data.team = team;

    mGraveYardMap.insert(GraveYardMap::value_type(zoneId,data));

    // add link to DB
    if(inDB)
    {
        WorldDatabase.PExecute("INSERT INTO game_graveyard_zone ( id,ghost_zone,faction) "
            "VALUES ('%u', '%u','%u')",id,zoneId,team);
    }

    return true;
}

void ObjectMgr::RemoveGraveYardLink(uint32 id, uint32 zoneId, uint32 team, bool inDB)
{
    auto graveLow  = mGraveYardMap.lower_bound(zoneId);
    auto graveUp   = mGraveYardMap.upper_bound(zoneId);
    if(graveLow==graveUp)
    {
        //TC_LOG_ERROR("FIXME","Table `game_graveyard_zone` incomplete: Zone %u Team %u does not have a linked graveyard.",zoneId,team);
        return;
    }

    bool found = false;

    GraveYardMap::iterator itr;

    for(itr = graveLow; itr != graveUp; ++itr)
    {
        GraveYardData & data = itr->second;

        // skip not matching safezone id
        if(data.safeLocId != id)
            continue;

        // skip enemy faction graveyard at same map (normal area, city, or battleground)
        // team == 0 case can be at call from .neargrave
        if(data.team != 0 && team != 0 && data.team != team)
            continue;

        found = true;
        break;
    }

    // no match, return
    if(!found)
        return;

    // remove from links
    mGraveYardMap.erase(itr);

    // remove link from DB
    if(inDB)
    {
        WorldDatabase.PExecute("DELETE FROM game_graveyard_zone WHERE id = '%u' AND ghost_zone = '%u' AND faction = '%u'",id,zoneId,team);
    }

    return;
}


void ObjectMgr::LoadAreaTriggerTeleports()
{
    _areaTriggerStore.clear();                                  // need for reload case

    uint32 count = 0;

    //                                                0       1           2              3               4                   5                   6  
    QueryResult result = WorldDatabase.PQuery("SELECT id, access_id, target_map, target_position_x, target_position_y, target_position_z, target_orientation "
                                              "FROM areatrigger_teleport t1 WHERE patch=(SELECT max(patch) FROM areatrigger_teleport t2 WHERE t1.id = t2.id && patch <= %u)", sWorld->GetWowPatch());
    if( !result )
    {
        TC_LOG_INFO("server.loading", ">> Loaded %u area trigger teleport definitions", count );
        
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        ++count;

        uint32 Trigger_ID = fields[0].GetUInt32();

        AreaTrigger at;

        at.access_id                = fields[1].GetUInt32();
        at.target_mapId             = fields[2].GetUInt16();
        at.target_X                 = fields[3].GetFloat();
        at.target_Y                 = fields[4].GetFloat();
        at.target_Z                 = fields[5].GetFloat();
        at.target_Orientation       = fields[6].GetFloat();

        AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(Trigger_ID);
        if(!atEntry)
        {
            TC_LOG_ERROR("sql.sql","Area trigger (ID:%u) does not exist in `AreaTrigger.dbc`.",Trigger_ID);
            continue;
        }
        
        MapEntry const* mapEntry = sMapStore.LookupEntry(at.target_mapId);
        if(!mapEntry)
        {
            TC_LOG_ERROR("sql.sql","Area trigger (ID:%u) target map (ID: %u) does not exist in `Map.dbc`.",Trigger_ID,at.target_mapId);
            continue;
        }

        if(at.target_X==0 && at.target_Y==0 && at.target_Z==0)
        {
            TC_LOG_ERROR("sql.sql","Area trigger (ID:%u) target coordinates not provided.",Trigger_ID);
            continue;
        }

        _areaTriggerStore[Trigger_ID] = at;

    } while( result->NextRow() );

    TC_LOG_INFO("server.loading", ">> Loaded %u area trigger teleport definitions", count );
    
}

void ObjectMgr::LoadAccessRequirements()
{
    _accessRequirementStore.clear();                                  // need for reload case

    uint32 count = 0;

    //                                                0       1          2       3      4        5           6             7              8                   9                  10
    QueryResult result = WorldDatabase.Query("SELECT id, level_min, level_max, item, item2, heroic_key, heroic_key2, quest_done, quest_failed_text, heroic_quest_done, heroic_quest_failed_text FROM access_requirement");
    if( !result )
    {
        TC_LOG_INFO("server.loading", ">> Loaded %u access requirement definitions", count );
        
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        ++count;

        uint32 requiremt_ID = fields[0].GetUInt32();

        AccessRequirement ar;

        ar.levelMin                 = fields[1].GetUInt8();
        ar.levelMax                 = fields[2].GetUInt8();
        ar.item                     = fields[3].GetUInt32();
        ar.item2                    = fields[4].GetUInt32();
        ar.heroicKey                = fields[5].GetUInt32();
        ar.heroicKey2               = fields[6].GetUInt32();
        ar.quest                    = fields[7].GetUInt32();
        ar.questFailedText          = fields[8].GetUInt32();
        ar.heroicQuest              = fields[9].GetUInt32();
        ar.heroicQuestFailedText    = fields[10].GetUInt32();

        if(ar.item)
        {
            ItemTemplate const *pProto = GetItemTemplate(ar.item);
            if(!pProto)
            {
                TC_LOG_ERROR("sql.sql","Key item %u does not exist for requirement %u, removing key requirement.", ar.item, requiremt_ID);
                ar.item = 0;
            }
        }

        if(ar.item2)
        {
            ItemTemplate const *pProto = GetItemTemplate(ar.item2);
            if(!pProto)
            {
                TC_LOG_ERROR("sql.sql","Second item %u does not exist for requirement %u, removing key requirement.", ar.item2, requiremt_ID);
                ar.item2 = 0;
            }
        }

        if(ar.heroicKey)
        {
            ItemTemplate const *pProto = GetItemTemplate(ar.heroicKey);
            if(!pProto)
            {
                TC_LOG_ERROR("sql.sql","Heroic key %u not exist for trigger %u, remove key requirement.", ar.heroicKey, requiremt_ID);
                ar.heroicKey = 0;
            }
        }

        if(ar.heroicKey2)
        {
            ItemTemplate const *pProto = GetItemTemplate(ar.heroicKey2);
            if(!pProto)
            {
                TC_LOG_ERROR("sql.sql","Second heroic key %u not exist for trigger %u, remove key requirement.", ar.heroicKey2, requiremt_ID);
                ar.heroicKey2 = 0;
            }
        }

        if(ar.heroicQuest)
        {
            if(!GetQuestTemplate(ar.heroicQuest))
            {
                TC_LOG_ERROR("sql.sql","Required Heroic Quest %u not exist for trigger %u, remove heroic quest done requirement.",ar.heroicQuest,requiremt_ID);
                ar.heroicQuest = 0;
            }
        }

        if(ar.quest)
        {
            if(!GetQuestTemplate(ar.quest))
            {
                TC_LOG_ERROR("sql.sql","Required Quest %u not exist for trigger %u, remove quest done requirement.",ar.quest,requiremt_ID);
                ar.quest = 0;
            }
        }

        if(ar.questFailedText)
        {
            if(!GetTrinityStringForDBCLocale(ar.questFailedText))
                TC_LOG_ERROR("sql.sql","access_requirement - text %u does not exist.",ar.questFailedText);
        }

        if(ar.heroicQuestFailedText)
        {
            if(!GetTrinityStringForDBCLocale(ar.heroicQuestFailedText))
                TC_LOG_ERROR("sql.sql","access_requirement - text %u does not exist.",ar.questFailedText);
        }

        _accessRequirementStore[requiremt_ID] = ar;

    } while( result->NextRow() );

    TC_LOG_INFO("server.loading", ">> Loaded %u access requirement definitions", count );
    
}

AreaTrigger const* ObjectMgr::GetGoBackTrigger(uint32 Map) const
{
    const MapEntry *mapEntry = sMapStore.LookupEntry(Map);
    if(!mapEntry) return nullptr;
    for (const auto & mAreaTrigger : _areaTriggerStore)
    {
        if(int32(mAreaTrigger.second.target_mapId) == mapEntry->entrance_map)
        {
            AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(mAreaTrigger.first);
            if(atEntry && atEntry->mapid == Map)
                return &mAreaTrigger.second;
        }
    }
    return nullptr;
}

AreaTrigger const* ObjectMgr::GetAreaTrigger(uint32 trigger) const
{
    auto itr = _areaTriggerStore.find(trigger);
    if (itr != _areaTriggerStore.end())
        return &itr->second;
    return nullptr;
}

AccessRequirement const* ObjectMgr::GetAccessRequirement(uint32 requirement) const
{
    return Trinity::Containers::MapGetValuePtr(_accessRequirementStore, requirement);
}

/**
 * Searches for the areatrigger which teleports players to the given map
 */
AreaTrigger const* ObjectMgr::GetMapEntranceTrigger(uint32 Map) const
{
    for (const auto & mAreaTrigger : _areaTriggerStore)
    {
        if(mAreaTrigger.second.target_mapId == Map)
        {
            AreaTriggerEntry const* atEntry = sAreaTriggerStore.LookupEntry(mAreaTrigger.first);
            if(atEntry)
                return &mAreaTrigger.second;
        }
    }
    return nullptr;
}

void ObjectMgr::SetHighestGuids()
{
    QueryResult result = CharacterDatabase.Query( "SELECT MAX(guid) FROM characters" );
    if( result )
        GetGuidSequenceGenerator<HighGuid::Player>().Set((*result)[0].GetUInt32() + 1);

    result = CharacterDatabase.Query( "SELECT MAX(guid) FROM item_instance" );
    if( result )
        GetGuidSequenceGenerator<HighGuid::Item>().Set((*result)[0].GetUInt32() + 1);

    // Cleanup other tables from not existed guids (>=_hiItemGuid)
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    trans->PAppend("DELETE FROM character_inventory WHERE item >= '%u'", GetGuidSequenceGenerator<HighGuid::Item>().GetNextAfterMaxUsed());
    trans->PAppend("DELETE FROM mail_items WHERE item_guid >= '%u'", GetGuidSequenceGenerator<HighGuid::Item>().GetNextAfterMaxUsed());
    trans->PAppend("DELETE FROM auctionhouse WHERE itemguid >= '%u'", GetGuidSequenceGenerator<HighGuid::Item>().GetNextAfterMaxUsed());
    trans->PAppend("DELETE FROM guild_bank_item WHERE item_guid >= '%u'", GetGuidSequenceGenerator<HighGuid::Item>().GetNextAfterMaxUsed());
    CharacterDatabase.CommitTransaction(trans);

    /* TC
    result = WorldDatabase.Query("SELECT MAX(guid) FROM transports");
    if (result)
        GetGuidSequenceGenerator<HighGuid::Mo_Transport>().Set((*result)[0].GetUInt32() + 1);
        */
    GetGuidSequenceGenerator<HighGuid::Mo_Transport>().Set(1);

    result = CharacterDatabase.Query("SELECT MAX(id) FROM auctionhouse" );
    if( result )
    {
        _auctionId = (*result)[0].GetUInt32()+1;
    }

    result = CharacterDatabase.Query( "SELECT MAX(id) FROM mail" );
    if( result )
    {
        _mailid = (*result)[0].GetUInt32()+1;
    }

    result = CharacterDatabase.Query( "SELECT MAX(id) FROM item_text" );
    if( result )
    {
        _ItemTextId = (*result)[0].GetUInt32()+1;
    }

    result = CharacterDatabase.Query("SELECT MAX(guildid) FROM guild");
    if (result)
        sGuildMgr->SetNextGuildId((*result)[0].GetUInt32() + 1);

    result = CharacterDatabase.Query("SELECT MAX(guid) FROM `groups`");
    if (result)
        sGroupMgr->SetGroupDbStoreSize((*result)[0].GetUInt32() + 1);

    result = WorldDatabase.Query("SELECT MAX(spawnId) FROM creature");
    if (result)
        _creatureSpawnId = (*result)[0].GetUInt32() + 1;

    result = WorldDatabase.Query("SELECT MAX(guid) FROM gameobject");
    if (result)
        _gameObjectSpawnId = (*result)[0].GetUInt32() + 1;
}

uint32 ObjectMgr::GenerateAuctionID()
{
    if(_auctionId>=0xFFFFFFFE)
    {
        TC_LOG_ERROR("misc","Auctions ids overflow!! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _auctionId++;
}

uint32 ObjectMgr::GenerateMailID()
{
    if(_mailid>=0xFFFFFFFE)
    {
        TC_LOG_ERROR("misc","Mail ids overflow!! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _mailid++;
}

uint32 ObjectMgr::GenerateItemTextID()
{
    if(_ItemTextId>=0xFFFFFFFE)
    {
        TC_LOG_ERROR("misc","Item text ids overflow!! Can't continue, shutting down server. ");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _ItemTextId++;
}

uint32 ObjectMgr::GeneratePetNumber()
{
    if (_hiPetNumber >= 0xFFFFFFFE)
    {
        TC_LOG_ERROR("misc", "_hiPetNumber Id overflow!! Can't continue, shutting down server. Search on forum for TCE00007 for more info.");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _hiPetNumber++;
}

uint32 ObjectMgr::GenerateCreatureSpawnId()
{
    if (_creatureSpawnId >= uint32(0xFFFFFF))
    {
        TC_LOG_ERROR("misc", "Creature spawn id overflow!! Can't continue, shutting down server. Search on forum for TCE00007 for more info.");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _creatureSpawnId++;
}

uint32 ObjectMgr::GenerateGameObjectSpawnId()
{
    if (_gameObjectSpawnId >= uint32(0xFFFFFF))
    {
        TC_LOG_ERROR("misc", "Creature spawn id overflow!! Can't continue, shutting down server. Search on forum for TCE00007 for more info. ");
        World::StopNow(ERROR_EXIT_CODE);
    }
    return _gameObjectSpawnId++;
}

uint32 ObjectMgr::CreateItemText(SQLTransaction& charTrans, std::string const& text)
{
    uint32 newItemTextId = GenerateItemTextID();
    //insert new itempage to container
    mItemTexts[newItemTextId] = text;
    //save new itempage
    std::string escapedText(text);
    CharacterDatabase.EscapeString(escapedText);
    //any Delete query needed, itemTextId is maximum of all ids
    charTrans->PAppend("INSERT INTO item_text(id, text) VALUES(%u, \"%s\")", newItemTextId, escapedText.c_str());
    return newItemTextId;
}

uint32 ObjectMgr::CreateItemText(std::string const& text)
{
    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    uint32 id = CreateItemText(trans, text);
    CharacterDatabase.CommitTransaction(trans);
    return id;
}

std::string ObjectMgr::GetItemText(uint32 id)
{
    ItemTextMap::const_iterator itr = mItemTexts.find(id);
    if (itr != mItemTexts.end())
        return itr->second;
    else
        return "<>";
}

void ObjectMgr::LoadGameObjectLocales()
{
    _gameObjectLocaleStore.clear();                           // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT entry,"
        "name_loc1,name_loc2,name_loc3,name_loc4,name_loc5,name_loc6,name_loc7,name_loc8,"
        "castbarcaption_loc1,castbarcaption_loc2,castbarcaption_loc3,castbarcaption_loc4,"
        "castbarcaption_loc5,castbarcaption_loc6,castbarcaption_loc7,castbarcaption_loc8 FROM locales_gameobject");

    if(!result)
    {
        TC_LOG_INFO("server.loading",">> Loaded 0 gameobject locale strings. DB table `locales_gameobject` is empty.");
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        GameObjectLocale& data = _gameObjectLocaleStore[entry];

        for (uint8 i = TOTAL_LOCALES - 1; i > 0; --i)
        {
            AddLocaleString(fields[i].GetString(), LocaleConstant(i), data.Name);
            AddLocaleString(fields[i + (TOTAL_LOCALES - 1)].GetString(), LocaleConstant(i), data.CastBarCaption);
        }
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded " UI64FMTD " gameobject locale strings", _gameObjectLocaleStore.size());
    
}

void ObjectMgr::LoadGameObjectTemplate()
{
    uint32 oldMSTime = GetMSTime();

    //                                                 0      1      2        3          4            5       6      7      8    9      10     11     12            14             16            18              20 
    QueryResult result = WorldDatabase.PQuery("SELECT entry, type, displayId, name, castBarCaption, faction, flags, size, data0, data1, data2, data3, data4, data5, data6, data7, data8, data9, data10, data11, data12, "
    //                                          21               23             25              27                      30              32        33
                                             "data13, data14, data15, data16, data17, data18, data19, data20, data21, data22, data23, AIName, ScriptName "
    //
                                             "FROM gameobject_template t1 WHERE patch = (SELECT max(patch) FROM gameobject_template t2 WHERE t1.entry = t2.entry && patch <= %u)", sWorld->GetWowPatch());

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 gameobject definitions. DB table `gameobject_template` is empty.");
        return;
    }

    _gameObjectTemplateStore.rehash(result->GetRowCount());
    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();

        GameObjectTemplate& got = _gameObjectTemplateStore[entry];

        got.entry          = entry;
        got.type           = uint32(fields[1].GetUInt8());
        got.displayId      = fields[2].GetUInt32();
        got.name           = fields[3].GetString();
        got.IconName       = ""; //TODO
        got.castBarCaption = fields[4].GetString();
        got.faction        = uint32(fields[5].GetUInt16());
        got.flags          = fields[6].GetUInt32();
        got.size           = fields[7].GetFloat();

        for (uint8 i = 0; i < MAX_GAMEOBJECT_DATA; ++i)
            got.raw.data[i] = fields[8 + i].GetInt32(); // data1 and data6 can be -1

        got.AIName = fields[32].GetString();
        got.ScriptId = GetScriptId(fields[33].GetCString());

        // Checks
        if (!got.AIName.empty() && !sGameObjectAIRegistry->HasItem(got.AIName))
        {
            TC_LOG_ERROR("sql.sql", "GameObject (Entry: %u) has non-registered `AIName` '%s' set, removing", got.entry, got.AIName.c_str());
            got.AIName.clear();
        }

        switch(got.type)
        {
            case GAMEOBJECT_TYPE_DOOR:                      //0
            {
                if(got.door.lockId)
                {
                    if(!sLockStore.LookupEntry(got.door.lockId))
                        TC_LOG_ERROR("sql.sql","Gameobject (Entry: %u GoType: %u) have data1=%u but lock (Id: %u) not found.",
                            entry,got.type,got.door.lockId,got.door.lockId);
                }
                break;
            }
            case GAMEOBJECT_TYPE_BUTTON:                    //1
            {
                if(got.button.lockId)
                {
                    if(!sLockStore.LookupEntry(got.button.lockId))
                        TC_LOG_ERROR("sql.sql","Gameobject (Entry: %u GoType: %u) have data1=%u but lock (Id: %u) not found.",
                            entry,got.type,got.button.lockId,got.button.lockId);
                }
                break;
            }
            case GAMEOBJECT_TYPE_CHEST:                     //3
            {
                if(got.chest.lockId)
                {
                    if(!sLockStore.LookupEntry(got.chest.lockId))
                        TC_LOG_ERROR("sql.sql","Gameobject (Entry: %u GoType: %u) have data0=%u but lock (Id: %u) not found.",
                            entry,got.type,got.chest.lockId,got.chest.lockId);
                }
                if(got.chest.linkedTrapId)              // linked trap
                {
                    if(GameObjectTemplate const* trapInfo = GetGameObjectTemplate(got.chest.linkedTrapId))
                    {
                        if(trapInfo->type!=GAMEOBJECT_TYPE_TRAP)
                            TC_LOG_ERROR("sql.sql","Gameobject (Entry: %u GoType: %u) have data7=%u but GO (Entry %u) have not GAMEOBJECT_TYPE_TRAP (%u) type.",
                                entry,got.type,got.chest.linkedTrapId,got.chest.linkedTrapId,GAMEOBJECT_TYPE_TRAP);
                    }
                    /* disable check for while
                    else
                        TC_LOG_ERROR("sql.sql","Gameobject (Entry: %u GoType: %u) have data2=%u but trap GO (Entry %u) not exist in `gameobject_template`.",
                            id,got.type,got.chest.linkedTrapId,got.chest.linkedTrapId);
                    */
                }
                break;
            }
            case GAMEOBJECT_TYPE_TRAP:                      //6
            {
                /* disable check for while
                if(got.trap.spellId)                    // spell
                {
                    if(!sSpellMgr->GetSpellInfo(got.trap.spellId))
                        TC_LOG_ERROR("FIXME","Gameobject (Entry: %u GoType: %u) have data3=%u but Spell (Entry %u) not exist.",
                            id,got.type,got.trap.spellId,got.trap.spellId);
                }
                */
                break;
            }
            case GAMEOBJECT_TYPE_CHAIR:                     //7
                if(got.chair.height > 2)
                {
                    TC_LOG_ERROR("sql.sql","Gameobject (Entry: %u GoType: %u) have data1=%u but correct chair height in range 0..2.",
                        entry,got.type,got.chair.height);

                    // prevent client and server unexpected work
                    got.chair.height = 0;
                }
                break;
            case GAMEOBJECT_TYPE_SPELL_FOCUS:               //8
            {
                if(got.spellFocus.focusId)
                {
                    if(!sSpellFocusObjectStore.LookupEntry(got.spellFocus.focusId))
                        TC_LOG_ERROR("sql.sql","Gameobject (Entry: %u GoType: %u) have data0=%u but SpellFocus (Id: %u) not exist.",
                            entry,got.type,got.spellFocus.focusId,got.spellFocus.focusId);
                }

                if(got.spellFocus.linkedTrapId)         // linked trap
                {
                    if(GameObjectTemplate const* trapInfo = GetGameObjectTemplate(got.spellFocus.linkedTrapId))
                    {
                        if(trapInfo->type != GAMEOBJECT_TYPE_TRAP)
                            TC_LOG_ERROR("sql.sql","Gameobject (Entry: %u GoType: %u) have data2=%u but GO (Entry %u) have not GAMEOBJECT_TYPE_TRAP (%u) type.",
                                entry,got.type,got.spellFocus.linkedTrapId,got.spellFocus.linkedTrapId, GAMEOBJECT_TYPE_TRAP);
                    }
                    /* disable check for while
                    else
                        TC_LOG_ERROR("sql.sql","Gameobject (Entry: %u GoType: %u) have data2=%u but trap GO (Entry %u) not exist in `gameobject_template`.",
                            id,got.type,got.spellFocus.linkedTrapId,got.spellFocus.linkedTrapId);
                    */
                }
                break;
            }
            case GAMEOBJECT_TYPE_GOOBER:                    //10
            {
                if (got.goober.pageId)                  // pageId
                {
                    if (!GetPageText(got.goober.pageId))
                        TC_LOG_ERROR("sql.sql", "GameObject (Entry: %u GoType: %u) have data7=%u but PageText (Entry %u) not exist.",
                        entry, got.type, got.goober.pageId, got.goober.pageId);
                }
                /* disable check for while
                if(got.goober.spellId)                  // spell
                {
                    if(!sSpellMgr->GetSpellInfo(got.goober.spellId))
                        TC_LOG_ERROR("sql.sql","Gameobject (Entry: %u GoType: %u) have data2=%u but Spell (Entry %u) not exist.",
                            id,got.type,got.goober.spellId,got.goober.spellId);
                }
                */
                if(got.goober.linkedTrapId)             // linked trap
                {
                    if(GameObjectTemplate const* trapInfo = GetGameObjectTemplate(got.goober.linkedTrapId))
                    {
                        if(trapInfo->type!=GAMEOBJECT_TYPE_TRAP)
                            TC_LOG_ERROR("sql.sql","Gameobject (Entry: %u GoType: %u) have data12=%u but GO (Entry %u) have not GAMEOBJECT_TYPE_TRAP (%u) type.",
                                entry,got.type,got.goober.linkedTrapId, got.goober.linkedTrapId, GAMEOBJECT_TYPE_TRAP);
                    }
                    /* disable check for while
                    else
                        TC_LOG_ERROR("sql.sql","Gameobject (Entry: %u GoType: %u) have data12=%u but trap GO (Entry %u) not exist in `gameobject_template`.",
                            id,got.type,got.goober.linkedTrapId,got.goober.linkedTrapId);
                    */
                }
                break;
            }
            case GAMEOBJECT_TYPE_MO_TRANSPORT:              //15
            {
                if(got.moTransport.taxiPathId)
                {
                    if(got.moTransport.taxiPathId >= sTaxiPathNodesByPath.size() || sTaxiPathNodesByPath[got.moTransport.taxiPathId].empty())
                        TC_LOG_ERROR("sql.sql","Gameobject (Entry: %u GoType: %u) have data0=%u but TaxiPath (Id: %u) not exist.",
                            entry,got.type,got.moTransport.taxiPathId,got.moTransport.taxiPathId);
                }
                if (uint32 transportMap = got.moTransport.mapID)
                    _transportMaps.insert(transportMap);
                break;
            }
            case GAMEOBJECT_TYPE_SUMMONING_RITUAL:          //18
            {
                /* disabled
                if(got.summoningRitual.spellId)
                {
                    if(!sSpellMgr->GetSpellInfo(got.summoningRitual.spellId))
                        TC_LOG_ERROR("sql.sql","Gameobject (Entry: %u GoType: %u) have data1=%u but Spell (Entry %u) not exist.",
                            id,got.type,got.summoningRitual.spellId,got.summoningRitual.spellId);
                }
                */
                break;
            }
            case GAMEOBJECT_TYPE_SPELLCASTER:               //22
            {
                if(got.spellcaster.spellId)             // spell
                {
                    if(!sSpellMgr->GetSpellInfo(got.spellcaster.spellId))
                        TC_LOG_ERROR("sql.sql","Gameobject (Entry: %u GoType: %u) have data3=%u but Spell (Entry %u) not exist.",
                            entry,got.type,got.spellcaster.spellId,got.spellcaster.spellId);
                }
                break;
            }
        }
        count++;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u game object templates in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadExplorationBaseXP()
{
    uint32 count = 0;
    QueryResult result = WorldDatabase.Query("SELECT level,basexp FROM exploration_basexp");

    if( !result )
    {
        TC_LOG_INFO("server.loading", ">> Loaded %u BaseXP definitions", count );
        
        return;
    }

    do
    {
        Field *fields = result->Fetch();
        uint32 level  = fields[0].GetUInt8();
        uint32 basexp = fields[1].GetUInt32();
        mBaseXPTable[level] = basexp;
        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u BaseXP definitions", count );
    
}

uint32 ObjectMgr::GetBaseXP(uint32 level)
{
    return mBaseXPTable[level] ? mBaseXPTable[level] : 0;
}

uint32 ObjectMgr::GetXPForLevel(uint8 level) const
{
    return Trinity::XP::GetXPForLevel(level);
}

void ObjectMgr::LoadPetNames()
{
    uint32 count = 0;
    QueryResult result = WorldDatabase.Query("SELECT word,entry,half FROM pet_name_generation");

    if( !result )
    {
        TC_LOG_INFO("server.loading", ">> Loaded %u pet name parts", count );
        
        return;
    }

    do
    {
        Field *fields = result->Fetch();
        std::string word = fields[0].GetString();
        uint32 entry     = fields[1].GetUInt32();
        bool   half      = fields[2].GetBool();
        if(half)
            PetHalfName1[entry].push_back(word);
        else
            PetHalfName0[entry].push_back(word);
        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u pet name parts", count );
    
}

void ObjectMgr::LoadPetNumber()
{
    QueryResult result = CharacterDatabase.Query("SELECT MAX(id) FROM character_pet");
    if(result)
    {
        Field *fields = result->Fetch();
        _hiPetNumber = fields[0].GetUInt32()+1;
    }

    TC_LOG_INFO("server.loading", ">> Loaded the max pet number: %d", _hiPetNumber-1);
    
}

std::string ObjectMgr::GeneratePetName(uint32 entry)
{
    std::vector<std::string> & list0 = PetHalfName0[entry];
    std::vector<std::string> & list1 = PetHalfName1[entry];

    if(list0.empty() || list1.empty())
    {
        CreatureTemplate const *cinfo = GetCreatureTemplate(entry);
        std::string petname = GetPetName(cinfo->family, sWorld->GetDefaultDbcLocale());
        if(petname.empty())
            petname = cinfo->Name;
        return std::string(petname);
    }

    return *(list0.begin()+urand(0, list0.size()-1)) + *(list1.begin()+urand(0, list1.size()-1));
}

/*
void ObjectMgr::LoadCorpses()
{
    uint32 count = 0;
    //                                                     0           1           2           3            4    5     6     7            8         10
    QueryResult result = CharacterDatabase.Query("SELECT position_x, position_y, position_z, orientation, map, data, time, corpse_type, instanceId, guid FROM corpse WHERE corpse_type <> 0");

    if( !result )
    {
        TC_LOG_INFO("server.loading", ">> Loaded %u corpses", count );
        
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        ObjectGuid::LowType guid = fields[result->GetFieldCount()-1].GetUInt32();

        auto corpse = new Corpse;
        if(!corpse->LoadFromDB(guid,fields))
        {
            delete corpse;
            continue;
        }

        GetMap()->AddCorpse(corpse);

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u corpses", count );
    
}
*/

void ObjectMgr::LoadReputationOnKill()
{
    uint32 count = 0;

    //                                                0            1                     2
    QueryResult result = WorldDatabase.Query("SELECT creature_id, RewOnKillRepFaction1, RewOnKillRepFaction2,"
    //   3             4             5                   6             7             8                   9
        "IsTeamAward1, MaxStanding1, RewOnKillRepValue1, IsTeamAward2, MaxStanding2, RewOnKillRepValue2, TeamDependent "
        "FROM creature_onkill_reputation");

    if(!result)
    {
        TC_LOG_ERROR("sql.sql",">> Loaded 0 creature award reputation definitions. DB table `creature_onkill_reputation` is empty.");
        
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        uint32 creature_id = fields[0].GetUInt32();

        ReputationOnKillEntry repOnKill;
        repOnKill.RepFaction1          = fields[1].GetUInt32();
        repOnKill.RepFaction2          = fields[2].GetUInt32();
        repOnKill.is_teamaward1        = fields[3].GetBool();
        repOnKill.ReputationMaxCap1  = fields[4].GetUInt8();
        repOnKill.RepValue1            = fields[5].GetInt32();
        repOnKill.is_teamaward2        = fields[6].GetBool();
        repOnKill.ReputationMaxCap2  = fields[7].GetUInt8();
        repOnKill.RepValue2            = fields[8].GetInt32();
        repOnKill.TeamDependent       = fields[9].GetUInt8();

        if(!GetCreatureTemplate(creature_id))
        {
            TC_LOG_ERROR("sql.sql","Table `creature_onkill_reputation` have data for not existed creature entry (%u), skipped",creature_id);
            continue;
        }

        if(repOnKill.RepFaction1)
        {
            FactionEntry const *factionEntry1 = sFactionStore.LookupEntry(repOnKill.RepFaction1);
            if(!factionEntry1)
            {
                TC_LOG_ERROR("sql.sql","Faction (faction.dbc) %u does not exist but is used in `creature_onkill_reputation`",repOnKill.RepFaction1);
                continue;
            }
        }

        if(repOnKill.RepFaction2)
        {
            FactionEntry const *factionEntry2 = sFactionStore.LookupEntry(repOnKill.RepFaction2);
            if(!factionEntry2)
            {
                TC_LOG_ERROR("sql.sql","Faction (faction.dbc) %u does not exist but is used in `creature_onkill_reputation`",repOnKill.RepFaction2);
                continue;
            }
        }

        _repOnKillStore[creature_id] = repOnKill;

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading",">> Loaded %u creature award reputation definitions", count);
    
}

void ObjectMgr::LoadPointsOfInterest()
{
    uint32 oldMSTime = GetMSTime();

    _pointsOfInterestStore.clear();                              // need for reload case

    uint32 count = 0;

    //                                                  0   1  2   3      4     5       6
    QueryResult result = WorldDatabase.Query("SELECT entry, x, y, icon, flags, data, icon_name FROM points_of_interest");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 Points of Interest definitions. DB table `points_of_interest` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 point_id = fields[0].GetUInt32();

        PointOfInterest POI;
        POI.entry = point_id;
        POI.x = fields[1].GetFloat();
        POI.y = fields[2].GetFloat();
        POI.icon = fields[3].GetUInt32();
        POI.flags = fields[4].GetUInt32();
        POI.data = fields[5].GetUInt32();
        POI.icon_name = fields[6].GetString();

        if (!Trinity::IsValidMapCoord(POI.x, POI.y))
        {
            TC_LOG_ERROR("sql.sql", "Table `points_of_interest` (Entry: %u) have invalid coordinates (X: %f Y: %f), ignored.", point_id, POI.x, POI.y);
            continue;
        }

        _pointsOfInterestStore[point_id] = POI;

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u Points of Interest definitions in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadWeatherZoneChances()
{
    uint32 count = 0;

    //                                                0     1                   2                   3                    4                   5                   6                    7                 8                 9                  10                  11                  12
    QueryResult result = WorldDatabase.Query("SELECT zone, spring_rain_chance, spring_snow_chance, spring_storm_chance, summer_rain_chance, summer_snow_chance, summer_storm_chance, fall_rain_chance, fall_snow_chance, fall_storm_chance, winter_rain_chance, winter_snow_chance, winter_storm_chance FROM game_weather");

    if(!result)
    {
        TC_LOG_ERROR("server.loading",">> Loaded 0 weather definitions. DB table `game_weather` is empty.");
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        uint32 zone_id = fields[0].GetUInt32();

        WeatherZoneChances& wzc = mWeatherZoneMap[zone_id];

        for(int season = 0; season < WEATHER_SEASONS; ++season)
        {
            wzc.data[season].rainChance  = fields[season * (MAX_WEATHER_TYPE-1) + 1].GetUInt8();
            wzc.data[season].snowChance  = fields[season * (MAX_WEATHER_TYPE-1) + 2].GetUInt8();
            wzc.data[season].stormChance = fields[season * (MAX_WEATHER_TYPE-1) + 3].GetUInt8();

            if(wzc.data[season].rainChance > 100)
            {
                wzc.data[season].rainChance = 25;
                TC_LOG_ERROR("sql.sql","Weather for zone %u season %u has wrong rain chance > 100%%",zone_id,season);
            }

            if(wzc.data[season].snowChance > 100)
            {
                wzc.data[season].snowChance = 25;
                TC_LOG_ERROR("sql.sql","Weather for zone %u season %u has wrong snow chance > 100%%", zone_id, season);
            }

            if(wzc.data[season].stormChance > 100)
            {
                wzc.data[season].stormChance = 25;
                TC_LOG_ERROR("sql.sql","Weather for zone %u season %u has wrong storm chance > 100%%",zone_id,season);
            }
        }

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading",">> Loaded %u weather definitions", count);
}

void ObjectMgr::DeleteGameObjectData(ObjectGuid::LowType guid)
{
    // remove mapid*cellid -> guid_set map
    GameObjectData const* data = GetGameObjectData(guid);
    if (data)
    {
        RemoveGameobjectFromGrid(guid, data);
        OnDeleteSpawnData(data);
    }

    _gameObjectDataStore.erase(guid);
}

void ObjectMgr::LoadQuestRelationsHelper(QuestRelations& map, char const* table)
{
    map.clear();                                            // need for reload case

    uint32 count = 0;

    QueryResult result = WorldDatabase.PQuery("SELECT id,quest FROM %s",table);

    if(!result)
    {
        TC_LOG_ERROR("sql.sql",">> Loaded 0 quest relations from %s. DB table `%s` is empty.",table,table);
        
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        uint32 id    = fields[0].GetUInt32();
        uint32 quest = fields[1].GetUInt32();

        if(_questTemplates.find(quest) == _questTemplates.end())
        {
            TC_LOG_ERROR("sql.sql","Table `%s: Quest %u listed for entry %u does not exist.",table,quest,id);
            continue;
        }

        map.insert(QuestRelations::value_type(id,quest));

        ++count;
    } while (result->NextRow());

    TC_LOG_INFO("server.loading",">> Loaded %u quest relations from %s", count,table);
    
}

void ObjectMgr::LoadGameobjectQuestStarters()
{
    LoadQuestRelationsHelper(mGOQuestRelations,"gameobject_queststarter");

    for(auto itr = mGOQuestRelations.begin(); itr != mGOQuestRelations.end(); ++itr)
    {
        GameObjectTemplate const* goInfo = GetGameObjectTemplate(itr->first);
        if(!goInfo)
            TC_LOG_ERROR("sql.sql","Table `gameobject_queststarter` have data for not existed gameobject entry (%u) and existed quest %u",itr->first,itr->second);
        else if(goInfo->type != GAMEOBJECT_TYPE_QUESTGIVER)
            TC_LOG_ERROR("sql.sql","Table `gameobject_queststarter` have data gameobject entry (%u) for quest %u, but GO is not GAMEOBJECT_TYPE_QUESTGIVER",itr->first,itr->second);
    }
}

void ObjectMgr::LoadGameobjectQuestEnders()
{
    LoadQuestRelationsHelper(mGOQuestInvolvedRelations,"gameobject_questender");

    for(auto itr = mGOQuestInvolvedRelations.begin(); itr != mGOQuestInvolvedRelations.end(); ++itr)
    {
        GameObjectTemplate const* goInfo = GetGameObjectTemplate(itr->first);
        if(!goInfo)
            TC_LOG_ERROR("sql.sql","Table `gameobject_questender` have data for not existed gameobject entry (%u) and existed quest %u",itr->first,itr->second);
        else if(goInfo->type != GAMEOBJECT_TYPE_QUESTGIVER)
            TC_LOG_ERROR("sql.sql","Table `gameobject_questender` have data gameobject entry (%u) for quest %u, but GO is not GAMEOBJECT_TYPE_QUESTGIVER",itr->first,itr->second);
    }
}

void ObjectMgr::LoadCreatureQuestStarters()
{
    LoadQuestRelationsHelper(mCreatureQuestRelations,"creature_queststarter");

    for(auto itr = mCreatureQuestRelations.begin(); itr != mCreatureQuestRelations.end(); ++itr)
    {
        CreatureTemplate const* cInfo = GetCreatureTemplate(itr->first);
        if(!cInfo)
            TC_LOG_ERROR("sql.sql","Table `creature_queststarter` have data for not existed creature entry (%u) and existed quest %u",itr->first,itr->second);
        else if(!(cInfo->npcflag & UNIT_NPC_FLAG_QUESTGIVER))
            TC_LOG_ERROR("sql.sql","Table `creature_queststarter` has creature entry (%u) for quest %u, but npcflag does not include UNIT_NPC_FLAG_QUESTGIVER",itr->first,itr->second);
    }
}

void ObjectMgr::LoadCreatureQuestEnders()
{
    LoadQuestRelationsHelper(mCreatureQuestInvolvedRelations,"creature_questender");

    for(auto itr = mCreatureQuestInvolvedRelations.begin(); itr != mCreatureQuestInvolvedRelations.end(); ++itr)
    {
        CreatureTemplate const* cInfo = GetCreatureTemplate(itr->first);
        if(!cInfo)
            TC_LOG_ERROR("sql.sql","Table `creature_questender` have data for not existed creature entry (%u) and existed quest %u",itr->first,itr->second);
        else if(!(cInfo->npcflag & UNIT_NPC_FLAG_QUESTGIVER))
            TC_LOG_ERROR("sql.sql","Table `creature_questender` has creature entry (%u) for quest %u, but npcflag does not include UNIT_NPC_FLAG_QUESTGIVER",itr->first,itr->second);
    }
}

void ObjectMgr::LoadReservedPlayersNames()
{
    _reservedNamesStore.clear();                                // need for reload case

    QueryResult result = WorldDatabase.Query("SELECT name FROM reserved_name");

    uint32 count = 0;

    if( !result )
    {
        TC_LOG_INFO("server.loading", ">> Loaded %u reserved player names", count );
        
        return;
    }

    Field* fields;
    do
    {
        fields = result->Fetch();
        std::string name= fields[0].GetString();
        if(normalizePlayerName(name))
        {
            _reservedNamesStore.insert(name);
            ++count;
        }
    } while ( result->NextRow() );

    TC_LOG_INFO("server.loading", ">> Loaded %u reserved player names", count );
    
}

enum LanguageType
{
    LT_BASIC_LATIN    = 0x0000,
    LT_EXTENDEN_LATIN = 0x0001,
    LT_CYRILLIC       = 0x0002,
    LT_EAST_ASIA      = 0x0004,
    LT_ANY            = 0xFFFF
};

static LanguageType GetRealmLanguageType(bool create)
{
    switch(sWorld->getConfig(CONFIG_REALM_ZONE))
    {
        case REALM_ZONE_UNKNOWN:                            // any language
        case REALM_ZONE_DEVELOPMENT:
        case REALM_ZONE_TEST_SERVER:
        case REALM_ZONE_QA_SERVER:
            return LT_ANY;
        case REALM_ZONE_UNITED_STATES:                      // extended-Latin
        case REALM_ZONE_OCEANIC:
        case REALM_ZONE_LATIN_AMERICA:
        case REALM_ZONE_ENGLISH:
        case REALM_ZONE_GERMAN:
        case REALM_ZONE_FRENCH:
        case REALM_ZONE_SPANISH:
            return LT_EXTENDEN_LATIN;
        case REALM_ZONE_KOREA:                              // East-Asian
        case REALM_ZONE_TAIWAN:
        case REALM_ZONE_CHINA:
            return LT_EAST_ASIA;
        case REALM_ZONE_RUSSIAN:                            // Cyrillic
            return LT_CYRILLIC;
        default:
            return create ? LT_BASIC_LATIN : LT_ANY;        // basic-Latin at create, any at login
    }
}

bool isValidString(std::wstring wstr, uint32 strictMask, bool numericOrSpace, bool create = false)
{
    if(strictMask==0)                                       // any language, ignore realm
    {
        if(isExtendedLatinString(wstr,numericOrSpace))
            return true;
        if(isCyrillicString(wstr,numericOrSpace))
            return true;
        if(isEastAsianString(wstr,numericOrSpace))
            return true;
        return false;
    }

    if(strictMask & 0x2)                                    // realm zone specific
    {
        LanguageType lt = GetRealmLanguageType(create);
        if(lt & LT_EXTENDEN_LATIN)
            if(isExtendedLatinString(wstr,numericOrSpace))
                return true;
        if(lt & LT_CYRILLIC)
            if(isCyrillicString(wstr,numericOrSpace))
                return true;
        if(lt & LT_EAST_ASIA)
            if(isEastAsianString(wstr,numericOrSpace))
                return true;
    }

    if(strictMask & 0x1)                                    // basic Latin
    {
        if(isBasicLatinString(wstr,numericOrSpace))
            return true;
    }

    return false;
}

ResponseCodes ObjectMgr::CheckPlayerName( const std::string& name, bool create )
{
    std::wstring wname;
    if(!Utf8toWStr(name,wname))
        return CHAR_NAME_INVALID_CHARACTER;

    if( wname.size() > MAX_PLAYER_NAME)
        return CHAR_NAME_TOO_LONG;

    if(wname.size() < 2)
        return CHAR_NAME_TOO_SHORT;

    uint32 strictMask = sWorld->getConfig(CONFIG_STRICT_PLAYER_NAMES);

    if(!isValidString(wname,strictMask,false,create))
        return CHAR_NAME_MIXED_LANGUAGES;

    wstrToLower(wname);
    for (size_t i = 2; i < wname.size(); ++i)
        if (wname[i] == wname[i-1] && wname[i] == wname[i-2])
            return CHAR_NAME_THREE_CONSECUTIVE;

    return CHAR_NAME_SUCCESS;
}

bool ObjectMgr::IsValidCharterName( const std::string& name )
{
    std::wstring wname;
    if(!Utf8toWStr(name,wname))
        return false;

    if(wname.size() < 1)
        return false;

    if (wname.size() > MAX_CHARTER_NAME)
        return false;

    uint32 strictMask = sWorld->getConfig(CONFIG_STRICT_CHARTER_NAMES);

    return isValidString(wname,strictMask,true);
}

PetNameInvalidReason ObjectMgr::CheckPetName( const std::string& name )
{
    std::wstring wname;
    if(!Utf8toWStr(name,wname))
        return PET_NAME_INVALID;

    if(wname.size() < 1)
        return PET_NAME_INVALID;

    if (wname.size() > MAX_PET_NAME)
        return PET_NAME_TOO_LONG;

    uint32 strictMask = sWorld->getConfig(CONFIG_STRICT_PET_NAMES);

    if (!isValidString(wname, strictMask, false))
        return PET_NAME_MIXED_LANGUAGES;

    return PET_NAME_SUCCESS;
}

void ObjectMgr::LoadBattleMastersEntry()
{
    _battleMastersStore.clear();                              // need for reload case

    QueryResult result = WorldDatabase.Query( "SELECT entry, bg_template FROM battlemaster_entry" );

    uint32 count = 0;

    if( !result )
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 battlemaster entries - table is empty!" );
        
        return;
    }

    do
    {
        ++count;

        Field *fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();
        uint32 bgTypeId  = fields[1].GetUInt32();

        _battleMastersStore[entry] = BattlegroundTypeId(bgTypeId);

    } while( result->NextRow() );

    TC_LOG_INFO("server.loading", ">> Loaded %u battlemaster entries", count );
    
}

void ObjectMgr::LoadGameObjectForQuests()
{
    _gameObjectForQuestStore.clear();                         // need for reload case

    if (sObjectMgr->GetGameObjectTemplateStore()->empty())
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 GameObjects for quests");
        return;
    }

    uint32 count = 0;

    
    // collect GO entries for GO that must activated
    GameObjectTemplateContainer const* gotc = sObjectMgr->GetGameObjectTemplateStore();
    for (const auto & itr : *gotc)
    {
        switch(itr.second.type)
        {
            // scan GO chest with loot including quest items
            case GAMEOBJECT_TYPE_CHEST:
            {
                uint32 loot_id = itr.second.GetLootId();

                // find quest loot for GO
                if(LootTemplates_Gameobject.HaveQuestLootFor(loot_id))
                {
                    _gameObjectForQuestStore.insert(itr.second.entry);
                    ++count;
                }
                break;
            }
            case GAMEOBJECT_TYPE_GOOBER:
            {
                if(itr.second.goober.questId)                  //quests objects
                {
                    _gameObjectForQuestStore.insert(itr.second.entry);
                    count++;
                }
                break;
            }
            default:
                break;
        }
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u GameObject for quests", count );
    
}

bool ObjectMgr::LoadTrinityStrings(DatabaseWorkerPool<WorldDatabaseConnection>& db, char const* table, int32 min_value, int32 max_value)
{
    // cleanup affected map part for reloading case
    for(auto itr = _trinityStringStore.begin(); itr != _trinityStringStore.end();)
    {
        if(itr->first >= min_value && itr->first <= max_value)
        {
            auto itr2 = itr;
            ++itr;
            _trinityStringStore.erase(itr2);
        }
        else
            ++itr;
    }

    QueryResult result = db.PQuery("SELECT entry,content_default,content_loc1,content_loc2,content_loc3,content_loc4,content_loc5,content_loc6,content_loc7,content_loc8 FROM %s",table);

    if(!result)
    {
        if(min_value == MIN_TRINITY_STRING_ID)               // error only in case internal strings
            TC_LOG_ERROR("sql.sql",">> Loaded 0 trinity strings. DB table `%s` is empty. Cannot continue.",table);
        else
            TC_LOG_INFO("sql.sql",">> Loaded 0 string templates. DB table `%s` is empty.",table);
        return false;
    }

    uint32 count = 0;

    do
    {
        Field *fields = result->Fetch();

        int32 entry = fields[0].GetInt32();

        if(entry==0)
        {
            TC_LOG_ERROR("sql.sql","Table `%s` contain reserved entry 0, ignored.",table);
            continue;
        }
        else if(entry < min_value || entry > max_value)
        {
            int32 start = min_value > 0 ? min_value : max_value;
            int32 end   = min_value > 0 ? max_value : min_value;
            TC_LOG_ERROR("sql.sql","Table `%s` contain entry %i out of allowed range (%d - %d), ignored.",table,entry,start,end);
            continue;
        }

        TrinityStringLocale& data = _trinityStringStore[entry];

        if(data.Content.size() > 0)
        {
            TC_LOG_ERROR("sql.sql","Table `%s` contain data for already loaded entry  %i (from another table?), ignored.",table,entry);
            continue;
        }

        data.Content.resize(1);
        ++count;

        for (int8 i = MAX_LOCALE; i >= 0; --i)
            AddLocaleString(fields[i + 1].GetString(), LocaleConstant(i), data.Content);

    } while (result->NextRow());

    
    if(min_value == MIN_TRINITY_STRING_ID)               // internal Trinity strings
        TC_LOG_INFO("server.loading", ">> Loaded %u Trinity strings from table %s", count,table);
    else
        TC_LOG_INFO("server.loading", ">> Loaded %u string templates from %s", count,table);

    return true;
}

const char *ObjectMgr::GetTrinityString(int32 entry, LocaleConstant locale_idx) const
{
    // Content[0] always exist if exist TrinityStringLocale
    if(TrinityStringLocale const *msl = GetTrinityStringLocale(entry))
    {
        if(msl->Content.size() > locale_idx && !msl->Content[locale_idx].empty())
            return msl->Content[locale_idx].c_str();
        else
            return msl->Content[0].c_str();
    }

    if(entry > 0)
        TC_LOG_ERROR("sql.sql","Entry %i not found in `trinity_string` table.",entry);
    else
        TC_LOG_ERROR("sql.sql","Trinity string entry %i not found in DB.",entry);

    return "<error>";
}

void ObjectMgr::LoadSpellDisabledEntrys()
{
    m_DisabledPlayerSpells.clear();                                // need for reload case
    m_DisabledCreatureSpells.clear();
    m_DisabledPetSpells.clear();
    QueryResult result = WorldDatabase.Query("SELECT entry, disable_mask FROM spell_disabled");

    uint32 total_count = 0;

    if( !result )
    {
        TC_LOG_INFO("server.loading", ">> Loaded %u disabled spells", total_count );
        
        return;
    }

    Field* fields;
    do
    {
        fields = result->Fetch();
        uint32 spellid = fields[0].GetUInt32();
        if(!sSpellMgr->GetSpellInfo(spellid))
        {
            TC_LOG_ERROR("sql.sql","Spell entry %u from `spell_disabled` doesn't exist in dbc, ignoring.",spellid);
            continue;
        }
        uint32 disable_mask = fields[1].GetUInt32();
        if(disable_mask & SPELL_DISABLE_PLAYER)
            m_DisabledPlayerSpells.insert(spellid);
        if(disable_mask & SPELL_DISABLE_CREATURE)
            m_DisabledCreatureSpells.insert(spellid);
        if(disable_mask & SPELL_DISABLE_PET)
            m_DisabledPetSpells.insert(spellid);
        ++total_count;
   } while ( result->NextRow() );

    TC_LOG_INFO("server.loading", ">> Loaded %u disabled spells from `spell_disabled`", total_count);
    
}

void ObjectMgr::LoadFishingBaseSkillLevel()
{
    mFishingBaseForArea.clear();                            // for reload case

    uint32 count = 0;
    QueryResult result = WorldDatabase.Query("SELECT entry,skill FROM skill_fishing_base_level");

    if( !result )
    {
        TC_LOG_ERROR("sql.sql",">> Loaded `skill_fishing_base_level`, table is empty!");
        
        return;
    }

    do
    {
        Field *fields = result->Fetch();
        uint32 entry  = fields[0].GetUInt32();
        int32 skill   = fields[1].GetInt16();

        AreaTableEntry const* fArea = sAreaTableStore.LookupEntry(entry);
        if(!fArea)
        {
            TC_LOG_ERROR("sql.sql","AreaId %u defined in `skill_fishing_base_level` does not exist",entry);
            continue;
        }

        mFishingBaseForArea[entry] = skill;
        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u areas for fishing base skill level", count );
    
}

bool ObjectMgr::CheckDeclinedNames( std::wstring mainpart, DeclinedName const& names )
{
    for(int i =0; i < MAX_DECLINED_NAME_CASES; ++i)
    {
        std::wstring wname;
        if(!Utf8toWStr(names.name[i],wname))
            return false;

        if(mainpart!=GetMainPartOfName(wname,i+1))
            return false;
    }
    return true;
}

uint32 ObjectMgr::GetAreaTriggerScriptId(uint32 trigger_id)
{
    AreaTriggerScriptMap::const_iterator i = _areaTriggerScriptStore.find(trigger_id);
    if(i!= _areaTriggerScriptStore.end())
        return i->second;
    return 0;
}

SpellScriptsBounds ObjectMgr::GetSpellScriptsBounds(uint32 spell_id)
{
    return SpellScriptsBounds(_spellScriptsStore.lower_bound(spell_id), _spellScriptsStore.upper_bound(spell_id));
}

SkillRangeType GetSkillRangeType(SkillRaceClassInfoEntry const* rcEntry)
{
    SkillLineEntry const* skill = sSkillLineStore.LookupEntry(rcEntry->SkillId);
    if (!skill)
        return SKILL_RANGE_NONE;

    if (sSkillTiersStore.LookupEntry(rcEntry->SkillTier))
        return SKILL_RANGE_RANK;

#ifdef LICH_KING
    if (rcEntry->SkillId == SKILL_RUNEFORGING)
        return SKILL_RANGE_MONO;
#endif

    switch (skill->categoryId)
    {
    case SKILL_CATEGORY_ARMOR:
        return SKILL_RANGE_MONO;
    case SKILL_CATEGORY_LANGUAGES:
        return SKILL_RANGE_LANGUAGE;
    }

    return SKILL_RANGE_LEVEL;
}

void ObjectMgr::LoadGameTele()
{
    m_GameTeleMap.clear();                                  // for reload case

    uint32 count = 0;
    QueryResult result = WorldDatabase.Query("SELECT id, position_x, position_y, position_z, orientation, map, name FROM game_tele");

    if( !result )
    {
        TC_LOG_ERROR("server.loading",">> Loaded `game_tele`, table is empty!");
        
        return;
    }

    do
    {
        Field *fields = result->Fetch();

        uint32 id         = fields[0].GetUInt32();

        GameTele gt;

        gt.position_x     = fields[1].GetFloat();
        gt.position_y     = fields[2].GetFloat();
        gt.position_z     = fields[3].GetFloat();
        gt.orientation    = fields[4].GetFloat();
        gt.mapId          = fields[5].GetUInt16();
        gt.name           = fields[6].GetString();

        if(!MapManager::IsValidMapCoord(gt.mapId,gt.position_x,gt.position_y,gt.position_z,gt.orientation))
        {
            TC_LOG_ERROR("sql.sql","Wrong position for id %u (name: %s) in `game_tele` table, ignoring.",id,gt.name.c_str());
            continue;
        }

        if(!Utf8toWStr(gt.name,gt.wnameLow))
        {
            TC_LOG_ERROR("sql.sql","Wrong UTF8 name for id %u in `game_tele` table, ignoring.",id);
            continue;
        }

        wstrToLower( gt.wnameLow );

        m_GameTeleMap[id] = gt;

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u game tele's", count );
    
}

GameTele const* ObjectMgr::GetGameTele(const std::string& name) const
{
    // explicit name case
    std::wstring wname;
    if(!Utf8toWStr(name,wname))
        return nullptr;

    // converting string that we try to find to lower case
    wstrToLower( wname );

    // Alternative first GameTele what contains wnameLow as substring in case no GameTele location found
    const GameTele* alt = nullptr;
    for(const auto & itr : m_GameTeleMap)
    {
        if(itr.second.wnameLow == wname)
            return &itr.second;
        else if (alt == nullptr && itr.second.wnameLow.find(wname) != std::wstring::npos)
            alt = &itr.second;
    }

    return alt;
}

bool ObjectMgr::AddGameTele(GameTele& tele)
{
    // find max id
    uint32 new_id = 0;
    for(GameTeleContainer::const_iterator itr = m_GameTeleMap.begin(); itr != m_GameTeleMap.end(); ++itr)
        if(itr->first > new_id)
            new_id = itr->first;

    // use next
    ++new_id;

    if(!Utf8toWStr(tele.name,tele.wnameLow))
        return false;

    wstrToLower( tele.wnameLow );

    m_GameTeleMap[new_id] = tele;

    WorldDatabase.PExecute("INSERT INTO game_tele (id,position_x,position_y,position_z,orientation,map,name) VALUES (%u,%f,%f,%f,%f,%d,'%s')",
        new_id,tele.position_x,tele.position_y,tele.position_z,tele.orientation,tele.mapId,tele.name.c_str());
        
    return true;
}

bool ObjectMgr::DeleteGameTele(const std::string& name)
{
    // explicit name case
    std::wstring wname;
    if(!Utf8toWStr(name,wname))
        return false;

    // converting string that we try to find to lower case
    wstrToLower( wname );

    for(auto itr = m_GameTeleMap.begin(); itr != m_GameTeleMap.end(); ++itr)
    {
        if(itr->second.wnameLow == wname)
        {
            WorldDatabase.PExecute("DELETE FROM game_tele WHERE name = '%s'",itr->second.name.c_str());
            m_GameTeleMap.erase(itr);
            return true;
        }
    }

    return false;
}

void ObjectMgr::LoadTrainerSpell()
{
    // For reload case
    for (auto & itr : m_mCacheTrainerSpellMap)
        itr.second.Clear();

    m_mCacheTrainerSpellMap.clear();

    QueryResult result = WorldDatabase.Query("SELECT entry, spell,spellcost,reqskill,reqskillvalue,reqlevel FROM npc_trainer");

    if( !result )
    {
        TC_LOG_ERROR("server.loading",">> Loaded `npc_trainer`, table is empty!");
        
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry  = fields[0].GetUInt32();
        uint32 spell  = fields[1].GetUInt32();

        CreatureTemplate const* cInfo = GetCreatureTemplate(entry);
        if(!cInfo)
        {
            TC_LOG_ERROR("sql.sql","Table `npc_trainer` have entry for not existed creature template (Entry: %u), ignore", entry);
            continue;
        }

        if(!(cInfo->npcflag & UNIT_NPC_FLAG_TRAINER))
        {
            TC_LOG_ERROR("sql.sql","Table `npc_trainer` have data for not creature template (Entry: %u) without trainer flag, ignore", entry);
            continue;
        }

        SpellInfo const *spellinfo = sSpellMgr->GetSpellInfo(spell);
        if(!spellinfo)
        {
            TC_LOG_ERROR("sql.sql","Table `npc_trainer` for Trainer (Entry: %u ) has non existing spell %u, ignore", entry,spell);
            continue;
        }

        if(!SpellMgr::IsSpellValid(spellinfo))
        {
            TC_LOG_ERROR("sql.sql","Table `npc_trainer` for Trainer (Entry: %u) has broken learning spell %u, ignore", entry, spell);
            continue;
        }

        auto  pTrainerSpell = new TrainerSpell();
        pTrainerSpell->spell         = spell;
        pTrainerSpell->spellcost     = fields[2].GetUInt32();
        pTrainerSpell->reqskill      = fields[3].GetUInt16();
        pTrainerSpell->reqskillvalue = fields[4].GetUInt16();
        pTrainerSpell->reqlevel      = fields[5].GetUInt8();

        if(!pTrainerSpell->reqlevel)
            pTrainerSpell->reqlevel = spellinfo->SpellLevel;

        TrainerSpellData& data = m_mCacheTrainerSpellMap[entry];

        if(SpellMgr::IsProfessionSpell(spell))
            data.trainerType = 2;

        data.spellList.push_back(pTrainerSpell);
        ++count;

    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded Trainers %d", count );
    
}

void ObjectMgr::LoadVendors()
{
    // For reload case
    for (auto & itr : m_mCacheVendorItemMap)
        itr.second.Clear();

    m_mCacheVendorItemMap.clear();

    std::set<uint32> skip_vendors;

    QueryResult result = WorldDatabase.PQuery("SELECT entry, item, maxcount, incrtime, ExtendedCost "
                                              "FROM npc_vendor t1 WHERE ((%u >= patch_min) && (%u <= patch_max))", sWorld->GetWowPatch(), sWorld->GetWowPatch());

    if( !result )
    {
        TC_LOG_ERROR("server.loading",">> Loaded `npc_vendor`, table is empty!");
        
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint32 entry        = fields[0].GetUInt32();
        uint32 item_id      = fields[1].GetUInt32();
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(item_id);
        if(!proto)
        {
            TC_LOG_ERROR("sql.sql","Table `npc_vendor` for Vendor (Entry: %u) have in item list non-existed item (%u), ignore",entry,item_id);
            continue;
        }
        uint32 maxcount     = fields[2].GetUInt8();
        uint32 incrtime     = fields[3].GetUInt32();
        uint32 ExtendedCost = fields[4].GetUInt32();

        if(!IsVendorItemValid(entry,proto,maxcount,incrtime,ExtendedCost,nullptr,&skip_vendors))
            continue;

        VendorItemData& vList = m_mCacheVendorItemMap[entry];

        vList.AddItem(item_id, maxcount, incrtime, ExtendedCost);
        ++count;

    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %d Vendors ", count );
    
}

void ObjectMgr::LoadCreatureGossip()
{
    m_mCacheNpcMenuIdMap.clear();

    QueryResult result = WorldDatabase.Query("SELECT npc_guid, menu_id FROM creature_gossip");
    if( !result )
    {
        TC_LOG_ERROR("server.loading",">> Loaded `creature_gossip`, table is empty!");
        
        return;
    }

    uint32 count = 0;
    ObjectGuid::LowType guid;
    uint32 menuid;
    do
    {
        Field* fields = result->Fetch();

        guid   = fields[0].GetUInt32();
        menuid = fields[1].GetUInt32();

        if (!GetCreatureData(guid))
        {
            TC_LOG_ERROR("sql.sql","Table `creature_gossip` have not existed creature (SpawnId: %u) entry, ignore. ",guid);
            continue;
        }

        GossipMenusMapBounds bounds = sObjectMgr->GetGossipMenusMapBounds(menuid);
        /// if there are none.
        if (bounds.first == bounds.second)
        {
            TC_LOG_ERROR("sql.sql","Table `creature_gossip` for creature (SpawnId: %u) have wrong menuid (%u), ignore. ", guid, menuid);
            continue;
        }

        m_mCacheNpcMenuIdMap[guid] = menuid ;
        ++count;

    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %d creature gossips", count );
    
}

void ObjectMgr::LoadGossipMenu()
{
    uint32 oldMSTime = GetMSTime();

    _gossipMenusStore.clear();

    QueryResult result = WorldDatabase.Query("SELECT entry, text_id FROM gossip_menu");

    if (!result)
    {
        TC_LOG_ERROR("server.loading", ">> Loaded 0  gossip_menu entries. DB table `gossip_menu` is empty!");
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        GossipMenus gMenu;

        gMenu.entry             = fields[0].GetUInt16();
        gMenu.text_id           = fields[1].GetUInt32();

        if (!GetGossipText(gMenu.text_id))
        {
            TC_LOG_ERROR("sql.sql", "Table gossip_menu entry %u are using non-existing text_id %u", gMenu.entry, gMenu.text_id);
            continue;
        }

        _gossipMenusStore.insert(GossipMenusContainer::value_type(gMenu.entry, gMenu));

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u gossip_menu entries in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadGossipMenuItems()
{
    uint32 oldMSTime = GetMSTime();

    _gossipMenuItemsStore.clear(); //for reload case

    QueryResult result = WorldDatabase.Query(
        //      0        1   2            3            4                      5          6                   7               8              9          10         11        12
        "SELECT menu_id, id, option_icon, option_text, OptionBroadcastTextID, option_id, npc_option_npcflag, action_menu_id, action_poi_id, box_coded, box_money, box_text, BoxBroadcastTextID "
        "FROM gossip_menu_option ORDER BY menu_id, id");

    if (result)
    {
        uint32 count = 0;

        do
        {
            Field* fields = result->Fetch();

            GossipMenuItems gMenuItem;

            gMenuItem.MenuId = fields[0].GetUInt16();
            gMenuItem.OptionIndex = fields[1].GetUInt16();
            gMenuItem.OptionIcon = fields[2].GetUInt32();
            gMenuItem.OptionText = fields[3].GetString();
            gMenuItem.OptionBroadcastTextId = fields[4].GetUInt32();
            gMenuItem.OptionType = fields[5].GetUInt8();
            gMenuItem.OptionNpcflag = fields[6].GetUInt32();
            gMenuItem.ActionMenuId = fields[7].GetUInt16();
            gMenuItem.ActionPoiId = fields[8].GetUInt32();
            gMenuItem.BoxCoded = fields[9].GetBool();
            gMenuItem.BoxMoney = fields[10].GetUInt32();
            gMenuItem.BoxText = fields[11].GetString();
            gMenuItem.BoxBroadcastTextId = fields[12].GetUInt32();

            if (gMenuItem.MenuId == 0)
            {
                TC_LOG_ERROR("sql.sql", "Table `gossip_menu_option` has invalid menuId 0, skipping.");
                continue;
            }

            if (gMenuItem.OptionIcon >= GOSSIP_ICON_MAX)
            {
                TC_LOG_ERROR("sql.sql", "Table `gossip_menu_option` for menu %u, id %u has unknown icon id %u. Replacing with GOSSIP_ICON_CHAT", gMenuItem.MenuId, gMenuItem.OptionIndex, gMenuItem.OptionIcon);
                gMenuItem.OptionIcon = GOSSIP_ICON_CHAT;
            }

            if (gMenuItem.OptionBroadcastTextId)
            {
                if (!GetBroadcastText(gMenuItem.OptionBroadcastTextId))
                {
                    TC_LOG_ERROR("sql.sql", "Table `gossip_menu_option` for menu %u, id %u has non-existing or incompatible OptionBroadcastTextId %u, ignoring.", gMenuItem.MenuId, gMenuItem.OptionIndex, gMenuItem.OptionBroadcastTextId);
                    gMenuItem.OptionBroadcastTextId = 0;
                }
            }

            if (gMenuItem.OptionType >= GOSSIP_OPTION_MAX)
            {
                TC_LOG_ERROR("sql.sql", "Table `gossip_menu_option` for menu %u, id %u has unknown option id %u. Option will not be used", gMenuItem.MenuId, gMenuItem.OptionIndex, gMenuItem.OptionType);
                continue;
            }

            if (gMenuItem.ActionPoiId && !GetPointOfInterest(gMenuItem.ActionPoiId))
            {
                TC_LOG_ERROR("sql.sql", "Table `gossip_menu_option` for menu %u, id %u use non-existing action_poi_id %u, ignoring", gMenuItem.MenuId, gMenuItem.OptionIndex, gMenuItem.ActionPoiId);
                gMenuItem.ActionPoiId = 0;
            }

            if (gMenuItem.BoxBroadcastTextId)
            {
                if (!GetBroadcastText(gMenuItem.BoxBroadcastTextId))
                {
                    TC_LOG_ERROR("sql.sql", "Table `gossip_menu_option` for menu %u, id %u has non-existing or incompatible BoxBroadcastTextId %u, ignoring.", gMenuItem.MenuId, gMenuItem.OptionIndex, gMenuItem.BoxBroadcastTextId);
                    gMenuItem.BoxBroadcastTextId = 0;
                }
            }

            _gossipMenuItemsStore.insert(GossipMenuItemsContainer::value_type(gMenuItem.MenuId, gMenuItem));
            ++count;
        } while (result->NextRow());

        TC_LOG_INFO("server.loading", ">> Loaded %u gossip_menu_option entries in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    }
    else {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 gossip_menu_option entries. DB table `gossip_menu_option` is empty!");
    }

    oldMSTime = GetMSTime();

    result = WorldDatabase.Query(
        //      0                      2                          3
        "SELECT id, option_icon, option_text, option_id, npc_option_npcflag "
        "FROM gossip_menu_option_generic ORDER BY id");

    if (result)
    {
        uint32 count = 0;

        do
        {
            Field* fields = result->Fetch();

            GossipMenuItems gMenuItem;

            gMenuItem.MenuId = GENERIC_OPTIONS_MENU;
            gMenuItem.OptionIndex = fields[0].GetUInt16();
            gMenuItem.OptionIcon = fields[1].GetUInt32();
            gMenuItem.OptionText = fields[2].GetString();
            gMenuItem.OptionType = fields[3].GetUInt8();
            gMenuItem.OptionNpcflag = fields[4].GetUInt32();

            if (gMenuItem.OptionIcon >= GOSSIP_ICON_MAX)
            {
                TC_LOG_ERROR("sql.sql", "Table `gossip_menu_option_generic` for menu %u, id %u has unknown icon id %u. Replacing with GOSSIP_ICON_CHAT", gMenuItem.MenuId, gMenuItem.OptionIndex, gMenuItem.OptionIcon);
                gMenuItem.OptionIcon = GOSSIP_ICON_CHAT;
            }

            if (gMenuItem.OptionType >= GOSSIP_OPTION_MAX)
            {
                TC_LOG_ERROR("sql.sql", "Table `gossip_menu_option_generic` for menu %u, id %u has unknown option id %u. Option will not be used", gMenuItem.MenuId, gMenuItem.OptionIndex, gMenuItem.OptionType);
                continue;
            }

            _gossipMenuItemsStore.insert(GossipMenuItemsContainer::value_type(GENERIC_OPTIONS_MENU, gMenuItem));
            ++count;
        } while (result->NextRow());

        TC_LOG_INFO("server.loading", ">> Loaded %u gossip_menu_option_generic entries in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
    }
    else {
        TC_LOG_ERROR("server.loading", ">> Loaded 0 gossip_menu_option_generic entries. DB table `gossip_menu_option_generic` is empty!");
    }
}

void ObjectMgr::AddVendorItem( uint32 entry, ItemTemplate const *proto, uint32 maxcount, uint32 incrtime, uint32 extendedcost, bool savetodb)
{
    VendorItemData& vList = m_mCacheVendorItemMap[entry];
    vList.AddItem(proto->ItemId, maxcount, incrtime, extendedcost);

    if(savetodb) 
        WorldDatabase.PExecute("INSERT INTO npc_vendor (entry,item,maxcount,incrtime,extendedcost) VALUES('%u','%u','%u','%u','%u')", entry, proto->ItemId, maxcount,incrtime,extendedcost);
}

bool ObjectMgr::RemoveVendorItem( uint32 entry, ItemTemplate const *proto, bool savetodb)
{
    auto  iter = m_mCacheVendorItemMap.find(entry);
    if(iter == m_mCacheVendorItemMap.end())
        return false;

    if(!iter->second.FindItem(proto->ItemId))
        return false;

    iter->second.RemoveItem(proto->ItemId);
    if(savetodb) WorldDatabase.PExecute("DELETE FROM npc_vendor WHERE entry='%u' AND item='%u'",entry, proto->ItemId);
    return true;
}

bool ObjectMgr::IsVendorItemValid( uint32 vendor_entry, ItemTemplate const *proto, uint32 maxcount, uint32 incrtime, uint32 ExtendedCost, Player* pl, std::set<uint32>* skip_vendors, uint32 ORnpcflag ) const
{
    CreatureTemplate const* cInfo = sObjectMgr->GetCreatureTemplate(vendor_entry);
    if(!cInfo)
    {
        if(pl)
            ChatHandler(pl).SendSysMessage(LANG_COMMAND_VENDORSELECTION);
        else
            TC_LOG_ERROR("sql.sql","Table `(game_event_)npc_vendor` have data for not existed creature template (Entry: %u), ignore", vendor_entry);
        return false;
    }

    if(!((cInfo->npcflag | ORnpcflag) & UNIT_NPC_FLAG_VENDOR))
    {
        if(!skip_vendors || skip_vendors->count(vendor_entry)==0)
        {
            if(pl)
                ChatHandler(pl).SendSysMessage(LANG_COMMAND_VENDORSELECTION);
            else
                TC_LOG_ERROR("sql.sql","Table `(game_event_)npc_vendor` have data for not creature template (Entry: %u) without vendor flag, ignore", vendor_entry);

            if(skip_vendors)
                skip_vendors->insert(vendor_entry);
        }
        return false;
    }

    if(ExtendedCost && !GetItemExtendedCost(ExtendedCost))
    {
        if(pl)
            ChatHandler(pl).PSendSysMessage(LANG_EXTENDED_COST_NOT_EXIST,ExtendedCost);
        else
            TC_LOG_ERROR("sql.sql","Table `(game_event_)npc_vendor` have Item (Entry: %u) with wrong ExtendedCost (%u) for vendor (%u), ignore",proto->ItemId,ExtendedCost,vendor_entry);
        return false;
    }

    if(maxcount > 0 && incrtime == 0)
    {
        if(pl)
            ChatHandler(pl).PSendSysMessage("MaxCount!=0 (%u) but IncrTime==0", maxcount);
        else
            TC_LOG_ERROR( "sql.sql","Table `(game_event_)npc_vendor` has `maxcount` (%u) for item %u of vendor (Entry: %u) but `incrtime`=0, ignore", maxcount, proto->ItemId, vendor_entry);
        return false;
    }
    else if(maxcount==0 && incrtime > 0)
    {
        if(pl)
            ChatHandler(pl).PSendSysMessage("MaxCount==0 but IncrTime<>=0");
        else
            TC_LOG_ERROR( "FIXME","Table `(game_event_)npc_vendor` has `maxcount`=0 for item %u of vendor (Entry: %u) but `incrtime`<>0, ignore", proto->ItemId, vendor_entry);
        return false;
    }

    VendorItemData const* vItems = GetNpcVendorItemList(vendor_entry);
    if(!vItems)
        return true;                                        // later checks for non-empty lists

    if(vItems->FindItem(proto->ItemId))
    {
        if(pl)
            ChatHandler(pl).PSendSysMessage(LANG_ITEM_ALREADY_IN_LIST,proto->ItemId);
        else
            TC_LOG_ERROR("sql.sql", "Table `(game_event_)npc_vendor` has duplicate items %u for vendor (Entry: %u), ignore", proto->ItemId, vendor_entry);
        return false;
    }

    if(vItems->GetItemCount() >= MAX_VENDOR_ITEMS)
    {
        if(pl)
            ChatHandler(pl).SendSysMessage(LANG_COMMAND_ADDVENDORITEMITEMS);
        else
            TC_LOG_ERROR("sql.sql", "Table `npc_vendor` has too many items (%u >= %i) for vendor (Entry: %u), ignore", vItems->GetItemCount(), MAX_VENDOR_ITEMS, vendor_entry);
        return false;
    }

    return true;
}

bool ObjectMgr::AddTrainerSpell(uint32 creatureId, TrainerSpell const& spell)
{
    TrainerSpellData& data = m_mCacheTrainerSpellMap[creatureId];

    if (SpellMgr::IsProfessionSpell(spell.spell))
        data.trainerType = 2;

    auto  new_spell = new TrainerSpell(spell);
    data.spellList.push_back(new_spell);
    return true;
}

bool ObjectMgr::RemoveTrainerSpell(uint32 creatureId, uint32 spellId)
{
    bool removed = false;
    TrainerSpellData& data = m_mCacheTrainerSpellMap[creatureId];
    for (auto itr = data.spellList.begin(); itr != data.spellList.end();)
    {
        if ((*itr)->spell == spellId)
        {
            itr = data.spellList.erase(itr);
            removed = true;
            continue;
        }
        itr++;
    }
    return removed;
}

void ObjectMgr::LoadScriptNames()
{
    // We insert an empty placeholder here so we can use the
    // script id 0 as dummy for "no script found".
    _scriptNamesStore.emplace_back("");

    QueryResult result = WorldDatabase.Query(
      "SELECT DISTINCT(ScriptName) FROM creature_template WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM creature WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM gameobject_template WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM gameobject WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM item_template WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM areatrigger_scripts WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM spell_script_names WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM conditions WHERE ScriptName <> '' " 
      "UNION "
      "SELECT DISTINCT(ScriptName) FROM outdoorpvp_template WHERE ScriptName <> '' "
      "UNION "
      "SELECT DISTINCT(script) FROM instance_template WHERE script <> ''");
    
    //OnEvent Changes
    _scriptNamesStore.push_back("scripted_on_events"); 
    
    if(result)
    {
        _scriptNamesStore.reserve(result->GetRowCount() + 1);

        do
        {
            _scriptNamesStore.push_back((*result)[0].GetString());
        } while (result->NextRow());
    }

    std::sort(_scriptNamesStore.begin(), _scriptNamesStore.end());
}

ObjectMgr::ScriptNameContainer const& ObjectMgr::GetAllScriptNames() const
{
    return _scriptNamesStore;
}

std::string const& ObjectMgr::GetScriptName(uint32 id) const
{
    static std::string const empty = "";
    return (id < _scriptNamesStore.size()) ? _scriptNamesStore[id] : empty;
}

uint32 ObjectMgr::GetScriptId(std::string const& name)
{
    // use binary search to find the script name in the sorted vector
    // assume "" is the first element
    if (name.empty())
        return 0;

    ScriptNameContainer::const_iterator itr = std::lower_bound(_scriptNamesStore.begin(), _scriptNamesStore.end(), name);
    if (itr == _scriptNamesStore.end() || *itr != name) 
        return 0;

    return uint32(itr - _scriptNamesStore.begin());
}

// Functions for scripting access
uint32 GetAreaTriggerScriptId(uint32 trigger_id)
{
    return sObjectMgr->GetAreaTriggerScriptId(trigger_id);
}

bool LoadTrinityStrings(DatabaseWorkerPool<WorldDatabaseConnection>& db, char const* table,int32 start_value, int32 end_value)
{
    if(start_value >= 0 || start_value <= end_value)        // start/end reversed for negative values
    {
        TC_LOG_ERROR("server.loading","Table '%s' attempt loaded with invalid range (%d - %d), use (%d - %d) instead.",table,start_value,end_value,-1,std::numeric_limits<int32>::min());
        start_value = -1;
        end_value = std::numeric_limits<int32>::min();
    }

    // for scripting localized strings allowed use _only_ negative entries
    return sObjectMgr->LoadTrinityStrings(db,table,end_value,start_value);
}

uint64 ObjectMgr::GenerateGMTicketId()
{
  return ++_GMticketid;
}

void ObjectMgr::LoadGMTickets()
{
  m_GMTicketList.clear();

  QueryResult result = CharacterDatabase.Query( "SELECT `guid`, `playerGuid`, `message`, `createtime`, `map`, `posX`, `posY`, `posZ`, `timestamp`, `closed`, `assignedto`, `comment` FROM `gm_tickets`" );

  if(!result)
  {
    TC_LOG_INFO("server.loading"," \n>> GM Tickets table is empty, no tickets were loaded.\n" );
    
    return;
  }

  uint16 count = 0;
  GM_Ticket *ticket;
  do
  {
    Field *fields = result->Fetch();
    ticket = new GM_Ticket;
    ticket->guid = fields[0].GetUInt32();
    ticket->playerGuid = ObjectGuid(HighGuid::Player, fields[1].GetUInt32());
    ticket->message = fields[2].GetString();
    ticket->createtime = fields[3].GetUInt32();
    ticket->map = fields[4].GetUInt32();
    ticket->pos_x = fields[5].GetFloat();
    ticket->pos_y = fields[6].GetFloat();
    ticket->pos_z = fields[7].GetFloat();
    ticket->timestamp = fields[8].GetUInt32();
    ticket->closed = fields[9].GetUInt32();
    ticket->assignedToGM = ObjectGuid(HighGuid::Player, fields[10].GetUInt32());
    ticket->comment = fields[11].GetString();
    ++count;

    m_GMTicketList.push_back(ticket);

  } while( result->NextRow() );

  result = CharacterDatabase.PQuery("SELECT MAX(`guid`) from `gm_tickets`");
  _GMticketid = (*result)[0].GetUInt64(); 

  TC_LOG_INFO("server.loading",">>> %u GM Tickets loaded from the database.", count);
  
}

GM_Ticket* ObjectMgr::GetGMTicket(uint64 ticketGuid)
{
    for(GmTicketList::const_iterator i = m_GMTicketList.begin(); i != m_GMTicketList.end(); ++i)
        if((*i) && (*i)->guid == ticketGuid) 
            return (*i);

    return nullptr;
}

GM_Ticket* ObjectMgr::GetGMTicketByPlayer(ObjectGuid playerGuid)
{
    for(GmTicketList::const_iterator i = m_GMTicketList.begin(); i != m_GMTicketList.end(); ++i)
        if((*i) && (*i)->playerGuid == playerGuid && (*i)->closed == 0) 
            return (*i);

    return nullptr;        
}

void ObjectMgr::AddOrUpdateGMTicket(GM_Ticket &ticket, bool create)
{
  if(create)
    m_GMTicketList.push_back(&ticket);

  _AddOrUpdateGMTicket(ticket);    
}

void ObjectMgr::_AddOrUpdateGMTicket(GM_Ticket &ticket)
{
    std::string msg(ticket.message);
    std::string comment(ticket.comment); 
    CharacterDatabase.EscapeString(msg);
    CharacterDatabase.EscapeString(comment);
    std::ostringstream ss;
    ss << "REPLACE INTO `gm_tickets` (`guid`, `playerGuid`, `message`, `createtime`, `map`, `posX`, `posY`, `posZ`, `timestamp`, `closed`, `assignedto`, `comment`) VALUES('";
    ss << ticket.guid << "', '";
    ss << ticket.playerGuid << "', '";
    ss << msg << "', '" ;
    ss << ticket.createtime << "', '";
    ss << ticket.map << "', '";
    ss << ticket.pos_x << "', '";
    ss << ticket.pos_y << "', '";
    ss << ticket.pos_z << "', '";
    ss << ticket.timestamp << "', '";
    ss << ticket.closed << "', '";
    ss << ticket.assignedToGM << "', '";
    ss << comment << "');";

    CharacterDatabase.Execute(ss.str().c_str());
}

void ObjectMgr::RemoveGMTicket(GM_Ticket *ticket, int64 source, bool permanently)
{
  for(auto i = m_GMTicketList.begin(); i != m_GMTicketList.end(); ++i)
    if((*i)->guid == ticket->guid) 
    {
      if(permanently)
      {
        CharacterDatabase.PExecute("DELETE FROM `gm_tickets` WHERE `guid` = '%u'", ticket->guid);
        i = m_GMTicketList.erase(i);
        ticket = nullptr;
        return;
      }
      (*i)->closed = source;
      _AddOrUpdateGMTicket(*(*i)); 
    }
}

void ObjectMgr::RemoveGMTicket(uint64 ticketGuid, int64 source, bool permanently)
{
  GM_Ticket *ticket = GetGMTicket(ticketGuid);
  assert( ticket );
  RemoveGMTicket(ticket, source, permanently);
}

void ObjectMgr::LoadItemExtendedCost()
{
    sItemExtendedCostStore.clear();

    uint32 count = 0;

    //        0         1              2                      4                   6                     8                             10                            12               13
    //SELECT id, reqhonorpoints, reqarenapoints, reqitem0, reqitem1, reqitem2, reqitem3, reqitem4, reqitemcount0, reqitemcount1, reqitemcount2, reqitemcount3, reqitemcount4, reqpersonalarenarating FROM item_extended_cost
    PreparedStatement* stmt = WorldDatabase.GetPreparedStatement(WORLD_SEL_ITEM_EXTENDED_COST);
    if(PreparedQueryResult result = WorldDatabase.Query(stmt))
    {
        do
        {
            Field* fields = result->Fetch();

            uint32 fieldCount = 0;
            uint32 id = fields[fieldCount++].GetUInt32();

            ItemExtendedCostEntry entry;
            entry.reqhonorpoints = fields[fieldCount++].GetUInt32();
            entry.reqarenapoints = fields[fieldCount++].GetUInt32();
            entry.reqitem[0] = fields[fieldCount++].GetUInt32();
            entry.reqitem[1] = fields[fieldCount++].GetUInt32();
            entry.reqitem[2] = fields[fieldCount++].GetUInt32();
            entry.reqitem[3] = fields[fieldCount++].GetUInt32();
            entry.reqitem[4] = fields[fieldCount++].GetUInt32();
            entry.reqitemcount[0] = fields[fieldCount++].GetUInt32();
            entry.reqitemcount[1] = fields[fieldCount++].GetUInt32();
            entry.reqitemcount[2] = fields[fieldCount++].GetUInt32();
            entry.reqitemcount[3] = fields[fieldCount++].GetUInt32();
            entry.reqitemcount[4] = fields[fieldCount++].GetUInt32();
            entry.reqpersonalarenarating = fields[fieldCount++].GetUInt32();

            sItemExtendedCostStore[id] = std::move(entry);
            count++;
        } while (result->NextRow());
    }
    
    if(GetItemExtendedCost(2425) == nullptr)
    {
        TC_LOG_ERROR("server.loading","item_extended_cost is empty or missing some entries, cannot continue");
        std::this_thread::sleep_for(std::chrono::seconds(10));
        std::abort();
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u item extended cost", count );
}

bool ValidateSpell(SpellEntry const* entry)
{
    //special validation rules for override spells
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; i++)
        if (entry->Effect[i] >= TOTAL_SPELL_EFFECTS)
        {
            TC_LOG_ERROR("sql.sql", " Spell %u has invalid effect %u at index %u", entry->Id, entry->Effect[i], i);
            return false;
        }

    return true;
}

//This does not erase existing spells and keep pointers valids
void ObjectMgr::LoadSpellTemplates()
{
    uint32 count = 0;
    uint32 count_overrides = 0;
    uint32 id;
    Field* fields;
    
    std::string select_fields_str = "SELECT entry, category, dispel, mechanic, attributes, attributesEx, attributesEx2, attributesEx3, attributesEx4, attributesEx5, attributesEx6, "
        "stances, stancesNot, targets, targetCreatureType, requiresSpellFocus, facingCasterFlags, casterAuraState, targetAuraState, casterAuraStateNot, targetAuraStateNot, castingTimeIndex, recoveryTime, "
        "categoryRecoveryTime, interruptFlags, auraInterruptFlags, channelInterruptFlags, ProcFlags, procChance, procCharges, MaxLevel, BaseLevel, SpellLevel, durationIndex, powerType, ManaCost, "
        "ManaCostPerlevel, manaPerSecond, ManaPerSecondPerLevel, rangeIndex, speed, stackAmount, totem1, totem2, reagent1, reagent2, reagent3, reagent4, reagent5, reagent6, reagent7, "
        "reagent8, reagentCount1, reagentCount2, reagentCount3, reagentCount4, reagentCount5, reagentCount6, reagentCount7, reagentCount8, equippedItemClass, equippedItemSubClassMask, "
        "equippedItemInventoryTypeMask, effect1, effect2, effect3, effectDieSides1, effectDieSides2, effectDieSides3, effectBaseDice1, effectBaseDice2, effectBaseDice3, effectDicePerLevel1, "
        "effectDicePerLevel2, effectDicePerLevel3, effectRealPointsPerLevel1, effectRealPointsPerLevel2, effectRealPointsPerLevel3, effectBasePoints1, effectBasePoints2, effectBasePoints3, effectMechanic1, "
        "effectMechanic2, effectMechanic3, effectImplicitTargetA1, effectImplicitTargetA2, effectImplicitTargetA3, effectImplicitTargetB1, effectImplicitTargetB2, effectImplicitTargetB3, effectRadiusIndex1, "
        "effectRadiusIndex2, effectRadiusIndex3, effectApplyAuraName1, effectApplyAuraName2, effectApplyAuraName3, effectAmplitude1, effectAmplitude2, effectAmplitude3, effectMultipleValue1, "
        "effectMultipleValue2, effectMultipleValue3, effectChainTarget1, effectChainTarget2, effectChainTarget3, effectItemType1, effectItemType2, effectItemType3, effectMiscValue1, effectMiscValue2, "
        "effectMiscValue3, effectMiscValueB1, effectMiscValueB2, effectMiscValueB3, effectTriggerSpell1, effectTriggerSpell2, effectTriggerSpell3, effectPointsPerComboPoint1, effectPointsPerComboPoint2, "
        "effectPointsPerComboPoint3, spellVisual, spellIconID, activeIconID, priority, spellName1, spellName2, spellName3, spellName4, spellName5, spellName6, spellName7, spellName8, "
        "spellName9, spellName10, spellName11, spellName12, spellName13, spellName14, spellName15, spellName16, rank1, rank2, rank3, rank4, rank5, rank6, rank7, rank8, rank9, rank10, "
        "rank11, rank12, rank13, rank14, rank15, rank16, ManaCostPercentage, startRecoveryCategory, startRecoveryTime, "
        "maxTargetLevel, spellFamilyName, spellFamilyFlags, maxAffectedTargets, dmgClass, preventionType, dmgMultiplier1, dmgMultiplier2, dmgMultiplier3, "
        "totemCategory1, totemCategory2, areaId, schoolMask";

    std::string request = select_fields_str + std::string(" FROM spell_template ORDER BY entry");
    std::string request_override = select_fields_str + std::string(", customAttributesFlags FROM spell_template_override ORDER BY entry");
    QueryResult result = WorldDatabase.Query(request.c_str());
    QueryResult result_override = WorldDatabase.Query(request_override.c_str());
    if (!result) 
    {
        TC_LOG_ERROR("server.loading", "Table spell_template loading failed");
        ABORT();
    }

    if (!result_override) 
    {
        TC_LOG_INFO("server.loading", "Table spell_template_override loading faield");
        ABORT();
    }

    do 
    {
        fields = result->Fetch();
        id = fields[0].GetUInt32();        
        auto itr = spellTemplates.find(id);
        SpellEntry* spell = nullptr;
        if (itr != spellTemplates.end()) { // Already existing
            spell = itr->second;
        }
        else {
            spell = new SpellEntry();
        }

        spell->Id = fields[0].GetUInt32();
        spell->Category = fields[1].GetUInt32();
        spell->Dispel = fields[2].GetUInt32();
        spell->Mechanic = Mechanics(fields[3].GetUInt32());
        spell->Attributes = fields[4].GetUInt32();
        spell->AttributesEx = fields[5].GetUInt32();
        spell->AttributesEx2 = fields[6].GetUInt32();
        spell->AttributesEx3 = fields[7].GetUInt32();
        spell->AttributesEx4 = fields[8].GetUInt32();
        spell->AttributesEx5 = fields[9].GetUInt32();
        spell->AttributesEx6 = fields[10].GetUInt32();
        spell->Stances = fields[11].GetUInt32();
        spell->StancesNot = fields[12].GetUInt32();
        spell->Targets = fields[13].GetUInt32();
        spell->TargetCreatureType = fields[14].GetUInt32();
        spell->RequiresSpellFocus = fields[15].GetUInt32();
        spell->FacingCasterFlags = fields[16].GetUInt32();
        spell->CasterAuraState = AuraStateType(fields[17].GetUInt32());
        spell->TargetAuraState = AuraStateType(fields[18].GetUInt32());
        spell->CasterAuraStateNot = AuraStateType(fields[19].GetUInt32());
        spell->TargetAuraStateNot = AuraStateType(fields[20].GetUInt32());
        spell->CastingTimeIndex = fields[21].GetUInt32();
        spell->RecoveryTime = fields[22].GetUInt32();
        spell->CategoryRecoveryTime = fields[23].GetUInt32();
        spell->InterruptFlags = fields[24].GetUInt32();
        spell->AuraInterruptFlags = fields[25].GetUInt32();
        spell->ChannelInterruptFlags = fields[26].GetUInt32();
        spell->ProcFlags = fields[27].GetUInt32();
        spell->procChance = fields[28].GetUInt32();
        spell->procCharges = fields[29].GetUInt32();
        spell->MaxLevel = fields[30].GetUInt32();
        spell->BaseLevel = fields[31].GetUInt32();
        spell->SpellLevel = fields[32].GetUInt32();
        spell->DurationIndex = fields[33].GetUInt32();
        spell->PowerType = fields[34].GetUInt32();
        spell->ManaCost = fields[35].GetUInt32();
        spell->ManaCostPerlevel = fields[36].GetUInt32();
        spell->manaPerSecond = fields[37].GetUInt32();
        spell->ManaPerSecondPerLevel = fields[38].GetUInt32();
        spell->rangeIndex = fields[39].GetUInt32();
        spell->speed = fields[40].GetFloat();
        spell->StackAmount = fields[41].GetUInt32();
        spell->Totem[0] = fields[42].GetUInt32();
        spell->Totem[1] = fields[43].GetUInt32();
        for (uint8 i = 0; i < 8; i++) {
            spell->Reagent[i] = fields[i+44].GetInt32();
            spell->ReagentCount[i] = fields[i+52].GetUInt32();
        }
        spell->EquippedItemClass = fields[60].GetInt32();
        spell->EquippedItemSubClassMask = fields[61].GetInt32();
        spell->EquippedItemInventoryTypeMask = fields[62].GetInt32();
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; i++) {
            spell->Effect[i] = fields[i+63].GetUInt32();
            spell->EffectDieSides[i] = fields[i+66].GetInt32();
            spell->EffectBaseDice[i] = fields[i+69].GetInt32();
            spell->EffectDicePerLevel[i] = fields[i+72].GetFloat();
            spell->EffectRealPointsPerLevel[i] = fields[i+75].GetFloat();
            spell->EffectBasePoints[i] = fields[i+78].GetInt32();
            spell->EffectMechanic[i] = fields[i+81].GetUInt32();
            spell->EffectImplicitTargetA[i]= fields[i+84].GetUInt32();
            spell->EffectImplicitTargetB[i] = fields[i+87].GetUInt32();
            spell->EffectRadiusIndex[i] = fields[i+90].GetUInt32();
            spell->EffectApplyAuraName[i] = fields[i+93].GetUInt32();
            spell->EffectAmplitude[i] = fields[i+96].GetUInt32();
            spell->EffectValueMultiplier[i] = fields[i+99].GetFloat();
            spell->EffectChainTarget[i] = fields[i+102].GetUInt32();
            spell->EffectItemType[i] = fields[i+105].GetUInt32();
            spell->EffectMiscValue[i] = fields[i+108].GetInt32();
            spell->EffectMiscValueB[i] = fields[i+111].GetInt32();
            spell->EffectTriggerSpell[i] = fields[i+114].GetUInt32();
            spell->EffectPointsPerComboPoint[i] = fields[i+117].GetFloat();
        }
        spell->SpellVisual = fields[120].GetUInt32();
        spell->SpellIconID = fields[121].GetUInt32();
        spell->activeIconID = fields[122].GetUInt32();
        spell->spellPriority = fields[123].GetUInt32();
        for (uint8 i = 0; i < 16; i++) {
            strcpy(spell->SpellName[i], fields[i+124].GetCString());
            if (strcmp(spell->SpellName[i], "0.000000") == 0)
                strcpy(spell->SpellName[i], "");
            strcpy(spell->Rank[i], fields[i+140].GetCString());
            if (strcmp(spell->Rank[i], "0.000000") == 0)
                strcpy(spell->Rank[i], "");
        }
        spell->ManaCostPercentage = fields[156].GetUInt32();
        spell->StartRecoveryCategory = fields[157].GetUInt32();
        spell->StartRecoveryTime = fields[158].GetUInt32();
        spell->MaxTargetLevel = fields[159].GetUInt32();
        spell->SpellFamilyName = fields[160].GetUInt32();
        spell->SpellFamilyFlags = fields[161].GetUInt64();
        spell->MaxAffectedTargets = fields[162].GetUInt32();
        spell->DmgClass = fields[163].GetUInt32();
        spell->PreventionType = fields[164].GetUInt32();
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; i++)
            spell->EffectDamageMultiplier[i] = fields[i+165].GetFloat();
        spell->TotemCategory[0] = fields[168].GetUInt32();
        spell->TotemCategory[1] = fields[169].GetUInt32();
        spell->AreaId = fields[170].GetUInt32();
        spell->SchoolMask = fields[171].GetUInt32();
        spell->CustomAttributesFlags = 0; //only loaded in spell_template_override

        spellTemplates[id] = spell;
        maxSpellId = id;
        count++;
    } while (result->NextRow());

    do
    {
        fields = result_override->Fetch();
        ASSERT(result_override->GetFieldCount() == 173);
        id = fields[0].GetUInt32();
        auto itr = spellTemplates.find(id);
        SpellEntry* spell = nullptr;
        bool newSpell = false;
        if (itr != spellTemplates.end()) { // Already existing
            spell = itr->second;
        }
        else
        {
            //we allow new spell in overrides for serverside spells
            spell = new SpellEntry();
            spell->Id = id;
            newSpell = true;
        }

        for (uint32 i = 1; i < result_override->GetFieldCount(); i++)
        {
            if (fields[i].IsNull())
            {
                if (newSpell)
                {
                    TC_LOG_ERROR("sql.sql", "spell_template_override has new spell %u with some NULL fields. All fields should be filled for new spells. Skipping.", id);
                    delete spell;
                    spell = nullptr;
                    newSpell = false;
                    break;
                }

                continue;
            }

            switch (i)
            {
            case 1: spell->Category = fields[i].GetUInt32(); break;
            case 2: spell->Dispel = fields[i].GetUInt32(); break;
            case 3: spell->Mechanic = Mechanics(fields[i].GetUInt32()); break;
            case 4: spell->Attributes = fields[i].GetUInt32(); break;
            case 5: spell->AttributesEx = fields[i].GetUInt32(); break;
            case 6: spell->AttributesEx2 = fields[i].GetUInt32(); break;
            case 7: spell->AttributesEx3 = fields[i].GetUInt32(); break;
            case 8: spell->AttributesEx4 = fields[i].GetUInt32(); break;
            case 9: spell->AttributesEx5 = fields[i].GetUInt32(); break;
            case 10: spell->AttributesEx6 = fields[i].GetUInt32(); break;
            case 11: spell->Stances = fields[i].GetUInt32(); break;
            case 12: spell->StancesNot = fields[i].GetUInt32(); break;
            case 13: spell->Targets = fields[i].GetUInt32(); break;
            case 14: spell->TargetCreatureType = fields[i].GetUInt32(); break;
            case 15: spell->RequiresSpellFocus = fields[i].GetUInt32(); break;
            case 16: spell->FacingCasterFlags = fields[i].GetUInt32(); break;
            case 17: spell->CasterAuraState = AuraStateType(fields[i].GetUInt32()); break;
            case 18: spell->TargetAuraState = AuraStateType(fields[i].GetUInt32()); break;
            case 19: spell->CasterAuraStateNot = AuraStateType(fields[i].GetUInt32()); break;
            case 20: spell->TargetAuraStateNot = AuraStateType(fields[i].GetUInt32()); break;
            case 21: spell->CastingTimeIndex = fields[i].GetUInt32(); break;
            case 22: spell->RecoveryTime = fields[i].GetUInt32(); break;
            case 23: spell->CategoryRecoveryTime = fields[i].GetUInt32(); break;
            case 24: spell->InterruptFlags = fields[i].GetUInt32(); break;
            case 25: spell->AuraInterruptFlags = fields[i].GetUInt32(); break;
            case 26: spell->ChannelInterruptFlags = fields[i].GetUInt32(); break;
            case 27: spell->ProcFlags = fields[i].GetUInt32(); break;
            case 28: spell->procChance = fields[i].GetUInt32(); break;
            case 29: spell->procCharges = fields[i].GetUInt32(); break;
            case 30: spell->MaxLevel = fields[i].GetUInt32(); break;
            case 31: spell->BaseLevel = fields[i].GetUInt32(); break;
            case 32: spell->SpellLevel = fields[i].GetUInt32(); break;
            case 33: spell->DurationIndex = fields[i].GetUInt32(); break;
            case 34: spell->PowerType = fields[i].GetUInt32(); break;
            case 35: spell->ManaCost = fields[i].GetUInt32(); break;
            case 36: spell->ManaCostPerlevel = fields[i].GetUInt32(); break;
            case 37: spell->manaPerSecond = fields[i].GetUInt32(); break;
            case 38: spell->ManaPerSecondPerLevel = fields[i].GetUInt32(); break;
            case 39: spell->rangeIndex = fields[i].GetUInt32(); break;
            case 40: spell->speed = fields[i].GetFloat(); break;
            case 41: spell->StackAmount = fields[i].GetUInt32(); break;
            case 42: spell->Totem[0] = fields[i].GetUInt32(); break;
            case 43: spell->Totem[1] = fields[i].GetUInt32(); break;
            case 44: //Reagent start
            case 45:
            case 46:
            case 47:
            case 48:
            case 49:
            case 50:
            case 51:
                spell->Reagent[i - 44] = fields[i].GetInt32(); break;
            case 52: //ReagentCount start
            case 53:
            case 54:
            case 55:
            case 56:
            case 57:
            case 58:
            case 59:
                spell->ReagentCount[i - 52] = fields[i].GetUInt32(); break;
            case 60: spell->EquippedItemClass = fields[i].GetInt32(); break;
            case 61: spell->EquippedItemSubClassMask = fields[i].GetInt32(); break;
            case 62: spell->EquippedItemInventoryTypeMask = fields[i].GetInt32(); break;
            case 63:
            case 64:
            case 65:
                spell->Effect[i - 63] = fields[i].GetUInt32(); break;
            case 66:
            case 67:
            case 68:
                spell->EffectDieSides[i - 66] = fields[i].GetInt32(); break;
            case 69:
            case 70:
            case 71:
                spell->EffectBaseDice[i - 69] = fields[i].GetInt32(); break;
            case 72:
            case 73:
            case 74:
                spell->EffectDicePerLevel[i - 72] = fields[i].GetFloat(); break;
            case 75:
            case 76:
            case 77:
                spell->EffectRealPointsPerLevel[i - 75] = fields[i].GetFloat(); break;
            case 78:
            case 79:
            case 80:
                spell->EffectBasePoints[i - 78] = fields[i].GetInt32(); break;
            case 81:
            case 82:
            case 83:
                spell->EffectMechanic[i - 81] = fields[i].GetUInt32(); break;
            case 84:
            case 85:
            case 86:
                spell->EffectImplicitTargetA[i - 84] = fields[i].GetUInt32(); break;
            case 87:
            case 88:
            case 89:
                spell->EffectImplicitTargetB[i - 87] = fields[i].GetUInt32(); break;
            case 90:
            case 91:
            case 92:
                spell->EffectRadiusIndex[i - 90] = fields[i].GetUInt32(); break;
            case 93:
            case 94:
            case 95:
                spell->EffectApplyAuraName[i - 93] = fields[i].GetUInt32(); break;
            case 96:
            case 97:
            case 98:
                spell->EffectAmplitude[i - 96] = fields[i].GetUInt32(); break;
            case 99:
            case 100:
            case 101:
                spell->EffectValueMultiplier[i - 99] = fields[i].GetFloat(); break;
            case 102:
            case 103:
            case 104:
                spell->EffectChainTarget[i - 102] = fields[i].GetUInt32(); break;
            case 105:
            case 106:
            case 107:
                spell->EffectItemType[i - 105] = fields[i].GetUInt32(); break;
            case 108:
            case 109:
            case 110:
                spell->EffectMiscValue[i - 108] = fields[i].GetInt32(); break;
            case 111:
            case 112:
            case 113:
                spell->EffectMiscValueB[i - 111] = fields[i].GetInt32(); break;
            case 114:
            case 115:
            case 116:
                spell->EffectTriggerSpell[i - 114] = fields[i].GetUInt32(); break;
            case 117:
            case 118:
            case 119:
                spell->EffectPointsPerComboPoint[i - 117] = fields[i].GetFloat(); break;
            case 120: spell->SpellVisual = fields[i].GetUInt32(); break;
            case 121: spell->SpellIconID = fields[i].GetUInt32(); break;
            case 122: spell->activeIconID = fields[i].GetUInt32(); break;
            case 123: spell->spellPriority = fields[i].GetUInt32(); break;
            case 124:
            case 125:
            case 126:
            case 127:
            case 128:
            case 129:
            case 130:
            case 131:
            case 132:
            case 133:
            case 134:
            case 135:
            case 136:
            case 137:
            case 138:
            case 139:
                strcpy(spell->SpellName[i - 124], fields[i].GetCString()); break;
            case 140:
            case 141:
            case 142:
            case 143:
            case 144:
            case 145:
            case 146:
            case 147:
            case 148:
            case 149:
            case 150:
            case 151:
            case 152:
            case 153:
            case 154:
            case 155:
                strcpy(spell->Rank[i - 140], fields[i].GetCString()); break;
            case 156: spell->ManaCostPercentage = fields[i].GetUInt32(); break;
            case 157: spell->StartRecoveryCategory = fields[i].GetUInt32(); break;
            case 158: spell->StartRecoveryTime = fields[i].GetUInt32(); break;
            case 159: spell->MaxTargetLevel = fields[i].GetUInt32(); break;
            case 160: spell->SpellFamilyName = fields[i].GetUInt32(); break;
            case 161: spell->SpellFamilyFlags = fields[i].GetUInt64(); break;
            case 162: spell->MaxAffectedTargets = fields[i].GetUInt32(); break;
            case 163: spell->DmgClass = fields[i].GetUInt32(); break;
            case 164: spell->PreventionType = fields[i].GetUInt32(); break;
            case 165:
            case 166:
            case 167:
                spell->EffectDamageMultiplier[i - 165] = fields[i].GetFloat(); break;
            case 168: spell->TotemCategory[0] = fields[i].GetUInt32(); break;
            case 169: spell->TotemCategory[1] = fields[i].GetUInt32(); break;
            case 170: spell->AreaId = fields[i].GetUInt32(); break;
            case 171: spell->SchoolMask = fields[i].GetUInt32(); break;
            case 172: spell->CustomAttributesFlags = fields[i].GetUInt32(); break;
            default:
                ASSERT(false); //logic failure
            }
        }

        if (!ValidateSpell(spell))
        {
            if (newSpell)
                spellTemplates.erase(id);
            else
                delete spell;
        }

        if (newSpell)
        {
            if (id > maxSpellId)
                maxSpellId = id;

            spellTemplates[id] = spell;
        }
        count_overrides++;
    } while (result_override->NextRow());
    
    for (auto & spellTemplate : spellTemplates) 
    {
        SpellEntry const* spell = spellTemplate.second;
        if(spell && spell->Category)
            sSpellsByCategoryStore[spell->Category].insert(spellTemplate.first);
    }

    for (SkillLineAbilityEntry const* skillLine : sSkillLineAbilityStore)
    {
        SpellEntry const* spellInfo = GetSpellTemplate(skillLine->spellId);
        if (spellInfo && spellInfo->Attributes & SPELL_ATTR0_PASSIVE)
        {
            for (CreatureFamilyEntry const* cFamily : sCreatureFamilyStore)
            {
                if (skillLine->skillId != cFamily->skillLine[0] && skillLine->skillId != cFamily->skillLine[1])
                    continue;

                if (spellInfo->SpellLevel)
                    continue;

                if (skillLine->AutolearnType != SKILL_LINE_ABILITY_LEARNED_ON_SKILL_LEARN)
                    continue;

                sPetFamilySpellsStore[cFamily->ID].insert(spellInfo->Id);
            }
        }
    }

    //(re) apply custom attr
    sSpellMgr->LoadSpellCustomAttr();

    TC_LOG_INFO("server.loading",">> Loaded %u spell templates (as well as %u overrides).", count, count_overrides);
}

SpellEntry const* ObjectMgr::GetSpellTemplate(uint32 id) const
{
    auto itr = spellTemplates.find(id);
    if (itr != spellTemplates.end())
        return itr->second;
        
    return nullptr;
}

void ObjectMgr::LoadFactionChangeItems()
{
    factionchange_items.clear();
    
    QueryResult result = WorldDatabase.Query("SELECT alliance_id, horde_id FROM player_factionchange_items");

    if (!result)
    {
        TC_LOG_INFO("server.loading",">> Loaded 0 faction change items. DB table `player_factionchange_items` is empty.");
        
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 alliance = fields[0].GetUInt32();
        uint32 horde = fields[1].GetUInt32();

        // TODO: add item template validation
        /*if (!GetItemTemplate(alliance))
            TC_LOG_ERROR("sql.sql","Item %u referenced in `player_factionchange_items` does not exist, pair skipped!", alliance);
        else if (!GetItemTemplate(horde))
            TC_LOG_ERROR("sql.sql","Item %u referenced in `player_factionchange_items` does not exist, pair skipped!", horde);
        else*/
            factionchange_items[alliance] = horde;

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading",">> Loaded %u faction change items", count);
    
}

void ObjectMgr::LoadFactionChangeSpells()
{
    factionchange_spells.clear();
    
    QueryResult result = WorldDatabase.Query("SELECT alliance_id, horde_id FROM player_factionchange_spells");

    if (!result)
    {
        TC_LOG_ERROR("server.loading",">> Loaded 0 faction change spells. DB table `player_factionchange_spells` is empty.");
        
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 alliance = fields[0].GetUInt32();
        uint32 horde = fields[1].GetUInt32();

        if (!sSpellMgr->GetSpellInfo(alliance))
            TC_LOG_ERROR("sql.sql","Spell %u referenced in `player_factionchange_spells` does not exist, skipped!", alliance);
        else if (!sSpellMgr->GetSpellInfo(horde))
            TC_LOG_ERROR("sql.sql","Spell %u referenced in `player_factionchange_spells` does not exist, skipped!", horde);
        else
            factionchange_spells[alliance] = horde;

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading",">> Loaded %u faction change spells", count);
    
}

void ObjectMgr::LoadFactionChangeTitles()
{
    factionchange_titles.clear();
    
    QueryResult result = WorldDatabase.Query("SELECT alliance_id, horde_id FROM player_factionchange_titles");

    if (!result)
    {
        TC_LOG_ERROR("server.loading",">> Loaded 0 faction change titles. DB table `player_factionchange_titles` is empty.");
        
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 alliance = fields[0].GetUInt32();
        uint32 horde = fields[1].GetUInt32();

        factionchange_titles[alliance] = horde;

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading",">> Loaded %u faction change titles", count);
    
}

void ObjectMgr::LoadFactionChangeQuests()
{
    factionchange_quests.clear();
    
    QueryResult result = WorldDatabase.Query("SELECT alliance_id, horde_id FROM player_factionchange_quests");

    if (!result)
    {
        TC_LOG_ERROR("server.loading",">> Loaded 0 faction change quest. DB table `player_factionchange_quests` is empty.");
        
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 alliance = fields[0].GetUInt32();
        uint32 horde = fields[1].GetUInt32();

        factionchange_quests[alliance] = horde;

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading",">> Loaded %u faction change quests", count);
    
}

void ObjectMgr::LoadFactionChangeReputGeneric()
{
    factionchange_reput_generic.clear();
    
    QueryResult result = WorldDatabase.Query("SELECT alliance_id, horde_id FROM player_factionchange_reputations_generic");

    if (!result)
    {
        TC_LOG_INFO("server.loading",">> Loaded 0 faction change reputations (generic). DB table `player_factionchange_reputations_generic` is empty.");
        
        return;
    }

    uint32 count = 0;

    do
    {
        Field* fields = result->Fetch();

        uint32 alliance = fields[0].GetUInt32();
        uint32 horde = fields[1].GetUInt32();

        // TODO: add item template validation
        /*if (!GetItemTemplate(alliance))
            TC_LOG_ERROR("sql.sql","Item %u referenced in `player_factionchange_items` does not exist, pair skipped!", alliance);
        else if (!GetItemTemplate(horde))
            TC_LOG_ERROR("sql.sql","Item %u referenced in `player_factionchange_items` does not exist, pair skipped!", horde);
        else*/
            factionchange_reput_generic[alliance] = horde;

        ++count;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading",">> Loaded %u faction change reputations (generic)", count);
    
}

void ObjectMgr::AddLocaleString(std::string const& s, LocaleConstant locale, StringVector& data)
{
    if (!s.empty())
    {
        if (data.size() <= size_t(locale))
            data.resize(locale + 1);

        data[locale] = s;
    }
}

CreatureBaseStats const* ObjectMgr::GetCreatureBaseStats(uint8 level, uint8 unitClass)
{
    CreatureBaseStatsContainer::const_iterator it = _creatureBaseStatsStore.find(MAKE_PAIR16(level, unitClass));

    if (it != _creatureBaseStatsStore.end())
        return &(it->second);

    struct DefaultCreatureBaseStats : public CreatureBaseStats
    {
        DefaultCreatureBaseStats()
        {
            BaseArmor = 1;
            for (uint8 j = 0; j < MAX_EXPANSIONS; ++j)
            {
                BaseHealth[j] = 1;
                BaseDamage[j] = 0.0f;
            }
            BaseMana = 0;
            AttackPower = 0;
            RangedAttackPower = 0;
        }
    };
    static const DefaultCreatureBaseStats defStats;
    return &defStats;
}

void ObjectMgr::LoadCreatureClassLevelStats()
{
    uint32 oldMSTime = GetMSTime();

    QueryResult result = WorldDatabase.Query("SELECT level, class, basehp0, basehp1, basehp2, basemana, basearmor, attackpower, rangedattackpower, damage_base, damage_exp1, damage_exp2 FROM creature_classlevelstats");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 creature base stats. DB table `creature_classlevelstats` is empty.");
        return;
    }

    uint32 count = 0;
    do
    {
        Field* fields = result->Fetch();

        uint8 Level = fields[0].GetUInt8();
        uint8 Class = fields[1].GetUInt8();

        if (!Class || ((1 << (Class - 1)) & CLASSMASK_ALL_CREATURES) == 0)
            TC_LOG_ERROR("sql.sql", "Creature base stats for level %u has invalid class %u", Level, Class);

        CreatureBaseStats stats;

        for (uint8 i = 0; i < MAX_EXPANSIONS; ++i)
        {
            stats.BaseHealth[i] = fields[2 + i].GetUInt16();

            if (stats.BaseHealth[i] == 0)
            {
                TC_LOG_ERROR("sql.sql", "Creature base stats for class %u, level %u has invalid zero base HP[%u] - set to 1", Class, Level, i);
                stats.BaseHealth[i] = 1;
            }

            stats.BaseDamage[i] = fields[9 + i].GetFloat();
            if (stats.BaseDamage[i] < 0.0f)
            {
                TC_LOG_ERROR("sql.sql", "Creature base stats for class %u, level %u has invalid negative base damage[%u] - set to 0.0", Class, Level, i);
                stats.BaseDamage[i] = 0.0f;
            }
        }

        stats.BaseMana = fields[5].GetUInt16();
        stats.BaseArmor = fields[6].GetUInt16();

        stats.AttackPower = fields[7].GetUInt16();
        stats.RangedAttackPower = fields[8].GetUInt16();

        _creatureBaseStatsStore[MAKE_PAIR16(Level, Class)] = stats;

        ++count;
    }
    while (result->NextRow());

    CreatureTemplateContainer const& ctc = sObjectMgr->GetCreatureTemplates();
    for (auto itr = ctc.begin(); itr != ctc.end(); ++itr)
    {
        for (uint16 lvl = itr->second.minlevel; lvl <= itr->second.maxlevel; ++lvl)
        {
            if (_creatureBaseStatsStore.find(MAKE_PAIR16(lvl, itr->second.unit_class)) == _creatureBaseStatsStore.end())
                TC_LOG_ERROR("sql.sql", "Missing base stats for creature class %u level %u", itr->second.unit_class, lvl);
        }
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u creature base stats in %u ms", count, GetMSTimeDiffToNow(oldMSTime));
}

ItemExtendedCostEntry const* ObjectMgr::GetItemExtendedCost(uint32 id) const
{
    return Trinity::Containers::MapGetValuePtr(sItemExtendedCostStore, id);
}

void ObjectMgr::LoadBroadcastTexts()
{
    uint32 oldMSTime = GetMSTime();

    _broadcastTextStore.clear(); // for reload case

    //                                               0   1         2         3           4         5         6         7            8            9            10       11    12
    QueryResult result = WorldDatabase.Query("SELECT ID, Language, MaleText, FemaleText, EmoteID0, EmoteID1, EmoteID2, EmoteDelay0, EmoteDelay1, EmoteDelay2, SoundId, Unk1, Unk2 FROM broadcast_text");
    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 broadcast texts. DB table `broadcast_text` is empty.");
        return;
    }

    _broadcastTextStore.rehash(result->GetRowCount());

    do
    {
        Field* fields = result->Fetch();

        BroadcastText bct;

        bct.Id = fields[0].GetUInt32();
        bct.Language = fields[1].GetUInt32();
        bct.MaleText[DEFAULT_LOCALE] = fields[2].GetString();
        bct.FemaleText[DEFAULT_LOCALE] = fields[3].GetString();
        bct.EmoteId0 = fields[4].GetUInt32();
        bct.EmoteId1 = fields[5].GetUInt32();
        bct.EmoteId2 = fields[6].GetUInt32();
        bct.EmoteDelay0 = fields[7].GetUInt32();
        bct.EmoteDelay1 = fields[8].GetUInt32();
        bct.EmoteDelay2 = fields[9].GetUInt32();
        bct.SoundId = fields[10].GetUInt32();
        bct.Unk1 = fields[11].GetUInt32();
        bct.Unk2 = fields[12].GetUInt32();

        if (bct.SoundId)
        {
            if (!sSoundEntriesStore.LookupEntry(bct.SoundId))
            {
                TC_LOG_DEBUG("broadcasttext", "BroadcastText (Id: %u) in table `broadcast_text` has SoundId %u but sound does not exist.", bct.Id, bct.SoundId);
                bct.SoundId = 0;
            }
        }

        if (!GetLanguageDescByID(bct.Language))
        {
            TC_LOG_DEBUG("broadcasttext", "BroadcastText (Id: %u) in table `broadcast_text` using Language %u but Language does not exist.", bct.Id, bct.Language);
            bct.Language = LANG_UNIVERSAL;
        }
        
        if (bct.EmoteId0)
        {
            if (!sEmotesStore.LookupEntry(bct.EmoteId0))
            {
                TC_LOG_DEBUG("broadcasttext", "BroadcastText (Id: %u) in table `broadcast_text` has EmoteId0 %u but emote does not exist.", bct.Id, bct.EmoteId0);
                bct.EmoteId0 = 0;
            }
        }

        if (bct.EmoteId1)
        {
            if (!sEmotesStore.LookupEntry(bct.EmoteId1))
            {
                TC_LOG_DEBUG("broadcasttext", "BroadcastText (Id: %u) in table `broadcast_text` has EmoteId1 %u but emote does not exist.", bct.Id, bct.EmoteId1);
                bct.EmoteId1 = 0;
            }
        }

        if (bct.EmoteId2)
        {
            if (!sEmotesStore.LookupEntry(bct.EmoteId2))
            {
                TC_LOG_DEBUG("broadcasttext", "BroadcastText (Id: %u) in table `broadcast_text` has EmoteId2 %u but emote does not exist.", bct.Id, bct.EmoteId2);
                bct.EmoteId2 = 0;
            }
        }

        _broadcastTextStore[bct.Id] = bct;
    }
    while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded " SZFMTD " broadcast texts in %u ms", _broadcastTextStore.size(), GetMSTimeDiffToNow(oldMSTime));
}

void ObjectMgr::LoadBroadcastTextLocales()
{
    uint32 oldMSTime = GetMSTime();

    QueryResult result = WorldDatabase.Query("SELECT ID, locale, MaleText, FemaleText FROM locales_broadcast_text");

    if (!result)
    {
        TC_LOG_INFO("server.loading", ">> Loaded 0 broadcast text locales. DB table `broadcast_text_locale` is empty.");
        return;
    }

    do
    {
        Field* fields = result->Fetch();

        uint32 id = fields[0].GetUInt32();
        std::string localeName = fields[1].GetString();
        std::string MaleText = fields[2].GetString();
        std::string FemaleText = fields[3].GetString();

        BroadcastTextContainer::iterator bct = _broadcastTextStore.find(id);
        if (bct == _broadcastTextStore.end())
        {
            TC_LOG_ERROR("sql.sql", "BroadcastText (Id: %u) in table `broadcast_text_locale` does not exist. Skipped!", id);
            continue;
        }

        LocaleConstant locale = GetLocaleByName(localeName);
        if (locale == LOCALE_enUS)
            continue;

        AddLocaleString(MaleText, locale, bct->second.MaleText);
        AddLocaleString(FemaleText, locale, bct->second.FemaleText);
    } while (result->NextRow());

    TC_LOG_INFO("server.loading", ">> Loaded %u broadcast text locales in %u ms", uint32(_broadcastTextStore.size()), GetMSTimeDiffToNow(oldMSTime));
}

GameObject* ObjectMgr::CreateGameObject(uint32 entry)
{
    GameObjectTemplate const* goInfo = GetGameObjectTemplate(entry);
    if (goInfo && goInfo->type == GAMEOBJECT_TYPE_TRANSPORT)
        return new StaticTransport;
    else
        return new GameObject;
}

std::string ScriptInfo::GetDebugInfo() const
{
    char sz[256];
    sprintf(sz, "%s (script id: %u)", GetScriptCommandName(ScriptCommands(command)).c_str(), /* GetScriptsTableNameByType(type).c_str(), */ id);
    return std::string(sz);
}

std::string ScriptInfo::GetScriptCommandName(ScriptCommands command)
{
    std::string res = "";
    switch (command)
    {
        case SCRIPT_COMMAND_TALK: res = "SCRIPT_COMMAND_TALK"; break;
        case SCRIPT_COMMAND_EMOTE: res = "SCRIPT_COMMAND_EMOTE"; break;
        case SCRIPT_COMMAND_FIELD_SET: res = "SCRIPT_COMMAND_FIELD_SET"; break;
        case SCRIPT_COMMAND_MOVE_TO: res = "SCRIPT_COMMAND_MOVE_TO"; break;
        case SCRIPT_COMMAND_FLAG_SET: res = "SCRIPT_COMMAND_FLAG_SET"; break;
        case SCRIPT_COMMAND_FLAG_REMOVE: res = "SCRIPT_COMMAND_FLAG_REMOVE"; break;
        case SCRIPT_COMMAND_TELEPORT_TO: res = "SCRIPT_COMMAND_TELEPORT_TO"; break;
        case SCRIPT_COMMAND_QUEST_EXPLORED: res = "SCRIPT_COMMAND_QUEST_EXPLORED"; break;
        case SCRIPT_COMMAND_KILL_CREDIT: res = "SCRIPT_COMMAND_KILL_CREDIT"; break;
        case SCRIPT_COMMAND_RESPAWN_GAMEOBJECT: res = "SCRIPT_COMMAND_RESPAWN_GAMEOBJECT"; break;
        case SCRIPT_COMMAND_TEMP_SUMMON_CREATURE: res = "SCRIPT_COMMAND_TEMP_SUMMON_CREATURE"; break;
        case SCRIPT_COMMAND_OPEN_DOOR: res = "SCRIPT_COMMAND_OPEN_DOOR"; break;
        case SCRIPT_COMMAND_CLOSE_DOOR: res = "SCRIPT_COMMAND_CLOSE_DOOR"; break;
        case SCRIPT_COMMAND_ACTIVATE_OBJECT: res = "SCRIPT_COMMAND_ACTIVATE_OBJECT"; break;
        case SCRIPT_COMMAND_REMOVE_AURA: res = "SCRIPT_COMMAND_REMOVE_AURA"; break;
        case SCRIPT_COMMAND_CAST_SPELL: res = "SCRIPT_COMMAND_CAST_SPELL"; break;
        case SCRIPT_COMMAND_PLAY_SOUND: res = "SCRIPT_COMMAND_PLAY_SOUND"; break;

        case SCRIPT_COMMAND_LOAD_PATH: res = "SCRIPT_COMMAND_LOAD_PATH"; break;
        case SCRIPT_COMMAND_CALLSCRIPT_TO_UNIT: res = "SCRIPT_COMMAND_CALLSCRIPT_TO_UNIT"; break;
        case SCRIPT_COMMAND_KILL: res = "SCRIPT_COMMAND_KILL"; break;
        case SCRIPT_COMMAND_SMART_SET_DATA: res = "SCRIPT_COMMAND_SMART_SET_DATA"; break;
        default:
        {
            char sz[32];
            sprintf(sz, "Unknown command: %d", command);
            res = sz;
            break;
        }
    }
    return res;
}

void ObjectMgr::InitializeQueriesData(QueryDataGroup mask)
{
    // cache disabled
    if (!sWorld->getBoolConfig(CONFIG_CACHE_DATA_QUERIES))
        return;

    // Initialize Query data for creatures
    if (mask & QUERY_DATA_CREATURES)
        for (auto& creatureTemplatePair : _creatureTemplateStore)
            creatureTemplatePair.second.InitializeQueryData();

    // Initialize Query Data for gameobjects
    if (mask & QUERY_DATA_GAMEOBJECTS)
        for (auto& gameObjectTemplatePair : _gameObjectTemplateStore)
            gameObjectTemplatePair.second.InitializeQueryData();

    // Initialize Query Data for items
    if (mask & QUERY_DATA_ITEMS)
        for (auto& itemTemplatePair : _itemTemplateStore)
            itemTemplatePair.second.InitializeQueryData();

    // Initialize Query Data for quests
    if (mask & QUERY_DATA_QUESTS)
        for (auto& questTemplatePair : _questTemplates)
            questTemplatePair.second.InitializeQueryData();

#ifdef LICH_KING
    // Initialize Quest POI data
    if (mask & QUERY_DATA_POIS)
        for (auto& poiWrapperPair : _questPOIStore)
            poiWrapperPair.second.InitializeQueryData();
#endif
}
