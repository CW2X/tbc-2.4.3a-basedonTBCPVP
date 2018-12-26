#include "TotemAI.h"
#include "Totem.h"
#include "Creature.h"
#include "Player.h"
#include "DBCStores.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "SpellMgr.h"

#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"

int
TotemAI::Permissible(const Creature *creature)
{
    if( creature->IsTotem() )
        return PERMIT_BASE_PROACTIVE;

    return PERMIT_BASE_NO;
}

TotemAI::TotemAI(Creature *c) : CreatureAI(c), i_totem(static_cast<Totem&>(*c))
{
}

TotemAI::~TotemAI()
{
}

void TotemAI::MoveInLineOfSight(Unit *)
{
}

void TotemAI::EnterEvadeMode(EvadeReason why)
{
    i_totem.CombatStop();
}

void TotemAI::UpdateAI(const uint32 /*diff*/)
{
    if (i_totem.GetTotemType() != TOTEM_ACTIVE)
        return;

    if (!i_totem.IsAlive() || i_totem.IsNonMeleeSpellCast(false))
        return;

    // Search spell
    SpellInfo const *spellInfo = sSpellMgr->GetSpellInfo(i_totem.GetSpell());
    if (!spellInfo)
        return;

    // Get spell rangy
    float max_range = spellInfo->GetMaxRange(false, me->GetSpellModOwner());

    // pointer to appropriate target if found any
    Unit* victim = i_victimGuid ? ObjectAccessor::GetUnit(i_totem, i_victimGuid) : nullptr;

    // Search victim if no, not attackable, or out of range, or friendly (possible in case duel end)
    if( !victim ||
        i_totem.CanCreatureAttack(victim) != CAN_ATTACK_RESULT_OK || !i_totem.IsWithinDistInMap(victim, max_range) ||
        i_totem.IsFriendlyTo(victim) || !me->CanSeeOrDetect(victim) )
    {
        victim = nullptr;
        Trinity::NearestAttackableUnitInObjectRangeCheck u_check(me, me->GetCharmerOrOwnerOrSelf(), max_range);
        Trinity::UnitLastSearcher<Trinity::NearestAttackableUnitInObjectRangeCheck> checker(me, victim, u_check);
        Cell::VisitGridObjects(me, checker, max_range);
    }

    // If have target
    if (victim)
    {
        // remember
        i_victimGuid = victim->GetGUID();

        // attack
        i_totem.SetInFront(victim);                         // client change orientation by self
        i_totem.CastSpell(victim, i_totem.GetSpell(), TRIGGERED_NONE);
    }
    else
        i_victimGuid.Clear();
}

void
TotemAI::AttackStart(Unit *)
{
    // Sentry totem sends ping on attack
    if (i_totem.GetEntry() == SENTRY_TOTEM_ENTRY && i_totem.GetOwner()->GetTypeId() == TYPEID_PLAYER)
        (i_totem.GetOwner()->ToPlayer())->GetSession()->SendMinimapPing(i_totem.GetGUID(), i_totem.GetPositionX(), i_totem.GetPositionY());
}

