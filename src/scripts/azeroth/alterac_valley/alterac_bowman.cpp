#include "precompiled.h"

#define MAX_RANGE 75.0f
#define SPELL_SHOOT 22121
#define SPELL_INFINITE_ROOT 53107
#define TIMER_SHOOT urand(2000,3000)

struct alterac_bowmanAI : public ScriptedAI
{
    alterac_bowmanAI(Creature *c) : ScriptedAI(c) 
    {   
        shoot_timer = 0;
        SetCombatDistance(80.0f); //Disable melee visual
        SetCombatMovementAllowed(false);
        me->SetSheath(SHEATH_STATE_RANGED);
    }

    uint16 shoot_timer;
    uint64 targetGUID;

    void JustReachedHome()
    {
       me->AddAura(SPELL_INFINITE_ROOT,me); //this creature can't be displaced even via CM
    }

    void Reset()
    {   
       targetGUID = 0;
    }

    void UpdateAI(const uint32 diff)
    {    
        if(!targetGUID)
        {
            if(me->GetVictim())
                EnterEvadeMode();
            return;
        }

        Unit* target = me->GetUnit(*me,targetGUID);
        if(!target)
        {
            targetGUID = 0;
            return;
        }

        if(!isValidTarget(target))
        {
            EnterEvadeMode();
            return;
        }

        if(me->GetDistance(target) > NOMINAL_MELEE_RANGE)
        {
            if (shoot_timer < diff)
            {
                if(me->GetVictim() != target)
                {
                    AttackStart(target);
                }
                    
               // me->SetInFront(target);
                DoCast(target,SPELL_SHOOT,false);
                shoot_timer = TIMER_SHOOT;
            } else shoot_timer -= diff;
        } else {
            DoMeleeAttackIfReady();
        }
    }
    
    void MoveInLineOfSight(Unit *who)
    {    
        if (   !targetGUID
            && isValidTarget(who))
                targetGUID = who->GetGUID();
    }

    bool isValidTarget(Unit* target)
    {
        float distance = me->GetDistance(target);
        if (me->canAttack(target,false) 
            && (distance < MAX_RANGE) 
            && (distance > NOMINAL_MELEE_RANGE)
            && me->IsWithinLOSInMap(target))
                return true;
        
        return false;
    }
};

CreatureAI* GetAI_alterac_bowman(Creature *_Creature)
{
    return new alterac_bowmanAI (_Creature);
}

void AddSC_alterac_bowman()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="alterac_bowman";
    newscript->GetAI = &GetAI_alterac_bowman;
    newscript->RegisterSelf();
}

