//
#include "../../playerbot.h"
#include "DuelTargetValue.h"

using namespace ai;

Unit* DuelTargetValue::Calculate()
{
    return bot->duel ? bot->duel->Opponent : NULL;
}
