#include "TestCase.h"
#include "TestPlayer.h"
#include "World.h"

class NextMeleeHitTest : public TestCaseScript
{
public:
    NextMeleeHitTest() : TestCaseScript("spells next_melee_hit") { }

    class NextMeleeHitTestImpt : public TestCase
    {
        void Test() override
        {
            SECTION("WIP", STATUS_WIP, [&] {
                // 1 - No rage is expended on parry or dodge
                //For example:
                // http://wowwiki.wikia.com/wiki/Heroic_Strike?oldid=1513476
                // TODO: No Rage is expended if Heroic Strike is parried or dodged. Rage is expended if Heroic Strike hits or is blocked.

                // 2 - outgoing parry / dodge generates rage
                //Proof: https ://youtu.be/X_JFagC8klM?t=5m20s
            });
        }
    };

    std::unique_ptr<TestCase> GetTest() const override
    {
        return std::make_unique<NextMeleeHitTestImpt>();
    }
};

class EnvironmentalTrapTest : public TestCaseScript
{
public:
    EnvironmentalTrapTest() : TestCaseScript("environmental traps") { }

    class EnvironmentalTrapImpl : public TestCase
    {
    public:


        void Test() override
        {
            TestPlayer* p = SpawnRandomPlayer(CLASS_WARRIOR, 1); //lvl 1
            Wait(Seconds(1));
            //use player to spawn object... we don't have another good and simple way to spawn a gameoject right now
            GameObject* obj = p->SummonGameObject(2061, p->GetPosition(), G3D::Quat(), 0); //campfire
            TEST_ASSERT(obj != nullptr);
            obj->SetOwnerGUID(ObjectGuid::Empty); //remove owner, environmental traps don't have any

            // Now, player can resist the damage... loop for a while to make sure he has very little chance of resisting all
            // Just test if player has taken any damage
            uint32 initialHealth = p->GetHealth();
            bool lostHealth = false;
            for (uint32 i = 0; i < 30 && !lostHealth; i++)
            {
                Wait(Seconds(1));
                uint32 newHealth = p->GetHealth();
                if (newHealth < initialHealth)
                    lostHealth = true;
            }
            TEST_ASSERT(lostHealth);
        }
    };

    std::unique_ptr<TestCase> GetTest() const override
    {
        return std::make_unique<EnvironmentalTrapImpl>();
    }
};

class StackingRulesTest : public TestCaseScript
{
public:
    StackingRulesTest() : TestCaseScript("spells rules stacking") { }

    class StackingTestImpl : public TestCase
    {
        TestPlayer* p1;
        TestPlayer* p2;
        Creature* creatureTarget;
        Creature* creatureTarget2;
        Creature* creatureTarget3;

        void TestStack(uint32 id1, uint32 id2 = 0, bool _not = false, bool sameCaster = false, Unit* target = nullptr)
        {
            if (!id2)
                id2 = id1;

            Unit* caster1 = p1;
            Unit* caster2 = sameCaster ? p1 : p2;
            if (!target)
                target = creatureTarget;

            target->RemoveAllAuras();
            Aura* a;
            a = caster1->AddAura(id1, target);
            ASSERT_INFO("Failed to add aura %u", id1);
            TEST_ASSERT(a != nullptr);
            a = caster2->AddAura(id2, target);
            ASSERT_INFO("Failed to add aura %u", id2);
            TEST_ASSERT(a != nullptr);

            if (id1 == id2)
            {
                uint32 auraCount = target->GetAuraCount(id1);
                if (_not)
                {
                    ASSERT_INFO("Testing if %u NOT stacks with %u. Aura %u has wrong count %u (should be 1)", id1, id2, id1, auraCount);
                    TEST_ASSERT(auraCount == 1)
                } 
                else 
                {
                    ASSERT_INFO("Testing if %u stacks with %u. Aura %u has wrong count %u (should be 2)", id1, id2, id1, auraCount);
                    TEST_ASSERT(target->GetAuraCount(id1) == 2)
                }
            }
            else 
            {
                bool hasAura1 = target->HasAura(id1);
                bool hasAura2 = target->HasAura(id2);
                if (_not)
                {
                    ASSERT_INFO("Testing if %u NOT stacks with %u. Target should not have aura %u", id1, id2, id1);
                    TEST_ASSERT(!hasAura1);
                    ASSERT_INFO("Testing if %u NOT stacks with %u. Target should have aura %u", id1, id2, id2);
                    TEST_ASSERT(hasAura2);
                }
                else
                {
                    ASSERT_INFO("Testing if %u stacks with %u. Target should have aura %u", id1, id2, id1);
                    TEST_ASSERT(hasAura1);
                    ASSERT_INFO("Testing if %u stacks with %u. Target should have aura %u", id1, id2, id2);
                    TEST_ASSERT(hasAura2);
                }
            }
        }

        void TestNotStack(uint32 id1, uint32 id2 = 0, bool sameCaster = false, Unit* target = nullptr)
        {
            TestStack(id1, id2, true, sameCaster, target);
        }

        void Test() override
        {
            p1 = SpawnRandomPlayer(RACE_HUMAN);
            p2 = SpawnRandomPlayer(RACE_HUMAN);
            creatureTarget = SpawnCreature();
            creatureTarget2 = SpawnCreature();
            creatureTarget3 = SpawnCreature();
            
            //well fed buffs
            uint32 const herbBakedEgg = 19705;
            uint32 const windblossomBerries = 18191;
            uint32 const rumseyRum = 25804;
            uint32 const blackenedBasilick = 33263;
            uint32 const fishermanFeast = 33257;
            TestNotStack(herbBakedEgg, windblossomBerries);
            TestNotStack(windblossomBerries, rumseyRum);
            TestNotStack(rumseyRum, blackenedBasilick);
            TestNotStack(blackenedBasilick, fishermanFeast);
            TestNotStack(fishermanFeast, herbBakedEgg);

            //endurance group
            uint32 const scrollOfStamina = 8099;
            uint32 const PWFortitude = 1243;
            uint32 const prayerFortitude = 21562;
            TestNotStack(scrollOfStamina, PWFortitude);
            TestNotStack(PWFortitude, prayerFortitude);
            TestNotStack(prayerFortitude, scrollOfStamina);

            //spirit group
            uint32 const prayerSpirit = 27681;
            uint32 const divineSpirit = 14752;
            uint32 const scrollSpirit = 8112;
            TestNotStack(prayerSpirit, divineSpirit);
            TestNotStack(divineSpirit, scrollSpirit);
            TestNotStack(scrollSpirit, prayerSpirit);

            //intellect group
            uint32 const arcaneBrilliance = 23028;
            uint32 const arcaneIntellect = 1459;
            uint32 const scrollIntellect = 8096;
            TestNotStack(arcaneBrilliance, arcaneIntellect);
            TestNotStack(arcaneIntellect, scrollIntellect);
            TestNotStack(scrollIntellect, arcaneBrilliance);

            //blessings
            uint32 const wisdomBlessing = 19742;
            uint32 const greaterWisdom = 25894;
            uint32 const mightBlessing = 19740;
            uint32 const greaterMight = 25782;
            uint32 const kingsBlessing = 20217;
            uint32 const greaterKings = 25898;
            uint32 const lightBlessing = 19977;
            uint32 const greaterLight = 25890;
            // test blessing vs greater
            TestNotStack(wisdomBlessing, greaterWisdom);
            TestNotStack(mightBlessing, greaterMight);
            TestNotStack(kingsBlessing, greaterKings);
            TestNotStack(lightBlessing, greaterLight);
            // test between different blessings when same caster
            TestNotStack(wisdomBlessing, mightBlessing, true);
            TestNotStack(mightBlessing, greaterKings, true);
            TestNotStack(greaterLight, greaterWisdom, true);

            //haste group
            uint32 const heroism = 32182;
            uint32 const bloodlust = 41185;
            uint32 const powerInfusion = 10060;
            uint32 const icyVeins = 12472;

            TestNotStack(heroism, bloodlust);
            TestNotStack(bloodlust, powerInfusion);
            TestNotStack(powerInfusion, icyVeins);
            TestNotStack(icyVeins, heroism);
            TestNotStack(heroism, powerInfusion);
            TestNotStack(bloodlust, icyVeins);

            //earth shield
            TestNotStack(974, 974);

            //armor debuffs
            uint32 const sunderArmor = 7386;
            uint32 const exposeArmor = 8647;
            uint32 const crystalYield = 15235;
            TestNotStack(sunderArmor, exposeArmor);
            TestNotStack(sunderArmor, crystalYield);
            TestNotStack(exposeArmor, crystalYield);

            //totems http://www.twentytotems.com/2007/11/stackable-totems-guide_27.html
            GroupPlayer(p1, p2); //needed for area auras testing
            uint32 const healingStreamTotemEffect = 5672;
            TestStack(healingStreamTotemEffect, 0, false, false, p2);
            uint32 const manaStreamTotemEffect = 5677; 
            TestNotStack(manaStreamTotemEffect, 0, false, p2);
            uint32 const tranquilAirEffect = 25909;
            TestNotStack(tranquilAirEffect, 0, false, p2);
            uint32 const wrathOfAirEffect = 2895;
            TestNotStack(wrathOfAirEffect, 0, false, p2);
            uint32 const natureResistanceEffect = 10596;
            TestNotStack(natureResistanceEffect, 0, false, p2);
            uint32 const windWallEffect = 15108;
            TestNotStack(windWallEffect, 0, false, p2);
            uint32 const graceOfAirEffect = 8836;
            TestNotStack(graceOfAirEffect, 0, false, p2);
            uint32 const fireResistanceTotem = 8185;
            TestNotStack(fireResistanceTotem, 0, false, p2);
            uint32 const frostResistanceTotem = 8182;
            TestNotStack(frostResistanceTotem, 0, false, p2);
            uint32 const stoneSkinTotem = 8072;
            TestNotStack(stoneSkinTotem, 0, false, p2);
            uint32 const earthBindTotemEffect = 3600;
            TestNotStack(earthBindTotemEffect, 0, false, p2);
            uint32 const strenghtOfEarthTotem = 8076;
            TestNotStack(strenghtOfEarthTotem, 0, false, p2);
            uint32 const totemOfWrath = 30708;
            TestStack(totemOfWrath, 0, false, false, p2); //does stack!

            //misc spells
            TestStack(15407); //Mind flay
            TestStack(6074); //Renew
            TestNotStack(33876, 33878); //Mangle cat + bear
            TestNotStack(19977); //blessing of light
            TestNotStack(31944); // Doomfire DoT - only one per target

            SECTION("WIP", STATUS_WIP, [&] {
                //More powerful spells?
            });
             
            SECTION("Immunity stack", STATUS_PASSING, [&] {
                //Need friendly casters to get aura stacking
                creatureTarget2->AddAura(38112, creatureTarget);
                creatureTarget3->AddAura(38112, creatureTarget);
                TEST_ASSERT(creatureTarget->GetAuraCount(38112) == 2);
                creatureTarget->RemoveAura(38112, creatureTarget3->GetGUID());
                TEST_ASSERT(creatureTarget->GetAuraCount(38112) == 1);
                //check if we're still immune 
                TEST_ASSERT(creatureTarget->IsImmunedToDamage(SPELL_SCHOOL_MASK_FIRE));
            });
        }
    };

    std::unique_ptr<TestCase> GetTest() const override
    {
        return std::make_unique<StackingTestImpl>();
    }
};

class SpellPositivity : public TestCaseScript
{
public:
    SpellPositivity() : TestCaseScript("spells positivity") { }

    class SpellPositivityImpl : public TestCase
    {
        void Test() override
        {
            static std::list<std::pair<uint32, bool>> spellList = {
                { 17624, false }, // Petrification
                { 24740, true  }, // Wisp Costume
                { 36897, false }, // Transporter Malfunction (race mutation to horde)
                { 36899, false }, // Transporter Malfunction (race mutation to alliance)
                { 36900, false }, // Soul Split: Evil!
                { 36901, false }, // Soul Split: Good
                { 36893, false }, // Transporter Malfunction (decrease size case)
                { 36895, false }, // Transporter Malfunction (increase size case)
                { 42792, false }, // Recently Dropped Flag (prevent cancel)
                { 7268,  false }, // Arcane Missile
                { 38044, true  }, // Surge (SSC Vashj fight)
                //Add much more spells here as we fix them
            };

            SECTION("Positivity", [&] {
                for (auto itr : spellList)
                {
                    uint32 const spellId = itr.first;
                    bool const positive = itr.second;

                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
                    ASSERT_INFO("Failed to find spell %u", spellId);
                    TEST_ASSERT(spellInfo != nullptr);

                    ASSERT_INFO("Spell %u is positivity is %u when it should be %u", spellId, uint32(positive), uint32(spellInfo->IsPositive()));
                    TEST_ASSERT(spellInfo->IsPositive() == positive);
                }
            });
        }
    };

    std::unique_ptr<TestCase> GetTest() const override
    {
        return std::make_unique<SpellPositivityImpl>();
    }
};

/* "spells delayed stacks"
Test a spell applying aura with a fly time and stacks
*/
class SpellDelayedStacks : public TestCase
{
    void Test() override
    {
        uint32 const STACK_COUNT = 5;
        uint32 const BUFF_ID = 30531; //apply aura on nearby entries with 17256, has a fly time and does stack

        //Player* p = SpawnRandomPlayer();
        Creature* caster = SpawnCreature(17256); //hellfire channeler
        Creature* other  = SpawnCreature(17256); //hellfire channeler

        SECTION("stacks", STATUS_KNOWN_BUG, [&] 
        {
            for(uint32 i = 0; i < STACK_COUNT; i++)
                TEST_CAST(caster, caster, BUFF_ID, SPELL_CAST_OK, TRIGGERED_FULL_MASK);

            WaitNextUpdate();
            Wait(5000); //let it fly

            TEST_AURA_STACK(other, BUFF_ID, STACK_COUNT);
        });
    }
};

/* "spells targets aoetrigger"
Check some spell triggering aoe
*/
class SpellTargetsAoETrigger : public TestCase
{
    /* Code currently has a fix for this using SPELL_ATTR0_UNK11
    Not sure what it does, but for spells that had a problem with aoe targeting, only spells targeting allies had it.
    I'd interpret it as "Cast by target", but after checking some other spells (such as 27162), this is wrong. Still needs a general rule for this!
    */
    void Test() override
    {
        TestPlayer* player = SpawnRandomPlayer();
        TestPlayer* player2 = SpawnRandomPlayer();
        Creature* creature = SpawnCreature();

        {
            uint32 const sporeSpellId = 34168;  //periodic cast of 31689 on TARGET_UNIT_SRC_AREA_ENEMY + direct damage aoe
            uint32 const sporeSpellTriggeredIt = 31689; //dot on TARGET_UNIT_SRC_AREA_ENEMY 
            SECTION("Spore clouds", [&] {
                player->SetFullHealth();
                creature->SetFullHealth();
                creature->CastSpell(creature, sporeSpellId);
                ASSERT_INFO("Spell direct damage did not hit player");
                TEST_ASSERT(!player->IsFullHealth());
                TEST_ASSERT(creature->HasAura(sporeSpellId));
                Wait(3500); //Wait first tick (+3000)
                ASSERT_INFO("Creature affected itself instead of player");
                TEST_ASSERT(!creature->HasAura(sporeSpellTriggeredIt));
                ASSERT_INFO("Creature trigger spell did not affected player");
                TEST_ASSERT(player->HasAura(sporeSpellTriggeredIt));
            });
            player->RemoveAurasDueToSpell(sporeSpellId);
            player->RemoveAurasDueToSpell(sporeSpellTriggeredIt);
            creature->RemoveAurasDueToSpell(sporeSpellId);
            creature->RemoveAurasDueToSpell(sporeSpellTriggeredIt);
        }

        {
            uint32 const staticChargeSpellId = 38280; //periodic cast of 38280 on TARGET_UNIT_TARGET_ENEMY  
            uint32 const staticChargeSpellTriggerId = 38281; //dmg on TARGET_UNIT_SRC_AREA_ALLY
            SECTION("Static Charge", [&] {
                player->SetFullHealth();
                player2->SetFullHealth();
                creature->SetFullHealth();
                creature->CastSpell(player, staticChargeSpellId);
                TEST_ASSERT(player->HasAura(staticChargeSpellId));
                Wait(2500);  //Wait first tick (+2000)
                ASSERT_INFO("Creature it itself instead of player!");
                TEST_ASSERT(creature->IsFullHealth());
                ASSERT_INFO("Player2 was not hit by ally aoe on player1");
                TEST_ASSERT(!player2->IsFullHealth());
            });
            player->RemoveAurasDueToSpell(staticChargeSpellId);
            player->RemoveAurasDueToSpell(staticChargeSpellTriggerId);
            player2->RemoveAurasDueToSpell(staticChargeSpellId);
            player2->RemoveAurasDueToSpell(staticChargeSpellTriggerId);
            creature->RemoveAurasDueToSpell(staticChargeSpellId);
            creature->RemoveAurasDueToSpell(staticChargeSpellTriggerId);
        }
    }
};

void AddSC_test_spells_misc()
{
    new NextMeleeHitTest();
    new EnvironmentalTrapTest();
    new StackingRulesTest();
    new SpellPositivity();
    RegisterTestCase("spells delayed stacks", SpellDelayedStacks);
    RegisterTestCase("spells targets aoetrigger", SpellTargetsAoETrigger);
}
