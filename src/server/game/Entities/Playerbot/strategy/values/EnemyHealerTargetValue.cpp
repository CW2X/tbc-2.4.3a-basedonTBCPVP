//
#include "../../playerbot.h"
#include "EnemyHealerTargetValue.h"
#include "../../PlayerbotAIConfig.h"

using namespace ai;

Unit* EnemyHealerTargetValue::Calculate()
{
    std::string spell = qualifier;

    list<ObjectGuid> attackers = ai->GetAiObjectContext()->GetValue<list<ObjectGuid> >("attackers")->Get();
    Unit* target = ai->GetAiObjectContext()->GetValue<Unit*>("current target")->Get();
    for (list<ObjectGuid>::iterator i = attackers.begin(); i != attackers.end(); ++i)
    {
        Unit* unit = ai->GetUnit(*i);
        if (!unit || unit == target)
            continue;

        if (bot->GetDistance(unit) > sPlayerbotAIConfig.spellDistance)
            continue;

        if (!ai->IsInterruptableSpellCasting(unit, spell))
            continue;

        Spell* _spell = unit->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        if (_spell && _spell->m_spellInfo->IsPositive())
            return unit;

        _spell = unit->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
        if (_spell && _spell->m_spellInfo->IsPositive())
            return unit;
    }

    return NULL;
}
