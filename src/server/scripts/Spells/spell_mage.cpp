#include "SpellMgr.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"

enum MageSpells
{
    SPELL_MAGE_BLAZING_SPEED = 31643,
    SPELL_MAGE_BURNOUT = 29077,
    SPELL_MAGE_COLD_SNAP = 11958,
    SPELL_MAGE_FROST_WARDING_R1 = 11189,
    SPELL_MAGE_MOLTEN_SHIELD_R1 = 11094,
    SPELL_MAGE_IGNITE = 12654,
    SPELL_MAGE_MASTER_OF_ELEMENTS_ENERGIZE = 29077,
    SPELL_MAGE_SQUIRREL_FORM = 32813,
    SPELL_MAGE_GIRAFFE_FORM = 32816,
    SPELL_MAGE_SERPENT_FORM = 32817,
    SPELL_MAGE_DRAGONHAWK_FORM = 32818,
    SPELL_MAGE_WORGEN_FORM = 32819,
    SPELL_MAGE_SHEEP_FORM = 32820,
    SPELL_MAGE_GLYPH_OF_BLAST_WAVE = 62126,
    SPELL_MAGE_CHILLED = 12484,
    SPELL_MAGE_MANA_SURGE = 37445,
    SPELL_MAGE_MAGIC_ABSORPTION_MANA = 29442,
    SPELL_MAGE_ARCANE_SURGE = 37436,
    SPELL_MAGE_COMBUSTION = 11129,
    SPELL_MAGE_COMBUSTION_PROC = 28682,
#ifdef LICH_KING
    SPELL_MAGE_INCANTERS_ABSORBTION_R1 = 44394,
    SPELL_MAGE_INCANTERS_ABSORBTION_TRIGGERED = 44413,
    SPELL_MAGE_HOT_STREAK_PROC = 48108,
    SPELL_MAGE_FOCUS_MAGIC_PROC = 54648,
    SPELL_MAGE_FROST_WARDING_TRIGGERED = 57776,
    SPELL_MAGE_GLYPH_OF_ETERNAL_WATER = 70937,
    SPELL_MAGE_SHATTERED_BARRIER = 55080,
    SPELL_MAGE_SUMMON_WATER_ELEMENTAL_PERMANENT = 70908,
    SPELL_MAGE_SUMMON_WATER_ELEMENTAL_TEMPORARY = 70907,
    SPELL_MAGE_ARCANE_POTENCY_RANK_1 = 57529,
    SPELL_MAGE_ARCANE_POTENCY_RANK_2 = 57531,
    SPELL_MAGE_MISSILE_BARRAGE = 44401,
    SPELL_MAGE_EMPOWERED_FIRE_PROC = 67545,
    SPELL_MAGE_T10_2P_BONUS = 70752,
    SPELL_MAGE_T10_2P_BONUS_EFFECT = 70753,
    SPELL_MAGE_T8_4P_BONUS = 64869,
    SPELL_MAGE_FINGERS_OF_FROST_AURASTATE_AURA = 44544,
#endif
};

// -29441 - Magic Absorption
class spell_mage_magic_absorption : public SpellScriptLoader
{
public:
    spell_mage_magic_absorption() : SpellScriptLoader("spell_mage_magic_absorption") { }

    class spell_mage_magic_absorption_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_mage_magic_absorption_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ SPELL_MAGE_MAGIC_ABSORPTION_MANA });
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* caster = eventInfo.GetActionTarget();
            CastSpellExtraArgs args(aurEff);
            args.AddSpellBP0(CalculatePct(caster->GetMaxPower(POWER_MANA), aurEff->GetAmount()));
            caster->CastSpell(caster, SPELL_MAGE_MAGIC_ABSORPTION_MANA, args);
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(spell_mage_magic_absorption_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_mage_magic_absorption_AuraScript();
    }
};

// -29074 - Master of Elements
class spell_mage_master_of_elements : public SpellScriptLoader
{
public:
    spell_mage_master_of_elements() : SpellScriptLoader("spell_mage_master_of_elements") { }

    class spell_mage_master_of_elements_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_mage_master_of_elements_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ SPELL_MAGE_MASTER_OF_ELEMENTS_ENERGIZE });
        }

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            DamageInfo* damageInfo = eventInfo.GetDamageInfo();
            if (!damageInfo || !damageInfo->GetSpellInfo())
                return false;

            return true;
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            int32 mana = eventInfo.GetDamageInfo()->GetSpellInfo()->CalcPowerCost(GetTarget(), eventInfo.GetDamageInfo()->GetSchoolMask());
            mana = CalculatePct(mana, aurEff->GetAmount());

            if (mana > 0)
            {
                CastSpellExtraArgs args(aurEff);
                args.AddSpellBP0(mana);
                GetTarget()->CastSpell(GetTarget(), SPELL_MAGE_MASTER_OF_ELEMENTS_ENERGIZE, args);
            }
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_mage_master_of_elements_AuraScript::CheckProc);
            OnEffectProc += AuraEffectProcFn(spell_mage_master_of_elements_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_mage_master_of_elements_AuraScript();
    }
};

// -543  - Fire Ward
// -6143 - Frost Ward
class spell_mage_fire_frost_ward : public SpellScriptLoader
{
public:
    spell_mage_fire_frost_ward() : SpellScriptLoader("spell_mage_fire_frost_ward") { }

    class spell_mage_fire_frost_ward_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_mage_fire_frost_ward_AuraScript);

        void CalculateAmount0(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
        {
            canBeRecalculated = false;
            if (Unit* caster = GetCaster())
            {
#ifdef LICH_KING
                // +80.68% from sp bonus
                float bonus = 0.8068f;
#else
                //+10% from +spd bonus, to confirm
                float bonus = 0.1;
#endif

                bonus *= caster->SpellBaseHealingBonusDone(GetSpellInfo()->GetSchoolMask());
                bonus *= caster->CalculateSpellpowerCoefficientLevelPenalty(GetSpellInfo());

                amount += int32(bonus);
            }
        }

        void CalculateAmount1(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
        {
            canBeRecalculated = false;
            if (Unit* caster = GetCaster())
            {
                //both molten shield and frost warding have a dummy effect containing the reflect % value
                //Also note fire ward has its own family flag but frost ward does not, making it complicated to implement this as an aura modifier
                switch (GetSpellInfo()->SchoolMask)
                {
                case SPELL_SCHOOL_MASK_FROST:
                    if (AuraEffect* effect = caster->GetAuraEffectOfRankedSpell(SPELL_MAGE_FROST_WARDING_R1, EFFECT_0))
                        amount += effect->GetSpellInfo()->Effects[EFFECT_1].CalcValue();
                    break;
                case SPELL_SCHOOL_MASK_FIRE:
                    if (AuraEffect* effect = caster->GetAuraEffectOfRankedSpell(SPELL_MAGE_MOLTEN_SHIELD_R1, EFFECT_1))
                        amount += effect->GetSpellInfo()->Effects[EFFECT_0].CalcValue();
                    break;
                }
            }
        }

        void Register() override
        {
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_mage_fire_frost_ward_AuraScript::CalculateAmount0, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_mage_fire_frost_ward_AuraScript::CalculateAmount1, EFFECT_1, SPELL_AURA_REFLECT_SPELLS_SCHOOL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_mage_fire_frost_ward_AuraScript();
    }
};

// -11426 - Ice Barrier
class spell_mage_ice_barrier : public SpellScriptLoader
{
public:
    spell_mage_ice_barrier() : SpellScriptLoader("spell_mage_ice_barrier") { }

    class spell_mage_ice_barrier_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_mage_ice_barrier_AuraScript);

        void CalculateAmount(AuraEffect const* aurEff, int32& amount, bool& canBeRecalculated)
        {
            canBeRecalculated = false;
            if (Unit* caster = GetCaster())
            {
#ifdef LICH_KING
                // +80.68% from sp bonus
                float bonus = 0.8068f;
#else
                //+10% from +spd bonus, to confirm
                float bonus = 0.1;
#endif

                bonus *= caster->SpellBaseHealingBonusDone(GetSpellInfo()->GetSchoolMask());

                // TC: Glyph of Ice Barrier: its weird having a SPELLMOD_ALL_EFFECTS here but its blizzards doing 
                // TC: Glyph of Ice Barrier is only applied at the spell damage bonus because it was already applied to the base value in CalculateSpellDamage
                bonus = caster->ApplyEffectModifiers(GetSpellInfo(), aurEff->GetEffIndex(), bonus);

                bonus *= caster->CalculateSpellpowerCoefficientLevelPenalty(GetSpellInfo());

                amount += int32(bonus);
            }
        }

        void Register() override
        {
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_mage_ice_barrier_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_mage_ice_barrier_AuraScript();
    }
};

// -11185 - Improved Blizzard
class spell_mage_imp_blizzard : public SpellScriptLoader
{
public:
    spell_mage_imp_blizzard() : SpellScriptLoader("spell_mage_imp_blizzard") { }

    class spell_mage_imp_blizzard_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_mage_imp_blizzard_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ SPELL_MAGE_CHILLED });
        }

        void HandleChill(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();
            uint32 triggerSpellId = sSpellMgr->GetSpellWithRank(SPELL_MAGE_CHILLED, GetSpellInfo()->GetRank());
            eventInfo.GetActor()->CastSpell(eventInfo.GetProcTarget(), triggerSpellId, aurEff);
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(spell_mage_imp_blizzard_AuraScript::HandleChill, EFFECT_0, SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_mage_imp_blizzard_AuraScript();
    }
};

// 37424 - Improved Mana Shield (Incanter's Regalia set 4 pieces)
class spell_mage_improved_mana_shield : public SpellScriptLoader
{
public:
    spell_mage_improved_mana_shield() : SpellScriptLoader("spell_mage_improved_mana_shield") { }

    class spell_mage_improved_mana_shield_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_mage_improved_mana_shield_AuraScript);

        bool Validate(SpellInfo const* spellInfo) override
        {
            return ValidateSpellInfo({ SPELL_MAGE_ARCANE_SURGE });
        }

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            return GetTarget()->HasAuraType(SPELL_AURA_MANA_SHIELD);
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            Unit* caster = eventInfo.GetActionTarget();
            caster->CastSpell(caster, SPELL_MAGE_ARCANE_SURGE, aurEff);
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(spell_mage_improved_mana_shield_AuraScript::HandleProc, EFFECT_1, SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
            DoCheckProc += AuraCheckProcFn(spell_mage_improved_mana_shield_AuraScript::CheckProc);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_mage_improved_mana_shield_AuraScript();
    }
};

// -1463 - Mana Shield
// Improved mana shield is handled in 37424 aura
class spell_mage_mana_shield : public SpellScriptLoader
{
public:
    spell_mage_mana_shield() : SpellScriptLoader("spell_mage_mana_shield") { }

    class spell_mage_mana_shield_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_mage_mana_shield_AuraScript);

        void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& canBeRecalculated)
        {
            canBeRecalculated = false;
            if (Unit* caster = GetCaster())
            {
#ifdef LICH_KING
                // +80.53% from sp bonus
                float bonus = 0.8053f;
#else

                // +50% from +spd bonus, to confirm
                float bonus = 0.5f;
#endif

                bonus *= caster->SpellBaseHealingBonusDone(GetSpellInfo()->GetSchoolMask());
                bonus *= caster->CalculateSpellpowerCoefficientLevelPenalty(GetSpellInfo());

                amount += int32(bonus);
            }
        }

        void Register() override
        {
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_mage_mana_shield_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_MANA_SHIELD);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_mage_mana_shield_AuraScript();
    }
};

// -11119 - Ignite
class spell_mage_ignite : public SpellScriptLoader
{
public:
    spell_mage_ignite() : SpellScriptLoader("spell_mage_ignite") { }

    class spell_mage_ignite_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_mage_ignite_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ SPELL_MAGE_IGNITE });
        }

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            return eventInfo.GetDamageInfo() && eventInfo.GetProcTarget();
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            SpellInfo const* igniteDot = sSpellMgr->AssertSpellInfo(SPELL_MAGE_IGNITE);
            int32 pct = 8 * GetSpellInfo()->GetRank();

            ASSERT(igniteDot->GetMaxTicks() > 0);
            int32 amount = int32(CalculatePct(eventInfo.GetDamageInfo()->GetDamage(), pct) / igniteDot->GetMaxTicks());

            CastSpellExtraArgs args(aurEff);
            args.AddSpellBP0(amount);
            GetTarget()->CastSpell(eventInfo.GetProcTarget(), SPELL_MAGE_IGNITE, args);
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_mage_ignite_AuraScript::CheckProc);
            OnEffectProc += AuraEffectProcFn(spell_mage_ignite_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_mage_ignite_AuraScript();
    }
};

// 11129 - Combustion
class spell_mage_combustion : public SpellScriptLoader
{
public:
    spell_mage_combustion() : SpellScriptLoader("spell_mage_combustion") { }

    class spell_mage_combustion_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_mage_combustion_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ SPELL_MAGE_COMBUSTION_PROC });
        }

        void HandleProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            if (GetCharges() == 0)
                eventInfo.GetActor()->RemoveAurasDueToSpell(SPELL_MAGE_COMBUSTION_PROC);
        }

        void Register() override
        {
            OnEffectProc += AuraEffectProcFn(spell_mage_combustion_AuraScript::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_mage_combustion_AuraScript();
    }
};

// 28682 - Combustion proc
class spell_mage_combustion_proc : public SpellScriptLoader
{
public:
    spell_mage_combustion_proc() : SpellScriptLoader("spell_mage_combustion_proc") { }

    class spell_mage_combustion_proc_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_mage_combustion_proc_AuraScript);

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ SPELL_MAGE_COMBUSTION });
        }

        void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            GetTarget()->RemoveAurasDueToSpell(SPELL_MAGE_COMBUSTION);
        }

        void Register() override
        {
            AfterEffectRemove += AuraEffectRemoveFn(spell_mage_combustion_proc_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_ADD_FLAT_MODIFIER, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const override
    {
        return new spell_mage_combustion_proc_AuraScript();
    }
};

void AddSC_mage_spell_scripts()
{
    new spell_mage_fire_frost_ward();
    new spell_mage_ice_barrier();
    new spell_mage_improved_mana_shield();
    new spell_mage_mana_shield();
    new spell_mage_imp_blizzard();
    new spell_mage_magic_absorption();
    new spell_mage_master_of_elements();
    new spell_mage_ignite();
    new spell_mage_combustion();
    new spell_mage_combustion_proc();
}
