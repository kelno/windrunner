/* Copyright (C) 2006 - 2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* ScriptData
SDName: Boss_Gatewatcher_Ironhand
SD%Complete: 75
SDComment:
SDCategory: Tempest Keep, The Mechanar
EndScriptData */

#include "precompiled.h"
#include "def_mechanar.h"

#define SAY_AGGRO_1                     -1554006
#define SAY_HAMMER_1                    -1554007
#define SAY_HAMMER_2                    -1554008
#define SAY_SLAY_1                      -1554009
#define SAY_SLAY_2                      -1554010
#define SAY_DEATH_1                     -1554011
#define EMOTE_HAMMER                    -1554012

// Spells to be casted
#define SPELL_SHADOW_POWER              35322
#define H_SPELL_SHADOW_POWER            39193
#define SPELL_HAMMER_PUNCH              35326
#define SPELL_JACKHAMMER                35327
#define H_SPELL_JACKHAMMER              39194
#define SPELL_STREAM_OF_MACHINE_FLUID   35311

// Gatewatcher Iron-Hand AI
struct boss_gatewatcher_iron_handAI : public ScriptedAI
{
    boss_gatewatcher_iron_handAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        HeroicMode = m_creature->GetMap()->IsHeroic();
    }

    ScriptedInstance* pInstance;

    bool HeroicMode;

    uint32 Shadow_Power_Timer;
    uint32 Jackhammer_Timer;
    uint32 Jackhammer_CastTime;
    uint8 Jackhammer_Progression;
    uint32 Stream_of_Machine_Fluid_Timer;

    void Reset()
    {
        Shadow_Power_Timer = 25000;
        Jackhammer_Timer = 45000;
        Stream_of_Machine_Fluid_Timer = 55000;
        Jackhammer_CastTime = 0;
        Jackhammer_Progression = 0;
    }
    void Aggro(Unit *who)
    {
        DoScriptText(SAY_AGGRO_1, m_creature);
    }

    void KilledUnit(Unit* victim)
    {
        if (rand()%2)
            return;

        DoScriptText(RAND(SAY_SLAY_1, SAY_SLAY_2), m_creature);
    }

    void JustDied(Unit* Killer)
    {
        DoScriptText(SAY_DEATH_1, m_creature);

        if (!pInstance)
            return;

        //SetData will open the Moarg Door 2
        pInstance->SetData(DATA_GATEWATCHER_IRONHAND, DONE);
    }

    void UpdateAI(const uint32 diff)
    {
        //Return since we have no target
        if (!UpdateVictim() )
            return;

        //Shadow Power
        if(Shadow_Power_Timer < diff)
        {
            DoCast(m_creature,HeroicMode ? H_SPELL_SHADOW_POWER : SPELL_SHADOW_POWER);
            Shadow_Power_Timer = 20000 + rand()%8000;
        }else Shadow_Power_Timer -= diff;

        //Jack Hammer
        if(Jackhammer_Timer < diff)
        {
            switch(Jackhammer_Progression)
            {
            case 0: 
                if(Jackhammer_CastTime == 0)
                {
                    DoCast(m_creature->getVictim(),HeroicMode ? H_SPELL_JACKHAMMER : SPELL_JACKHAMMER);
                    DoScriptText(EMOTE_HAMMER, m_creature);
                    Jackhammer_Progression++;
                }
                break;
            case 1: 
                if(Jackhammer_CastTime > 2500)
                {
                    DoScriptText(SAY_HAMMER_1, m_creature); 
                    Jackhammer_Progression++;
                }
                break;
            case 2: 
                if(Jackhammer_CastTime > 5500)
                {
                    DoScriptText(SAY_HAMMER_2, m_creature); 
                    Jackhammer_Progression++;
                }
                break;
            case 3:
                if(Jackhammer_CastTime > 8000)
                {
                    Jackhammer_Timer = 30000;
                    Jackhammer_CastTime = 0;
                    Jackhammer_Progression = 0;
                }
                break;
            }
            
            Jackhammer_CastTime += diff;
        }else Jackhammer_Timer -= diff;

        //Stream of Machine Fluid
        if(Stream_of_Machine_Fluid_Timer < diff)
        {
            DoCast(m_creature->getVictim(),SPELL_STREAM_OF_MACHINE_FLUID);
            Stream_of_Machine_Fluid_Timer = 35000 + rand()%15000;
        }else Stream_of_Machine_Fluid_Timer -= diff;

        DoMeleeAttackIfReady();
    }
};
CreatureAI* GetAI_boss_gatewatcher_iron_hand(Creature *_Creature)
{
    return new boss_gatewatcher_iron_handAI (_Creature);
}

void AddSC_boss_gatewatcher_iron_hand()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name="boss_gatewatcher_iron_hand";
    newscript->GetAI = &GetAI_boss_gatewatcher_iron_hand;
    newscript->RegisterSelf();
}

