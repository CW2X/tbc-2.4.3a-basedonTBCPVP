#include "../generic/NonCombatStrategy.h"
#pragma once

namespace ai
{
    class DpsAssistStrategy : public NonCombatStrategy
    {
    public:
        DpsAssistStrategy(PlayerbotAI* ai) : NonCombatStrategy(ai) {}
        virtual std::string getName() { return "dps assist"; }
        virtual int GetType() { return STRATEGY_TYPE_DPS; }

    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
    };
}
