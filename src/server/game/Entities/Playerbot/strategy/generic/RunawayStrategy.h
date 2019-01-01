#include "../generic/NonCombatStrategy.h"
#pragma once

namespace ai
{
    class RunawayStrategy : public NonCombatStrategy
       {
       public:
           RunawayStrategy(PlayerbotAI* ai) : NonCombatStrategy(ai) {}
           virtual std::string getName() { return "runaway"; }
           virtual ActionList getDefaultActions();
           virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
       };


}
