//
#include "../../playerbot.h"
#include "GenericTriggers.h"
#include "CureTriggers.h"

using namespace ai;

bool NeedCureTrigger::IsActive() 
{
    Unit* target = GetTarget();
    return target && ai->HasAuraToDispel(target, dispelType);
}

std::shared_ptr<Value<Unit*>> PartyMemberNeedCureTrigger::GetTargetValue()
{
    return context->GetValue<Unit*>("party member to dispel", dispelType);
}
