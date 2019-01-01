#pragma once

namespace ai
{
    class RacialsStrategy : public Strategy
    {
    public:
        RacialsStrategy(PlayerbotAI* ai);
        virtual std::string getName() { return "racials"; }
    
    public:
        virtual void InitTriggers(std::list<std::shared_ptr<TriggerNode>> &triggers);
    };

}