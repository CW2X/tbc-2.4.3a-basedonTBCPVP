
/* ScriptData
SDName: SimpleAI
SD%Complete: 100
SDComment: Base Class for SimpleAI creatures
SDCategory: Creatures
EndScriptData */


#include "SimpleAI.h"
#include "ScriptedCreature.h"

SimpleAI::SimpleAI(Creature *c) : ScriptedAI(c)
{
    //Clear all data
    Aggro_Text[0] = nullptr;
    Aggro_Text[1] = nullptr;
    Aggro_Text[2] = nullptr;
    Aggro_Say[0] = false;
    Aggro_Say[1] = false;
    Aggro_Say[2] = false;
    Aggro_Sound[0] = 0;
    Aggro_Sound[1] = 0;
    Aggro_Sound[2] = 0;

    Death_Text[0] = nullptr;
    Death_Text[1] = nullptr;
    Death_Text[2] = nullptr;
    Death_Say[0] = false;
    Death_Say[1] = false;
    Death_Say[2] = false;
    Death_Sound[0] = 0;
    Death_Sound[1] = 0;
    Death_Sound[2] = 0;
    Death_Spell = 0;
    Death_Target_Type = 0;

    Kill_Text[0] = nullptr;
    Kill_Text[1] = nullptr;
    Kill_Text[2] = nullptr;
    Kill_Say[0] = false;
    Kill_Say[1] = false;
    Kill_Say[2] = false;
    Kill_Sound[0] = 0;
    Kill_Sound[1] = 0;
    Kill_Sound[2] = 0;
    Kill_Spell = 0;
    Kill_Target_Type = 0;

    memset(Spell,0,sizeof(Spell));
}

void SimpleAI::Reset()
{
}

void SimpleAI::JustEngagedWith(Unit *who)
{
            //Reset cast timers
            if (Spell[0].First_Cast >= 0)
                Spell_Timer[0] = Spell[0].First_Cast;
            else Spell_Timer[0] = 1000;
            if (Spell[1].First_Cast >= 0)
                Spell_Timer[1] = Spell[1].First_Cast;
            else Spell_Timer[1] = 1000;
            if (Spell[2].First_Cast >= 0)
                Spell_Timer[2] = Spell[2].First_Cast;
            else Spell_Timer[2] = 1000;
            if (Spell[3].First_Cast >= 0)
                Spell_Timer[3] = Spell[3].First_Cast;
            else Spell_Timer[3] = 1000;
            if (Spell[4].First_Cast >= 0)
                Spell_Timer[4] = Spell[4].First_Cast;
            else Spell_Timer[4] = 1000;
            if (Spell[5].First_Cast >= 0)
                Spell_Timer[5] = Spell[5].First_Cast;
            else Spell_Timer[5] = 1000;
            if (Spell[6].First_Cast >= 0)
                Spell_Timer[6] = Spell[6].First_Cast;
            else Spell_Timer[6] = 1000;
            if (Spell[7].First_Cast >= 0)
                Spell_Timer[7] = Spell[7].First_Cast;
            else Spell_Timer[7] = 1000;
            if (Spell[8].First_Cast >= 0)
                Spell_Timer[8] = Spell[8].First_Cast;
            else Spell_Timer[8] = 1000;
            if (Spell[9].First_Cast >= 0)
                Spell_Timer[9] = Spell[9].First_Cast;
            else Spell_Timer[9] = 1000;

            uint32 random_text = rand()%3;

            //Random yell
            if (Aggro_Text[random_text])
            {
                if (Aggro_Say[random_text])
                    me->Say(Aggro_Text[random_text], LANG_UNIVERSAL, who);
                else 
                    me->Yell(Aggro_Text[random_text], LANG_UNIVERSAL, who);
            }

            //Random sound
            if (Aggro_Sound[random_text])
                DoPlaySoundToSet(me, Aggro_Sound[random_text]);
}

void SimpleAI::KilledUnit(Unit *victim)
{
    uint32 random_text = rand()%3;

    //Random yell
    if (Kill_Text[random_text])
    {
        if (Kill_Say[random_text])
            me->Say(Kill_Text[random_text], LANG_UNIVERSAL, victim);
        else 
            me->Yell(Kill_Text[random_text], LANG_UNIVERSAL, victim);
    }

    //Random sound
    if (Kill_Sound[random_text])
        DoPlaySoundToSet(me, Kill_Sound[random_text]);

    if (!Kill_Spell)
        return;

    Unit* target = nullptr;

    switch (Kill_Target_Type)
    {
    case CAST_SELF:
        target = me;
        break;
    case CAST_HOSTILE_TARGET:
        target = me->GetVictim();
        break;
    case CAST_HOSTILE_SECOND_AGGRO:
        target = SelectTarget(SELECT_TARGET_MAXTHREAT,1);
        break;
    case CAST_HOSTILE_LAST_AGGRO:
        target = SelectTarget(SELECT_TARGET_MINTHREAT,0);
        break;
    case CAST_HOSTILE_RANDOM:
        target = SelectTarget(SELECT_TARGET_RANDOM,0);
        break;
    case CAST_KILLEDUNIT_VICTIM:
        target = victim;
        break;
    }

    //Target is ok, cast a spell on it
    if (target)
        DoCast(target, Kill_Spell);
}

void SimpleAI::DamageTaken(Unit *killer, uint32 &damage)
{
    //Return if damage taken won't kill us
    if (me->GetHealth() > damage)
        return;

    uint32 random_text = rand()%3;

    //Random yell
    if (Death_Text[random_text])
    {
        if (Death_Say[random_text])
            me->Say(Death_Text[random_text], LANG_UNIVERSAL, killer);
        else
            me->Yell(Death_Text[random_text], LANG_UNIVERSAL, killer);
    }

    //Random sound
    if (Death_Sound[random_text])
        DoPlaySoundToSet(me, Death_Sound[random_text]);

    if (!Death_Spell)
        return;

    Unit* target = nullptr;

    switch (Death_Target_Type)
    {
    case CAST_SELF:
        target = me;
        break;
    case CAST_HOSTILE_TARGET:
        target = me->GetVictim();
        break;
    case CAST_HOSTILE_SECOND_AGGRO:
        target = SelectTarget(SELECT_TARGET_MAXTHREAT,1);
        break;
    case CAST_HOSTILE_LAST_AGGRO:
        target = SelectTarget(SELECT_TARGET_MINTHREAT,0);
        break;
    case CAST_HOSTILE_RANDOM:
        target = SelectTarget(SELECT_TARGET_RANDOM,0);
        break;
    case CAST_JUSTDIED_KILLER:
        target = killer;
        break;
    }

    //Target is ok, cast a spell on it
    if (target)
        DoCast(target, Death_Spell);
}

void SimpleAI::UpdateAI(const uint32 diff)
{
    //Return since we have no target
    if (!UpdateVictim())
        return;

    //Spells
    for (uint32 i = 0; i < 10; ++i)
    {
        //Spell not valid
        if (!Spell[i].Enabled || !Spell[i].Spell_Id)
            continue;

        if (Spell_Timer[i] < diff)
        {
            //Check if this is a percentage based
            if (Spell[i].First_Cast < 0 && Spell[i].First_Cast > -100 && me->GetHealth()*100 / me->GetMaxHealth() > -Spell[i].First_Cast)
                continue;

            //Check Current spell
            if (!(Spell[i].InterruptPreviousCast && me->IsNonMeleeSpellCast(false)))
            {
                Unit* target = nullptr;

                switch (Spell[i].Cast_Target_Type)
                {
                case CAST_SELF:
                    target = me;
                    break;
                case CAST_HOSTILE_TARGET:
                    target = me->GetVictim();
                    break;
                case CAST_HOSTILE_SECOND_AGGRO:
                    target = SelectTarget(SELECT_TARGET_MAXTHREAT,1);
                    break;
                case CAST_HOSTILE_LAST_AGGRO:
                    target = SelectTarget(SELECT_TARGET_MINTHREAT,0);
                    break;
                case CAST_HOSTILE_RANDOM:
                    target = SelectTarget(SELECT_TARGET_RANDOM,0);
                    break;
                }

                //Target is ok, cast a spell on it and then do our random yell
                if (target)
                {
                    if (me->IsNonMeleeSpellCast(false))
                        me->InterruptNonMeleeSpells(false);

                    DoCast(target, Spell[i].Spell_Id);

                    //Yell and sound use the same number so that you can make
                    //the creature yell with the correct sound effect attached
                    uint32 random_text = rand()%3;

                    //Random yell
                    if (Spell[i].Text[random_text])
                    {
                        if (Spell[i].Say[random_text])
                            me->Say(Spell[i].Text[random_text], LANG_UNIVERSAL, target);
                        else 
                            me->Yell(Spell[i].Text[random_text], LANG_UNIVERSAL, target);
                    }

                    //Random sound
                    if (Spell[i].Text_Sound[random_text])
                        DoPlaySoundToSet(me, Spell[i].Text_Sound[random_text]);
                }

            }

            //Spell will cast agian when the cooldown is up
            if (Spell[i].CooldownRandomAddition)
                Spell_Timer[i] = Spell[i].Cooldown + (rand() % Spell[i].CooldownRandomAddition);
            else Spell_Timer[i] = Spell[i].Cooldown;

        }else Spell_Timer[i] -= diff;

    }

    DoMeleeAttackIfReady();
}

