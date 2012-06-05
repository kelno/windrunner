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

#include "precompiled.h"
#include "def_razorfen_downs.h"

enum Spells
{
    SPELL_PUTRID_STENCH           = 12946,
};

struct boss_plaguemaw_the_rottingAI : public ScriptedAI
{
    boss_plaguemaw_the_rottingAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }
    
    ScriptedInstance *pInstance;
    uint32 eventTimer;
    uint32 putridstenchTimer;

    void Reset()
    {
        eventTimer = 500;
        putridstenchTimer = urand(8000, 12000);

        if (pInstance && pInstance->GetData(DATA_PLAGUEMAW_THE_ROTTING_EVENT) != DONE)
            pInstance->SetData(DATA_PLAGUEMAW_THE_ROTTING_EVENT, NOT_STARTED);
    }

    void Aggro(Unit *who)
    {
        if (pInstance)
            pInstance->SetData(DATA_PLAGUEMAW_THE_ROTTING_EVENT, IN_PROGRESS);
    }
    
    void JustDied(Unit *killer)
    {
        if (pInstance)
            pInstance->SetData(DATA_PLAGUEMAW_THE_ROTTING_EVENT, DONE);
    }

    void UpdateAI(const uint32 diff)
    {
        if (!UpdateVictim())
            return;

        if (eventTimer <= diff)
        {
            if (putridstenchTimer <= diff)
            {
                DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_PUTRID_STENCH);
                putridstenchTimer = urand(15000, 23000);
            }
            else
                putridstenchTimer -= diff;

            eventTimer = 500;
        }
        else
            eventTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_plaguemaw_the_rotting(Creature *_Creature)
{
    return new boss_plaguemaw_the_rottingAI (_Creature);
}

void AddSC_boss_plaguemaw_the_rotting()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name="boss_plaguemaw_the_rotting";
    newscript->GetAI = &GetAI_boss_plaguemaw_the_rotting;
    newscript->RegisterSelf();
}
