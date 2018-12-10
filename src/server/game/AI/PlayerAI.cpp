
#include "PlayerAI.h"
#include "CommonHelpers.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"

enum Spells
{
    /* Generic */
    SPELL_AUTO_SHOT         =    75,
    SPELL_SHOOT             =  3018,
    SPELL_THROW             =  2764,
    SPELL_SHOOT_WAND        =  5019,

    /* Warrior - Generic */
    SPELL_BATTLE_STANCE     =  2457,
    SPELL_BERSERKER_STANCE  =  2458,
    SPELL_DEFENSIVE_STANCE  =    71,
    SPELL_CHARGE            = 11578,
    SPELL_INTERCEPT         = 20252,
    SPELL_ENRAGED_REGEN     = 55694,
    SPELL_INTIMIDATING_SHOUT=  5246,
    SPELL_PUMMEL            =  6552,
    SPELL_SHIELD_BASH       =    72,
    SPELL_BLOODRAGE         =  2687,

    /* Warrior - Arms */
    SPELL_SWEEPING_STRIKES  = 12328,
    SPELL_MORTAL_STRIKE     = 12294,
    SPELL_BLADESTORM        = 46924,
    SPELL_REND              = 47465,
    SPELL_RETALIATION       = 20230,
    SPELL_THUNDER_CLAP      = 47502,
#ifdef LICH_KING
    SPELL_SHATTERING_THROW  = 64382,
#endif

    /* Warrior - Fury */
    SPELL_DEATH_WISH        = 12292,
    SPELL_BLOODTHIRST       = 23881,
    SPELL_RECKLESSNESS      =  1719,
    SPELL_PIERCING_HOWL     = 12323,
#ifdef LICH_KING
    SPELL_DEMO_SHOUT        = 47437,
    SPELL_HEROIC_FURY       = 60970,
    SPELL_EXECUTE           = 47471,
    PASSIVE_TITANS_GRIP     = 46917,
#else
    SPELL_DEMO_SHOUT        = 25203,
    SPELL_EXECUTE           = 25236,
#endif

    /* Warrior - Protection */
#ifdef LICH_KING
    SPELL_VIGILANCE         = 50720,
    SPELL_SHOCKWAVE         = 46968,
    SPELL_SHIELD_SLAM       = 47488,
#else
    SPELL_SHIELD_SLAM       = 30356,
#endif
    SPELL_DEVASTATE         = 20243,
    SPELL_CONCUSSION_BLOW   = 12809,
    SPELL_DISARM            =   676,
    SPELL_LAST_STAND        = 12975,
    SPELL_SHIELD_BLOCK      =  2565,
    SPELL_SHIELD_WALL       =   871,
    SPELL_SPELL_REFLECTION  = 23920,

    /* Paladin - Generic */
    SPELL_AURA_MASTERY          = 31821,
#ifdef LICH_KING
    SPELL_LAY_ON_HANDS          = 48788,
    SPELL_BLESSING_OF_MIGHT     = 48932,
#else
    SPELL_LAY_ON_HANDS          = 27154,
    SPELL_BLESSING_OF_MIGHT     = 27140,
#endif
    SPELL_AVENGING_WRATH        = 31884,
    SPELL_DIVINE_PROTECTION     =   498,
    SPELL_DIVINE_SHIELD         =   642,
    SPELL_HAMMER_OF_JUSTICE     = 10308,
    SPELL_HAND_OF_FREEDOM       =  1044,
    SPELL_HAND_OF_PROTECTION    = 10278,
    SPELL_HAND_OF_SACRIFICE     =  6940,

    /* Paladin - Holy*/
    PASSIVE_ILLUMINATION        = 20215,
    SPELL_HOLY_SHOCK            = 20473,
#ifdef LICH_KING
    SPELL_BEACON_OF_LIGHT       = 53563,
    SPELL_FLASH_OF_LIGHT        = 48785,
    SPELL_CONSECRATION          = 48819,
    SPELL_HOLY_LIGHT            = 48782,
#else
    SPELL_FLASH_OF_LIGHT        = 27137,
    SPELL_CONSECRATION          = 27173,
    SPELL_HOLY_LIGHT            = 27136,
#endif
    SPELL_DIVINE_FAVOR          = 20216,
    SPELL_DIVINE_ILLUMINATION   = 31842,

    /* Paladin - Protection */
    SPELL_BLESS_OF_SANC         = 20911,
    SPELL_HOLY_SHIELD           = 20925,
#ifdef LICH_KING
    SPELL_DIVINE_SACRIFICE      = 64205,
    SPELL_HAMMER_OF_RIGHTEOUS   = 53595,
    SPELL_AVENGERS_SHIELD       = 48827,
    SPELL_SHIELD_OF_RIGHTEOUS   = 61411,
    #else
    SPELL_AVENGERS_SHIELD       = 32700,
#endif
    SPELL_RIGHTEOUS_FURY        = 25780,

    /* Paladin - Retribution */
    SPELL_SEAL_OF_COMMAND       = 20375,
    SPELL_CRUSADER_STRIKE       = 35395,
#ifdef LICH_KING
    SPELL_DIVINE_STORM          = 53385,
    SPELL_HAMMER_OF_WRATH       = 48806,
#else
    SPELL_HAMMER_OF_WRATH       = 27180,
#endif
    SPELL_JUDGEMENT_            = 20271,

    /* Hunter - Generic */
    SPELL_DETERRENCE        = 19263,
#ifdef LICH_KING
    SPELL_EXPLOSIVE_TRAP    = 49067,
    SPELL_FREEZING_ARROW    = 60192,
    SPELL_MULTI_SHOT        = 49048,
    SPELL_KILL_SHOT         = 61006,
#else
    SPELL_MULTI_SHOT        = 27021,
    SPELL_EXPLOSIVE_TRAP    = 27025,
#endif
    SPELL_RAPID_FIRE        =  3045,
    SPELL_VIPER_STING       =  3034,

    /* Hunter - Beast Mastery */
    SPELL_BESTIAL_WRATH     = 19574,
    PASSIVE_BEAST_WITHIN    = 34692,
#ifdef LICH_KING
    PASSIVE_BEAST_MASTERY   = 53270,
#endif

    /* Hunter - Marksmanship */
    SPELL_AIMED_SHOT        = 19434,
    PASSIVE_TRUESHOT_AURA   = 19506,
#ifdef LICH_KING
    SPELL_ARCANE_SHOT       = 49045,
    SPELL_STEADY_SHOT       = 49052,
    SPELL_CHIMERA_SHOT      = 53209,
#else
    SPELL_ARCANE_SHOT       = 27019,
    SPELL_STEADY_SHOT       = 34120,
#endif
    SPELL_READINESS         = 23989,
    SPELL_SILENCING_SHOT    = 34490,

    /* Hunter - Survival */
#ifdef LICH_KING
    PASSIVE_LOCK_AND_LOAD   = 56344,
    SPELL_EXPLOSIVE_SHOT    = 53301,
#endif
    SPELL_WYVERN_STING      = 19386,
    SPELL_BLACK_ARROW       =  3674,

    /* Rogue - Generic */
#ifdef LICH_KING
    SPELL_DISMANTLE         = 51722,
#endif
    SPELL_EVASION           = 26669,
    SPELL_KICK              =  1766,
    SPELL_VANISH            = 26889,
    SPELL_BLIND             =  2094,
    SPELL_CLOAK_OF_SHADOWS  = 31224,

    /* Rogue - Assassination */
    SPELL_COLD_BLOOD        = 14177,
    SPELL_MUTILATE          =  1329,
#ifdef LICH_KING
    SPELL_HUNGER_FOR_BLOOD  = 51662,
    SPELL_ENVENOM           = 57993,
#else
    SPELL_ENVENOM           = 32684,
#endif

    /* Rogue - Combat */
#ifdef LICH_KING
    SPELL_SINISTER_STRIKE   = 48637,
    SPELL_EVISCERATE        = 48668,
    SPELL_KILLING_SPREE     = 51690,
#else
    SPELL_EVISCERATE        = 26865,
    SPELL_SINISTER_STRIKE   = 26862,
#endif
    SPELL_BLADE_FLURRY      = 13877,
    SPELL_ADRENALINE_RUSH   = 13750,

    /* Rogue - Sublety */
    SPELL_HEMORRHAGE        = 16511,
    SPELL_PREMEDITATION     = 14183,
#ifdef LICH_KING
    SPELL_SHADOW_DANCE      = 51713,
#endif
    SPELL_PREPARATION       = 14185,
    SPELL_SHADOWSTEP        = 36554,

    /* Priest - Generic */
    SPELL_FEAR_WARD         =  6346,
#ifdef LICH_KING
    SPELL_POWER_WORD_FORT   = 48161,
    SPELL_DIVINE_SPIRIT     = 48073,
    SPELL_SHADOW_PROTECTION = 48169,
    SPELL_DIVINE_HYMN       = 64843,
    SPELL_HYMN_OF_HOPE      = 64901,
    SPELL_SHADOW_WORD_DEATH = 48158,
#else
    SPELL_POWER_WORD_FORT   = 25389,
    SPELL_DIVINE_SPIRIT     = 25312,
    SPELL_SHADOW_PROTECTION = 39374,
    SPELL_SHADOW_WORD_DEATH = 32996,
#endif
    SPELL_PSYCHIC_SCREAM    = 10890,

    /* Priest - Discipline */
#ifdef LICH_KING
    SPELL_PENANCE             = 47540,
    SPELL_POWER_WORD_SHIELD   = 48066,
    PASSIVE_SOUL_WARDING      = 63574,
#else
    SPELL_POWER_WORD_SHIELD   = 25218,
    SPELL_PAIN_SUPPRESSION    = 33206,
#endif
    SPELL_POWER_INFUSION      = 10060,
    SPELL_INNER_FOCUS         = 14751,

    /* Priest - Holy */
    PASSIVE_SPIRIT_REDEMPTION = 20711,
    SPELL_DESPERATE_PRAYER    = 19236,
#ifdef LICH_KING
    SPELL_GUARDIAN_SPIRIT     = 47788,
    SPELL_FLASH_HEAL          = 48071,
    SPELL_RENEW               = 48068,
#else
    SPELL_FLASH_HEAL          = 25235,
    SPELL_RENEW               = 25222,
#endif

    /* Priest - Shadow */
    SPELL_VAMPIRIC_EMBRACE    = 15286,
    SPELL_SHADOWFORM          = 15473,
    SPELL_VAMPIRIC_TOUCH      = 34914,
    SPELL_MIND_FLAY           = 15407,
#ifdef LICH_KING
    SPELL_MIND_BLAST          = 48127,
    SPELL_SHADOW_WORD_PAIN    = 48125,
    SPELL_DEVOURING_PLAGUE    = 48300,
    SPELL_DISPERSION          = 47585,
#else
    SPELL_MIND_BLAST          = 25375,
    SPELL_SHADOW_WORD_PAIN    = 32996,
#endif

    /* Death Knight - Generic */
#ifdef LICH_KING
    SPELL_DEATH_GRIP        = 49576,
    SPELL_STRANGULATE       = 47476,
    SPELL_EMPOWER_RUNE_WEAP = 47568,
    SPELL_ICEBORN_FORTITUDE = 48792,
    SPELL_ANTI_MAGIC_SHELL  = 48707,
    SPELL_DEATH_COIL_DK     = 49895,
    SPELL_MIND_FREEZE       = 47528,
    SPELL_ICY_TOUCH         = 49909,
    AURA_FROST_FEVER        = 55095,
    SPELL_PLAGUE_STRIKE     = 49921,
    AURA_BLOOD_PLAGUE       = 55078,
    SPELL_PESTILENCE        = 50842,

    /* Death Knight - Blood */
    SPELL_RUNE_TAP          = 48982,
    SPELL_HYSTERIA          = 49016,
    SPELL_HEART_STRIKE      = 55050,
    SPELL_DEATH_STRIKE      = 49924,
    SPELL_BLOOD_STRIKE      = 49930,
    SPELL_MARK_OF_BLOOD     = 49005,
    SPELL_VAMPIRIC_BLOOD    = 55233,

    /* Death Knight - Frost */
    PASSIVE_ICY_TALONS      = 50887,
    SPELL_FROST_STRIKE      = 49143,
    SPELL_HOWLING_BLAST     = 49184,
    SPELL_UNBREAKABLE_ARMOR = 51271,
    SPELL_OBLITERATE        = 51425,
    SPELL_DEATHCHILL        = 49796,

    /* Death Knight - Unholy */
    PASSIVE_UNHOLY_BLIGHT   = 49194,
    PASSIVE_MASTER_OF_GHOUL = 52143,
    SPELL_SCOURGE_STRIKE    = 55090,
    SPELL_DEATH_AND_DECAY   = 49938,
    SPELL_ANTI_MAGIC_ZONE   = 51052,
    SPELL_SUMMON_GARGOYLE   = 49206,
#endif

    /* Shaman - Generic */
    SPELL_HEROISM           = 32182,
    SPELL_BLOODLUST         =  2825,
    SPELL_GROUNDING_TOTEM   =  8177,

    /* Shaman - Elemental*/
    PASSIVE_ELEMENTAL_FOCUS = 16164,
    SPELL_TOTEM_OF_WRATH    = 30706,
#ifdef LICH_KING
    SPELL_THUNDERSTORM      = 51490,
    SPELL_LIGHTNING_BOLT    = 49238,
    SPELL_EARTH_SHOCK       = 49231,
    SPELL_FLAME_SHOCK       = 49233,
    SPELL_CHAIN_LIGHTNING   = 49271,
    SPELL_LAVA_BURST        = 60043,
#else
    SPELL_LIGHTNING_BOLT    = 25449,
    SPELL_EARTH_SHOCK       = 25454,
    SPELL_CHAIN_LIGHTNING   = 25442,
    SPELL_FLAME_SHOCK       = 29228,
#endif
    SPELL_ELEMENTAL_MASTERY = 16166,

    /* Shaman - Enhancement */
    PASSIVE_SPIRIT_WEAPONS  = 16268,
#ifdef LICH_KING
    SPELL_LAVA_LASH         = 60103,
    SPELL_FERAL_SPIRIT      = 51533,
    AURA_MAELSTROM_WEAPON   = 53817,
#endif
    SPELL_STORMSTRIKE       = 17364,
    SPELL_SHAMANISTIC_RAGE  = 30823,

    /* Shaman - Restoration*/
#ifdef LICH_KING
    SPELL_EARTH_SHIELD      = 49284,
    SPELL_RIPTIDE           = 61295,
    SPELL_HEALING_WAVE      = 49273,
    SPELL_LESSER_HEAL_WAVE  = 49276,
    SPELL_TIDAL_FORCE       = 55198,
    SPELL_MANA_TIDE_TOTEM   =   590,
    SPELL_SHA_NATURE_SWIFT  =   591,
#else
    SPELL_SHA_NATURE_SWIFT  = 16188,
    SPELL_MANA_TIDE_TOTEM   = 16190,
    SPELL_EARTH_SHIELD      = 32594,
    SPELL_HEALING_WAVE      = 25396,
    SPELL_LESSER_HEAL_WAVE  = 25420,
#endif

    /* Mage - Generic */
#ifdef LICH_KING
    SPELL_DAMPEN_MAGIC      = 43015,
    SPELL_MANA_SHIELD       = 43020,
    SPELL_MIRROR_IMAGE      = 55342,
    SPELL_ICE_BLOCK         = 45438,
#else
    SPELL_DAMPEN_MAGIC      = 33944,
    SPELL_MANA_SHIELD       = 27131,
    SPELL_ICE_BLOCK         = 27619,
#endif
    SPELL_EVOCATION         = 12051,
    SPELL_SPELLSTEAL        = 30449,
    SPELL_COUNTERSPELL      =  2139,
        
    /* Mage - Arcane */
#ifdef LICH_KING
    SPELL_FOCUS_MAGIC       = 54646,
    SPELL_ARCANE_BARRAGE    = 44425,
    SPELL_ARCANE_BLAST      = 42897,
    SPELL_ARCANE_MISSILES   = 42846,
#else
    SPELL_ARCANE_BLAST      = 24857,
    SPELL_ARCANE_MISSILES   = 38704,
    AURA_MIND_MASTERY       = 31588,
#endif
    SPELL_ARCANE_POWER      = 12042,
    AURA_ARCANE_BLAST       = 36032,
    SPELL_PRESENCE_OF_MIND  = 12043,

    /* Mage - Fire */
    SPELL_PYROBLAST         = 11366,
    SPELL_COMBUSTION        = 11129,
#ifdef LICH_KING
    SPELL_LIVING_BOMB       = 44457,
    SPELL_FIREBALL          = 42833,
    SPELL_FIRE_BLAST        = 42873,
#else
    SPELL_FIREBALL          = 38692,
    SPELL_FIRE_BLAST        = 27079,
#endif
    SPELL_DRAGONS_BREATH    = 31661,
    SPELL_BLAST_WAVE        = 11113,

    /* Mage - Frost */
    SPELL_ICY_VEINS         = 12472,
    SPELL_ICE_BARRIER       = 11426,
#ifdef LICH_KING
    SPELL_FROST_NOVA        = 42917,
    SPELL_FROSTBOLT         = 42842,
    SPELL_ICE_LANCE         = 42914,
    SPELL_DEEP_FREEZE       = 44572,
#else
    SPELL_FROST_NOVA        = 27088,
    SPELL_FROSTBOLT         = 38697,
    SPELL_ICE_LANCE         = 30455,
#endif
    SPELL_COLD_SNAP         = 11958,

    /* Warlock - Generic */
    SPELL_FEAR                 =  6215,
    SPELL_HOWL_OF_TERROR       = 17928,
#ifdef LICH_KING
    SPELL_CORRUPTION           = 47813, 
    SPELL_DEATH_COIL_W         = 47860,
    SPELL_SHADOW_BOLT          = 47809,
    SPELL_INCINERATE           = 47838,
    SPELL_IMMOLATE             = 47811,
    SPELL_SEED_OF_CORRUPTION   = 47836,
#else
    SPELL_CORRUPTION           = 27216,
    SPELL_DEATH_COIL_W         = 27223,
    SPELL_SHADOW_BOLT          = 27209,
    SPELL_INCINERATE           = 32231,
    SPELL_IMMOLATE             = 27215,
    SPELL_SEED_OF_CORRUPTION   = 27243,
#endif

    /* Warlock - Affliction */
#ifdef LICH_KING
    PASSIVE_SIPHON_LIFE        = 63108,
    SPELL_HAUNT                = 48181,
    SPELL_CURSE_OF_AGONY       = 47864,
    SPELL_DRAIN_SOUL           = 47855,
#else
    SPELL_CURSE_OF_AGONY       = 27218,
    SPELL_DRAIN_SOUL           = 27217,
    AURA_CONTAGION             = 30064,
#endif
    SPELL_UNSTABLE_AFFLICTION  = 30108,

    /* Warlock - Demonology */
    SPELL_SOUL_LINK            = 19028,
#ifdef LICH_KING
    SPELL_METAMORPHOSIS        = 59672,
    SPELL_IMMOLATION_AURA      = 50589,
    SPELL_DEMON_CHARGE         = 54785,
    SPELL_DEMONIC_EMPOWERMENT  = 47193,
    AURA_DECIMATION            = 63167,
    AURA_MOLTEN_CORE           = 71165,
    SPELL_SOUL_FIRE            = 47825,
#else
    SPELL_SOUL_FIRE            = 27211,
    SPELL_SUMMON_FELGUARD      = 30146,
#endif

    /* Warlock - Destruction */
    SPELL_SHADOWBURN           = 17877,
    SPELL_CONFLAGRATE          = 17962,
#ifdef LICH_KING
    SPELL_CHAOS_BOLT           = 50796,
    SPELL_SHADOWFURY           = 47847,
#else
    SPELL_CHAOS_BOLT           = 27209,
    SPELL_SHADOWFURY           = 30414,
#endif

    /* Druid - Generic */
    SPELL_BARKSKIN             = 22812,
    SPELL_INNERVATE            = 29166,

    /* Druid - Balance */
    SPELL_INSECT_SWARM        =  5570,
    SPELL_MOONKIN_FORM        = 24858,
#ifdef LICH_KING
    SPELL_STARFALL            = 48505,
    SPELL_TYPHOON             = 61384,
    SPELL_MOONFIRE            = 48463,
    SPELL_WRATH               = 48461,
    AURA_ECLIPSE_LUNAR        = 48518,
    SPELL_STARFIRE            = 48465,
#else
    SPELL_MOONFIRE            = 26988,
    SPELL_WRATH               = 26985,
    SPELL_STARFIRE            = 26986,
#endif

    /* Druid - Feral */
    SPELL_CAT_FORM            =   768,
#ifdef LICH_KING
    SPELL_SURVIVAL_INSTINCTS  = 61336,
    SPELL_FERAL_CHARGE_CAT    = 49376,
    SPELL_BERSERK             = 50334,
    SPELL_MANGLE_CAT          = 48566,
    SPELL_RAKE                = 48574,
    SPELL_RIP                 = 49800,
    SPELL_SAVAGE_ROAR         = 52610,
    SPELL_TIGER_FURY          = 50213,
    SPELL_CLAW                = 48570,
    SPELL_MAIM                = 49802,
#else
    SPELL_MANGLE_CAT          = 33983,
    SPELL_RAKE                = 27003,
    SPELL_RIP                 = 27008,
    SPELL_TIGER_FURY          = 9846,
    SPELL_CLAW                = 27000,
    SPELL_MAIM                = 22570,
#endif
    SPELL_MANGLE              = 33917,
    SPELL_DASH                = 33357,

    /* Druid - Restoration */
    SPELL_SWIFTMEND           = 18562,
    SPELL_TREE_OF_LIFE        = 33891,
#ifdef LICH_KING
    SPELL_WILD_GROWTH         = 48438,
    SPELL_TRANQUILITY         = 48447,
    SPELL_NOURISH             = 50464,
    SPELL_HEALING_TOUCH       = 48378,
    SPELL_REJUVENATION        = 48441,
    SPELL_REGROWTH            = 48443,
    SPELL_LIFEBLOOM           = 48451,
#else
    SPELL_TRANQUILITY         = 26983,
    SPELL_HEALING_TOUCH       = 26979,
    SPELL_REJUVENATION        = 26982,
    SPELL_REGROWTH            = 26980,
    SPELL_LIFEBLOOM           = 33763,
#endif
    SPELL_NATURE_SWIFTNESS    = 17116,
};

PlayerAI::PlayerAI(Player* player) : UnitAI(player), me(player),
    _selfSpec(Trinity::Helpers::Entity::GetPlayerSpecialization(player)),
    _isSelfHealer(Trinity::Helpers::Entity::IsPlayerHealer(player)),
    _isSelfRangedAttacker(Trinity::Helpers::Entity::IsPlayerRangedAttacker(player))
{
}

uint8 PlayerAI::GetSpec(Player const * who) const
{
    return (!who || who == me) ? _selfSpec : Trinity::Helpers::Entity::GetPlayerSpecialization(who);
}

bool PlayerAI::IsHealer(Player const * who) const
{
    return (!who || who == me) ? _isSelfHealer : Trinity::Helpers::Entity::IsPlayerHealer(who);
}

bool PlayerAI::IsRangedAttacker(Player const * who) const
{
    return (!who || who == me) ? _isSelfRangedAttacker : Trinity::Helpers::Entity::IsPlayerRangedAttacker(who);
}

PlayerAI::TargetedSpell PlayerAI::VerifySpellCast(uint32 spellId, Unit* target)
{
    // Find highest spell rank that we know
    uint32 knownRank, nextRank;
    if (me->HasSpell(spellId))
    {
        // this will save us some lookups if the player has the highest rank (expected case)
        knownRank = spellId;
        nextRank = sSpellMgr->GetNextSpellInChain(knownRank);
    }
    else
    {
        knownRank = 0;
        nextRank = sSpellMgr->GetFirstSpellInChain(spellId);
    }

    while (nextRank && me->HasSpell(nextRank))
    {
        knownRank = nextRank;
        nextRank = sSpellMgr->GetNextSpellInChain(knownRank);
    }

    if (!knownRank)
        return {};

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(knownRank);
    if (!spellInfo)
        return {};

    if (me->GetSpellHistory()->HasGlobalCooldown(spellInfo))
        return {};

    auto spell = new Spell(me, spellInfo, TRIGGERED_NONE);
    if (spell->CanAutoCast(target))
        return{ spell, target };

    delete spell;
    return {};
}

PlayerAI::TargetedSpell PlayerAI::VerifySpellCast(uint32 spellId, SpellTarget target)
{
    Unit* pTarget = nullptr;
    switch (target)
    {
        case TARGET_NONE:
            break;
        case TARGET_VICTIM:
            pTarget = me->GetVictim();
            if (!pTarget)
                return {};
            break;
        case TARGET_CHARMER:
            pTarget = me->GetCharmer();
            if (!pTarget)
                return {};
            break;
        case TARGET_SELF:
            pTarget = me;
            break;
    }

    return VerifySpellCast(spellId, pTarget);
}

PlayerAI::TargetedSpell PlayerAI::SelectSpellCast(PossibleSpellVector& spells)
{
    uint32 totalWeights = 0;
    for (PossibleSpell const& wSpell : spells)
        totalWeights += wSpell.second;

    TargetedSpell selected;
    uint32 randNum = urand(0, totalWeights - 1);
    for (PossibleSpell const& wSpell : spells)
    {
        if (selected)
        {
            delete wSpell.first.first;
            continue;
        }

        if (randNum < wSpell.second)
            selected = wSpell.first;
        else
        {
            randNum -= wSpell.second;
            delete wSpell.first.first;
        }
    }

    spells.clear();
    return selected;
}

void PlayerAI::DoCastAtTarget(TargetedSpell spell)
{
    SpellCastTargets targets;
    targets.SetUnitTarget(spell.second);
    spell.first->prepare(targets);
}

void PlayerAI::DoRangedAttackIfReady()
{
    if (me->HasUnitState(UNIT_STATE_CASTING))
        return;

    if (!me->IsAttackReady(RANGED_ATTACK))
        return;

    Unit* victim = me->GetVictim();
    if (!victim)
        return;

    uint32 rangedAttackSpell = 0;

    Item const* rangedItem = me->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED);
    if (ItemTemplate const* rangedTemplate = rangedItem ? rangedItem->GetTemplate() : nullptr)
    {
        switch (rangedTemplate->SubClass)
        {
            case ITEM_SUBCLASS_WEAPON_BOW:
            case ITEM_SUBCLASS_WEAPON_GUN:
            case ITEM_SUBCLASS_WEAPON_CROSSBOW:
                rangedAttackSpell = SPELL_SHOOT;
                break;
            case ITEM_SUBCLASS_WEAPON_THROWN:
                rangedAttackSpell = SPELL_THROW;
                break;
            case ITEM_SUBCLASS_WEAPON_WAND:
                rangedAttackSpell = SPELL_SHOOT_WAND;
                break;
        }
    }

    if (!rangedAttackSpell)
        return;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(rangedAttackSpell);
    if (!spellInfo)
        return;
 
    Spell* spell = new Spell(me, spellInfo, TRIGGERED_CAST_DIRECTLY);
    if (spell->CheckPetCast(victim) != SPELL_CAST_OK)
    {
        delete spell;
        return;
    }
 
    SpellCastTargets targets;
    targets.SetUnitTarget(victim);
    spell->prepare(targets);

    me->CastSpell(victim, rangedAttackSpell, TRIGGERED_CAST_DIRECTLY);
    me->ResetAttackTimer(RANGED_ATTACK);
}

void PlayerAI::DoAutoAttackIfReady()
{
    if (IsRangedAttacker())
        DoRangedAttackIfReady();
    else
        DoMeleeAttackIfReady();
}

void PlayerAI::CancelAllShapeshifts()
{
#ifdef LICH_KING
    std::list<AuraEffect*> const& shapeshiftAuras = me->GetAuraEffectsByType(SPELL_AURA_MOD_SHAPESHIFT);
    std::set<Aura*> removableShapeshifts;
    for (AuraEffect* auraEff : shapeshiftAuras)
    {
        Aura* aura = auraEff->GetBase();
        if (!aura)
            continue;
        SpellInfo const* auraInfo = aura->GetSpellInfo();
        if (!auraInfo)
            continue;
        if (auraInfo->HasAttribute(SPELL_ATTR0_CANT_CANCEL))
            continue;
        if (!auraInfo->IsPositive() || auraInfo->IsPassive())
            continue;
        removableShapeshifts.insert(aura);
    }

    for (Aura* aura : removableShapeshifts)
        me->RemoveOwnedAura(aura, AURA_REMOVE_BY_CANCEL);
#else
    //quick hack fix for bc :
    me->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT);
#endif
}

struct ValidTargetSelectPredicate
{
    ValidTargetSelectPredicate(UnitAI const* ai) : _ai(ai) { }
    UnitAI const* const _ai;

    bool operator()(Unit const* target) const
    {
        return _ai->CanAIAttack(target);
    }
};

bool SimpleCharmedPlayerAI::CanAIAttack(Unit const* who) const
{
    if (!me->IsValidAttackTarget(who) || who->HasBreakableByDamageCrowdControlAura())
        return false;
    if (Unit* charmer = me->GetCharmer())
        if (!charmer->IsValidAttackTarget(who))
            return false;
    return UnitAI::CanAIAttack(who);
}

Unit* SimpleCharmedPlayerAI::SelectAttackTarget() const
{
    if (Unit* charmer = me->GetCharmer())
    {
        if (UnitAI* charmerAI = charmer->GetAI())
            return charmerAI->SelectTarget(SELECT_TARGET_RANDOM, 0, ValidTargetSelectPredicate(this));
        return charmer->GetVictim();
    }
    return nullptr;
}

PlayerAI::TargetedSpell SimpleCharmedPlayerAI::SelectAppropriateCastForSpec()
{
    PossibleSpellVector spells;

    switch (me->GetClass())
    {
        case CLASS_WARRIOR:
            if (!me->IsWithinMeleeRange(me->GetVictim()))
            {
                VerifyAndPushSpellCast(spells, SPELL_CHARGE, TARGET_VICTIM, 15);
                VerifyAndPushSpellCast(spells, SPELL_INTERCEPT, TARGET_VICTIM, 10);
            }
            VerifyAndPushSpellCast(spells, SPELL_ENRAGED_REGEN, TARGET_NONE, 3);
            VerifyAndPushSpellCast(spells, SPELL_INTIMIDATING_SHOUT, TARGET_VICTIM, 4);
            if (me->GetVictim() && me->GetVictim()->HasUnitState(UNIT_STATE_CASTING))
            {
                VerifyAndPushSpellCast(spells, SPELL_PUMMEL, TARGET_VICTIM, 15);
                VerifyAndPushSpellCast(spells, SPELL_SHIELD_BASH, TARGET_VICTIM, 15);
            }
            VerifyAndPushSpellCast(spells, SPELL_BLOODRAGE, TARGET_NONE, 5);
            switch (GetSpec())
            {
                case SPEC_WARRIOR_PROTECTION:
#ifdef LICH_KING
                    VerifyAndPushSpellCast(spells, SPELL_SHOCKWAVE, TARGET_VICTIM, 3);
#endif
                    VerifyAndPushSpellCast(spells, SPELL_CONCUSSION_BLOW, TARGET_VICTIM, 5);
                    VerifyAndPushSpellCast(spells, SPELL_DISARM, TARGET_VICTIM, 2);
                    VerifyAndPushSpellCast(spells, SPELL_LAST_STAND, TARGET_NONE, 5);
                    VerifyAndPushSpellCast(spells, SPELL_SHIELD_BLOCK, TARGET_NONE, 1);
                    VerifyAndPushSpellCast(spells, SPELL_SHIELD_SLAM, TARGET_VICTIM, 4);
                    VerifyAndPushSpellCast(spells, SPELL_SHIELD_WALL, TARGET_NONE, 5);
                    VerifyAndPushSpellCast(spells, SPELL_SPELL_REFLECTION, TARGET_NONE, 3);
                    VerifyAndPushSpellCast(spells, SPELL_DEVASTATE, TARGET_VICTIM, 2);
                    VerifyAndPushSpellCast(spells, SPELL_REND, TARGET_VICTIM, 1);
                    VerifyAndPushSpellCast(spells, SPELL_THUNDER_CLAP, TARGET_VICTIM, 2);
                    VerifyAndPushSpellCast(spells, SPELL_DEMO_SHOUT, TARGET_VICTIM, 1);
                    break;
                case SPEC_WARRIOR_ARMS:
                    VerifyAndPushSpellCast(spells, SPELL_SWEEPING_STRIKES, TARGET_NONE, 2);
                    VerifyAndPushSpellCast(spells, SPELL_MORTAL_STRIKE, TARGET_VICTIM, 5);
                    VerifyAndPushSpellCast(spells, SPELL_BLADESTORM, TARGET_NONE, 10);
                    VerifyAndPushSpellCast(spells, SPELL_REND, TARGET_VICTIM, 1);
                    VerifyAndPushSpellCast(spells, SPELL_RETALIATION, TARGET_NONE, 3);
#ifdef LICH_KING
                    VerifyAndPushSpellCast(spells, SPELL_SHATTERING_THROW, TARGET_VICTIM, 3);
#endif
                    VerifyAndPushSpellCast(spells, SPELL_SWEEPING_STRIKES, TARGET_NONE, 5);
                    VerifyAndPushSpellCast(spells, SPELL_THUNDER_CLAP, TARGET_VICTIM, 1);
                    VerifyAndPushSpellCast(spells, SPELL_EXECUTE, TARGET_VICTIM, 15);
                    break;
                case SPEC_WARRIOR_FURY:
                    VerifyAndPushSpellCast(spells, SPELL_DEATH_WISH, TARGET_NONE, 10);
                    VerifyAndPushSpellCast(spells, SPELL_BLOODTHIRST, TARGET_VICTIM, 4);
                    VerifyAndPushSpellCast(spells, SPELL_DEMO_SHOUT, TARGET_VICTIM, 2);
                    VerifyAndPushSpellCast(spells, SPELL_EXECUTE, TARGET_VICTIM, 15);
#ifdef LICH_KING
                    VerifyAndPushSpellCast(spells, SPELL_HEROIC_FURY, TARGET_NONE, 5);
#endif
                    VerifyAndPushSpellCast(spells, SPELL_RECKLESSNESS, TARGET_NONE, 8);
                    VerifyAndPushSpellCast(spells, SPELL_PIERCING_HOWL, TARGET_VICTIM, 2);
                    break;
            }
            break;
        case CLASS_PALADIN:
            VerifyAndPushSpellCast(spells, SPELL_AURA_MASTERY, TARGET_NONE, 3);
            VerifyAndPushSpellCast(spells, SPELL_LAY_ON_HANDS, TARGET_CHARMER, 8);
            VerifyAndPushSpellCast(spells, SPELL_BLESSING_OF_MIGHT, TARGET_CHARMER, 8);
            VerifyAndPushSpellCast(spells, SPELL_AVENGING_WRATH, TARGET_NONE, 5);
            VerifyAndPushSpellCast(spells, SPELL_DIVINE_PROTECTION, TARGET_NONE, 4);
            VerifyAndPushSpellCast(spells, SPELL_DIVINE_SHIELD, TARGET_NONE, 2);
            VerifyAndPushSpellCast(spells, SPELL_HAMMER_OF_JUSTICE, TARGET_VICTIM, 6);
            VerifyAndPushSpellCast(spells, SPELL_HAND_OF_FREEDOM, TARGET_SELF, 3);
            VerifyAndPushSpellCast(spells, SPELL_HAND_OF_PROTECTION, TARGET_SELF, 1);
            if (Creature* creatureCharmer = ObjectAccessor::GetCreature(*me, me->GetCharmerGUID()))
            {
                if (creatureCharmer->IsDungeonBoss() || creatureCharmer->IsWorldBoss())
                    VerifyAndPushSpellCast(spells, SPELL_HAND_OF_SACRIFICE, creatureCharmer, 10);
                else
                    VerifyAndPushSpellCast(spells, SPELL_HAND_OF_PROTECTION, creatureCharmer, 3);
            }

            switch (GetSpec())
            {
                case SPEC_PALADIN_PROTECTION:
#ifdef LICH_KING
                    VerifyAndPushSpellCast(spells, SPELL_HAMMER_OF_RIGHTEOUS, TARGET_VICTIM, 3);
                    VerifyAndPushSpellCast(spells, SPELL_DIVINE_SACRIFICE, TARGET_NONE, 2);
                    VerifyAndPushSpellCast(spells, SPELL_SHIELD_OF_RIGHTEOUS, TARGET_VICTIM, 4);
#endif
                    VerifyAndPushSpellCast(spells, SPELL_JUDGEMENT_, TARGET_VICTIM, 2);
                    VerifyAndPushSpellCast(spells, SPELL_CONSECRATION, TARGET_VICTIM, 2);
                    VerifyAndPushSpellCast(spells, SPELL_HOLY_SHIELD, TARGET_NONE, 1);
                    break;
                case SPEC_PALADIN_HOLY:
                    VerifyAndPushSpellCast(spells, SPELL_HOLY_SHOCK, TARGET_CHARMER, 3);
                    VerifyAndPushSpellCast(spells, SPELL_HOLY_SHOCK, TARGET_VICTIM, 1);
                    VerifyAndPushSpellCast(spells, SPELL_FLASH_OF_LIGHT, TARGET_CHARMER, 4);
                    VerifyAndPushSpellCast(spells, SPELL_HOLY_LIGHT, TARGET_CHARMER, 3);
                    VerifyAndPushSpellCast(spells, SPELL_DIVINE_FAVOR, TARGET_NONE, 5);
                    VerifyAndPushSpellCast(spells, SPELL_DIVINE_ILLUMINATION, TARGET_NONE, 3);
                    break;
                case SPEC_PALADIN_RETRIBUTION:
                    VerifyAndPushSpellCast(spells, SPELL_CRUSADER_STRIKE, TARGET_VICTIM, 4);
#ifdef LICH_KING
                    VerifyAndPushSpellCast(spells, SPELL_DIVINE_STORM, TARGET_VICTIM, 5);
#endif
                    VerifyAndPushSpellCast(spells, SPELL_JUDGEMENT_, TARGET_VICTIM, 3);
                    VerifyAndPushSpellCast(spells, SPELL_HAMMER_OF_WRATH, TARGET_VICTIM, 5);
                    VerifyAndPushSpellCast(spells, SPELL_RIGHTEOUS_FURY, TARGET_NONE, 2);
                    break;
            }
            break;
        case CLASS_HUNTER:
            VerifyAndPushSpellCast(spells, SPELL_DETERRENCE, TARGET_NONE, 3);
            VerifyAndPushSpellCast(spells, SPELL_EXPLOSIVE_TRAP, TARGET_NONE, 1);
#ifdef LICH_KING
            VerifyAndPushSpellCast(spells, SPELL_FREEZING_ARROW, TARGET_VICTIM, 2);
#endif
            VerifyAndPushSpellCast(spells, SPELL_RAPID_FIRE, TARGET_NONE, 10);
#ifdef LICH_KING
            VerifyAndPushSpellCast(spells, SPELL_KILL_SHOT, TARGET_VICTIM, 10);
#endif
            if (me->GetVictim() && me->GetVictim()->GetPowerType() == POWER_MANA && !me->GetVictim()->GetAuraApplicationOfRankedSpell(SPELL_VIPER_STING, me->GetGUID()))
                VerifyAndPushSpellCast(spells, SPELL_VIPER_STING, TARGET_VICTIM, 5);

            switch (GetSpec())
            {
                case SPEC_HUNTER_BEAST_MASTERY:
                    VerifyAndPushSpellCast(spells, SPELL_AIMED_SHOT, TARGET_VICTIM, 2);
                    VerifyAndPushSpellCast(spells, SPELL_ARCANE_SHOT, TARGET_VICTIM, 3);
                    VerifyAndPushSpellCast(spells, SPELL_STEADY_SHOT, TARGET_VICTIM, 2);
                    VerifyAndPushSpellCast(spells, SPELL_MULTI_SHOT, TARGET_VICTIM, 2);
                    break;
                case SPEC_HUNTER_MARKSMANSHIP:
                    VerifyAndPushSpellCast(spells, SPELL_AIMED_SHOT, TARGET_VICTIM, 2);
#ifdef LICH_KING
                    VerifyAndPushSpellCast(spells, SPELL_CHIMERA_SHOT, TARGET_VICTIM, 5);
#endif
                    VerifyAndPushSpellCast(spells, SPELL_ARCANE_SHOT, TARGET_VICTIM, 3);
                    VerifyAndPushSpellCast(spells, SPELL_STEADY_SHOT, TARGET_VICTIM, 2);
                    VerifyAndPushSpellCast(spells, SPELL_READINESS, TARGET_NONE, 10);
                    VerifyAndPushSpellCast(spells, SPELL_SILENCING_SHOT, TARGET_VICTIM, 5);
                    break;
                case SPEC_HUNTER_SURVIVAL:
#ifdef LICH_KING
                    VerifyAndPushSpellCast(spells, SPELL_EXPLOSIVE_SHOT, TARGET_VICTIM, 8);
#endif
                    VerifyAndPushSpellCast(spells, SPELL_BLACK_ARROW, TARGET_VICTIM, 5);
                    VerifyAndPushSpellCast(spells, SPELL_MULTI_SHOT, TARGET_VICTIM, 3);
                    VerifyAndPushSpellCast(spells, SPELL_STEADY_SHOT, TARGET_VICTIM, 1);
                    break;
            }
            break;
        case CLASS_ROGUE:
        {
#ifdef LICH_KING
            VerifyAndPushSpellCast(spells, SPELL_DISMANTLE, TARGET_VICTIM, 8);
#endif
            VerifyAndPushSpellCast(spells, SPELL_EVASION, TARGET_NONE, 8);
            VerifyAndPushSpellCast(spells, SPELL_VANISH, TARGET_NONE, 4);
            VerifyAndPushSpellCast(spells, SPELL_BLIND, TARGET_VICTIM, 2);
            VerifyAndPushSpellCast(spells, SPELL_CLOAK_OF_SHADOWS, TARGET_NONE, 2);

            uint32 builder = 0, finisher = 0;
            switch (GetSpec())
            {
                case SPEC_ROGUE_ASSASSINATION:
                    builder = SPELL_MUTILATE, finisher = SPELL_ENVENOM;
                    VerifyAndPushSpellCast(spells, SPELL_COLD_BLOOD, TARGET_NONE, 20);
                    break;
                case SPEC_ROGUE_COMBAT:
                    builder = SPELL_SINISTER_STRIKE, finisher = SPELL_EVISCERATE;
                    VerifyAndPushSpellCast(spells, SPELL_ADRENALINE_RUSH, TARGET_NONE, 6);
                    VerifyAndPushSpellCast(spells, SPELL_BLADE_FLURRY, TARGET_NONE, 5);
#ifdef LICH_KING
                    VerifyAndPushSpellCast(spells, SPELL_KILLING_SPREE, TARGET_NONE, 25);
#endif
                    break;
                case SPEC_ROGUE_SUBLETY:
                    builder = SPELL_HEMORRHAGE, finisher = SPELL_EVISCERATE;
                    VerifyAndPushSpellCast(spells, SPELL_PREPARATION, TARGET_NONE, 10);
                    if (!me->IsWithinMeleeRange(me->GetVictim()))
                        VerifyAndPushSpellCast(spells, SPELL_SHADOWSTEP, TARGET_VICTIM, 25);
#ifdef LICH_KING
                    VerifyAndPushSpellCast(spells, SPELL_SHADOW_DANCE, TARGET_NONE, 10);
#endif
                    break;
            }

            if (Unit* victim = me->GetVictim())
            {
                if (victim->HasUnitState(UNIT_STATE_CASTING))
                    VerifyAndPushSpellCast(spells, SPELL_KICK, TARGET_VICTIM, 25);

                uint8 const cp = (me->GetComboTarget() == victim->GetGUID()) ? me->GetComboPoints() : 0;
                if (cp >= 4)
                    VerifyAndPushSpellCast(spells, finisher, TARGET_VICTIM, 10);
                if (cp <= 4)
                    VerifyAndPushSpellCast(spells, builder, TARGET_VICTIM, 5);
            }
            break;
        }
        case CLASS_PRIEST:
            VerifyAndPushSpellCast(spells, SPELL_FEAR_WARD, TARGET_SELF, 2);
            VerifyAndPushSpellCast(spells, SPELL_POWER_WORD_FORT, TARGET_CHARMER, 1);
            VerifyAndPushSpellCast(spells, SPELL_DIVINE_SPIRIT, TARGET_CHARMER, 1);
            VerifyAndPushSpellCast(spells, SPELL_SHADOW_PROTECTION, TARGET_CHARMER, 2);
#ifdef LICH_KING
            VerifyAndPushSpellCast(spells, SPELL_DIVINE_HYMN, TARGET_NONE, 5);
            VerifyAndPushSpellCast(spells, SPELL_HYMN_OF_HOPE, TARGET_NONE, 5);
#endif
            VerifyAndPushSpellCast(spells, SPELL_SHADOW_WORD_DEATH, TARGET_VICTIM, 1);
            VerifyAndPushSpellCast(spells, SPELL_PSYCHIC_SCREAM, TARGET_VICTIM, 3);
            switch (GetSpec())
            {
                case SPEC_PRIEST_DISCIPLINE:
                    VerifyAndPushSpellCast(spells, SPELL_POWER_WORD_SHIELD, TARGET_CHARMER, 3);
                    VerifyAndPushSpellCast(spells, SPELL_INNER_FOCUS, TARGET_NONE, 3);
                    VerifyAndPushSpellCast(spells, SPELL_PAIN_SUPPRESSION, TARGET_CHARMER, 15);
                    VerifyAndPushSpellCast(spells, SPELL_POWER_INFUSION, TARGET_CHARMER, 10);
#ifdef LICH_KING
                    VerifyAndPushSpellCast(spells, SPELL_PENANCE, TARGET_CHARMER, 3);
#endif
                    VerifyAndPushSpellCast(spells, SPELL_FLASH_HEAL, TARGET_CHARMER, 1);
                    break;
                case SPEC_PRIEST_HOLY:
                    VerifyAndPushSpellCast(spells, SPELL_DESPERATE_PRAYER, TARGET_NONE, 3);
#ifdef LICH_KING
                    VerifyAndPushSpellCast(spells, SPELL_GUARDIAN_SPIRIT, TARGET_CHARMER, 5);
#endif
                    VerifyAndPushSpellCast(spells, SPELL_FLASH_HEAL, TARGET_CHARMER, 1);
                    VerifyAndPushSpellCast(spells, SPELL_RENEW, TARGET_CHARMER, 3);
                    break;
                case SPEC_PRIEST_SHADOW:
                    if (!me->HasAura(SPELL_SHADOWFORM))
                    {
                        VerifyAndPushSpellCast(spells, SPELL_SHADOWFORM, TARGET_NONE, 100);
                        break;
                    }
                    if (Unit* victim = me->GetVictim())
                    {
                        if (!victim->GetAuraApplicationOfRankedSpell(SPELL_VAMPIRIC_TOUCH, me->GetGUID()))
                            VerifyAndPushSpellCast(spells, SPELL_VAMPIRIC_TOUCH, TARGET_VICTIM, 4);
                        if (!victim->GetAuraApplicationOfRankedSpell(SPELL_SHADOW_WORD_PAIN, me->GetGUID()))
                            VerifyAndPushSpellCast(spells, SPELL_SHADOW_WORD_PAIN, TARGET_VICTIM, 3);
#ifdef LICH_KING
                        if (!victim->GetAuraApplicationOfRankedSpell(SPELL_DEVOURING_PLAGUE, me->GetGUID()))
                            VerifyAndPushSpellCast(spells, SPELL_DEVOURING_PLAGUE, TARGET_VICTIM, 4);
#endif
                    }
                    VerifyAndPushSpellCast(spells, SPELL_MIND_BLAST, TARGET_VICTIM, 3);
                    VerifyAndPushSpellCast(spells, SPELL_MIND_FLAY, TARGET_VICTIM, 2);
#ifdef LICH_KING
                    VerifyAndPushSpellCast(spells, SPELL_DISPERSION, TARGET_NONE, 10);
#endif
                    break;
            }
            break;
#ifdef LICH_KING
        case CLASS_DEATH_KNIGHT:
        {
            if (!me->IsWithinMeleeRange(me->GetVictim()))
                VerifyAndPushSpellCast(spells, SPELL_DEATH_GRIP, TARGET_VICTIM, 25);
            VerifyAndPushSpellCast(spells, SPELL_STRANGULATE, TARGET_VICTIM, 15);
            VerifyAndPushSpellCast(spells, SPELL_EMPOWER_RUNE_WEAP, TARGET_NONE, 5);
            VerifyAndPushSpellCast(spells, SPELL_ICEBORN_FORTITUDE, TARGET_NONE, 15);
            VerifyAndPushSpellCast(spells, SPELL_ANTI_MAGIC_SHELL, TARGET_NONE, 10);

            bool hasFF = false, hasBP = false;
            if (Unit* victim = me->GetVictim())
            {
                if (victim->HasUnitState(UNIT_STATE_CASTING))
                    VerifyAndPushSpellCast(spells, SPELL_MIND_FREEZE, TARGET_VICTIM, 25);

                hasFF = !!victim->GetAuraApplicationOfRankedSpell(AURA_FROST_FEVER, me->GetGUID()), hasBP = !!victim->GetAuraApplicationOfRankedSpell(AURA_BLOOD_PLAGUE, me->GetGUID());
                if (hasFF && hasBP)
                    VerifyAndPushSpellCast(spells, SPELL_PESTILENCE, TARGET_VICTIM, 3);
                if (!hasFF)
                    VerifyAndPushSpellCast(spells, SPELL_ICY_TOUCH, TARGET_VICTIM, 4);
                if (!hasBP)
                    VerifyAndPushSpellCast(spells, SPELL_PLAGUE_STRIKE, TARGET_VICTIM, 4);
            }
            switch (GetSpec())
            {
                case SPEC_DEATH_KNIGHT_BLOOD:
                    VerifyAndPushSpellCast(spells, SPELL_RUNE_TAP, TARGET_NONE, 2);
                    VerifyAndPushSpellCast(spells, SPELL_HYSTERIA, TARGET_SELF, 5);
                    if (Creature* creatureCharmer = ObjectAccessor::GetCreature(*me, me->GetCharmerGUID()))
                        if (!creatureCharmer->IsDungeonBoss() && !creatureCharmer->IsWorldBoss())
                            VerifyAndPushSpellCast(spells, SPELL_HYSTERIA, creatureCharmer, 15);
                    VerifyAndPushSpellCast(spells, SPELL_HEART_STRIKE, TARGET_VICTIM, 2);
                    if (hasFF && hasBP)
                        VerifyAndPushSpellCast(spells, SPELL_DEATH_STRIKE, TARGET_VICTIM, 8);
                    VerifyAndPushSpellCast(spells, SPELL_DEATH_COIL_DK, TARGET_VICTIM, 1);
                    VerifyAndPushSpellCast(spells, SPELL_MARK_OF_BLOOD, TARGET_VICTIM, 20);
                    VerifyAndPushSpellCast(spells, SPELL_VAMPIRIC_BLOOD, TARGET_NONE, 10);
                    break;
                case SPEC_DEATH_KNIGHT_FROST:
                    if (hasFF && hasBP)
                        VerifyAndPushSpellCast(spells, SPELL_OBLITERATE, TARGET_VICTIM, 5);
                    VerifyAndPushSpellCast(spells, SPELL_HOWLING_BLAST, TARGET_VICTIM, 2);
                    VerifyAndPushSpellCast(spells, SPELL_UNBREAKABLE_ARMOR, TARGET_NONE, 10);
                    VerifyAndPushSpellCast(spells, SPELL_DEATHCHILL, TARGET_NONE, 10);
                    VerifyAndPushSpellCast(spells, SPELL_FROST_STRIKE, TARGET_VICTIM, 3);
                    VerifyAndPushSpellCast(spells, SPELL_BLOOD_STRIKE, TARGET_VICTIM, 1);
                    break;
                case SPEC_DEATH_KNIGHT_UNHOLY:
                    if (hasFF && hasBP)
                        VerifyAndPushSpellCast(spells, SPELL_SCOURGE_STRIKE, TARGET_VICTIM, 5);
                    VerifyAndPushSpellCast(spells, SPELL_DEATH_AND_DECAY, TARGET_VICTIM, 2);
                    VerifyAndPushSpellCast(spells, SPELL_ANTI_MAGIC_ZONE, TARGET_NONE, 8);
                    VerifyAndPushSpellCast(spells, SPELL_SUMMON_GARGOYLE, TARGET_VICTIM, 7);
                    VerifyAndPushSpellCast(spells, SPELL_BLOOD_STRIKE, TARGET_VICTIM, 1);
                    VerifyAndPushSpellCast(spells, SPELL_DEATH_COIL_DK, TARGET_VICTIM, 3);
                    break;
            }
            break;
        }
#endif
        case CLASS_SHAMAN:
            VerifyAndPushSpellCast(spells, SPELL_HEROISM, TARGET_NONE, 25);
            VerifyAndPushSpellCast(spells, SPELL_BLOODLUST, TARGET_NONE, 25);
            VerifyAndPushSpellCast(spells, SPELL_GROUNDING_TOTEM, TARGET_NONE, 2);
            switch (GetSpec())
            {
                case SPEC_SHAMAN_RESTORATION:
                    if (Unit* charmer = me->GetCharmer())
                        if (!charmer->GetAuraApplicationOfRankedSpell(SPELL_EARTH_SHIELD, me->GetGUID()))
                            VerifyAndPushSpellCast(spells, SPELL_EARTH_SHIELD, charmer, 2);
                    if (me->HasAura(SPELL_SHA_NATURE_SWIFT))
                        VerifyAndPushSpellCast(spells, SPELL_HEALING_WAVE, TARGET_CHARMER, 20);
                    else
                        VerifyAndPushSpellCast(spells, SPELL_LESSER_HEAL_WAVE, TARGET_CHARMER, 1);
#ifdef LICH_KING
                    VerifyAndPushSpellCast(spells, SPELL_TIDAL_FORCE, TARGET_NONE, 4);
#endif
                    VerifyAndPushSpellCast(spells, SPELL_SHA_NATURE_SWIFT, TARGET_NONE, 4);
                    VerifyAndPushSpellCast(spells, SPELL_MANA_TIDE_TOTEM, TARGET_NONE, 3);
                    break;
                case SPEC_SHAMAN_ELEMENTAL:
                    if (Unit* victim = me->GetVictim())
                    {
#ifdef LICH_KING
                        if (victim->GetAuraOfRankedSpell(SPELL_FLAME_SHOCK, GetGUID()))
                            VerifyAndPushSpellCast(spells, SPELL_LAVA_BURST, TARGET_VICTIM, 5);
                        else
#endif
                            VerifyAndPushSpellCast(spells, SPELL_FLAME_SHOCK, TARGET_VICTIM, 3);
                    }
                    VerifyAndPushSpellCast(spells, SPELL_CHAIN_LIGHTNING, TARGET_VICTIM, 2);
                    VerifyAndPushSpellCast(spells, SPELL_LIGHTNING_BOLT, TARGET_VICTIM, 1);
                    VerifyAndPushSpellCast(spells, SPELL_ELEMENTAL_MASTERY, TARGET_VICTIM, 5);
#ifdef LICH_KING
                    VerifyAndPushSpellCast(spells, SPELL_THUNDERSTORM, TARGET_NONE, 3);
#endif
                    break;
                case SPEC_SHAMAN_ENHANCEMENT:
#ifdef LICH_KING
                    if (Aura const* maelstrom = me->GetAura(AURA_MAELSTROM_WEAPON))
                        if (maelstrom->GetStackAmount() == 5)
                            VerifyAndPushSpellCast(spells, SPELL_LIGHTNING_BOLT, TARGET_VICTIM, 5);
#endif
                    VerifyAndPushSpellCast(spells, SPELL_STORMSTRIKE, TARGET_VICTIM, 3);
                    VerifyAndPushSpellCast(spells, SPELL_EARTH_SHOCK, TARGET_VICTIM, 2);
#ifdef LICH_KING
                    VerifyAndPushSpellCast(spells, SPELL_LAVA_LASH, TARGET_VICTIM, 1);
#endif
                    VerifyAndPushSpellCast(spells, SPELL_SHAMANISTIC_RAGE, TARGET_NONE, 10);
                    break;
            }
            break;
        case CLASS_MAGE:
            if (me->GetVictim() && me->GetVictim()->HasUnitState(UNIT_STATE_CASTING))
                VerifyAndPushSpellCast(spells, SPELL_COUNTERSPELL, TARGET_VICTIM, 25);
            VerifyAndPushSpellCast(spells, SPELL_DAMPEN_MAGIC, TARGET_CHARMER, 2);
            VerifyAndPushSpellCast(spells, SPELL_EVOCATION, TARGET_NONE, 3);
            VerifyAndPushSpellCast(spells, SPELL_MANA_SHIELD, TARGET_NONE, 1);
#ifdef LICH_KING
            VerifyAndPushSpellCast(spells, SPELL_MIRROR_IMAGE, TARGET_NONE, 3);
#endif
            VerifyAndPushSpellCast(spells, SPELL_SPELLSTEAL, TARGET_VICTIM, 2);
            VerifyAndPushSpellCast(spells, SPELL_ICE_BLOCK, TARGET_NONE, 1);
            VerifyAndPushSpellCast(spells, SPELL_ICY_VEINS, TARGET_NONE, 3);
            switch (GetSpec())
            {
                case SPEC_MAGE_ARCANE:
                    if (Aura* abAura = me->GetAura(AURA_ARCANE_BLAST))
                        if (abAura->GetStackAmount() >= 3)
                            VerifyAndPushSpellCast(spells, SPELL_ARCANE_MISSILES, TARGET_VICTIM, 7);
                    VerifyAndPushSpellCast(spells, SPELL_ARCANE_BLAST, TARGET_VICTIM, 5);
#ifdef LICH_KING
                    VerifyAndPushSpellCast(spells, SPELL_ARCANE_BARRAGE, TARGET_VICTIM, 1);
#endif
                    VerifyAndPushSpellCast(spells, SPELL_ARCANE_POWER, TARGET_NONE, 8);
                    VerifyAndPushSpellCast(spells, SPELL_PRESENCE_OF_MIND, TARGET_NONE, 7);
                    break;
                case SPEC_MAGE_FIRE:
#ifdef LICH_KING
                    if (me->GetVictim() && !me->GetVictim()->GetAuraApplicationOfRankedSpell(SPELL_LIVING_BOMB))
                        VerifyAndPushSpellCast(spells, SPELL_LIVING_BOMB, TARGET_VICTIM, 3);
#endif
                    VerifyAndPushSpellCast(spells, SPELL_COMBUSTION, TARGET_VICTIM, 3);
                    VerifyAndPushSpellCast(spells, SPELL_FIREBALL, TARGET_VICTIM, 2);
                    VerifyAndPushSpellCast(spells, SPELL_FIRE_BLAST, TARGET_VICTIM, 1);
                    VerifyAndPushSpellCast(spells, SPELL_DRAGONS_BREATH, TARGET_VICTIM, 2);
                    VerifyAndPushSpellCast(spells, SPELL_BLAST_WAVE, TARGET_VICTIM, 1);
                    break;
                case SPEC_MAGE_FROST:
#ifdef LICH_KING
                    VerifyAndPushSpellCast(spells, SPELL_DEEP_FREEZE, TARGET_VICTIM, 10);
                    VerifyAndPushSpellCast(spells, SPELL_FROST_NOVA, TARGET_VICTIM, 3);
#endif
                    VerifyAndPushSpellCast(spells, SPELL_FROSTBOLT, TARGET_VICTIM, 3);
                    VerifyAndPushSpellCast(spells, SPELL_COLD_SNAP, TARGET_VICTIM, 5);
                    if (me->GetVictim() && me->GetVictim()->HasAuraState(AURA_STATE_FROZEN, nullptr, me))
                        VerifyAndPushSpellCast(spells, SPELL_ICE_LANCE, TARGET_VICTIM, 5);
                    break;
            }
            break;
        case CLASS_WARLOCK:
            VerifyAndPushSpellCast(spells, SPELL_DEATH_COIL_W, TARGET_VICTIM, 2);
            VerifyAndPushSpellCast(spells, SPELL_FEAR, TARGET_VICTIM, 2);
            VerifyAndPushSpellCast(spells, SPELL_SEED_OF_CORRUPTION, TARGET_VICTIM, 4);
            VerifyAndPushSpellCast(spells, SPELL_HOWL_OF_TERROR, TARGET_NONE, 2);
            if (me->GetVictim() && !me->GetVictim()->GetAuraApplicationOfRankedSpell(SPELL_CORRUPTION, me->GetGUID()))
                VerifyAndPushSpellCast(spells, SPELL_CORRUPTION, TARGET_VICTIM, 10);
            switch (GetSpec())
            {
                case SPEC_WARLOCK_AFFLICTION:
                    if (Unit* victim = me->GetVictim())
                    {
                        VerifyAndPushSpellCast(spells, SPELL_SHADOW_BOLT, TARGET_VICTIM, 7);
                        if (!victim->GetAuraApplicationOfRankedSpell(SPELL_UNSTABLE_AFFLICTION, me->GetGUID()))
                            VerifyAndPushSpellCast(spells, SPELL_UNSTABLE_AFFLICTION, TARGET_VICTIM, 8);
#ifdef LICH_KING
                        if (!victim->GetAuraApplicationOfRankedSpell(SPELL_HAUNT, me->GetGUID()))
                            VerifyAndPushSpellCast(spells, SPELL_HAUNT, TARGET_VICTIM, 8);
#endif
                        if (!victim->GetAuraApplicationOfRankedSpell(SPELL_CURSE_OF_AGONY, me->GetGUID()))
                            VerifyAndPushSpellCast(spells, SPELL_CURSE_OF_AGONY, TARGET_VICTIM, 4);
                        if (victim->HealthBelowPct(25))
                            VerifyAndPushSpellCast(spells, SPELL_DRAIN_SOUL, TARGET_VICTIM, 100);
                    }
                    break;
                case SPEC_WARLOCK_DEMONOLOGY:
#ifdef LICH_KING
                    VerifyAndPushSpellCast(spells, SPELL_METAMORPHOSIS, TARGET_NONE, 15);
#endif
                    VerifyAndPushSpellCast(spells, SPELL_SHADOW_BOLT, TARGET_VICTIM, 7);
#ifdef LICH_KING
                    if (me->HasAura(AURA_DECIMATION))
                        VerifyAndPushSpellCast(spells, SPELL_SOUL_FIRE, TARGET_VICTIM, 100);
      
                    if (me->HasAura(SPELL_METAMORPHOSIS))
                    {
                        VerifyAndPushSpellCast(spells, SPELL_IMMOLATION_AURA, TARGET_NONE, 30);
                        if (!me->IsWithinMeleeRange(me->GetVictim()))
                            VerifyAndPushSpellCast(spells, SPELL_DEMON_CHARGE, TARGET_VICTIM, 20);
                    }
#endif

                    if (me->GetVictim() && !me->GetVictim()->GetAuraApplicationOfRankedSpell(SPELL_IMMOLATE, me->GetGUID()))
                        VerifyAndPushSpellCast(spells, SPELL_IMMOLATE, TARGET_VICTIM, 5);
#ifdef LICH_KING
                    if (me->HasAura(AURA_MOLTEN_CORE))
                        VerifyAndPushSpellCast(spells, SPELL_INCINERATE, TARGET_VICTIM, 10);
#endif
                    break;
                case SPEC_WARLOCK_DESTRUCTION:
                    if (me->GetVictim() && !me->GetVictim()->GetAuraApplicationOfRankedSpell(SPELL_IMMOLATE, me->GetGUID()))
                        VerifyAndPushSpellCast(spells, SPELL_IMMOLATE, TARGET_VICTIM, 8);
                    if (me->GetVictim() && me->GetVictim()->GetAuraApplicationOfRankedSpell(SPELL_IMMOLATE, me->GetGUID()))
                        VerifyAndPushSpellCast(spells, SPELL_CONFLAGRATE, TARGET_VICTIM, 8);
                    VerifyAndPushSpellCast(spells, SPELL_SHADOWFURY, TARGET_VICTIM, 5);
                    VerifyAndPushSpellCast(spells, SPELL_CHAOS_BOLT, TARGET_VICTIM, 10);
                    VerifyAndPushSpellCast(spells, SPELL_SHADOWBURN, TARGET_VICTIM, 3);
                    VerifyAndPushSpellCast(spells, SPELL_INCINERATE, TARGET_VICTIM, 7);
                    break;
            }
            break;
        case CLASS_DRUID:
            VerifyAndPushSpellCast(spells, SPELL_INNERVATE, TARGET_CHARMER, 5);
            VerifyAndPushSpellCast(spells, SPELL_BARKSKIN, TARGET_NONE, 5);
            switch (GetSpec())
            {
                case SPEC_DRUID_RESTORATION:
                    if (!me->HasAura(SPELL_TREE_OF_LIFE))
                    {
                        CancelAllShapeshifts();
                        VerifyAndPushSpellCast(spells, SPELL_TREE_OF_LIFE, TARGET_NONE, 100);
                        break;
                    }
                    VerifyAndPushSpellCast(spells, SPELL_TRANQUILITY, TARGET_NONE, 10);
                    VerifyAndPushSpellCast(spells, SPELL_NATURE_SWIFTNESS, TARGET_NONE, 7);
                    if (Creature* creatureCharmer = ObjectAccessor::GetCreature(*me, me->GetCharmerGUID()))
                    {
#ifdef LICH_KING
                        VerifyAndPushSpellCast(spells, SPELL_NOURISH, creatureCharmer, 5);
                        VerifyAndPushSpellCast(spells, SPELL_WILD_GROWTH, creatureCharmer, 5);
#endif
                        if (!creatureCharmer->GetAuraApplicationOfRankedSpell(SPELL_REJUVENATION, me->GetGUID()))
                            VerifyAndPushSpellCast(spells, SPELL_REJUVENATION, creatureCharmer, 8);
                        if (!creatureCharmer->GetAuraApplicationOfRankedSpell(SPELL_REGROWTH, me->GetGUID()))
                            VerifyAndPushSpellCast(spells, SPELL_REGROWTH, creatureCharmer, 8);
                        uint8 lifebloomStacks = 0;
                        if (Aura const* lifebloom = creatureCharmer->GetAura(SPELL_LIFEBLOOM, me->GetGUID()))
                            lifebloomStacks = lifebloom->GetStackAmount();
                        if (lifebloomStacks < 3)
                            VerifyAndPushSpellCast(spells, SPELL_LIFEBLOOM, creatureCharmer, 5);
                        if (creatureCharmer->GetAuraApplicationOfRankedSpell(SPELL_REJUVENATION) ||
                            creatureCharmer->GetAuraApplicationOfRankedSpell(SPELL_REGROWTH))
                            VerifyAndPushSpellCast(spells, SPELL_SWIFTMEND, creatureCharmer, 10);
                        if (me->HasAura(SPELL_NATURE_SWIFTNESS))
                            VerifyAndPushSpellCast(spells, SPELL_HEALING_TOUCH, creatureCharmer, 100);
                    }
                    break;
                case SPEC_DRUID_BALANCE:
                {
                    if (!me->HasAura(SPELL_MOONKIN_FORM))
                    {
                        CancelAllShapeshifts();
                        VerifyAndPushSpellCast(spells, SPELL_MOONKIN_FORM, TARGET_NONE, 100);
                        break;
                    }
#ifdef LICH_KING
                    uint32 const mainAttackSpell = me->HasAura(AURA_ECLIPSE_LUNAR) ? SPELL_STARFIRE : SPELL_WRATH;
                    VerifyAndPushSpellCast(spells, SPELL_STARFALL, TARGET_NONE, 20);
                    VerifyAndPushSpellCast(spells, mainAttackSpell, TARGET_VICTIM, 10);
#else
                    VerifyAndPushSpellCast(spells, SPELL_WRATH, TARGET_VICTIM, 10);
#endif
                    if (me->GetVictim() && !me->GetVictim()->GetAuraApplicationOfRankedSpell(SPELL_INSECT_SWARM, me->GetGUID()))
                        VerifyAndPushSpellCast(spells, SPELL_INSECT_SWARM, TARGET_VICTIM, 7);
                    if (me->GetVictim() && !me->GetVictim()->GetAuraApplicationOfRankedSpell(SPELL_MOONFIRE, me->GetGUID()))
                        VerifyAndPushSpellCast(spells, SPELL_MOONFIRE, TARGET_VICTIM, 5);
#ifdef LICH_KING
                    if (me->GetVictim() && me->GetVictim()->HasUnitState(UNIT_STATE_CASTING))
                        VerifyAndPushSpellCast(spells, SPELL_TYPHOON, TARGET_NONE, 15);
#endif
                    break;
                }
                case SPEC_DRUID_FERAL:
                    if (!me->HasAura(SPELL_CAT_FORM))
                    {
                        CancelAllShapeshifts();
                        VerifyAndPushSpellCast(spells, SPELL_CAT_FORM, TARGET_NONE, 100);
                        break;
                    }
#ifdef LICH_KING
                    VerifyAndPushSpellCast(spells, SPELL_BERSERK, TARGET_NONE, 20);
                    VerifyAndPushSpellCast(spells, SPELL_SURVIVAL_INSTINCTS, TARGET_NONE, 15);
#endif
                    VerifyAndPushSpellCast(spells, SPELL_TIGER_FURY, TARGET_NONE, 15);
                    VerifyAndPushSpellCast(spells, SPELL_DASH, TARGET_NONE, 5);
                    if (Unit* victim = me->GetVictim())
                    {
                        uint8 const cp = (me->GetComboTarget() == victim->GetGUID()) ? me->GetComboPoints() : 0;
                        if (victim->HasUnitState(UNIT_STATE_CASTING) && cp >= 1)
                            VerifyAndPushSpellCast(spells, SPELL_MAIM, TARGET_VICTIM, 25);
#ifdef LICH_KING
                        if (!me->IsWithinMeleeRange(victim))
                            VerifyAndPushSpellCast(spells, SPELL_FERAL_CHARGE_CAT, TARGET_VICTIM, 25);
#endif
                        if (cp >= 4)
                            VerifyAndPushSpellCast(spells, SPELL_RIP, TARGET_VICTIM, 50);
                        if (cp <= 4)
                        {
                            VerifyAndPushSpellCast(spells, SPELL_MANGLE_CAT, TARGET_VICTIM, 10);
                            VerifyAndPushSpellCast(spells, SPELL_CLAW, TARGET_VICTIM, 5);
                            if (!victim->GetAuraApplicationOfRankedSpell(SPELL_RAKE, me->GetGUID()))
                                VerifyAndPushSpellCast(spells, SPELL_RAKE, TARGET_VICTIM, 8);
#ifdef LICH_KING
                            if (!me->HasAura(SPELL_SAVAGE_ROAR))
                                VerifyAndPushSpellCast(spells, SPELL_SAVAGE_ROAR, TARGET_NONE, 15);
#endif
                        }
                    }
                    break;
            }
            break;
    }

    return SelectSpellCast(spells);
}

static const float CASTER_CHASE_DISTANCE = 28.0f;
void SimpleCharmedPlayerAI::UpdateAI(const uint32 diff)
{
    Creature* charmer = me->GetCharmer() ? me->GetCharmer()->ToCreature() : nullptr;
    if (!charmer)
        return;

    //kill self if charm aura has infinite duration
    if (charmer->IsInEvadeMode())
    {
        Player::AuraEffectList const& auras = me->GetAuraEffectsByType(SPELL_AURA_MOD_CHARM);
        for (auto aura : auras)
            if (aura->GetCasterGUID() == charmer->GetGUID() && aura->GetBase()->IsPermanent())
            {
                me->KillSelf();
                return;
            }
    }

    if (charmer->IsInCombat())
    {
        Unit* target = me->GetVictim();
        if (!target || !CanAIAttack(target))
        {
            target = SelectAttackTarget();
            if (!target || !CanAIAttack(target))
            {
                if (!_isFollowing)
                {
                    _isFollowing = true;
                    me->AttackStop();
                    me->CastStop();
                    me->StopMoving();
                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MoveFollow(charmer, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                }
                return;
            }
            _isFollowing = false;

            if (IsRangedAttacker())
            {
                _chaseCloser = !me->IsWithinLOSInMap(target, LINEOFSIGHT_ALL_CHECKS, VMAP::ModelIgnoreFlags::M2);
                if (_chaseCloser)
                    AttackStart(target);
                else
                    AttackStartCaster(target, CASTER_CHASE_DISTANCE);
            }
            else
                AttackStart(target);
            _forceFacing = true;
        }

        if (me->IsStopped() && !me->HasUnitState(UNIT_STATE_CANNOT_TURN))
        {
            float targetAngle = me->GetAbsoluteAngle(target);
            if (_forceFacing || fabs(me->GetOrientation() - targetAngle) > 0.4f)
            {
                me->SetFacingTo(targetAngle);
                _forceFacing = false;
            }
        }

        if (_castCheckTimer <= diff)
        {
            if (me->HasUnitState(UNIT_STATE_CASTING))
                _castCheckTimer = 0;
            else
            {
                if (IsRangedAttacker())
                { // chase to zero if the target isn't in line of sight
                    bool inLOS = me->IsWithinLOSInMap(target, LINEOFSIGHT_ALL_CHECKS, VMAP::ModelIgnoreFlags::M2);
                    if (_chaseCloser != !inLOS)
                    {
                        _chaseCloser = !inLOS;
                        if (_chaseCloser)
                            AttackStart(target);
                        else
                            AttackStartCaster(target, CASTER_CHASE_DISTANCE);
                    }
                }
                if (TargetedSpell shouldCast = SelectAppropriateCastForSpec())
                    DoCastAtTarget(shouldCast);
                _castCheckTimer = 500;
            }
        }
        else
            _castCheckTimer -= diff;

        DoAutoAttackIfReady();
    }
    else if (!_isFollowing)
    {
        _isFollowing = true;
        me->AttackStop();
        me->CastStop();
        if (me->GetMotionMaster()->GetCurrentMovementGeneratorType() != FOLLOW_MOTION_TYPE)
        {
            me->StopMoving();
            me->GetMotionMaster()->Clear();
            me->GetMotionMaster()->MoveFollow(charmer, PET_FOLLOW_DIST, me->GetFollowAngle());
        }
    }
}

void SimpleCharmedPlayerAI::OnCharmed(bool isNew)
{
    if (me->IsCharmed())
    {
        me->CastStop();
        me->AttackStop();
        me->StopMoving();
        me->GetMotionMaster()->Clear();
        me->GetMotionMaster()->MovePoint(0, me->GetPosition(), false); // force re-sync of current position for all clients
    }
    else
    {
        me->CastStop();
        me->AttackStop();
        // @todo only voluntary movement (don't cancel stuff like death grip or charge mid-animation)
        me->GetMotionMaster()->Clear();
        me->StopMoving();
    }
    PlayerAI::OnCharmed(isNew);
}
