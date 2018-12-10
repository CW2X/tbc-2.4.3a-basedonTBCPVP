#pragma once

#include "GenericMageStrategy.h"
#include "../generic/NonCombatStrategy.h"

namespace ai
{
    class GenericMageNonCombatStrategy : public NonCombatStrategy
    {
    public:
        GenericMageNonCombatStrategy(PlayerbotAI* ai);
        virtual std::string getName() { return "nc"; }

    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
    };

    class MageBuffManaStrategy : public Strategy
    {
    public:
        MageBuffManaStrategy(PlayerbotAI* ai) : Strategy(ai) {}

    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
        virtual std::string getName() { return "bmana"; }
    };

    class MageBuffDpsStrategy : public Strategy
    {
    public:
        MageBuffDpsStrategy(PlayerbotAI* ai) : Strategy(ai) {}

    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
        virtual std::string getName() { return "bdps"; }
    };
}
