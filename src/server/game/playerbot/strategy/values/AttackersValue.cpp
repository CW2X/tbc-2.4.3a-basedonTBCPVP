////
#include "../../playerbot.h"
#include "AttackersValue.h"

#include "Pet.h"

using namespace ai;

list<ObjectGuid> AttackersValue::Calculate()
{
    set<Unit*> targets;

    AddAttackersOf(bot, targets);

    Group* group = bot->GetGroup();
    if (group)
        AddAttackersOf(group, targets);

    RemoveNonThreating(targets);

    list<ObjectGuid> result;
    for (set<Unit*>::iterator i = targets.begin(); i != targets.end(); i++)
        result.push_back(ObjectGuid(ObjectGuid((*i)->GetGUID())));

    if (bot->duel && bot->duel->Opponent)
        result.push_back(ObjectGuid(bot->duel->Opponent->GetGUID()));

    return result;
}

void AttackersValue::AddAttackersOf(Group* group, set<Unit*>& targets)
{
    Group::MemberSlotList const& groupSlot = group->GetMemberSlots();
    for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); ++itr)
    {
        Player *member = ObjectAccessor::FindPlayer(itr->guid);
        if (!member || !member->IsAlive() || member == bot)
            continue;

        if (member->IsBeingTeleported())
            return;

        AddAttackersOf(member, targets);

        Pet* pet = member->GetPet();
        if (pet)
            AddAttackersOf(pet, targets);
    }
}

void AttackersValue::AddAttackersOf(Unit* unit, set<Unit*>& targets)
{
    for (auto itr : unit->GetThreatManager().GetUnsortedThreatList())
    {
        Unit *attacker = itr->GetOwner();
        Unit *victim = itr->GetVictim();
        if (victim == unit)
            targets.insert(attacker);
    }
}

void AttackersValue::RemoveNonThreating(set<Unit*>& targets)
{
    for(set<Unit *>::iterator tIter = targets.begin(); tIter != targets.end();)
    {
        Unit* unit = *tIter;
        if(!bot->IsWithinLOSInMap(unit, LINEOFSIGHT_ALL_CHECKS, VMAP::ModelIgnoreFlags::M2) || bot->GetMapId() != unit->GetMapId() || !hasRealThreat(unit))
        {
            set<Unit *>::iterator tIter2 = tIter;
            ++tIter;
            targets.erase(tIter2);
        }
        else
            ++tIter;
    }
}

bool AttackersValue::hasRealThreat(Unit *attacker)
{
    return attacker &&
        attacker->IsInWorld() &&
        attacker->IsAlive() &&
        !attacker->IsPolymorphed() &&
        !attacker->IsInRoots() &&
        !attacker->IsFriendlyTo(bot) &&
        (attacker->GetThreatManager().GetCurrentVictim() || dynamic_cast<Player*>(attacker));
}
