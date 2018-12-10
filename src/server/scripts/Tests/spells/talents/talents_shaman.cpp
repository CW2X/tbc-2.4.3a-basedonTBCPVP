#include "../../ClassSpellsDamage.h"
#include "../../ClassSpellsCoeff.h"

class ConvectionTest : public TestCaseScript
{
public:
	ConvectionTest() : TestCaseScript("talents shaman convection") { }

    //Reduces the mana cost of your Shock, Lightning Bolt and Chain Lightning spells by 10 % .
	class ConvectionTestImpt : public TestCase
	{
	public:


		void Test() override
		{
			TestPlayer* player = SpawnRandomPlayer(CLASS_SHAMAN);

			uint32 const lightningBoltMana = 300;
			uint32 const chainLightningMana = 760;
			uint32 const earthShocMana = 535;
			uint32 const flameShockMana = 500;
			uint32 const frostShockMana = 525;

            TEST_POWER_COST(player, ClassSpells::Shaman::LIGHTNING_BOLT_RNK_12, POWER_MANA, lightningBoltMana);
			TEST_POWER_COST(player, ClassSpells::Shaman::LIGHTNING_BOLT_RNK_12, POWER_MANA, lightningBoltMana);
			TEST_POWER_COST(player, ClassSpells::Shaman::CHAIN_LIGHTNING_RNK_6, POWER_MANA, chainLightningMana);
			TEST_POWER_COST(player, ClassSpells::Shaman::EARTH_SHOCK_RNK_8, POWER_MANA, earthShocMana);
			TEST_POWER_COST(player, ClassSpells::Shaman::FLAME_SHOCK_RNK_7, POWER_MANA, flameShockMana);
			TEST_POWER_COST(player, ClassSpells::Shaman::FROST_SHOCK_RNK_5, POWER_MANA, frostShockMana);
		}
	};

	std::unique_ptr<TestCase> GetTest() const override
	{
		return std::make_unique<ConvectionTestImpt>();
	}
};

class ConcussionTest : public TestCaseScript
{
public:
	ConcussionTest() : TestCaseScript("talents shaman concussion") { }

    //Increases the damage done by your Lightning Bolt, Chain Lightning and Shock spells by 5%.
	class ConcussionTestImpt : public TestCase
	{
		void Test() override
		{
			TestPlayer* player = SpawnRandomPlayer(CLASS_SHAMAN);
			// Lightning Bolt rank 12
			uint32 const lightningBoltMinDamage = 563;
			uint32 const lightningBoltMaxDamage = 634;

			// Chain Lightning rank 6
			uint32 const chainLightningMinDamage = 734;
			uint32 const chainLightningMaxDamage = 838;

			// Earth Shock rank 8
			uint32 const earthShockMinDamage = 658;
			uint32 const earthShockMaxDamage = 692;

			// Flame Shock rank 7
			uint32 const flameShockMinDamage = 377;
			uint32 const flameShockMinDoT = 420; //blazeit

												 // Frost Shock rank 5
			uint32 const frostShockMinDamage = 640;
			uint32 const frostShockMaxDamage = 676;

			Creature* dummyTarget = SpawnCreature();

            //TODO: replace magic numbers in this test
            SECTION("Improved damage", STATUS_WIP, [&] {
                //Test improved damage 5%
                TEST_DIRECT_SPELL_DAMAGE(player, dummyTarget, ClassSpells::Shaman::LIGHTNING_BOLT_RNK_12, lightningBoltMinDamage * 1.05f, lightningBoltMaxDamage * 1.05f, false);
                TEST_DIRECT_SPELL_DAMAGE(player, dummyTarget, ClassSpells::Shaman::CHAIN_LIGHTNING_RNK_6, chainLightningMinDamage * 1.05f, chainLightningMaxDamage * 1.05f, false);
                TEST_DIRECT_SPELL_DAMAGE(player, dummyTarget, ClassSpells::Shaman::EARTH_SHOCK_RNK_8, earthShockMinDamage * 1.05f, earthShockMaxDamage * 1.05f, false);
                TEST_DIRECT_SPELL_DAMAGE(player, dummyTarget, ClassSpells::Shaman::FLAME_SHOCK_RNK_7, flameShockMinDamage * 1.05f, flameShockMinDoT * 1.05f, false);
                TEST_DIRECT_SPELL_DAMAGE(player, dummyTarget, ClassSpells::Shaman::FROST_SHOCK_RNK_5, frostShockMinDamage * 1.05f, frostShockMaxDamage * 1.05f, false);
            });
		}
	};

	std::unique_ptr<TestCase> GetTest() const override
	{
		return std::make_unique<ConcussionTestImpt>();
	}
};

class AncestralKnowledgeTest : public TestCaseScript
{
public:
	AncestralKnowledgeTest() : TestCaseScript("talents shaman ancestral_knowledge") { }

	class AncestralKnowledgeTestImpt : public TestCase
	{
	public:


		void Test() override
		{
			TestPlayer* player = SpawnRandomPlayer(CLASS_SHAMAN);
			uint32 const startMana = player->GetMaxPower(POWER_MANA);
			LearnTalent(player, Talents::Shaman::ANCESTRAL_KNOWLEDGE_RNK_5);
			uint32 const expectedMana = startMana * 1.05f;
			TEST_ASSERT(Between<float>(player->GetMaxPower(POWER_MANA), expectedMana - 1, expectedMana + 1));
		}
	};

	std::unique_ptr<TestCase> GetTest() const override
	{
		return std::make_unique<AncestralKnowledgeTestImpt>();
	}
};

class EnhancingTotemsTest : public TestCaseScript
{
public:
	EnhancingTotemsTest() : TestCaseScript("talents shaman enhancing_totems") { }

    //Increases the effect of your Strength of Earth and Grace of Air Totems by 15%.
	class EnhancingTotemsTestImpt : public TestCase
	{
		void Test() override
		{
            SECTION("WIP", STATUS_WIP, [&] {
                TestPlayer* player = SpawnRandomPlayer(CLASS_SHAMAN);

                float const startAgi = player->GetStat(STAT_AGILITY);
                float const startStr = player->GetStat(STAT_STRENGTH);

                LearnTalent(player, Talents::Shaman::ENHANCING_TOTEMS_RNK_2);

                //TODO: replace magic numbers
                float const expectedAgi = startAgi + std::floor(77 * 1.15f);
                float const expectedStr = startAgi + std::floor(86 * 1.15f);

                //TODO: summon totems

                ASSERT_INFO("actual agi: %f - expected %f", player->GetStat(STAT_AGILITY), expectedAgi);
                TEST_ASSERT(Between<float>(player->GetStat(STAT_AGILITY), expectedAgi - 1, expectedAgi + 1));
                ASSERT_INFO("actual str: %f - expected %f", player->GetStat(STAT_STRENGTH), expectedStr);
                TEST_ASSERT(Between<float>(player->GetStat(STAT_STRENGTH), expectedStr - 1, expectedStr + 1));
            });
		}
	};

	std::unique_ptr<TestCase> GetTest() const override
	{
		return std::make_unique<EnhancingTotemsTestImpt>();
	}
};

class ToughnessTest : public TestCaseScript
{
public:
	ToughnessTest() : TestCaseScript("talents shaman toughness") { }

	class ToughnessTestImpt : public TestCase
	{
	public:
		void Test() override
		{
            //Fixme: 
            // - What to do with returns? Fail test?
            // - Comparison of floats with GetMaxDuration result
            SECTION("main", STATUS_WIP, [&] {
                TestPlayer* player = SpawnRandomPlayer(CLASS_SHAMAN);

                // Armor effect
                uint32 const startArmor = player->GetArmor();
                uint32 const expectedArmor = startArmor * 1.1f;

                LearnTalent(player, Talents::Shaman::TOUGHNESS_RNK_5);
                TEST_ASSERT(Between<float>(player->GetArmor(), expectedArmor - 1, expectedArmor + 1));

                // Slowing effects
                player->AddAura(ClassSpells::Warrior::HAMSTRING_RNK_4, player);
                Aura* hamstring = player->GetAura(ClassSpells::Warrior::HAMSTRING_RNK_4);
                if (!hamstring)
                    return;
                TEST_ASSERT(hamstring->GetMaxDuration() < 7.6f);
                player->RemoveAurasDueToSpell(ClassSpells::Warrior::HAMSTRING_RNK_4);

                player->AddAura(ClassSpells::Rogue::CRIPPLING_POISON_II_RNK_2, player);
                Aura* poison = player->GetAura(ClassSpells::Rogue::CRIPPLING_POISON_II_RNK_2);
                if (!poison)
                    return;
                TEST_ASSERT(poison->GetMaxDuration() < 6.1f);
                player->RemoveAurasDueToSpell(ClassSpells::Rogue::CRIPPLING_POISON_II_RNK_2);

                player->AddAura(ClassSpells::Shaman::FROST_SHOCK_RNK_5, player);
                Aura* shock = player->GetAura(ClassSpells::Shaman::FROST_SHOCK_RNK_5);
                if (!shock)
                    return;
                TEST_ASSERT(shock->GetMaxDuration() < 4.1f);
                player->RemoveAurasDueToSpell(ClassSpells::Shaman::FROST_SHOCK_RNK_5);
            });
		}
	};

	std::unique_ptr<TestCase> GetTest() const override
	{
		return std::make_unique<ToughnessTestImpt>();
	}
};

class WeaponMasteryTest : public TestCaseScript
{
public:
	WeaponMasteryTest() : TestCaseScript("talents shaman weapon_mastery") { }

	class WeaponMasteryTestImpt : public TestCase
	{
		void Test() override
		{
            SECTION("WIP", STATUS_WIP, [&] {
                TestPlayer* player = SpawnRandomPlayer(CLASS_SHAMAN);
                LearnTalent(player, Talents::Shaman::DUAL_WIELD_RNK_1);
                player->SetSkill(44, 0, 350, 350); // Axe
                EQUIP_NEW_ITEM(player, 34331); // Rising Tide MH
                player->SetSkill(473, 0, 350, 350); // Fist weapon
                EQUIP_NEW_ITEM(player, 34203); // Grip of Mannoroth OH

                uint32 const minOH = 113;
                uint32 const maxMH = 313;

                //Creature* dummyTarget = SpawnCreature();
                // Test regular damage
                //TestDamage(player, dummyTarget, minOH, maxMH);

                // Test improved 10%
                LearnTalent(player, Talents::Shaman::WEAPON_MASTERY_RNK_5);
                //TestDamage(player, dummyTarget, minOH * 1.1f, maxMH * 1.1f);
            });
		}
	};

	std::unique_ptr<TestCase> GetTest() const override
	{
		return std::make_unique<WeaponMasteryTestImpt>();
	}
};

class ImprovedHealingWaveTest : public TestCaseScript
{
public:
	ImprovedHealingWaveTest() : TestCaseScript("talents shaman improved_healing_wave") { }

	class ImprovedHealingWaveTestImpt : public TestCase
	{
		void Test() override
		{
            SECTION("WIP", STATUS_WIP, [&] {
                TestPlayer* player = SpawnRandomPlayer(CLASS_SHAMAN);

                float const startCastTime = 3.0f;
                float const expectedCastTime = 2.5f;

                // Test before
                LearnTalent(player, Talents::Shaman::IMPROVED_HEALING_WAVE_RNK_5);
                // Test after
            });
		}
	};

	std::unique_ptr<TestCase> GetTest() const override
	{
		return std::make_unique<ImprovedHealingWaveTestImpt>();
	}
};

class TidalFocusTest : public TestCaseScript
{
public:
	TidalFocusTest() : TestCaseScript("talents shaman tidal_focus") { }

	class TidalFocusTestImpt : public TestCase
	{
		void Test() override
		{
            SECTION("WIP", STATUS_WIP, [&] {
                TestPlayer* player = SpawnRandomPlayer(CLASS_SHAMAN);

                uint32 const startCH = 540;
                uint32 const startHW = 720;
                uint32 const startLHW = 440;

                uint32 const expectedCH = 513;
                uint32 const expectedHW = 684;
                uint32 const expectedLHW = 418;

                // Test regular
                player->SetMaxPower(POWER_MANA, startCH);
                //TEST_ASSERT(player->CanCastSpell(ClassSpells::Shaman::CHAIN_HEAL_RNK_5, player);
                player->SetMaxPower(POWER_MANA, startHW);
                //TEST_ASSERT(player->CanCastSpell(ClassSpells::Shaman::HEALING_WAVE_RNK_12, player);
                player->SetMaxPower(POWER_MANA, startLHW);
                //TEST_ASSERT(player->CanCastSpell(ClassSpells::Shaman::LESSER_HEALING_WAVE_RNK_7, player);

                // Test improved
                LearnTalent(player, Talents::Shaman::TIDAL_FOCUS_RNK_5);
                player->SetMaxPower(POWER_MANA, expectedCH);
                //TEST_ASSERT(player->CanCastSpell(ClassSpells::Shaman::CHAIN_HEAL_RNK_5, player);
                player->SetMaxPower(POWER_MANA, expectedHW);
                //TEST_ASSERT(player->CanCastSpell(ClassSpells::Shaman::HEALING_WAVE_RNK_12, player);
                player->SetMaxPower(POWER_MANA, expectedLHW);
                //TEST_ASSERT(player->CanCastSpell(ClassSpells::Shaman::LESSER_HEALING_WAVE_RNK_7, player);
            });
		}
	};

	std::unique_ptr<TestCase> GetTest() const override
	{
		return std::make_unique<TidalFocusTestImpt>();
	}
};

class NaturesBlessingTest : public TestCaseScript
{
public:
	NaturesBlessingTest() : TestCaseScript("talents shaman natures_blessing") { }

	class NaturesBlessingTestImpt : public TestCase
	{
	public:


		void Test() override
		{
			TestPlayer* player = SpawnRandomPlayer(CLASS_SHAMAN);

			uint32 const intellect = player->GetStat(STAT_INTELLECT);
			uint32 const startHealing = player->SpellBaseHealingBonusDone(SPELL_SCHOOL_MASK_ALL);
			uint32 const startDamage = player->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_ALL);

			uint32 const expectedHealing = startHealing + intellect * 0.3;
			uint32 const expectedDamage = startDamage + intellect * 0.3;

			// Test improved
			LearnTalent(player, Talents::Shaman::NATURES_BLESSING_RNK_3);
			TEST_ASSERT(Between<float>(player->SpellBaseHealingBonusDone(SPELL_SCHOOL_MASK_ALL), expectedHealing - 1, expectedHealing + 1));
			TEST_ASSERT(Between<float>(player->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_ALL), expectedDamage - 1, expectedDamage + 1));
		}
	};

	std::unique_ptr<TestCase> GetTest() const override
	{
		return std::make_unique<NaturesBlessingTestImpt>();
	}
};

class ImprovedChainHealTest : public TestCaseScript
{
public:
    ImprovedChainHealTest() : TestCaseScript("talents shaman improved_chain_heal") {}

	class ImprovedChainHealTestImpt : public TestCase
	{
		void Test() override
		{
			TestPlayer* player = SpawnRandomPlayer(CLASS_SHAMAN);

			uint32 const minChainHeal = 833;
			uint32 const maxChainHeal = 950;

            float const improvedChainHealFactor = 1.2; //IMPROVED_CHAIN_HEAL_RNK_2

			// Test regular
            TEST_DIRECT_HEAL(player, player, ClassSpells::Shaman::CHAIN_HEAL_RNK_5, minChainHeal, maxChainHeal, false);

			// Test improved
			LearnTalent(player, Talents::Shaman::IMPROVED_CHAIN_HEAL_RNK_2);
            TEST_DIRECT_HEAL(player, player, ClassSpells::Shaman::CHAIN_HEAL_RNK_5, minChainHeal * improvedChainHealFactor, maxChainHeal * improvedChainHealFactor, false);

            SECTION("WIP", STATUS_WIP, [&] {
                // TODO: test bounces : spawn 2 players, group players, each bounce 50% less
            });
		}
	};

	std::unique_ptr<TestCase> GetTest() const override
	{
		return std::make_unique<ImprovedChainHealTestImpt>();
	}
};

void AddSC_test_talents_shaman()
{
	// Total: 10/61
	// Elemental: 2/20
	new ConvectionTest();
	new ConcussionTest();
	// Enhancement: 4/21
	new AncestralKnowledgeTest();
	new EnhancingTotemsTest();
	new ToughnessTest();
	new WeaponMasteryTest();
	// Restoration: 4/20
	new ImprovedHealingWaveTest();
	new TidalFocusTest();
	new NaturesBlessingTest();
	new ImprovedChainHealTest();
}
