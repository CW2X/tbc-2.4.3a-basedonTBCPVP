
#include "../../playerbot.h"
#include "PossibleTargetsValue.h"

#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"

using namespace ai;
using namespace Trinity;

void PossibleTargetsValue::FindUnits(list<Unit*> &targets)
{
    AnyUnfriendlyUnitInObjectRangeCheck u_check(bot, bot, range);
    UnitListSearcher<AnyUnfriendlyUnitInObjectRangeCheck> searcher(bot, targets, u_check);
    Cell::VisitAllObjects(bot, searcher, bot->GetMap()->GetVisibilityRange());
}

bool PossibleTargetsValue::AcceptUnit(Unit* unit)
{
    return !unit->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE) &&
            (unit->IsHostileTo(bot) || (unit->GetLevel() > 1 && !unit->IsFriendlyTo(bot)));
}
