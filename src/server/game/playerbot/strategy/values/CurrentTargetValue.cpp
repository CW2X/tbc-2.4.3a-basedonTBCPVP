//
#include "../../playerbot.h"
#include "CurrentTargetValue.h"

using namespace ai;

Unit* CurrentTargetValue::Get()
{
    if (selection.IsEmpty())
        return NULL;

    Unit* unit = ObjectAccessor::GetUnit(*bot, selection);
    if (unit && !bot->IsWithinLOSInMap(unit, LINEOFSIGHT_ALL_CHECKS, VMAP::ModelIgnoreFlags::M2))
        return NULL;

    return unit;
}

void CurrentTargetValue::Set(Unit* target)
{
    selection = target ? ObjectGuid(target->GetGUID()) : ObjectGuid::Empty;
}
