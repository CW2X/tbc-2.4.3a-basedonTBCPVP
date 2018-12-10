
/*
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "npc_pet_sha_".
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"

enum ShamanSpells
{
    SPELL_SHAMAN_ANGEREDEARTH   = 36213,
#ifdef LICH_KING
    SPELL_SHAMAN_FIREBLAST      = 57984,
#endif
    SPELL_SHAMAN_FIRENOVA       = 12470,
    SPELL_SHAMAN_FIRESHIELD     = 13376
};

enum ShamanEvents
{
    // Earth Elemental
    EVENT_SHAMAN_ANGEREDEARTH   = 1,
    // Fire Elemental
    EVENT_SHAMAN_FIRENOVA       = 1,
    EVENT_SHAMAN_FIRESHIELD     = 2,
#ifdef LICH_KING
    EVENT_SHAMAN_FIREBLAST      = 3
#endif
};

class npc_pet_shaman_earth_elemental : public CreatureScript
{
    public:
        npc_pet_shaman_earth_elemental() : CreatureScript("npc_pet_shaman_earth_elemental") { }

        struct npc_pet_shaman_earth_elementalAI : public ScriptedAI
        {
            npc_pet_shaman_earth_elementalAI(Creature* creature) : ScriptedAI(creature) { }


            void Reset() override
            {
                _events.Reset();
                _events.ScheduleEvent(EVENT_SHAMAN_ANGEREDEARTH, 0);
                me->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_NATURE, true);
            }

            void UpdateAI(const uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                _events.Update(diff);

                if (_events.ExecuteEvent() == EVENT_SHAMAN_ANGEREDEARTH)
                {
                    DoCastVictim(SPELL_SHAMAN_ANGEREDEARTH);
                    _events.ScheduleEvent(EVENT_SHAMAN_ANGEREDEARTH, 5s, 20s);
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_pet_shaman_earth_elementalAI(creature);
        }
};

class npc_pet_shaman_fire_elemental : public CreatureScript
{
    public:
        npc_pet_shaman_fire_elemental() : CreatureScript("npc_pet_shaman_fire_elemental") { }

        struct npc_pet_shaman_fire_elementalAI : public ScriptedAI
        {
            npc_pet_shaman_fire_elementalAI(Creature* creature) : ScriptedAI(creature) { }

            void Reset() override
            {
                _events.Reset();
                _events.ScheduleEvent(EVENT_SHAMAN_FIRENOVA, 5s, 20s);
#ifdef LICH_KING
                _events.ScheduleEvent(EVENT_SHAMAN_FIREBLAST, 5s, 20s);
#endif
                _events.ScheduleEvent(EVENT_SHAMAN_FIRESHIELD, 0);
                me->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FIRE, true);
            }

            void UpdateAI(const uint32 diff) override
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SHAMAN_FIRENOVA:
                            DoCastVictim(SPELL_SHAMAN_FIRENOVA);
                            _events.ScheduleEvent(EVENT_SHAMAN_FIRENOVA, 5s, 20s);
                            break;
                        case EVENT_SHAMAN_FIRESHIELD:
                            DoCastVictim(SPELL_SHAMAN_FIRESHIELD);
                            _events.ScheduleEvent(EVENT_SHAMAN_FIRESHIELD, 2s);
                            break;
#ifdef LICH_KING
                        case EVENT_SHAMAN_FIREBLAST:
                            DoCastVictim(SPELL_SHAMAN_FIREBLAST);
                            _events.ScheduleEvent(EVENT_SHAMAN_FIREBLAST, 5s, 20s);
                            break;
#endif
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap _events;
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_pet_shaman_fire_elementalAI(creature);
        }
};

void AddSC_shaman_pet_scripts()
{
    new npc_pet_shaman_earth_elemental();
    new npc_pet_shaman_fire_elemental();
}
