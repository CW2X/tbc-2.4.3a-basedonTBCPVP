#pragma once

#include "../Strategy.h"
#include "../generic/RangedCombatStrategy.h"

namespace ai
{
    class GenericWarlockStrategy : public RangedCombatStrategy
    {
    public:
        GenericWarlockStrategy(PlayerbotAI* ai);
        virtual std::string getName() { return "warlock"; }

    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
        virtual ActionList getDefaultActions();
    };
}
