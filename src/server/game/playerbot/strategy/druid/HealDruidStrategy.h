#pragma once

#include "GenericDruidStrategy.h"

namespace ai
{
    class HealDruidStrategy : public GenericDruidStrategy
    {
    public:
        HealDruidStrategy(PlayerbotAI* ai);

    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
        virtual std::string getName() { return "heal"; }
        virtual int GetType() { return STRATEGY_TYPE_HEAL; }
    };

}
