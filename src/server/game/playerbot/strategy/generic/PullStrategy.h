#pragma once

#include "RangedCombatStrategy.h"

namespace ai
{
    class PullStrategy : public RangedCombatStrategy
    {
    public:
        PullStrategy(PlayerbotAI* ai, std::string action) : RangedCombatStrategy(ai) 
        {
            this->action = action;
        }

    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
        virtual void InitMultipliers(std::list<std::shared_ptr<Multiplier>> &multipliers);
        virtual std::string getName() { return "pull"; }
        virtual ActionList getDefaultActions();

    private:
        std::string action;
    };
}
