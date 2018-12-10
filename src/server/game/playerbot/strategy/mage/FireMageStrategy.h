#pragma once

#include "GenericMageStrategy.h"
#include "../generic/CombatStrategy.h"

namespace ai
{
    class FireMageStrategy : public GenericMageStrategy
    {
    public:
        FireMageStrategy(PlayerbotAI* ai) : GenericMageStrategy(ai) {}

    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
        virtual std::string getName() { return "fire"; }
        virtual ActionList getDefaultActions();
    };

    class FireMageAoeStrategy : public CombatStrategy
    {
    public:
        FireMageAoeStrategy(PlayerbotAI* ai) : CombatStrategy(ai) {}

    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
        virtual std::string getName() { return "fire aoe"; }
    };
}
