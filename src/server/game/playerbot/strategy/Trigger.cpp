#include "../playerbot.h"
#include "Trigger.h"
#include "Action.h"

using namespace ai;

Event Trigger::Check()
{
    if (IsActive())
    {
        Event event(getName());
        return event;
    }
    Event event;
    return event;
}

std::shared_ptr<Value<Unit*>> Trigger::GetTargetValue()
{
    return context->GetValue<Unit*>(GetTargetName());
}

Unit* Trigger::GetTarget()
{
    return GetTargetValue()->Get();
}
