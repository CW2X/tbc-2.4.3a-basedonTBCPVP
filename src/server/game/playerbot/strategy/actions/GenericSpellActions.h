#pragma once

#include "../Action.h"
#include "../../PlayerbotAIConfig.h"

#define BEGIN_SPELL_ACTION(clazz, name) \
class clazz : public CastSpellAction \
        { \
        public: \
        clazz(PlayerbotAI* ai) : CastSpellAction(ai, name) {} \


#define END_SPELL_ACTION() \
    };

#define BEGIN_DEBUFF_ACTION(clazz, name) \
class clazz : public CastDebuffSpellAction \
        { \
        public: \
        clazz(PlayerbotAI* ai) : CastDebuffSpellAction(ai, name) {} \

#define BEGIN_RANGED_SPELL_ACTION(clazz, name) \
class clazz : public CastSpellAction \
        { \
        public: \
        clazz(PlayerbotAI* ai) : CastSpellAction(ai, name) {} \

#define BEGIN_MELEE_SPELL_ACTION(clazz, name) \
class clazz : public CastMeleeSpellAction \
        { \
        public: \
        clazz(PlayerbotAI* ai) : CastMeleeSpellAction(ai, name) {} \


#define END_RANGED_SPELL_ACTION() \
    };


#define BEGIN_BUFF_ON_PARTY_ACTION(clazz, name) \
class clazz : public BuffOnPartyAction \
        { \
        public: \
        clazz(PlayerbotAI* ai) : BuffOnPartyAction(ai, name) {}

namespace ai
{
    class CastSpellAction : public Action
    {
    public:
        CastSpellAction(PlayerbotAI* ai, std::string spell) : Action(ai, spell),
            range(sPlayerbotAIConfig.spellDistance)
        {
            this->spell = spell;
        }

        virtual std::string GetTargetName() { return "current target"; };
        virtual bool Execute(Event event);
        virtual bool isPossible();
        virtual bool isUseful();
        virtual ActionThreatType getThreatType() { return ACTION_THREAT_SINGLE; }

        virtual ActionList getPrerequisites()
        {
            if (range > sPlayerbotAIConfig.spellDistance)
                return ActionList();
            else if (range > ATTACK_DISTANCE)
                return NextAction::merge(NextAction::array({ std::make_shared<NextAction>("reach spell") }), Action::getPrerequisites());
            else
                return NextAction::merge(NextAction::array({ std::make_shared<NextAction>("reach melee") }), Action::getPrerequisites());
        }

    protected:
        std::string spell;
        float range;
    };

    //---------------------------------------------------------------------------------------------------------------------
    class CastAuraSpellAction : public CastSpellAction
    {
    public:
        CastAuraSpellAction(PlayerbotAI* ai, std::string spell) : CastSpellAction(ai, spell) {}

        virtual bool isUseful();
    };

    //---------------------------------------------------------------------------------------------------------------------
    class CastMeleeSpellAction : public CastSpellAction
    {
    public:
        CastMeleeSpellAction(PlayerbotAI* ai, std::string spell) : CastSpellAction(ai, spell) {
            range = ATTACK_DISTANCE;
        }
    };

    //---------------------------------------------------------------------------------------------------------------------
    class CastDebuffSpellAction : public CastAuraSpellAction
    {
    public:
        CastDebuffSpellAction(PlayerbotAI* ai, std::string spell) : CastAuraSpellAction(ai, spell) {}
    };

    class CastDebuffSpellOnAttackerAction : public CastAuraSpellAction
    {
    public:
        CastDebuffSpellOnAttackerAction(PlayerbotAI* ai, std::string spell) : CastAuraSpellAction(ai, spell) {}
        std::shared_ptr<Value<Unit*>> GetTargetValue()
        {
            return context->GetValue<Unit*>("attacker without aura", spell);
        }
        virtual std::string getName() { return spell + " on attacker"; }
        virtual ActionThreatType getThreatType() { return ACTION_THREAT_AOE; }
    };

    class CastBuffSpellAction : public CastAuraSpellAction
    {
    public:
        CastBuffSpellAction(PlayerbotAI* ai, std::string spell) : CastAuraSpellAction(ai, spell)
        {
            range = sPlayerbotAIConfig.spellDistance;
        }

        virtual std::string GetTargetName() { return "self target"; }
    };

    class CastEnchantItemAction : public CastSpellAction
    {
    public:
        CastEnchantItemAction(PlayerbotAI* ai, std::string spell) : CastSpellAction(ai, spell)
        {
            range = sPlayerbotAIConfig.spellDistance;
        }

        virtual bool isUseful();
        virtual std::string GetTargetName() { return "self target"; }
    };

    //---------------------------------------------------------------------------------------------------------------------

    class CastHealingSpellAction : public CastAuraSpellAction
    {
    public:
        CastHealingSpellAction(PlayerbotAI* ai, std::string spell, uint8 estAmount = 15.0f) : CastAuraSpellAction(ai, spell)
        {
            this->estAmount = estAmount;
            range = sPlayerbotAIConfig.spellDistance;
        }
        virtual std::string GetTargetName() { return "self target"; }
        virtual bool isUseful();
        virtual ActionThreatType getThreatType() { return ACTION_THREAT_AOE; }

    protected:
        uint8 estAmount;
    };

    class CastAoeHealSpellAction : public CastHealingSpellAction
    {
    public:
        CastAoeHealSpellAction(PlayerbotAI* ai, std::string spell, uint8 estAmount = 15.0f) : CastHealingSpellAction(ai, spell, estAmount) {}
        virtual std::string GetTargetName() { return "party member to heal"; }
        virtual bool isUseful();
    };

    class CastCureSpellAction : public CastSpellAction
    {
    public:
        CastCureSpellAction(PlayerbotAI* ai, std::string spell) : CastSpellAction(ai, spell)
        {
            range = sPlayerbotAIConfig.spellDistance;
        }

        virtual std::string GetTargetName() { return "self target"; }
    };

    class PartyMemberActionNameSupport {
    public:
        PartyMemberActionNameSupport(std::string spell)
        {
            name = std::string(spell) + " on party";
        }

        virtual std::string getName() { return name; }

    private:
        string name;
    };

    class HealPartyMemberAction : public CastHealingSpellAction, public PartyMemberActionNameSupport
    {
    public:
        HealPartyMemberAction(PlayerbotAI* ai, std::string spell, uint8 estAmount = 15.0f) :
            CastHealingSpellAction(ai, spell, estAmount), PartyMemberActionNameSupport(spell) {}

        virtual std::string GetTargetName() { return "party member to heal"; }
        virtual std::string getName() { return PartyMemberActionNameSupport::getName(); }
    };

    class ResurrectPartyMemberAction : public CastSpellAction
    {
    public:
        ResurrectPartyMemberAction(PlayerbotAI* ai, std::string spell) : CastSpellAction(ai, spell) {}

        virtual std::string GetTargetName() { return "party member to resurrect"; }
    };
    //---------------------------------------------------------------------------------------------------------------------

    class CurePartyMemberAction : public CastSpellAction, public PartyMemberActionNameSupport
    {
    public:
        CurePartyMemberAction(PlayerbotAI* ai, std::string spell, uint32 dispelType) :
            CastSpellAction(ai, spell), PartyMemberActionNameSupport(spell)
        {
            this->dispelType = dispelType;
        }

        virtual std::shared_ptr<Value<Unit*>> GetTargetValue();
        virtual std::string getName() { return PartyMemberActionNameSupport::getName(); }

    protected:
        uint32 dispelType;
    };

    //---------------------------------------------------------------------------------------------------------------------

    class BuffOnPartyAction : public CastBuffSpellAction, public PartyMemberActionNameSupport
    {
    public:
        BuffOnPartyAction(PlayerbotAI* ai, std::string spell) :
            CastBuffSpellAction(ai, spell), PartyMemberActionNameSupport(spell) {}
    public:
        virtual std::shared_ptr<Value<Unit*>> GetTargetValue();
        virtual std::string getName() { return PartyMemberActionNameSupport::getName(); }
    };

    //---------------------------------------------------------------------------------------------------------------------

    class CastShootAction : public CastSpellAction
    {
    public:
        CastShootAction(PlayerbotAI* ai) : CastSpellAction(ai, "shoot") {}
        virtual ActionThreatType getThreatType() { return ACTION_THREAT_NONE; }
    };

    class CastLifeBloodAction : public CastHealingSpellAction
    {
    public:
        CastLifeBloodAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "lifeblood") {}
    };

    class CastGiftOfTheNaaruAction : public CastHealingSpellAction
    {
    public:
        CastGiftOfTheNaaruAction(PlayerbotAI* ai) : CastHealingSpellAction(ai, "gift of the naaru") {}
    };

    class CastArcaneTorrentAction : public CastBuffSpellAction
    {
    public:
        CastArcaneTorrentAction(PlayerbotAI* ai) : CastBuffSpellAction(ai, "arcane torrent") {}
    };

    class CastSpellOnEnemyHealerAction : public CastSpellAction
    {
    public:
        CastSpellOnEnemyHealerAction(PlayerbotAI* ai, std::string spell) : CastSpellAction(ai, spell) {}
        std::shared_ptr<Value<Unit*>> GetTargetValue()
        {
            return context->GetValue<Unit*>("enemy healer target", spell);
        }
        virtual std::string getName() { return spell + " on enemy healer"; }
    };
}
