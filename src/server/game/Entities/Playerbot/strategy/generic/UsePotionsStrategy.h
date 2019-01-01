#pragma once

namespace ai
{
    class UsePotionsStrategy : public Strategy
    {
    public:
        UsePotionsStrategy(PlayerbotAI* ai) : Strategy(ai) {}

    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
        virtual std::string getName() { return "potions"; }
    };

}
