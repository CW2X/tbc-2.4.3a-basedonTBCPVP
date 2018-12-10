
#include "../../playerbot.h"
#include "NearestNpcsValue.h"

#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"


using namespace ai;
using namespace Trinity;

void NearestNpcsValue::FindUnits(list<Unit*> &targets)
{
    AnyFriendlyUnitInObjectRangeCheck u_check(bot, bot, range);
    UnitListSearcher<AnyFriendlyUnitInObjectRangeCheck> searcher(bot, targets, u_check);
    Cell::VisitAllObjects(bot, searcher, bot->GetMap()->GetVisibilityRange());
}

bool NearestNpcsValue::AcceptUnit(Unit* unit)
{
    return !dynamic_cast<Player*>(unit);
}
