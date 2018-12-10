#include "../generic/NonCombatStrategy.h"
#pragma once

namespace ai
{
    class MoveRandomStrategy : public NonCombatStrategy
    {
    public:
        MoveRandomStrategy(PlayerbotAI* ai) : NonCombatStrategy(ai) {}
        virtual std::string getName() { return "move random"; }

    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
    };

}
