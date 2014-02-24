/* Copyright (C) 2006 - 2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* ScriptData
SDName: AV Marshals
SD%Complete:
SDComment: Returns AV marshals to inside the building
SDCategory: Bosses
EndScriptData */

#include "precompiled.h"

#define SPELL_CHARGE               22911
#define SPELL_CLEAVE               40504
#define SPELL_DEMORALIZING_SHOUT   23511
#define SPELL_ENRAGE               8599
#define SPELL_WHIRLWIND1           15589
#define SPELL_WHIRLWIND2           13736

struct TRINITY_DLL_DECL AV_MarshalsAI : public ScriptedAI
{
     AV_MarshalsAI(Creature *c) : ScriptedAI(c) {Reset();}
	 
	 uint32 ChargeTimer;
	 uint32 CleaveTimer;
     uint32 DemoralizingShoutTimer;
     uint32 Whirlwind1Timer;
     uint32 Whirlwind2Timer;
     uint32 EnrageTimer;
     uint32 ResetTimer;
	 
	 void Reset()
     {
		ChargeTimer             = (2+rand()%10)*1000;
    	CleaveTimer			    = (1+rand()%10)*1000;
    	DemoralizingShoutTimer  = (2+rand()%18)*1000;
		Whirlwind1Timer			= (1+rand()%12)*1000;
		Whirlwind2Timer			= (5+rand()%15)*1000;
		EnrageTimer		        = (5+rand()%20)*1000;
		ResetTimer			    = 5000;
     }
     
	 void EnterCombat(Unit *who){}

	 void JustRespawned()
     {
         InCombat = false;
         Reset();
     }
	 
	 void KilledUnit(Unit* victim){}
	 
	 void JustDied(Unit* Killer){}
	 
	 void UpdateAI(const uint32 diff)
     {
        if (!UpdateVictim())
            return;

		if (ChargeTimer <diff)
        {
            if(DoCast(m_creature->GetVictim(), SPELL_CHARGE) == SPELL_CAST_OK)
                ChargeTimer = (10+rand()%15)*1000;
        }else ChargeTimer -= diff;			
		
		if (CleaveTimer < diff)
        {
            if(DoCast(m_creature->GetVictim(), SPELL_CLEAVE) == SPELL_CAST_OK)
                CleaveTimer =  (10+rand()%6)*1000;
        }else CleaveTimer -= diff;

        if (DemoralizingShoutTimer < diff)
        {
            if(DoCast(m_creature->GetVictim(), SPELL_DEMORALIZING_SHOUT) == SPELL_CAST_OK)
                DemoralizingShoutTimer = (10+rand()%5)*1000;
        }else DemoralizingShoutTimer -= diff;

        if (Whirlwind1Timer < diff)
        {
            if(DoCast(m_creature->GetVictim(), SPELL_WHIRLWIND1) == SPELL_CAST_OK)
                Whirlwind1Timer = (6+rand()%14)*1000;
        }else Whirlwind1Timer -= diff;

        if (Whirlwind2Timer < diff)
        {
            if(DoCast(m_creature->GetVictim(), SPELL_WHIRLWIND2) == SPELL_CAST_OK)
                Whirlwind2Timer = (10+rand()%15)*1000;
        }else Whirlwind2Timer -= diff;

        if (EnrageTimer < diff)
        {
            if(DoCast(m_creature->GetVictim(), SPELL_ENRAGE) == SPELL_CAST_OK)
                EnrageTimer = (10+rand()%20)*1000;
        }else EnrageTimer -= diff;	
			

        // check if creature is not outside of building
        /*if(ResetTimer < diff)
        {
            float x, y, z;
            m_creature->GetPosition(x, y, z);
            if(x < 700)
		    {
	            EnterEvadeMode();
		    }
            ResetTimer = 2000;
        }else ResetTimer -= diff;*/

        DoMeleeAttackIfReady();
     }
};

CreatureAI* GetAI_AV_Marshals(Creature *_Creature)
{
    return new AV_MarshalsAI (_Creature);
}

void AddSC_AV_Marshals()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "AV_Marshals";
    newscript->GetAI = &GetAI_AV_Marshals;
    newscript->RegisterSelf();
}
