#ifndef DBCENUMS_H
#define DBCENUMS_H

struct DBCPosition3D
{
    float X;
    float Y;
    float Z;
};

enum LevelLimit
{
    // Client expected level limitation, like as used in DBC item max levels for "until max player level"
    // use as default max player level, must be fit max level for used client
    // also see MAX_LEVEL and STRONG_MAX_LEVEL define
#ifdef LICH_KING
    DEFAULT_MAX_LEVEL = 80,
#else
    DEFAULT_MAX_LEVEL = 70,
#endif

    // client supported max level for player/pets/etc. Avoid overflow or client stability affected.
    // also see GT_MAX_LEVEL define
    MAX_LEVEL = 100,

    // Server side limitation. Base at used code requirements.
    // also see MAX_LEVEL and GT_MAX_LEVEL define
    STRONG_MAX_LEVEL = 255,
};

// Server side limitation. Base at used code requirements.
// also see MAX_LEVEL and GT_MAX_LEVEL define
#define STRONG_MAX_LEVEL 255

enum BattlegroundBracketId                                  // bracketId for level ranges
{
    BG_BRACKET_ID_FIRST = 0,
    BG_BRACKET_ID_LAST = 15
};

// must be max value in PvPDificulty slot+1
#define MAX_BATTLEGROUND_BRACKETS  16

enum AreaTeams
{
    AREATEAM_NONE  = 0,
    AREATEAM_ALLY  = 2,
    AREATEAM_HORDE = 4
};

enum AreaFlags
{
    AREA_FLAG_SNOW             = 0x00000001,                // snow (only Dun Morogh, Naxxramas, Razorfen Downs and Winterspring)
    AREA_FLAG_UNK1             = 0x00000002,                // unknown, (only Naxxramas and Razorfen Downs)
    AREA_FLAG_UNK2             = 0x00000004,                // Only used on development map
    AREA_FLAG_SLAVE_CAPITAL    = 0x00000008,                // city and city subzones
    AREA_FLAG_UNK3             = 0x00000010,                // unknown
    AREA_FLAG_SLAVE_CAPITAL2   = 0x00000020,                // slave capital city flag?
    AREA_FLAG_UNK4             = 0x00000040,                // many zones have this flag
    AREA_FLAG_ARENA            = 0x00000080,                // arena, both instanced and world arenas
    AREA_FLAG_CAPITAL          = 0x00000100,                // main capital city flag
    AREA_FLAG_CITY             = 0x00000200,                // only for one zone named "City" (where it located?)
    AREA_FLAG_OUTLAND          = 0x00000400,                // outland zones? (only Eye of the Storm not have this flag, but have 0x00004000 flag)
    AREA_FLAG_SANCTUARY        = 0x00000800,                // sanctuary area (PvP disabled)
    AREA_FLAG_NEED_FLY         = 0x00001000,                // only Netherwing Ledge, Socrethar's Seat, Tempest Keep, The Arcatraz, The Botanica, The Mechanar, Sorrow Wing Point, Dragonspine Ridge, Netherwing Mines, Dragonmaw Base Camp, Dragonmaw Skyway
    AREA_FLAG_UNUSED1          = 0x00002000,                // not used now (no area/zones with this flag set in 2.4.2)
    AREA_FLAG_OUTLAND2         = 0x00004000,                // outland zones? (only Circle of Blood Arena not have this flag, but have 0x00000400 flag)
    AREA_FLAG_PVP              = 0x00008000,                // pvp objective area? (Death's Door also has this flag although it's no pvp object area)
    AREA_FLAG_ARENA_INSTANCE   = 0x00010000,                // used by instanced arenas only
    AREA_FLAG_UNUSED2          = 0x00020000,                // not used now (no area/zones with this flag set in 2.4.2)
    AREA_FLAG_UNK5             = 0x00040000,                // just used for Amani Pass, Hatchet Hills
    AREA_FLAG_LOWLEVEL         = 0x00100000,                 // used for some starting areas with area_level <=15
    AREA_FLAG_INSIDE           = 0x02000000,
    AREA_FLAG_OUTSIDE          = 0x04000000,
#ifdef LICH_KING
    AREA_FLAG_WINTERGRASP_2    = 0x08000000,                // Can Hearth And Resurrect From Area
    AREA_FLAG_NO_FLY_ZONE      = 0x20000000                 // Marks zones where you cannot fly
#endif
};

enum Difficulty : uint8
{
    REGULAR_DIFFICULTY = 0,

    DUNGEON_DIFFICULTY_NORMAL = 0,
    DUNGEON_DIFFICULTY_HEROIC = 1,
#ifdef LICH_KING
    DUNGEON_DIFFICULTY_EPIC   = 2,

    RAID_DIFFICULTY_10MAN_NORMAL = 0,
    RAID_DIFFICULTY_25MAN_NORMAL = 1,
    RAID_DIFFICULTY_10MAN_HEROIC = 2,
    RAID_DIFFICULTY_25MAN_HEROIC = 3
#else
    RAID_DIFFICULTY_NORMAL     = 0,
#endif
};

#ifdef LICH_KING
#define MAX_DUNGEON_DIFFICULTY     3
#define MAX_RAID_DIFFICULTY        4
#define MAX_DIFFICULTY             4
#else
#define MAX_DUNGEON_DIFFICULTY     2
#define MAX_RAID_DIFFICULTY        1
#define MAX_DIFFICULTY             2
#endif //LICH_KING

enum SpawnMask
{
    SPAWNMASK_CONTINENT         = (1 << REGULAR_DIFFICULTY), // any maps without spawn modes

    SPAWNMASK_DUNGEON_NORMAL    = (1 << DUNGEON_DIFFICULTY_NORMAL),
    SPAWNMASK_DUNGEON_HEROIC    = (1 << DUNGEON_DIFFICULTY_HEROIC),
    SPAWNMASK_DUNGEON_ALL       = (SPAWNMASK_DUNGEON_NORMAL | SPAWNMASK_DUNGEON_HEROIC),
//LK
#ifdef LICH_KING
    SPAWNMASK_RAID_10MAN_NORMAL = (1 << RAID_DIFFICULTY_10MAN_NORMAL),
    SPAWNMASK_RAID_25MAN_NORMAL = (1 << RAID_DIFFICULTY_25MAN_NORMAL),
    SPAWNMASK_RAID_NORMAL_ALL   = (SPAWNMASK_RAID_10MAN_NORMAL | SPAWNMASK_RAID_25MAN_NORMAL),

    SPAWNMASK_RAID_10MAN_HEROIC = (1 << RAID_DIFFICULTY_10MAN_HEROIC),
    SPAWNMASK_RAID_25MAN_HEROIC = (1 << RAID_DIFFICULTY_25MAN_HEROIC),
    SPAWNMASK_RAID_HEROIC_ALL   = (SPAWNMASK_RAID_10MAN_HEROIC | SPAWNMASK_RAID_25MAN_HEROIC),
    SPAWNMASK_RAID_ALL          = (SPAWNMASK_RAID_NORMAL_ALL | SPAWNMASK_RAID_HEROIC_ALL)
#else
    SPAWNMASK_RAID_NORMAL_ALL   = RAID_DIFFICULTY_NORMAL,
    SPAWNMASK_RAID_ALL          = RAID_DIFFICULTY_NORMAL,
#endif
};

enum FactionTemplateFlags
{
    FACTION_TEMPLATE_FLAG_PVP = 0x00000800,   // flagged for PvP
    FACTION_TEMPLATE_FLAG_CONTESTED_GUARD   =   0x00001000, // faction will attack players that were involved in PvP combats
    FACTION_TEMPLATE_FLAG_HOSTILE_BY_DEFAULT = 0x00002000,
};

enum FactionMasks
{
    FACTION_MASK_PLAYER   = 1,                              // any player
    FACTION_MASK_ALLIANCE = 2,                              // player or creature from alliance team
    FACTION_MASK_HORDE    = 4,                              // player or creature from horde team
    FACTION_MASK_MONSTER  = 8                               // aggressive creature from monster team
    // if none flags set then non-aggressive creature
};

enum MapTypes
{
    MAP_COMMON          = 0,
    MAP_INSTANCE        = 1,
    MAP_RAID            = 2,
    MAP_BATTLEGROUND    = 3,
    MAP_ARENA           = 4
};

enum AbilityLearnType
{
    SKILL_LINE_ABILITY_LEARNED_ON_SKILL_VALUE = 1, // Spell state will update depending on skill value
    SKILL_LINE_ABILITY_LEARNED_ON_SKILL_LEARN = 2  // Spell will be learned/removed together with entire skill
};

enum ItemEnchantmentType
{
    ITEM_ENCHANTMENT_TYPE_NONE         = 0,
    ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL = 1,
    ITEM_ENCHANTMENT_TYPE_DAMAGE       = 2,
    ITEM_ENCHANTMENT_TYPE_EQUIP_SPELL  = 3,
    ITEM_ENCHANTMENT_TYPE_RESISTANCE   = 4,
    ITEM_ENCHANTMENT_TYPE_STAT         = 5,
    ITEM_ENCHANTMENT_TYPE_TOTEM        = 6
};

enum ItemEnchantmentAuraId
{
    ITEM_ENCHANTMENT_AURAID_POISON     = 26,
    ITEM_ENCHANTMENT_AURAID_NORMAL     = 28,
    ITEM_ENCHANTMENT_AURAID_FIRE       = 32,
    ITEM_ENCHANTMENT_AURAID_FROST      = 33,
    ITEM_ENCHANTMENT_AURAID_NATURE     = 81,
    ITEM_ENCHANTMENT_AURAID_SHADOW     = 107
};

enum SkillRaceClassInfoFlags
{
    SKILL_FLAG_NO_SKILLUP_MESSAGE       = 0x2,
    SKILL_FLAG_ALWAYS_MAX_VALUE         = 0x10,
    SKILL_FLAG_UNLEARNABLE              = 0x20,     // Skill can be unlearned
    SKILL_FLAG_INCLUDE_IN_SORT          = 0x80,     // Spells belonging to a skill with this flag will additionally compare skill ids when sorting spellbook in client
    SKILL_FLAG_NOT_TRAINABLE            = 0x100,
    SKILL_FLAG_MONO_VALUE               = 0x400     // Skill always has value 1 - clientside display flag, real value can be different
};

enum SpellCategoryFlags
{
    SPELL_CATEGORY_FLAG_COOLDOWN_SCALES_WITH_WEAPON_SPEED   = 0x01, // unused
    SPELL_CATEGORY_FLAG_COOLDOWN_STARTS_ON_EVENT            = 0x04
};

enum TotemCategoryType
{
    TOTEM_CATEGORY_TYPE_KNIFE   = 1,
    TOTEM_CATEGORY_TYPE_TOTEM   = 2,
    TOTEM_CATEGORY_TYPE_ROD     = 3,
    TOTEM_CATEGORY_TYPE_PICK    = 21,
    TOTEM_CATEGORY_TYPE_STONE   = 22,
    TOTEM_CATEGORY_TYPE_HAMMER  = 23,
    TOTEM_CATEGORY_TYPE_SPANNER = 24
};

#define MAX_SPELL_EFFECTS 3
#define MAX_EFFECT_MASK 7
#define MAX_SPELL_REAGENTS 8

#endif

