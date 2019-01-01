#pragma once

#include "HealPriestStrategy.h"

namespace ai
{
    class HolyPriestStrategy : public HealPriestStrategy
    {
    public:
        HolyPriestStrategy(PlayerbotAI* ai);

    public:
        virtual ActionList getDefaultActions();
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
        virtual std::string getName() { return "holy"; }
        virtual int GetType() { return STRATEGY_TYPE_DPS|STRATEGY_TYPE_RANGED; }
    };
}
