#pragma once

#include "../Strategy.h"
#include "../generic/CombatStrategy.h"

namespace ai
{
    class GenericShamanStrategy : public CombatStrategy
    {
    public:
        GenericShamanStrategy(PlayerbotAI* ai);

    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);

    };

    class ShamanBuffDpsStrategy : public Strategy
    {
    public:
        ShamanBuffDpsStrategy(PlayerbotAI* ai) : Strategy(ai) {}

    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
        virtual std::string getName() { return "bdps"; }

    };

    class ShamanBuffManaStrategy : public Strategy
    {
    public:
        ShamanBuffManaStrategy(PlayerbotAI* ai) : Strategy(ai) {}

    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
        virtual std::string getName() { return "bmana"; }

    };
}
