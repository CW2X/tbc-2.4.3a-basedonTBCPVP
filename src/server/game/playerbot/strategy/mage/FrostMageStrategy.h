#pragma once

#include "GenericMageStrategy.h"
#include "../generic/CombatStrategy.h"

namespace ai
{
    class FrostMageStrategy : public GenericMageStrategy
    {
    public:
        FrostMageStrategy(PlayerbotAI* ai);

    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
        virtual std::string getName() { return "frost"; }
        virtual ActionList getDefaultActions();
    };

    class FrostMageAoeStrategy : public CombatStrategy
    {
    public:
        FrostMageAoeStrategy(PlayerbotAI* ai) : CombatStrategy(ai) {}

    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
        virtual std::string getName() { return "frost aoe"; }
    };
}
