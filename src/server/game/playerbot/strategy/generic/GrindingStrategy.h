#include "../generic/NonCombatStrategy.h"
#pragma once

namespace ai
{
    class GrindingStrategy : public NonCombatStrategy
    {
    public:
        GrindingStrategy(PlayerbotAI* ai) : NonCombatStrategy(ai) {}
        virtual std::string getName() { return "grind"; }
        virtual int GetType() { return STRATEGY_TYPE_DPS; }
        ActionList getDefaultActions();

    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
    };



}
