#pragma once

#include "GenericPaladinStrategy.h"

namespace ai
{
    class TankPaladinStrategy : public GenericPaladinStrategy
    {
    public:
        TankPaladinStrategy(PlayerbotAI* ai);

    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
        virtual std::string getName() { return "tank"; }
        virtual ActionList getDefaultActions();
        virtual int GetType() { return STRATEGY_TYPE_TANK | STRATEGY_TYPE_MELEE; }
    };
}
