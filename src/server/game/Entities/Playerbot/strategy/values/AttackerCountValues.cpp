////
#include "../../playerbot.h"
#include "AttackerCountValues.h"
#include "../../PlayerbotAIConfig.h"

using namespace ai;

uint8 MyAttackerCountValue::Calculate()
{
    return bot->GetAttackers().size();
}

bool HasAggroValue::Calculate()
{
    Unit* target = GetTarget();
    if (!target)
        return true;

    if (!bot->GetThreatManager().GetAnyTarget())
        return true;  // simulate as target is not atacking anybody yet

    for (auto itr : bot->GetThreatManager().GetUnsortedThreatList())
    {
        Unit *attacker = itr->GetOwner();
        Unit *victim = itr->GetVictim();
        if (victim == bot && target == attacker)
            return true;
    }
    return false;
}

uint8 AttackerCountValue::Calculate()
{
    int count = 0;
    float range = sPlayerbotAIConfig.sightDistance;

    list<ObjectGuid> attackers = context->GetValue<list<ObjectGuid> >("attackers")->Get();
    for (list<ObjectGuid>::iterator i = attackers.begin(); i != attackers.end(); i++)
    {
        Unit* unit = ai->GetUnit(*i);
        if (!unit || !unit->IsAlive())
            continue;

        float distance = bot->GetDistance(unit);
        if (distance <= range)
            count++;
    }

    return count;
}

uint8 BalancePercentValue::Calculate()
{
    float playerLevel = 0,
        attackerLevel = 0;

    Group* group = bot->GetGroup();
    if (group)
    {
        Group::MemberSlotList const& groupSlot = group->GetMemberSlots();
        for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); ++itr)
        {
            Player *player = ObjectAccessor::FindPlayer(itr->guid);
            if( !player || !player->IsAlive())
                continue;

            playerLevel += player->GetLevel();
        }
    }

    list<ObjectGuid> v = context->GetValue<list<ObjectGuid> >("attackers")->Get();

    for (list<ObjectGuid>::iterator i = v.begin(); i!=v.end(); i++)
    {
        Creature* creature = ai->GetCreature((*i));
        if (!creature || !creature->IsAlive())
            continue;

        uint32 level = creature->GetLevel();

        switch (creature->GetCreatureTemplate()->rank) {
        case CREATURE_ELITE_RARE:
            level *= 2;
            break;
        case CREATURE_ELITE_ELITE:
            level *= 3;
            break;
        case CREATURE_ELITE_RAREELITE:
            level *= 3;
            break;
        case CREATURE_ELITE_WORLDBOSS:
            level *= 5;
            break;
        }
        attackerLevel += level;
    }

    if (!attackerLevel)
        return 100;

    float percent = playerLevel * 100 / attackerLevel;
    return percent <= 200 ? (uint8)percent : 200;
}

