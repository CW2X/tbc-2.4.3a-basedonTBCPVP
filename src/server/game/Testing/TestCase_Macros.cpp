
#include "TestCase.h"
#include "MapManager.h"
#include "TestThread.h"
#include "RandomPlayerbotMgr.h"
#include "TestPlayer.h"
#include "RandomPlayerbotFactory.h"
#include "PlayerbotFactory.h"
#include "CharacterCache.h"
#include "SpellHistory.h"
#include "SpellAuraEffects.h"
#include "Unit.h"
//#include "ClassSpells.h" //I'm avoiding including this for now since it is changed often and will break script rebuild it is modified and TestCase.cpp has to be rebuilt too

#include <algorithm>

void TestCase::_TestStacksCount(TestPlayer* caster, Unit* target, uint32 castSpellID, uint32 testSpell, uint32 requireCount)
{
    //TODO: cast!
    INTERNAL_TEST_ASSERT(false);
    /*
	uint32 auraCount = target->GetAuraCount(testSpell);
	INTERNAL_TEST_ASSERT(auraCount == requireCount);
    */
}

void TestCase::_TestPowerCost(TestPlayer* caster, uint32 castSpellID, Powers powerType, uint32 expectedPowerCost)
{
    SpellInfo const* spellInfo = _GetSpellInfo(castSpellID);

    INTERNAL_ASSERT_INFO("%s has wrong power type %u (instead of %u)", _SpellString(castSpellID).c_str(), uint32(spellInfo->PowerType), uint32(powerType));
    INTERNAL_TEST_ASSERT(spellInfo->PowerType == powerType);

    Spell* spell = new Spell(caster, spellInfo, TRIGGERED_NONE);
    uint32 actualCost = spellInfo->CalcPowerCost(caster, spellInfo->GetSchoolMask(), spell);
    delete spell;
    spell = nullptr;

    INTERNAL_ASSERT_INFO("%s has cost %u power instead of %u", _SpellString(castSpellID).c_str(), actualCost, expectedPowerCost);
    if (expectedPowerCost != 0)
    {
        INTERNAL_TEST_ASSERT(Between(actualCost, expectedPowerCost - 1, expectedPowerCost + 1));
    }
    else
    {
        INTERNAL_TEST_ASSERT(actualCost == expectedPowerCost);
    }
}

void TestCase::_TestCooldown(Unit* caster, Unit* target, uint32 castSpellID, uint32 cooldownMS)
{
    SpellInfo const* spellInfo = _GetSpellInfo(castSpellID);

    //special case for channeled spell, spell system currently does not allow casting them instant
    if (spellInfo->IsChanneled())
    {
        //For now we can't test CD in this case, channel and CD are only starting at next update and when testing CD we already get different values
        return;
    }

    caster->GetSpellHistory()->ResetCooldown(castSpellID);
    CastSpellExtraArgs args(TriggerCastFlags(TRIGGERED_FULL_MASK & ~TRIGGERED_IGNORE_SPELL_AND_CATEGORY_CD));
    args.ForceHitResult = SPELL_MISS_NONE;
    _TestCast(caster, target, castSpellID, SPELL_CAST_OK, args);

    //all setup, proceed to test CD
    _TestHasCooldown(caster, castSpellID, cooldownMS);

    //Cleaning up
    caster->GetSpellHistory()->ResetCooldown(castSpellID);
    caster->GetSpellHistory()->ResetGlobalCooldown();
}

void TestCase::_TestCast(Unit* caster, Unit* victim, uint32 spellID, SpellCastResult expectedCode, CastSpellExtraArgs args /*= {}*/)
{
    uint32 res = caster->CastSpell(victim, spellID, args);
    if (expectedCode == SPELL_CAST_OK)
    {
        INTERNAL_ASSERT_INFO("Caster couldn't cast %s, error %s", _SpellString(spellID).c_str(), StringifySpellCastResult(res).c_str());
    }
    else
    {
        INTERNAL_ASSERT_INFO("Caster cast %s returned unexpected result %s", _SpellString(spellID).c_str(), StringifySpellCastResult(res).c_str());
    }

	INTERNAL_TEST_ASSERT(res == uint32(expectedCode));
}

void TestCase::_ForceCast(Unit* caster, Unit* victim, uint32 spellID, SpellMissInfo forcedMissInfo, CastSpellExtraArgs args /*= {}*/)
{
    //forcedMissInfo and args both contains info for forceMissInfo... but we'll keep it to still be able to call this function as a one liner
    //miss info from args will be ignored
    args.ForceHitResult = forcedMissInfo;
    uint32 res = caster->CastSpell(victim, spellID, args);
    INTERNAL_ASSERT_INFO("Caster couldn't cast %s, error %s", _SpellString(spellID).c_str(), StringifySpellCastResult(res).c_str());
    INTERNAL_TEST_ASSERT(res == uint32(SPELL_CAST_OK));
}

void TestCase::_TestDirectValue(bool heal, Unit* caster, Unit* target, uint32 spellID, uint32 expectedMin, uint32 expectedMax, bool crit, TestCallback callback /* = {}*/, uint32 testedSpellId /*= 0*/)
{
    INTERNAL_TEST_ASSERT(expectedMax >= expectedMin);
    Player* _casterOwner = caster->GetCharmerOrOwnerPlayerOrPlayerItself();
    TestPlayer* casterOwner = dynamic_cast<TestPlayer*>(_casterOwner);
    INTERNAL_ASSERT_INFO("Caster in not a testing bot (or a pet/summon of testing bot)");
    INTERNAL_TEST_ASSERT(casterOwner != nullptr);
    auto AI = caster->ToPlayer()->GetTestingPlayerbotAI();
    INTERNAL_ASSERT_INFO("Caster in not a testing bot");
    INTERNAL_TEST_ASSERT(AI != nullptr);

    ResetSpellCast(caster);
    AI->ResetSpellCounters();
    _MaxHealth(caster);
    _MaxHealth(target);

    auto[sampleSize, maxPredictionError] = _GetApproximationParams(expectedMin, expectedMax);
    if (!testedSpellId)
        testedSpellId = spellID;

	EnableCriticals(caster, crit);

    for (uint32 i = 0; i < sampleSize; i++)
    {
        target->SetFullHealth();

        callback(caster, target);

        _ForceCast(caster, target, spellID, SPELL_MISS_NONE, TriggerCastFlags(TRIGGERED_FULL_MASK | TRIGGERED_IGNORE_SPEED | TRIGGERED_IGNORE_LOS | TRIGGERED_IGNORE_TARGET_AURASTATE));

        HandleThreadPause();
        HandleSpellsCleanup(caster);
    }

    uint32 dealtMin;
    uint32 dealtMax;
    if(heal)
        std::tie(dealtMin, dealtMax) = GetHealingPerSpellsTo(casterOwner, target, testedSpellId, crit, sampleSize);
    else
        std::tie(dealtMin, dealtMax) = GetDamagePerSpellsTo(casterOwner, target, testedSpellId, crit, sampleSize);

    TC_LOG_TRACE("test.unit_test", "%s -> dealtMin: %u - dealtMax %u - expectedMin: %u - expectedMax: %u - sampleSize: %u", _SpellString(testedSpellId).c_str(), dealtMin, dealtMax, expectedMin, expectedMax, sampleSize);

    uint32 allowedMin = expectedMin > maxPredictionError ? expectedMin - maxPredictionError : 0; //protect against underflow
    uint32 allowedMax = expectedMax + maxPredictionError;

    INTERNAL_ASSERT_INFO("Enforcing high result for %s. allowedMax: %u, dealtMax: %u", _SpellString(testedSpellId).c_str(), allowedMax, dealtMax);
    INTERNAL_TEST_ASSERT(dealtMax <= allowedMax);
    INTERNAL_ASSERT_INFO("Enforcing low result for %s. allowedMin: %u, dealtMin: %u", _SpellString(testedSpellId).c_str(), allowedMin, dealtMin);
    INTERNAL_TEST_ASSERT(dealtMin >= allowedMin);

    //Restoring
    RestoreCriticals(caster);
    _RestoreUnitState(caster);
    _RestoreUnitState(target);
}

void TestCase::_TestMeleeDamage(Unit* caster, Unit* target, WeaponAttackType attackType, uint32 expectedMin, uint32 expectedMax, bool crit, TestCallback callback /* = {}*/)
{
    auto AI = _GetCasterAI(caster);

    ResetSpellCast(caster);
    AI->ResetSpellCounters();
    _MaxHealth(target);

    auto[sampleSize, maxPredictionError] = _GetApproximationParams(expectedMin, expectedMax);

    if (attackType != RANGED_ATTACK)
    {
        MeleeHitOutcome previousForceMeleeResult = caster->_forceMeleeResult;
        caster->ForceMeleeHitResult(crit ? MELEE_HIT_CRIT : MELEE_HIT_NORMAL);
        for (uint32 i = 0; i < sampleSize; i++)
        {
            target->SetFullHealth();
            callback(caster, target);

            caster->AttackerStateUpdate(target, attackType);

            HandleThreadPause();
        }
        caster->ForceMeleeHitResult(previousForceMeleeResult);
    }
    else
    {
        EnableCriticals(caster, false);
        CastSpellExtraArgs args(TriggerCastFlags(TRIGGERED_FULL_MASK | TRIGGERED_IGNORE_SPEED | TRIGGERED_IGNORE_LOS));
        args.ForceHitResult = SPELL_MISS_NONE;
        uint32 const SHOOT_SPELL_ID = 75;
        for (uint32 i = 0; i < sampleSize; i++)
        {
            target->SetFullHealth();
            callback(caster, target);

            caster->CastSpell(target, SHOOT_SPELL_ID, args); //shoot

            HandleSpellsCleanup(caster);
            HandleThreadPause();
        }

        RestoreCriticals(caster);
    }
    auto [dealtMin, dealtMax] = GetWhiteDamageDoneTo(caster, target, attackType, crit, sampleSize);

    //TC_LOG_DEBUG("test.unit_test", "attackType: %u - crit %u -> dealtMin: %u - dealtMax %u - expectedMin: %u - expectedMax: %u - sampleSize: %u", uint32(attackType), uint32(crit), dealtMin, dealtMax, expectedMin, expectedMax, sampleSize);

    uint32 allowedMin = expectedMin > maxPredictionError ? expectedMin - maxPredictionError : 0; //protect against underflow
    uint32 allowedMax = expectedMax + maxPredictionError;

    INTERNAL_ASSERT_INFO("Enforcing high result for attackType: %u - crit %u. allowedMax: %u, dealtMax: %u", uint32(attackType), uint32(crit), allowedMax, dealtMax);
    INTERNAL_TEST_ASSERT(dealtMax <= allowedMax);
    INTERNAL_ASSERT_INFO("Enforcing low result for attackType: %u - crit %u. allowedMin: %u, dealtMin: %u", uint32(attackType), uint32(crit), allowedMin, dealtMin);
    INTERNAL_TEST_ASSERT(dealtMin >= allowedMin);

    //restoring
    _RestoreUnitState(target);
}

int32 TestCase::_CastDotAndWait(Unit* caster, Unit* target, uint32 spellID, bool crit)
{
    auto AI = _GetCasterAI(caster);
    SpellInfo const* spellInfo = _GetSpellInfo(spellID);

    EnableCriticals(caster, crit);

    ResetSpellCast(caster);
    AI->ResetSpellCounters();

    _ForceCast(caster, target, spellID, SPELL_MISS_NONE, TriggerCastFlags(TRIGGERED_FULL_MASK | TRIGGERED_IGNORE_SPEED));

    //Currently, channeled spells start at the next update so we need to wait for it to be applied.
    //Remove this line if spell system has changed and this is not true
    if (spellInfo->IsChanneled())
        WaitNextUpdate();

    Aura* aura = target->GetAura(spellID, caster->GetGUID());
    INTERNAL_ASSERT_INFO("Target has not %s aura with caster %u after spell successfully casted", _SpellString(spellID).c_str(), caster->GetGUID().GetCounter());
    INTERNAL_TEST_ASSERT(aura != nullptr);
    int32 auraAmount = 0;
    if(aura->GetEffect(EFFECT_0))
        auraAmount = aura->GetEffect(EFFECT_0)->GetAmount();

    //spell did hit, let's wait for dot duration
    uint32 waitTime = aura->GetDuration();
    Wait(waitTime);
    //aura may be deleted at this point, do not use anymore
    aura = nullptr;

    //make sure aura expired
    INTERNAL_ASSERT_INFO("Target still has %s aura after %u ms", _SpellString(spellID).c_str(), waitTime);
    INTERNAL_TEST_ASSERT(!target->HasAura(spellID, caster->GetGUID()));

    //Restoring
    RestoreCriticals(caster);
    return auraAmount;
}

void TestCase::_TestDotDamage(Unit* caster, Unit* target, uint32 spellID, int32 expectedTotalAmount, bool crit /* = false*/)
{
    auto AI = _GetCasterAI(caster);
    AI->ResetSpellCounters();
    bool heal = expectedTotalAmount < 0;
    _MaxHealth(target, heal);
    
    _CastDotAndWait(caster, target, spellID, crit);

    auto [dotDamageToTarget, tickCount] = AI->GetDotDamage(target, spellID);
	//TC_LOG_TRACE("test.unit_test", "%s -> dotDamageToTarget: %i - expectedTotalAmount: %i", _SpellString(spellID).c_str(), dotDamageToTarget, expectedTotalAmount);
    INTERNAL_ASSERT_INFO("Enforcing dot damage for %s. dotDamageToTarget: %i, expectedTotalAmount: %i", _SpellString(spellID).c_str(), dotDamageToTarget, expectedTotalAmount);
    INTERNAL_TEST_ASSERT(dotDamageToTarget >= (expectedTotalAmount - tickCount) && dotDamageToTarget <= (expectedTotalAmount + tickCount)); //dots have greater error since they got their damage divided in several ticks

    _RestoreUnitState(target);
}

void TestCase::_TestThreat(Unit* caster, Creature* target, uint32 spellID, float expectedThreatFactor, bool heal, TestCallback callback)
{
    // + It'd be nice to deduce heal arg from spell but I don't see any sure way to do it atm

    auto AI = _GetCasterAI(caster);
    //TestPlayer* playerCaster = static_cast<TestPlayer*>(caster); //_GetCasterAI guarantees the caster is a TestPlayer
    SpellInfo const* spellInfo = _GetSpellInfo(spellID);

    ResetSpellCast(caster);
    ResetSpellCast(target);
    AI->ResetSpellCounters();

    uint32 const startThreat = target->GetThreatManager().GetThreat(caster);
    bool applyAura = spellInfo->HasAnyAura();
    Unit* spellTarget = nullptr;

    //make sure the target is in combat with our caster so that threat is generated
    target->EngageWithTarget(caster);
    INTERNAL_ASSERT_INFO("Caster is not in combat with spell target.");
    INTERNAL_TEST_ASSERT(target->IsInCombatWith(caster));

    if (heal)
        spellTarget = caster;
    else
        spellTarget = target;

    _MaxHealth(spellTarget, heal);
    uint32 const spellTargetStartingHealth = spellTarget->GetHealth();
    uint32 const casterStartingHealth = caster->GetHealth();

    spellTarget->RemoveArenaAuras(false); //may help with already present hot and dots breaking the results

    callback(caster, target);

    uint32 auraAmount = 0;
    if (applyAura)
        auraAmount = _CastDotAndWait(caster, spellTarget, spellID, false);
    else
    {
        _ForceCast(caster, spellTarget, spellID, SPELL_MISS_NONE, TriggerCastFlags(TRIGGERED_FULL_MASK | TRIGGERED_IGNORE_SPEED | TRIGGERED_IGNORE_TARGET_AURASTATE | TRIGGERED_IGNORE_LOS));
        if (spellInfo->IsChanneled())
        {
            WaitNextUpdate();
            uint32 baseDurationTime = spellInfo->GetDuration(); 
            Wait(baseDurationTime); //reason we do this is that currently we can't instantly cast a channeled spell with our spell system
        }
    }

    //just make sure target is still in combat with caster
    INTERNAL_ASSERT_INFO("Caster is not in combat with spell target.");
    INTERNAL_TEST_ASSERT(target->IsInCombatWith(caster));
    int32 totalDamage = 0;
    if (spellInfo->HasAuraEffect(SPELL_AURA_MANA_SHIELD) || spellInfo->HasAuraEffect(SPELL_AURA_SCHOOL_ABSORB))
        totalDamage = auraAmount;
    else
    {
        totalDamage = spellTargetStartingHealth - spellTarget->GetHealth();
        INTERNAL_ASSERT_INFO("No damage or heal done to target");
        INTERNAL_TEST_ASSERT(totalDamage != 0);
    }
    
    if (!heal) //also add health leech if any
    {
        int32 const diff = caster->GetHealth() - casterStartingHealth;
        if (diff > 0)
            totalDamage += diff * 0.5f;
    }

    float const actualThreatDone = target->GetThreatManager().GetThreat(caster) - startThreat;
    float const expectedTotalThreat = std::abs(totalDamage) * expectedThreatFactor;

    INTERNAL_ASSERT_INFO("Enforcing threat for dot/hot %s. Creature should have %f threat but has %f.", _SpellString(spellID).c_str(), expectedTotalThreat, actualThreatDone);
    INTERNAL_TEST_ASSERT(Between<float>(actualThreatDone, expectedTotalThreat - 1.f, expectedTotalThreat + 1.f));

    //Cleanup
    target->GetThreatManager().ResetThreat(caster);
    target->GetThreatManager().AddThreat(caster, startThreat);
    caster->RemoveAurasDueToSpell(spellID);
    target->RemoveAurasDueToSpell(spellID);
    _RestoreUnitState(spellTarget);
}

void TestCase::_TestChannelDamage(bool healing, Unit* caster, Unit* target, uint32 spellID, uint32 expectedTickCount, int32 expectedTickAmount, uint32 testedSpell /* = 0*/)
{
    if (!testedSpell)
        testedSpell = spellID;

    auto AI = _GetCasterAI(caster);
    SpellInfo const* spellInfo = _GetSpellInfo(spellID);

    uint32 baseDurationTime = spellInfo->GetDuration();

    ResetSpellCast(caster);
    AI->ResetSpellCounters();

    EnableCriticals(caster, false);
    _ForceCast(caster, target, spellID, SPELL_MISS_NONE, TriggerCastFlags(TRIGGERED_FULL_MASK | TRIGGERED_IGNORE_SPEED | TRIGGERED_IGNORE_TARGET_AURASTATE));

    _StartUnitChannels(caster); //remove if spell system allow to cast channel instantly
    Wait(baseDurationTime); //reason we do this is that currently we can't instantly cast a channeled spell with our spell system
    Spell* currentSpell = caster->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
    INTERNAL_ASSERT_INFO("caster is still casting channel (%s) after baseDurationTime %u", _SpellString(spellID).c_str(), baseDurationTime);
    INTERNAL_TEST_ASSERT(currentSpell == nullptr);

    INTERNAL_ASSERT_INFO("Target still has aura %s", _SpellString(testedSpell).c_str());
    INTERNAL_TEST_ASSERT(!target->HasAura(testedSpell));

    INTERNAL_ASSERT_INFO("Caster still has aura %s", _SpellString(testedSpell).c_str());
    INTERNAL_TEST_ASSERT(!caster->HasAura(testedSpell));

    uint32 totalChannelDmg = 0; 
    if (healing)
        totalChannelDmg = GetChannelHealingTo(caster, target, testedSpell, expectedTickCount, {});
    else
        totalChannelDmg = GetChannelDamageTo(caster, target, testedSpell, expectedTickCount, {});

    uint32 actualTickAmount = totalChannelDmg / expectedTickCount;
    INTERNAL_ASSERT_INFO("Channel damage: resultTickAmount: %i, expectedTickAmount: %i", actualTickAmount, expectedTickAmount);
    INTERNAL_TEST_ASSERT(actualTickAmount >= (expectedTickAmount - 2) && actualTickAmount <= (expectedTickAmount + 2)); //channels have greater error since they got their damage divided in several ticks

    //Restoring
    RestoreCriticals(caster);
}

void TestCase::_TestSpellHitChance(Unit* caster, Unit* victim, uint32 spellID, float expectedResultPercent, SpellMissInfo missInfo, TestCallback callback)
{
    SpellInfo const* spellInfo = _GetSpellInfo(spellID);

    auto AI = _GetCasterAI(caster);

    ResetSpellCast(caster);
    AI->ResetSpellCounters();

    _EnsureAlive(caster, victim);
    _MaxHealth(victim);

    auto[sampleSize, resultingAbsoluteTolerance] = _GetPercentApproximationParams(expectedResultPercent);

    for (uint32 i = 0; i < sampleSize; i++)
    {
        callback(caster, victim);

        victim->SetFullHealth();
        _TestCast(caster, victim, spellID, SPELL_CAST_OK, TriggerCastFlags(TRIGGERED_FULL_MASK | TRIGGERED_IGNORE_SPEED | TRIGGERED_IGNORE_LOS));
        if (spellInfo->IsChanneled())
            _StartUnitChannels(caster);

        HandleThreadPause();
        HandleSpellsCleanup(caster);
    }

    _TestSpellOutcomePercentage(caster, victim, spellID, missInfo, expectedResultPercent, resultingAbsoluteTolerance, sampleSize);

    //Restore
    ResetSpellCast(caster); // some procs may have occured and may still be in flight, remove them
    _RestoreUnitState(victim);
}

void TestCase::_TestAuraTickProcChance(Unit* caster, Unit* target, uint32 spellID, SpellEffIndex index, float chance, uint32 procSpellId, bool checkSelf)
{
    auto callback = [checkSelf, procSpellId](Unit* caster, Unit* target) { 
        return (checkSelf ? caster : target)->HasAura(procSpellId); 
    };
    _TestAuraTickProcChanceCallback(caster, target, spellID, index, chance, procSpellId, callback);
}

void TestCase::_TestAuraTickProcChanceCallback(Unit* caster, Unit* target, uint32 spellID, SpellEffIndex effIndex, float expectedResultPercent, uint32 procSpellId, TestCallbackResult callback)
{
    _EnsureAlive(caster, target);

    Aura* aura = caster->AddAura(spellID, target);
    INTERNAL_ASSERT_INFO("_TestAuraTickProcChance failed to add aura %s on victim", _SpellString(spellID).c_str());
    INTERNAL_TEST_ASSERT(aura != nullptr);
    AuraEffect* effect = aura->GetEffect(effIndex);
    INTERNAL_ASSERT_INFO("_TestAuraTickProcChance failed to get aura %s effect %u on victim", _SpellString(spellID).c_str(), uint32(effIndex));
    INTERNAL_TEST_ASSERT(effect != nullptr);

    _MaxHealth(target);
    _MaxHealth(caster);

    uint32 successCount = 0;
    uint32 sampleSize = 0;
    float resultingAbsoluteTolerance = _GetPercentTestTolerance(expectedResultPercent);
    while (_ShouldIteratePercent(expectedResultPercent, successCount, sampleSize, resultingAbsoluteTolerance))
    {
        effect->PeriodicTick(caster);
        successCount += uint32(callback(caster, target));

        caster->RemoveAurasDueToSpell(procSpellId);
        target->RemoveAurasDueToSpell(procSpellId);

        caster->SetFullHealth();
        target->SetFullHealth();
        caster->ClearDiminishings();
        target->ClearDiminishings();

        HandleThreadPause();
        sampleSize++;
    }

    ResetSpellCast(caster); // some procs may have occured and may still be in flight, remove them

    float actualSuccessPercent = 100 * (successCount / float(sampleSize));
    INTERNAL_ASSERT_INFO("_TestAuraTickProcChance on %s: expected result: %f, result: %f", _SpellString(spellID).c_str(), expectedResultPercent, actualSuccessPercent);
    INTERNAL_TEST_ASSERT(Between<float>(expectedResultPercent, actualSuccessPercent - resultingAbsoluteTolerance, actualSuccessPercent + resultingAbsoluteTolerance));

    //Restoring
    _RestoreUnitState(caster);
    _RestoreUnitState(target);
}

void TestCase::_TestMeleeProcChance(Unit* attacker, Unit* victim, uint32 procSpellID, bool selfProc, float expectedChancePercent, MeleeHitOutcome meleeHitOutcome, WeaponAttackType attackType, TestCallback callback)
{
    MeleeHitOutcome previousForceOutcome = attacker->_forceMeleeResult;
    attacker->ForceMeleeHitResult(meleeHitOutcome);

    auto launchCallback = [&](Unit* caster, Unit* victim) {
        attacker->AttackerStateUpdate(victim, attackType);
    };
    auto[actualSuccessPercent, resultingAbsoluteTolerance] = _TestProcChance(attacker, victim, procSpellID, selfProc, expectedChancePercent, launchCallback, callback);

    INTERNAL_ASSERT_INFO("%s proc'd %f instead of expected %f", _SpellString(procSpellID).c_str(), actualSuccessPercent, expectedChancePercent);
    INTERNAL_TEST_ASSERT(Between<float>(expectedChancePercent, actualSuccessPercent - resultingAbsoluteTolerance, actualSuccessPercent + resultingAbsoluteTolerance));

    attacker->AttackStop();
    attacker->ForceMeleeHitResult(previousForceOutcome);
}

void TestCase::_TestSpellProcChance(Unit* caster, Unit* victim, uint32 spellID, uint32 procSpellID, bool selfProc, float expectedChancePercent, SpellMissInfo missInfo, bool crit, TestCallback callback)
{
    SpellInfo const* spellInfo = _GetSpellInfo(spellID);
    EnableCriticals(caster, crit);

    auto launchCallback = [&](Unit* caster, Unit* victim) {
        _ForceCast(caster, victim, spellID, missInfo, TriggerCastFlags(TRIGGERED_FULL_MASK | TRIGGERED_IGNORE_SPEED | TRIGGERED_TREAT_AS_NON_TRIGGERED | TRIGGERED_IGNORE_LOS));
        if (spellInfo->IsChanneled())
            _StartUnitChannels(caster);
    };
    auto[actualSuccessPercent, resultingAbsoluteTolerance] = _TestProcChance(caster, victim, procSpellID, selfProc, expectedChancePercent, launchCallback, callback);

    INTERNAL_ASSERT_INFO("%s proc'd %f instead of expected %f (by %s)", _SpellString(procSpellID).c_str(), actualSuccessPercent, expectedChancePercent, _SpellString(spellID).c_str());
    INTERNAL_TEST_ASSERT(Between<float>(expectedChancePercent, actualSuccessPercent - resultingAbsoluteTolerance, actualSuccessPercent + resultingAbsoluteTolerance));

    RestoreCriticals(caster);
}

std::pair<float /*procChance*/, float /*absoluteTolerance*/> TestCase::_TestProcChance(Unit* caster, Unit* victim, uint32 procSpellID, bool selfProc, float expectedChancePercent, TestCallback launchCallback, TestCallback callback)
{
    auto casterAI = _GetCasterAI(caster, false);
    auto victimAI = _GetCasterAI(victim, false);
    INTERNAL_ASSERT_INFO("Could not find Testing AI for neither caster or victim");
    INTERNAL_TEST_ASSERT(casterAI != nullptr || victimAI != nullptr);

    _EnsureAlive(caster, victim);
    /*SpellInfo const* procSpellInfo =*/ _GetSpellInfo(procSpellID);

    ResetSpellCast(caster);
    if(casterAI)
        casterAI->ResetSpellCounters();
    if(victimAI)
        victimAI->ResetSpellCounters();
    _MaxHealth(caster);
    _MaxHealth(victim);

    Unit* checkTarget = selfProc ? caster : victim;
    auto hasProcced = [checkTarget, procSpellID](PlayerbotTestingAI* AI) {
        auto damageToTarget = AI->GetSpellDamageDoneInfo(checkTarget);
        if (!damageToTarget)
            return false;

        for (auto itr : *damageToTarget)
        {
            if (itr.damageInfo.SpellID != procSpellID)
                continue;

            return true;
        }
        return false;
    };

    uint32 sampleSize = 0;
    uint32 procCount = 0;
    float resultingAbsoluteTolerance = _GetPercentTestTolerance(expectedChancePercent);
    while (_ShouldIteratePercent(expectedChancePercent, procCount, sampleSize, resultingAbsoluteTolerance))
    {
        victim->SetFullHealth();
        callback(caster, victim);

        launchCallback(caster, victim);

        //max 1 proc counted per loop
        if (((casterAI ? hasProcced(casterAI) : false)) || (victimAI ? hasProcced(victimAI) : false))
            procCount++;
        if (casterAI)
            casterAI->ResetSpellCounters();
        if (victimAI)
            victimAI->ResetSpellCounters();

        caster->RemoveAurasDueToSpell(procSpellID);
        victim->RemoveAurasDueToSpell(procSpellID);
        caster->SetFullHealth();
        victim->SetFullHealth();
        caster->ClearDiminishings();
        victim->ClearDiminishings();

        HandleThreadPause();
        HandleSpellsCleanup(caster);

        sampleSize++;
    }

    float actualSuccessPercent = 100 * (procCount / float(sampleSize));
    
    //Restore
    ResetSpellCast(caster); // some procs may have occured and may still be in flight, remove them
    _RestoreUnitState(victim);
    _RestoreUnitState(caster);

    return std::make_pair(actualSuccessPercent, resultingAbsoluteTolerance);
}

void TestCase::_TestPushBackResistChance(Unit* caster, Unit* target, uint32 spellID, float expectedResultPercent)
{
    _EnsureAlive(caster, target);
    SpellInfo const* spellInfo = _GetSpellInfo(spellID);

    uint32 startingHealth = caster->GetHealth();

    Creature* attackingUnit = SpawnCreature(0, false);
    attackingUnit->ForceMeleeHitResult(MELEE_HIT_NORMAL);

    bool channeled = spellInfo->IsChanneled();
    TriggerCastFlags castFlags = TRIGGERED_FULL_MASK;
    if (!channeled)
        castFlags = TriggerCastFlags(castFlags & ~TRIGGERED_CAST_DIRECTLY); //if channeled, we want finish the initial cast directly to start channeling

    _TestCast(caster, target, spellID, SPELL_CAST_OK, castFlags);
    if (channeled)
        WaitNextUpdate(); //currently we can't start a channel before next update
    Spell* spell = caster->GetCurrentSpell(channeled ? CURRENT_CHANNELED_SPELL:  CURRENT_GENERIC_SPELL);
    INTERNAL_ASSERT_INFO("_TestPushBackResistChance: Failed to get current %s", _SpellString(spellID).c_str());
    INTERNAL_TEST_ASSERT(spell != nullptr);

    uint32 pushbackCount = 0;
    uint32 sampleSize = 0;
    float resultingAbsoluteTolerance = _GetPercentTestTolerance(expectedResultPercent);
    while (_ShouldIteratePercent(expectedResultPercent, pushbackCount, sampleSize, resultingAbsoluteTolerance))
    {
        caster->SetFullHealth();

        //timer should be increased for cast, descreased for channels
        uint32 const startChannelTime = 10000;
        uint32 const startCastTime = 1;
        spell->m_timer = channeled ? startChannelTime : startCastTime;
        uint32 healthBefore = caster->GetHealth();

        attackingUnit->AttackerStateUpdate(caster, BASE_ATTACK);
        //health check is just here to ensure integrity
        INTERNAL_ASSERT_INFO("Caster has not lost hp, did melee hit failed?");
        INTERNAL_TEST_ASSERT(caster->GetHealth() < healthBefore);

        if (channeled ? spell->m_timer < startChannelTime : spell->m_timer > startCastTime)
            pushbackCount++;

        HandleThreadPause();
        sampleSize++;
    }

    float actualResistPercent = 100.0f * (1 - (pushbackCount / float(sampleSize)));
    INTERNAL_ASSERT_INFO("_TestPushBackResistChance on %s: expected result: %f, result: %f", _SpellString(spellID).c_str(), expectedResultPercent, actualResistPercent);
    INTERNAL_TEST_ASSERT(Between<float>(expectedResultPercent, actualResistPercent - resultingAbsoluteTolerance, actualResistPercent + resultingAbsoluteTolerance));

    //restoring
    attackingUnit->DisappearAndDie();
    caster->SetHealth(startingHealth);
}

void TestCase::_TestSpellDispelResist(Unit* caster, Unit* target, Unit* dispeler, uint32 spellID, float expectedResultPercent, TestCallback callback)
{
    //Dispel resist chance is not related to hit chance but is a separate roll
    //https://wow.gamepedia.com/index.php?title=Dispel&oldid=1432725

    _EnsureAlive(caster, target);
    SpellInfo const* spellInfo = _GetSpellInfo(spellID);
    uint32 dispelMask = spellInfo->GetDispelMask();
    uint32 dispelSpellId = 0;
    if (dispelMask & (1 << DISPEL_MAGIC))
        dispelSpellId = 527; //priest dispel
    else if (dispelMask & (1 << DISPEL_CURSE))
        dispelSpellId = 2782; //druid remove curse
    else if (dispelMask & (1 << DISPEL_DISEASE))
        dispelSpellId = 528; //priest curse disease
    else if (dispelMask & (1 << DISPEL_POISON))
        dispelSpellId = 8946;  //Druid Curse Poison
    else
    {
        INTERNAL_ASSERT_INFO("Dispel mask %u not handled", dispelSpellId);
        INTERNAL_TEST_ASSERT(false);
    }

    _MaxHealth(target);

    uint32 resistedCount = 0;
    uint32 sampleSize = 0;
    float resultingAbsoluteTolerance = _GetPercentTestTolerance(expectedResultPercent);
    while(_ShouldIteratePercent(expectedResultPercent, resistedCount, sampleSize, resultingAbsoluteTolerance))
    {
        target->SetFullHealth();

        callback(caster, target);

        _ForceCast(caster, target, spellID, SPELL_MISS_NONE, TriggerCastFlags(TRIGGERED_FULL_MASK | TRIGGERED_IGNORE_SPEED | TRIGGERED_IGNORE_LOS));
        if (spellInfo->IsChanneled())
            _StartUnitChannels(caster);
        INTERNAL_ASSERT_INFO("TestCase::_TestSpellDispelResist target has not aura of %s after cast", _SpellString(spellID).c_str());
        INTERNAL_TEST_ASSERT(target->HasAura(spellID));
        _ForceCast(dispeler, target, dispelSpellId, SPELL_MISS_NONE, TRIGGERED_FULL_MASK);
        resistedCount += uint32(target->HasAura(spellID));

        target->ClearDiminishings();
        target->RemoveAurasDueToSpell(spellID);

        HandleThreadPause();
        HandleSpellsCleanup(caster);
        HandleSpellsCleanup(dispeler);

        sampleSize++;
    }
    float actualResistPercent = 100.0f * (resistedCount / float(sampleSize));

    INTERNAL_ASSERT_INFO("_TestSpellDispelResist on %s: expected result: %f, result: %f", _SpellString(spellID).c_str(), expectedResultPercent, actualResistPercent);
    INTERNAL_TEST_ASSERT(Between<float>(expectedResultPercent, actualResistPercent - resultingAbsoluteTolerance, actualResistPercent + resultingAbsoluteTolerance));

    //Restore
    _RestoreUnitState(target);
}

void TestCase::_TestMeleeHitChance(Unit* caster, Unit* victim, WeaponAttackType weaponAttackType, float expectedResultPercent, MeleeHitOutcome meleeHitOutcome, TestCallback callback)
{
    _EnsureAlive(caster, victim);
    INTERNAL_ASSERT_INFO("_TestMeleeHitChance can only be used with BASE_ATTACK and OFF_ATTACK");
    INTERNAL_TEST_ASSERT(weaponAttackType <= OFF_ATTACK);

    _MaxHealth(victim);

    auto[sampleSize, resultingAbsoluteTolerance] = _GetPercentApproximationParams(expectedResultPercent);

    for (uint32 i = 0; i < sampleSize; i++)
    {
        victim->SetFullHealth();

        callback(caster, victim);

        caster->AttackerStateUpdate(victim, weaponAttackType);
        HandleThreadPause();
    }

    _TestMeleeOutcomePercentage(caster, victim, weaponAttackType, meleeHitOutcome, expectedResultPercent, resultingAbsoluteTolerance, sampleSize);

    //Restore
    ResetSpellCast(caster); // some procs may have occured and may still be in flight, remove them
    _RestoreUnitState(victim);
}

void TestCase::_TestMeleeOutcomePercentage(Unit* attacker, Unit* victim, WeaponAttackType weaponAttackType, MeleeHitOutcome meleeHitOutcome, float expectedResult, float allowedError, uint32 sampleSize /*= 0*/)
{
    auto AI = _GetCasterAI(attacker);

    auto damageToTarget = AI->GetMeleeDamageDoneInfo(victim);
    INTERNAL_ASSERT_INFO("_TestMeleeOutcomePercentage found no data for target (%s)", victim->GetName().c_str());
    INTERNAL_TEST_ASSERT(damageToTarget && !damageToTarget->empty());

    uint32 success = 0;
    uint32 attacks = 0; //total attacks with correct type
    //uint32 total = 0;
    for (auto itr : *damageToTarget)
    {
      /*  total++;
        float mean = (success / float(total))* 100.f;
        if(total % 100)
            TC_LOG_ERROR("test.unit_test", "%u error %f \t\t mean %f \t\t %f", total, std::abs(mean - expectedResult), mean, expectedResult);*/

        if (itr.damageInfo.AttackType != weaponAttackType)
            continue;

        attacks++;

        if (itr.damageInfo.HitOutCome != meleeHitOutcome)
            continue;

        success++;
    }

    if (sampleSize)
    {
        INTERNAL_ASSERT_INFO("_TestMeleeOutcomePercentage found %u results instead of expected sample size %u", attacks, sampleSize);
        INTERNAL_TEST_ASSERT(attacks == sampleSize)
    }

    float const result = success / float(damageToTarget->size()) * 100;;
    INTERNAL_ASSERT_INFO("_TestMeleeOutcomePercentage: expected result: %f, result: %f", expectedResult, result);
    INTERNAL_TEST_ASSERT(Between<float>(expectedResult, result - allowedError, result + allowedError));
}

void TestCase::_TestSpellOutcomePercentage(Unit* caster, Unit* victim, uint32 spellID, SpellMissInfo missInfo, float expectedResult, float allowedError, uint32 expectedSampleSize /*= 0*/)
{
    auto AI = _GetCasterAI(caster);

    auto damageToTarget = AI->GetSpellDamageDoneInfo(victim);
    INTERNAL_ASSERT_INFO("_TestSpellOutcomePercentage found no data of %s for target (%s)", _SpellString(spellID).c_str(), victim->GetName().c_str());
    INTERNAL_TEST_ASSERT(damageToTarget && !damageToTarget->empty());
    
    /*SpellInfo const* spellInfo =*/ _GetSpellInfo(spellID);

    uint32 actualDesiredOutcomeCount = 0;
    uint32 actualSampleCount = 0;
    for (auto itr : *damageToTarget)
    {
        if (itr.damageInfo.SpellID != spellID)
            continue;

        actualSampleCount++;

        if (itr.missInfo != missInfo)
            continue;

        actualDesiredOutcomeCount++;
    }

    if (expectedSampleSize)
    {
        INTERNAL_ASSERT_INFO("_TestSpellOutcomePercentage found %u results instead of expected sample size %u for %s", actualSampleCount, expectedSampleSize, _SpellString(spellID).c_str());
        INTERNAL_TEST_ASSERT(actualSampleCount == expectedSampleSize)
    }

    float const result = (actualDesiredOutcomeCount / float(actualSampleCount)) * 100.0f;
    INTERNAL_ASSERT_INFO("TestSpellOutcomePercentage on %s: expected result: %f, result: %f", _SpellString(spellID).c_str(), expectedResult, result);
    INTERNAL_TEST_ASSERT(Between<float>(expectedResult, result - allowedError, result + allowedError));
}

template <class T>
void GetCritDone(uint32& critCount, uint32& foundCount, std::vector<T> const* doneToTarget, std::function<bool(T const)> check)
{
    critCount = 0;
    foundCount = 0;

    if (!doneToTarget || doneToTarget->empty())
        return;

    for (auto itr : *doneToTarget)
    {
        if (!check(itr))
            continue;

        foundCount++;

        if (itr.crit)
            critCount++;
    }
};

void TestCase::_TestSpellCritPercentage(Unit* caster, Unit* victim, uint32 spellId, float expectedResult, float allowedError, uint32 sampleSize /* = 0*/)
{
    auto AI = _GetCasterAI(caster);

    /*SpellInfo const* spellInfo =*/ _GetSpellInfo(spellId);

    uint32 critCount = 0;
    uint32 foundCount = 0;
    GetCritDone<PlayerbotTestingAI::SpellDamageDoneInfo>(critCount, foundCount, AI->GetSpellDamageDoneInfo(victim), [&](auto info) { return info.damageInfo.SpellID == spellId; });
    if (!foundCount)
        GetCritDone<PlayerbotTestingAI::HealingDoneInfo>(critCount, foundCount, AI->GetHealingDoneInfo(victim), [&](auto info) { return info.spellID == spellId; });

    INTERNAL_ASSERT_INFO("_TestSpellCritPercentage found no data of %s for target (%s)", _SpellString(spellId).c_str(), victim->GetName().c_str());
    INTERNAL_TEST_ASSERT(foundCount != 0);

    if (sampleSize)
    {
        INTERNAL_ASSERT_INFO("_TestSpellCritPercentage found %u results instead of expected sample size %u for %s", foundCount, sampleSize, _SpellString(spellId).c_str());
        INTERNAL_TEST_ASSERT(foundCount == sampleSize)
    }

    float const result = (critCount / float(foundCount)) * 100.0f;
    INTERNAL_ASSERT_INFO("_TestSpellCritPercentage on %s: expected result: %f, result: %f", _SpellString(spellId).c_str(), expectedResult, result);
    INTERNAL_TEST_ASSERT(Between<float>(expectedResult, result - allowedError, result + allowedError));
}

void TestCase::_EnsureHasAura(Unit* target, int32 spellID)
{
    bool checkHasAura = spellID > 0;
    spellID = std::abs(spellID);
    bool hasAura = target->HasAura(spellID);
    if (checkHasAura)
    {
        INTERNAL_ASSERT_INFO("Target %u (%s) does not have aura of %s", target->GetGUID().GetCounter(), target->GetName().c_str(), _SpellString(spellID).c_str());
        INTERNAL_TEST_ASSERT(hasAura);
    }
    else 
    {
        INTERNAL_ASSERT_INFO("Target %u (%s) has aura of %s", target->GetGUID().GetCounter(), target->GetName().c_str(), _SpellString(spellID).c_str());
        INTERNAL_TEST_ASSERT(!hasAura);
    }
}

void TestCase::_TestHasCooldown(Unit* caster, uint32 castSpellID, uint32 cooldownMs)
{
    SpellInfo const* spellInfo = _GetSpellInfo(castSpellID);
    uint32 cooldown = caster->GetSpellHistory()->GetRemainingCooldown(spellInfo);
    INTERNAL_ASSERT_INFO("Caster %s has cooldown %u for %s instead of expected %u", caster->GetName().c_str(), cooldown, _SpellString(castSpellID).c_str(), cooldownMs);
    INTERNAL_TEST_ASSERT(cooldown == cooldownMs);
}

void TestCase::_TestAuraMaxDuration(Unit* target, uint32 spellID, uint32 durationMS)
{
    INTERNAL_ASSERT_INFO("Target %u (%s) is not alive", target->GetGUID().GetCounter(), target->GetName().c_str());
    INTERNAL_TEST_ASSERT(target->IsAlive());

    Aura* aura = target->GetAura(spellID);
    INTERNAL_ASSERT_INFO("Target %u (%s) does not have aura of %s", target->GetGUID().GetCounter(), target->GetName().c_str(), _SpellString(spellID).c_str());
    INTERNAL_TEST_ASSERT(aura != nullptr);

    uint32 auraDuration = aura->GetMaxDuration();
    INTERNAL_ASSERT_INFO("Target %u (%s) has aura (%s) with duration %u instead of %u", target->GetGUID().GetCounter(), target->GetName().c_str(), _SpellString(spellID).c_str(), auraDuration, durationMS);
    INTERNAL_TEST_ASSERT(auraDuration == durationMS);
}

void TestCase::_TestAuraStack(Unit* target, uint32 spellID, uint32 stacks, bool stack)
{
    Aura* aura = target->GetAura(spellID);
    INTERNAL_ASSERT_INFO("Target %u (%s) does not have aura of %s", target->GetGUID().GetCounter(), target->GetName().c_str(), _SpellString(spellID).c_str());
    INTERNAL_TEST_ASSERT(aura != nullptr);

    uint32 auraStacks = 0;
    std::string type = "stacks";
    if (stack)
        auraStacks = aura->GetStackAmount();
    else
    {
        auraStacks = aura->GetCharges();
        type = "charges";
    }
    INTERNAL_ASSERT_INFO("Target %u (%s) has aura (%s) with %u %s instead of %u", target->GetGUID().GetCounter(), target->GetName().c_str(), _SpellString(spellID).c_str(), auraStacks, type.c_str(), stacks);
    INTERNAL_TEST_ASSERT(auraStacks == stacks);
}

void TestCase::_TestUseItem(TestPlayer* caster, Unit* target, uint32 itemId)
{
    Item* firstItem = caster->GetFirstItem(itemId);
    INTERNAL_ASSERT_INFO("_TestUseItem failed to find any item with id %u", itemId);
    INTERNAL_TEST_ASSERT(firstItem != nullptr);

    SpellCastTargets targets;
    targets.SetUnitTarget(target);
    bool result = caster->GetSession()->_HandleUseItemOpcode(firstItem->GetBagSlot(), firstItem->GetSlot(), 1, 1, firstItem->GetGUID(), targets);
    INTERNAL_ASSERT_INFO("_TestUseItem failed to use item with id %u", itemId);
    INTERNAL_TEST_ASSERT(result);
}

void TestCase::_TestSpellCritChance(Unit* caster, Unit* victim, uint32 spellID, float expectedResultPercent, TestCallback callback)
{
    auto AI = _GetCasterAI(caster);

    ResetSpellCast(caster);
    AI->ResetSpellCounters();

    _EnsureAlive(caster, victim);

    _MaxHealth(victim);

    auto[sampleSize, resultingAbsoluteTolerance] = _GetPercentApproximationParams(expectedResultPercent);

    for (uint32 i = 0; i < sampleSize; i++)
    {
        callback(caster, victim);

        victim->SetFullHealth();
        _ForceCast(caster, victim, spellID, SPELL_MISS_NONE, TriggerCastFlags(TRIGGERED_FULL_MASK | TRIGGERED_IGNORE_SPEED | TRIGGERED_IGNORE_LOS));

        HandleThreadPause();
        HandleSpellsCleanup(caster);
    }

    _TestSpellCritPercentage(caster, victim, spellID, expectedResultPercent, resultingAbsoluteTolerance, sampleSize);

    //Restore
    ResetSpellCast(caster); // some procs may have occured and may still be in flight, remove them
    _RestoreUnitState(victim);
}

void TestCase::_TestSpellCastTime(Unit* caster, uint32 spellID, uint32 expectedCastTimeMS)
{
    SpellInfo const* spellInfo = _GetSpellInfo(spellID);

    Spell* spell = new Spell(caster, spellInfo, TRIGGERED_NONE);
    uint32 const actualCastTime = spellInfo->CalcCastTime(spell);
    delete spell;
    spell = nullptr;

    ASSERT_INFO("Cast time for %s did not match: Expected %u - Actual %u", _SpellString(spellID).c_str(), expectedCastTimeMS, actualCastTime);
    TEST_ASSERT(actualCastTime == expectedCastTimeMS);
}

void TestCase::_TestRange(TestPlayer* caster, Unit* target, uint32 spellID, float maxRange, float minRange /*= 0.0f*/)
{
    SpellInfo const* spellInfo = _GetSpellInfo(spellID);

    //spells range usually uses both caster and target combat reach
    /* Commented out, let the caller handle this part
    if (minRange)
    {
        if (spellInfo->RangeEntry->type & SPELL_RANGE_RANGED)
            minRange += caster->GetMeleeRange(target);
    }*/
    if (spellInfo->NeedsExplicitUnitTarget() || spellInfo->GetExplicitTargetMask() & TARGET_FLAG_CORPSE_MASK) //any others?
    {
        TEST_ASSERT(caster->GetCombatReach() == DEFAULT_PLAYER_COMBAT_REACH);
        maxRange += caster->GetCombatReach() + target->GetCombatReach();
        if (minRange && !(spellInfo->RangeEntry->type & SPELL_RANGE_RANGED))
            minRange += caster->GetCombatReach() + target->GetCombatReach();
    }

    Position originalTargetPos = target->GetPosition();
    _SaveUnitState(caster);
    _SaveUnitState(target);

    auto testRange = [&](float range, bool max)
    {
        //just in range, should pass
        Position inRangePos = caster->GetPosition();
        inRangePos.MoveInFront(caster, max ? range - 1.0f : range + 1.0f);
        target->Relocate(inRangePos);

        TriggerCastFlags const triggerFlags = TriggerCastFlags(TRIGGERED_FULL_MASK & ~TRIGGERED_IGNORE_RANGE);
        _TestCast(caster, target, spellID, SPELL_CAST_OK, triggerFlags);

        //just of ouf range
        Position oorPos = caster->GetPosition();
        oorPos.MoveInFront(caster, max ? range + 1.0f : range - 1.0f);
        target->Relocate(oorPos);
        _TestCast(caster, target, spellID, SPELL_FAILED_OUT_OF_RANGE, triggerFlags);
    };

    // -- Test max range
    testRange(maxRange, true);
    if (minRange)
        testRange(minRange, false);

    //Restore
    target->Relocate(originalTargetPos);
    _RestoreUnitState(caster);
    _RestoreUnitState(target);
}

void TestCase::_TestRadius(TestPlayer* caster, Unit* castTarget, Unit* checkTarget, uint32 spellID, float radius, bool heal, uint32 checkSpellID /*= 0*/, bool includeCasterReach /*= true*/)
{
    //spells radius uses caster combat reach in most cases, search for IsProximityBasedAoe in the code
    if (includeCasterReach)
    {
        TEST_ASSERT(caster->GetCombatReach() == DEFAULT_PLAYER_COMBAT_REACH);
        radius += caster->GetCombatReach();
    }
    radius += checkTarget->GetCombatReach();
    if (!checkSpellID)
        checkSpellID = spellID;

    Position originalTargetPos = checkTarget->GetPosition();
    auto ai = _GetCasterAI(caster);
    ai->ResetSpellCounters();
    _SaveUnitState(caster);
    _SaveUnitState(castTarget);
    _SaveUnitState(checkTarget);

    auto hasData = [&](bool heal) {
        if (heal)
            return !GetHealingDoneInfoTo(caster, checkTarget, checkSpellID, false).empty();
        else
            return !GetSpellDamageDoneInfoTo(caster, checkTarget, checkSpellID, false).empty();
    };

    auto moveTarget = [&](Position& pos) {
        checkTarget->NearTeleportTo(pos);
        if (checkTarget->GetTypeId() == TYPEID_PLAYER)
        {
            auto ai = this->_GetCasterAI(checkTarget);
            ai->HandleTeleportAck();
        }
    };

    auto moveTargetInFront = [&](float range) {
        Position pos = castTarget->GetPosition();
        pos.MoveInFront(caster, range);
        moveTarget(pos);
    };

    //Test given radius
    moveTargetInFront(radius - 0.5f); //allow for minor imprecision

    TriggerCastFlags triggerFlags = TriggerCastFlags(TRIGGERED_FULL_MASK & ~TRIGGERED_IGNORE_RANGE);
    _TestCast(caster, castTarget, spellID, SPELL_CAST_OK, triggerFlags);
    INTERNAL_ASSERT_INFO("Target was not affected by %s, radius may be wrong (or spell does not affect this target)", _SpellString(spellID).c_str());
    INTERNAL_TEST_ASSERT(hasData(heal));

    //Test ouf of range
    ai->ResetSpellCounters();
    moveTargetInFront(radius + 3.0f);
    _TestCast(caster, castTarget, spellID, SPELL_CAST_OK, triggerFlags);
    INTERNAL_ASSERT_INFO("Target was affected by %s but should have been out of range, radius may be wrong", _SpellString(spellID).c_str());
    INTERNAL_TEST_ASSERT(!hasData(heal));

    //Restore
    moveTarget(originalTargetPos);
    _RestoreUnitState(caster);
    _RestoreUnitState(castTarget);
    _RestoreUnitState(checkTarget);
}

Creature* TestCase::SpawnDatabaseCreature()
{
    Creature* creature = new Creature;
    if (!creature->Create(sObjectMgr->GenerateCreatureSpawnId(), GetMap(), PHASEMASK_NORMAL, TEST_CREATURE_ENTRY, _location))
    {
        delete creature;
        INTERNAL_TEST_ASSERT(false);
    }

    creature->SaveToDB(GetMap()->GetId(), (1 << GetMap()->GetSpawnMode()));
    ObjectGuid::LowType db_guid = creature->GetSpawnId();
    creature->LoadFromDB(db_guid, GetMap(), true, false);

    sObjectMgr->AddCreatureToGrid(db_guid, sObjectMgr->GetCreatureData(db_guid));
    return creature;
}

void TestCase::CleanupDBCreature(Creature* c)
{
    if (!c)
        return;

    c->DisappearAndDie();
    c->DeleteFromDB();
    delete c;
}