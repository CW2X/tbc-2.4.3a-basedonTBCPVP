#pragma once

#include "../Strategy.h"
#include "PaladinAiObjectContext.h"
#include "../generic/MeleeCombatStrategy.h"

namespace ai
{
    class GenericPaladinStrategy : public MeleeCombatStrategy
    {
    public:
        GenericPaladinStrategy(PlayerbotAI* ai);

    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
        virtual std::string getName() { return "paladin"; }
    };
}
