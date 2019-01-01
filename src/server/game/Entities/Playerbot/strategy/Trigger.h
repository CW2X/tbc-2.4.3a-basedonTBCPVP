#pragma once
#include "Action.h"
#include "Event.h"
#include "../PlayerbotAIAware.h"

#define NEXT_TRIGGERS(name, relevance) \
    virtual std::shared_ptr<NextAction> getNextAction() { return std::make_shared<NextAction>(name, relevance); }

#define BEGIN_TRIGGER(clazz, super) \
class clazz : public super \
    { \
    public: \
        clazz(PlayerbotAI* ai) : super(ai) {} \
    public: \
        virtual bool IsActive();

#define END_TRIGGER() \
    };

namespace ai
{
    class Trigger : public AiNamedObject
    {
    public:
        Trigger(PlayerbotAI* ai, std::string name = "trigger", int checkInterval = 1) : AiNamedObject(ai, name) {
            this->checkInterval = checkInterval;
            ticksElapsed = 0;
        }
        virtual ~Trigger() = default;

    public:
        virtual Event Check();
        virtual void ExternalEvent(std::string param, Player* owner = nullptr) {}
        virtual void ExternalEvent(WorldPacket &packet, Player* owner = nullptr) {}
        virtual bool IsActive() { return false; }
        virtual ActionList getHandlers() { return ActionList(); }
        void Update() {}
        virtual void Reset() {}
        virtual Unit* GetTarget();
        virtual std::shared_ptr<Value<Unit*>> GetTargetValue();
        virtual std::string GetTargetName() { return "self target"; }

        bool needCheck() {
            if (++ticksElapsed >= checkInterval) {
                ticksElapsed = 0;
                return true;
            }
            return false;
        }

    protected:
        int checkInterval;
        int ticksElapsed;
    };


    class TriggerNode
    {
    public:
        TriggerNode(std::string name, ActionList const handlers)
        {
            this->name = name;
            this->handlers = handlers;
            this->trigger = nullptr;
        }
        virtual ~TriggerNode()
        {
            NextAction::destroy(handlers);
        }

    public:
        std::shared_ptr<Trigger> getTrigger() { return trigger; }
        void setTrigger(std::shared_ptr<Trigger> _trigger) { trigger = _trigger; }
        std::string getName() { return name; }

    public:
        ActionList getHandlers() { return NextAction::merge(NextAction::clone(handlers), trigger->getHandlers()); }

    private:
        std::shared_ptr<Trigger> trigger;
        ActionList handlers;
        std::string name;
    };
}
