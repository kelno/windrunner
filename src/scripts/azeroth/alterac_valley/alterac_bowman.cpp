#include "precompiled.h"

#define MAX_RANGE 80.0f
#define SPELL_SHOOT 22121
#define SHOOT_COOLDOWN urand(2000,3000)

struct alterac_bowmanAI : public ScriptedAI
{
    alterac_bowmanAI(Creature *c) : ScriptedAI(c) 
	{   
        shoot_timer = SHOOT_COOLDOWN;
        target = NULL;
        SetCombatDistance(80.0f); //Disable melee visual
        SetCombatMovementAllowed(false);
        me->SetSheath(SHEATH_STATE_RANGED);
    }

    Unit* target; //we need to keep a target without being in combat with it
    uint16 shoot_timer;

    void Reset()
    {   }

    void UpdateAI(const uint32 diff)
    {    
        if(UpdateVictim())
        {
            //give priority to the aggro system target if any
            target = me->GetVictim();

            //reset combat if not close combat (should avoid to stuck players in combat in most cases)
            if(me->GetDistance(target) > 30.0f)
            {
                 me->SetInFront(target);
                 me->DeleteThreatList();
                 //stay in front at reset (purely visual)
                 float x,y,z,o;
                 me->GetHomePosition(x,y,z,o);
                 me->SetHomePosition(x,y,z,me->GetAngle(target));
            }
        }

        if(target)
        {
            if(me->GetDistance(target) > 5.0f)
		    {
                if (shoot_timer < diff)
                {
                    if(!isValidTarget(target))
                    {
                        target = NULL;
                        return;
                    }
                    
				    DoCast(target,SPELL_SHOOT,false);
                    shoot_timer = SHOOT_COOLDOWN;
                } else shoot_timer -= diff;
            } else {
                DoMeleeAttackIfReady();
            }
        }
	}
	
	void MoveInLineOfSight(Unit *who)
	{    
        if (!target
            && !me->IsInCombat() 
            && isValidTarget(who))
                target = who;
	}

    bool isValidTarget(Unit* target)
    {
        float distance = me->GetDistance(target);
        if (me->canAttack(target) 
            && (distance < MAX_RANGE) 
            && (distance > 10.0f)
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

