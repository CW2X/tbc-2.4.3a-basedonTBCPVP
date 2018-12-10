#pragma once

#include "FeralDruidStrategy.h"

namespace ai
{
    class BearTankDruidStrategy : public FeralDruidStrategy
    {
    public:
        BearTankDruidStrategy(PlayerbotAI* ai);

    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
        virtual std::string getName() { return "bear"; }
        virtual ActionList getDefaultActions();
        virtual int GetType() { return STRATEGY_TYPE_TANK | STRATEGY_TYPE_MELEE; }
    };
}
