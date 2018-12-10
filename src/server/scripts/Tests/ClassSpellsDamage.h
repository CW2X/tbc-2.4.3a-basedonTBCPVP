#ifndef H_CLASS_SPELLS_DAMAGE
#define H_CLASS_SPELLS_DAMAGE

#include "Define.h"

namespace ClassSpellsDamage
{
    namespace Druid
    {
        enum Druid
        {
            // Balance
            ENTANGLING_ROOTS_RNK_1_TOTAL = 20,
            ENTANGLING_ROOTS_RNK_2_TOTAL = 50,
            ENTANGLING_ROOTS_RNK_3_TOTAL = 90,
            ENTANGLING_ROOTS_RNK_4_TOTAL = 140,
            ENTANGLING_ROOTS_RNK_5_TOTAL = 200,
            ENTANGLING_ROOTS_RNK_6_TOTAL = 270,
            ENTANGLING_ROOTS_RNK_7_TOTAL = 351,
            ENTANGLING_ROOTS_RNK_7_TICK = 39,
            HURRICANE_RNK_1_TICK = 74,
            HURRICANE_RNK_2_TICK = 101,
            HURRICANE_RNK_3_TICK = 135,
            HURRICANE_RNK_4_TICK = 206,
            INSECT_SWARM_RNK_1_TOTAL = 108,
            INSECT_SWARM_RNK_2_TOTAL = 192,
            INSECT_SWARM_RNK_3_TOTAL = 300,
            INSECT_SWARM_RNK_4_TOTAL = 432,
            INSECT_SWARM_RNK_5_TOTAL = 594,
            INSECT_SWARM_RNK_6_TICK = 132,
            INSECT_SWARM_RNK_6_TOTAL = 792,
            MOONFIRE_RNK_1_MIN = 9,
            MOONFIRE_RNK_1_MAX = 12,
            MOONFIRE_RNK_1_TICK = 9,
            MOONFIRE_RNK_2_MIN = 17,
            MOONFIRE_RNK_2_MAX = 21,
            MOONFIRE_RNK_2_TICK = 32,
            MOONFIRE_RNK_3_MIN = 30,
            MOONFIRE_RNK_3_MAX = 37,
            MOONFIRE_RNK_3_TICK = 52,
            MOONFIRE_RNK_4_MIN = 47,
            MOONFIRE_RNK_4_MAX = 55,
            MOONFIRE_RNK_4_TICK = 80,
            MOONFIRE_RNK_5_MIN = 70,
            MOONFIRE_RNK_5_MAX = 82,
            MOONFIRE_RNK_5_TICK = 124,
            MOONFIRE_RNK_6_MIN = 91,
            MOONFIRE_RNK_6_MAX = 108,
            MOONFIRE_RNK_6_TICK = 164,
            MOONFIRE_RNK_7_MIN = 117,
            MOONFIRE_RNK_7_MAX = 137,
            MOONFIRE_RNK_7_TICK = 212,
            MOONFIRE_RNK_8_MIN = 143,
            MOONFIRE_RNK_8_MAX = 168,
            MOONFIRE_RNK_8_TICK = 264,
            MOONFIRE_RNK_9_MIN = 172,
            MOONFIRE_RNK_9_MAX = 200,
            MOONFIRE_RNK_9_TICK = 320,
            MOONFIRE_RNK_10_MIN = 205,
            MOONFIRE_RNK_10_MAX = 238,
            MOONFIRE_RNK_10_TICK = 384,
            MOONFIRE_RNK_11_MIN = 238,
            MOONFIRE_RNK_11_MAX = 237,
            MOONFIRE_RNK_11_TICK = 444,
            MOONFIRE_RNK_12_MIN = 305,
            MOONFIRE_RNK_12_MAX = 357,
            MOONFIRE_RNK_12_TICK = 150,
            STARFIRE_RNK_1_MIN = 95,
            STARFIRE_RNK_1_MAX = 115,
            STARFIRE_RNK_2_MIN = 146,
            STARFIRE_RNK_2_MAX = 177,
            STARFIRE_RNK_3_MIN = 212,
            STARFIRE_RNK_3_MAX = 253,
            STARFIRE_RNK_4_MIN = 293,
            STARFIRE_RNK_4_MAX = 348,
            STARFIRE_RNK_5_MIN = 378,
            STARFIRE_RNK_5_MAX = 445,
            STARFIRE_RNK_6_MIN = 463,
            STARFIRE_RNK_6_MAX = 543,
            STARFIRE_RNK_7_MIN = 514,
            STARFIRE_RNK_7_MAX = 603,
            STARFIRE_RNK_8_MIN = 550,
            STARFIRE_RNK_8_MAX = 647,
            THORNS_RNK_1_TICK = 3,
            THORNS_RNK_2_TICK = 6,
            THORNS_RNK_3_TICK = 9,
            THORNS_RNK_4_TICK = 12,
            THORNS_RNK_5_TICK = 15,
            THORNS_RNK_6_TICK = 18,
            THORNS_RNK_7_TICK = 25,
            WRATH_RNK_1_MIN = 13,
            WRATH_RNK_1_MAX = 16,
            WRATH_RNK_2_MIN = 28,
            WRATH_RNK_2_MAX = 33,
            WRATH_RNK_3_MIN = 48,
            WRATH_RNK_3_MAX = 57,
            WRATH_RNK_4_MIN = 69,
            WRATH_RNK_4_MAX = 79,
            WRATH_RNK_5_MIN = 108,
            WRATH_RNK_5_MAX = 123,
            WRATH_RNK_6_MIN = 148,
            WRATH_RNK_6_MAX = 167,
            WRATH_RNK_7_MIN = 198,
            WRATH_RNK_7_MAX = 221,
            WRATH_RNK_8_MIN = 248,
            WRATH_RNK_8_MAX = 277,
            WRATH_RNK_9_MIN = 292,
            WRATH_RNK_9_MAX = 327,
            WRATH_RNK_10_MIN = 383,
            WRATH_RNK_10_MAX = 432,

            // Feral
            CLAW_RNK_6 = 190,
            FEROCIOUS_BITE_RNK_6_CP_1_MIN = 259,
            FEROCIOUS_BITE_RNK_6_CP_1_MAX = 292,
            FEROCIOUS_BITE_RNK_6_CP_2_MIN = 428,
            FEROCIOUS_BITE_RNK_6_CP_2_MAX = 461,
            FEROCIOUS_BITE_RNK_6_CP_3_MIN = 597,
            FEROCIOUS_BITE_RNK_6_CP_3_MAX = 630,
            FEROCIOUS_BITE_RNK_6_CP_4_MIN = 766,
            FEROCIOUS_BITE_RNK_6_CP_4_MAX = 799,
            FEROCIOUS_BITE_RNK_6_CP_5_MIN = 935,
            FEROCIOUS_BITE_RNK_6_CP_5_MAX = 968,
            LACERATE_RNK_1 = 31,
            LACERATE_RNK_1_BLEED = 155,
            LACERATE_RNK_1_BLEED_TICK = 31,
            MAUL_RNK_8 = 176,
            POUNCE_RNK_4_TOTAL = 600,
            POUNCE_RNK_4_TICK = 100,
            RAKE_RNK_5 = 78,
            RAKE_RNK_5_BLEED = 108,
            RAKE_RNK_5_BLEED_TICK = 36,
            RAVAGE_RNK_5 = 514,
            SHRED_RNK_7 = 405,
            SWIPE_RNK_6 = 84,

            // Restoration
            HEALING_TOUCH_RNK_1_MIN = 40,
            HEALING_TOUCH_RNK_1_MAX = 55,
            HEALING_TOUCH_RNK_2_MIN = 94,
            HEALING_TOUCH_RNK_2_MAX = 119,
            HEALING_TOUCH_RNK_3_MIN = 204,
            HEALING_TOUCH_RNK_3_MAX = 253,
            HEALING_TOUCH_RNK_4_MIN = 376,
            HEALING_TOUCH_RNK_4_MAX = 459,
            HEALING_TOUCH_RNK_5_MIN = 589,
            HEALING_TOUCH_RNK_5_MAX = 712,
            HEALING_TOUCH_RNK_6_MIN = 762,
            HEALING_TOUCH_RNK_6_MAX = 914,
            HEALING_TOUCH_RNK_7_MIN = 958,
            HEALING_TOUCH_RNK_7_MAX = 1143,
            HEALING_TOUCH_RNK_8_MIN = 1225,
            HEALING_TOUCH_RNK_8_MAX = 1453,
            HEALING_TOUCH_RNK_9_MIN = 1545,
            HEALING_TOUCH_RNK_9_MAX = 1826,
            HEALING_TOUCH_RNK_10_MIN = 1923,
            HEALING_TOUCH_RNK_10_MAX = 2263,
            HEALING_TOUCH_RNK_11_MIN = 2303,
            HEALING_TOUCH_RNK_11_MAX = 2714,
            HEALING_TOUCH_RNK_12_MIN = 2401,
            HEALING_TOUCH_RNK_12_MAX = 2827,
            HEALING_TOUCH_RNK_13_MIN = 2715,
            HEALING_TOUCH_RNK_13_MAX = 3206,
            LIFEBLOOM_RNK_1_BURST = 600,
            LIFEBLOOM_RNK_1_TOTAL = 273,
            LIFEBLOOM_RNK_1_TICK = 39,
            REGROWTH_RNK_1_MIN = 93,
            REGROWTH_RNK_1_MAX = 107,
            REGROWTH_RNK_1_TOTAL = 98,
            REGROWTH_RNK_10_MIN = 1253,
            REGROWTH_RNK_10_MAX = 1394,
            REGROWTH_RNK_10_TOTAL = 1274,
            REGROWTH_RNK_10_TICK = 182,
            REJUVENATION_RNK_1_TOTAL = 32,
            REJUVENATION_RNK_2_TOTAL = 56,
            REJUVENATION_RNK_3_TOTAL = 116,
            REJUVENATION_RNK_4_TOTAL = 180,
            REJUVENATION_RNK_5_TOTAL = 244,
            REJUVENATION_RNK_6_TOTAL = 304,
            REJUVENATION_RNK_7_TOTAL = 388,
            REJUVENATION_RNK_8_TOTAL = 488,
            REJUVENATION_RNK_9_TOTAL = 608,
            REJUVENATION_RNK_10_TOTAL = 756,
            REJUVENATION_RNK_11_TOTAL = 888,
            REJUVENATION_RNK_12_TOTAL = 932,
            REJUVENATION_RNK_13_TOTAL = 1060,
            REJUVENATION_RNK_13_TICK = 265,
            TRANQUILITY_RNK_1_TICK = 364,
            TRANQUILITY_RNK_2_TICK = 530,
            TRANQUILITY_RNK_3_TICK = 785,
            TRANQUILITY_RNK_4_TICK = 1119,
            TRANQUILITY_RNK_5_TICK = 1518,
        };

        // These are the sunstrider formulas deduced from videos and informations from WoWWiki/WoWhead, may not be 100% accurate and may be subject to changes
        uint32 GetCatMinDmg(uint32 level = 70);
        uint32 GetCatMaxDmg(uint32 level = 70);
    };

    namespace Hunter
    {
        enum Hunter
        {
        };
    };

    namespace Mage
    {
        enum Mage
        {
            // Arcane
            ARCANE_BLAST_RNK_1_MIN = 648,
            ARCANE_BLAST_RNK_1_MAX = 753,
            ARCANE_EXPLOSION_RNK_8_MIN = 377,
            ARCANE_EXPLOSION_RNK_8_MAX = 407,
            ARCANE_MISSILES_RNK_10_TICK = 260,
            ARCANE_MISSILES_RNK_10_LVL_70_TICK = 264,
            // Fire
            FIRE_BLAST_RNK_9_MIN = 664,
            FIRE_BLAST_RNK_9_MAX = 786,
            FIREBALL_RNK_13_MIN_LVL_70 = 649,
            FIREBALL_RNK_13_MAX_LVL_70 = 821,
            FIREBALL_RNK_13_TICK = 21,
            FLAMESTRIKE_RNK_7_MIN_LVL_70 = 480,
            FLAMESTRIKE_RNK_7_MAX_LVL_70 = 585,
            FLAMESTRIKE_RNK_7_TICK = 106,
            MOLTEN_ARMOR_RNK_1 = 75,
            SCORCH_RNK_9_MIN = 305,
            SCORCH_RNK_9_MAX = 361,
            // Frost
            BLIZZARD_RNK_7_TICK = 184,
            CONE_OF_COLD_RNK_6_MIN = 418,
            CONE_OF_COLD_RNK_6_MAX = 457,
            FROST_NOVA_RNK_5_MIN = 100,
            FROST_NOVA_RNK_5_MAX = 113,
            FROSTBOLT_RNK_13_MIN = 597,
            FROSTBOLT_RNK_13_MAX = 644,
            FROSTBOLT_RNK_13_LVL_70_MIN = 600,
            FROSTBOLT_RNK_13_LVL_70_MAX = 647,
            ICE_LANCE_RNK_1_MIN = 173,
            ICE_LANCE_RNK_1_MAX = 200,
        };
    };

    namespace Paladin
    {
        enum Paladin
        {
            // Holy
            BLESSING_OF_WISDOM_RNK_7_MIN = 41,
            CONSECRATION_RNK_6_TOTAL = 512,
            EXORCISM_RNK_7_MIN = 626,
            EXORCISM_RNK_7_MAX = 698,
            FLASH_OF_LIGHT_RNK_7_MIN = 458,
            FLASH_OF_LIGHT_RNK_7_MAX = 513,
            HAMMER_OF_WRATH_RNK_4_MIN = 672, // 665 + 3.5 per level
            HAMMER_OF_WRATH_RNK_4_MAX = 742, // 735 + 3.5 per level
            HOLY_LIGHT_RNK_11_MIN = 2196,
            HOLY_LIGHT_RNK_11_MAX = 2446,
            HOLY_SHOCK_RNK_5_MIN_DAMAGE = 721,
            HOLY_SHOCK_RNK_5_MAX_DAMAGE = 779,
            HOLY_SHOCK_RNK_5_MIN_HEAL = 913,
            HOLY_SHOCK_RNK_5_MAX_HEAL = 987,
            HOLY_WRATH_RNK_3_MIN = 637,
            HOLY_WRATH_RNK_3_MAX = 748,
            SEAL_OF_RIGHTEOUSNESS_TICK = 41,
            JUDGEMENT_OF_RIGHTEOUSNESS_RNK_9_MIN = 225,
            JUDGEMENT_OF_RIGHTEOUSNESS_RNK_9_MAX = 246,
            SEAL_OF_VENGEANCE_DOT = 150,
            JUDGEMENT_OF_VENGEANCE_PER_STACK = 120,

            // Protection
            DEVOTION_AURA_RNK_8 = 861,
            HOLY_SHIELD_RNK_4_TICK = 155,

            // Retribution
            BLESSING_OF_MIGHT_RNK_8 = 220,
            JUDGEMENT_OF_BLOOD_RNK_1_MIN = 331, //295 + 6.1 per level 
            JUDGEMENT_OF_BLOOD_RNK_1_MAX = 361,
        };
    };

    namespace Priest
    {
        enum Priest
        {
            // Discipline
            MANA_BURN_RNK_7_MIN = 1021,
            MANA_BURN_RNK_7_MAX = 1079,
            POWER_WORD_SHIELD_RNK_12 = 1265,
            STARSHARDS_RNK_8_TICK = 157,
            STARSHARDS_RNK_8_TOTAL = 785,
            FEEDBACK_BURN_RNK_6 = 165,
            // Holy
            BINDING_HEAL_RNK_1_MIN = 1053,
            BINDING_HEAL_RNK_1_MAX = 1350,
            CHASTISE_RNK_6_MIN = 370,
            CHASTISE_RNK_6_MAX = 430,
            CIRCLE_OF_HEALING_RNK_5_MIN = 409,
            CIRCLE_OF_HEALING_RNK_5_MAX = 453,
            DESPERATE_PRAYER_RNK_8_MIN = 1637,
            DESPERATE_PRAYER_RNK_8_MAX = 1924,
            FLASH_HEAL_RNK_9_MIN = 1116,
            FLASH_HEAL_RNK_9_MAX = 1295,
            GREATER_HEAL_RNK_7_MIN = 2414,
            GREATER_HEAL_RNK_7_MAX = 2803,
            HEAL_RNK_4_MIN = 712,
            HEAL_RNK_4_MAX = 805,
            HEAL_RNK_4_MIN_LVL_70 = 734,
            HEAL_RNK_4_MAX_LVL_70 = 827,
            HOLY_FIRE_RNK_9_MIN = 426,
            HOLY_FIRE_RNK_9_MAX = 537,
            HOLY_FIRE_RNK_9_TICK = 33,
            HOLY_FIRE_RNK_9_TOTAL = 165,
            HOLY_NOVA_RNK_7_MIN = 242,
            HOLY_NOVA_RNK_7_MAX = 282,
            HOLY_NOVA_RNK_7_HEAL_MIN = 384,
            HOLY_NOVA_RNK_7_HEAL_MAX = 447,
            HOLY_NOVA_RNK_7_MIN_LVL_70 = 244,
            HOLY_NOVA_RNK_7_MAX_LVL_70 = 284,
            HOLY_NOVA_RNK_7_HEAL_MIN_LVL_70 = 386,
            HOLY_NOVA_RNK_7_HEAL_MAX_LVL_70 = 448,
            LESSER_HEAL_RNK_3_MIN = 135,
            LESSER_HEAL_RNK_3_MAX = 158,
            LESSER_HEAL_RNK_3_MIN_LVL_70 = 143,
            LESSER_HEAL_RNK_3_MAX_LVL_70 = 166,
            LIGHTWELL_RNK_4_TICK = 787,
            LIGHTWELL_RNK_4_TOTAL = 2361,
            PRAYER_OF_HEALING_RNK_6_MIN = 1251,
            PRAYER_OF_HEALING_RNK_6_MAX = 1322,
            PRAYER_OF_MENDING_RNK_1 = 800,
            RENEW_RNK_12_TICK = 222,
            RENEW_RNK_12_TOTAL = 1110,
            SMITE_RNK_10_MIN = 549,
            SMITE_RNK_10_MAX = 616,
            // Shadow
            DEVOURING_PLAGUE_RNK_7_TOTAL = 1216,
            DEVOURING_PLAGUE_RNK_7_TICK = 152,
            MIND_BLAST_RNK_11_MIN = 711,
            MIND_BLAST_RNK_11_MAX = 752,
            MIND_FLAY_RNK_7_TICK = 176,
            MIND_FLAY_RNK_7_TOTAL = 528,
            SHADOW_WORD_DEATH_RNK_2_MIN = 572,
            SHADOW_WORD_DEATH_RNK_2_MAX = 664,
            SHADOW_WORD_PAIN_RNK_10_TICK = 206,
            SHADOW_WORD_PAIN_RNK_10_TOTAL = 1236,
            SHADOWFIEND_RNK_1_MIN = 110,
            SHADOWFIEND_RNK_1_MAX = 121,
            TOUCH_OF_WEAKNESS_RNK_7 = 80,
            VAMPIRIC_TOUCH_RNK_3_TICK = 130,
            VAMPIRIC_TOUCH_RNK_3_TOTAL = 650,
            SHADOW_GUARD_RNK_7 = 130,
        };
    };

    namespace Rogue
    {
        enum Rogue
        {
        };
    };

    namespace Shaman
    {
        enum Shaman
        {
            // Elemental
            EARTH_SHOCK_RNK_8_MIN = 658,
            EARTH_SHOCK_RNK_8_MAX = 693,
            EARTH_SHOCK_RNK_8_MIN_LVL_70 = 661,
            EARTH_SHOCK_RNK_8_MAX_LVL_70 = 696,
        };
    };

    namespace Warlock
    {
        enum Warlock
        {
            // Affliction
            CORRUPTION_RNK_8_TICK = 150,
            CORRUPTION_RNK_8_TOTAL = 900,
            CURSE_OF_AGONY_RNK_7_TOTAL = 1356,
            CURSE_OF_DOOM_RNK_2 = 4200,
            CURSE_OF_WEAKNESS_RNK_7 = 350,
            DARK_PACT_RNK_4 = 700,
            DEATH_COIL_RNK_4 = 519,
            DEATH_COIL_RNK_4_LVL_70 = 525,
            DRAIN_LIFE_RNK_8_TICK = 108,
            DRAIN_MANA_RNK_6_TICK = 200,
            DRAIN_SOUL_RNK_5_TICK = 124,
            DRAIN_SOUL_RNK_5_TOTAL = 620,
            LIFE_TAP_RNK_7 = 580,
            SEED_OF_CORRUPTION_RNK_1_MIN = 1110,
            SEED_OF_CORRUPTION_RNK_1_MAX = 1290,
            SEED_OF_CORRUPTION_RNK_1_TOTAL = 1044,
            SEED_OF_CORRUPTION_RNK_1_TICK = 174,
            SIHPON_LIFE_RNK_6_TICK = 63,
            UNSTABLE_AFFLICTION_RNK_3_DISPELLED = 1575,
            UNSTABLE_AFFLICTION_RNK_3_TICK = 175,
            UNSTABLE_AFFLICTION_RNK_3_TOTAL = 1050,

            // Demonology
            CREATE_HEALTHSTONE_RNK_1_HP_RESTORED = 100,
            CREATE_HEALTHSTONE_RNK_2_HP_RESTORED = 250,
            CREATE_HEALTHSTONE_RNK_3_HP_RESTORED = 500,
            CREATE_HEALTHSTONE_RNK_4_HP_RESTORED = 800,
            CREATE_HEALTHSTONE_RNK_5_HP_RESTORED = 1200,
            CREATE_HEALTHSTONE_RNK_6_HP_RESTORED = 2080,
            HEALTH_FUNNEL_RNK_8_HP_COST = 99,
            HEALTH_FUNNEL_RNK_8_HP_PER_TICK = 65,
            HEALTH_FUNNEL_RNK_8_HEAL_PER_TICK = 188,

            // Destruction
            CONFLAGRATE_RNK_6_MIN = 579,
            CONFLAGRATE_RNK_6_MAX = 721,
            HELLFIRE_RNK_4_TICK = 306, // Self & enemies
            HELLFIRE_RNK_4_TICK_LVL_70 = 307, // Self & enemies
            IMMOLATE_RNK_9 = 327,
            IMMOLATE_RNK_9_LVL_70 = 331,
            IMMOLATE_RNK_9_DOT = 615,
            INCINERATE_RNK_2_MIN = 444,
            INCINERATE_RNK_2_MAX = 514,
            INCINERATE_RNK_2_IMMOLATE_BONUS_MIN = 111,
            INCINERATE_RNK_2_IMMOLATE_BONUS_MAX = 129,
            RAIN_OF_FIRE_RNK_5_TICK = 303,
            RAIN_OF_FIRE_RNK_5_TICK_LVL_70 = 303,
            RAIN_OF_FIRE_RNK_5_TOTAL = 1212,
            SEARING_PAIN_RNK_8_MIN = 270,
            SEARING_PAIN_RNK_8_MAX = 320,
            SHADOW_BOLT_RNK_11_MIN = 544,
            SHADOW_BOLT_RNK_11_MAX = 607,
            SHADOWBURN_RNK_8_MIN = 597,
            SHADOWBURN_RNK_8_MAX = 665,
            SHADOWFURY_RNK_3_MIN = 612,
            SHADOWFURY_RNK_3_MAX = 728,
            SOUL_FIRE_RNK_4_MIN = 1003,
            SOUL_FIRE_RNK_4_MAX = 1257,
        };
    };

    namespace Warrior
    {
        enum Warrior
        {
            // Arms
            HAMSTRING_RNK_4 = 63,
            HEROIC_STRIKE_RNK_10 = 176,
            MOCKING_BLOW_RNK_6 = 114,
            OVERPOWER_RNK_4 = 35,
            REND_RNK_8 = 182,
            REND_RNK_8_TICK = 26,
            THUNDER_CLAP_RNK_7 = 123,
            // Fury
            CLEAVE_RNK_6 = 70,
            EXECUTE_RNK_7 = 925,
            EXECUTE_RNK_7_RAGE = 21,
            INTERCEPT_RNK_5 = 105,
            PUMMEL_RNK_2 = 50,
            SLAM_RNK_6 = 140,
            // Protection
            REVENGE_RNK_8_MIN = 414,
            REVENGE_RNK_8_MAX = 506,
            SHIELD_BASH_RNK_4 = 63,
        };
    };
};

#endif // H_CLASS_SPELLS_DAMAGE