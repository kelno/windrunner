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
    SPELL_DISEASE_CLOUD           = 12627,
    SPELL_FRENZY                  = 12795,
};

#define SAY1                      -539
#define SAY2                      -540


struct boss_gluttonAI : public ScriptedAI
{
    boss_gluttonAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }
    
    ScriptedInstance *pInstance;
    uint32 eventTimer;
    uint32 perc;
    bool action1Reapeat;
    bool action2Reapeat;

    void Reset()
    {
        eventTimer = 500;
        perc = 0;
        action1Reapeat = false;
        action2Reapeat = false;
        DoCast(m_creature, SPELL_DISEASE_CLOUD);

        if (pInstance && pInstance->GetData(DATA_GLUTTON_EVENT) != DONE)
            pInstance->SetData(DATA_GLUTTON_EVENT, NOT_STARTED);
    }

    void Aggro(Unit *who)
    {
        if (pInstance)
            pInstance->SetData(DATA_GLUTTON_EVENT, IN_PROGRESS);
    }
    
    void JustDied(Unit *killer)
    {
        if (pInstance)
            pInstance->SetData(DATA_GLUTTON_EVENT, DONE);
    }

    void UpdateAI(const uint32 diff)
    {
        if (!UpdateVictim())
            return;

        if (eventTimer <= diff)
        {
            perc = (m_creature->GetHealth()*100) / m_creature->GetMaxHealth();
            if (perc <= 50 && perc >= 0)
            {
                if (!action1Reapeat)
                {
                    action1Reapeat = true;
                    m_creature->InterruptNonMeleeSpells(false);
                    DoCast(m_creature, SPELL_FRENZY);
                    DoScriptText(SAY1, m_creature);
                }
            }
            else if (perc <= 15 && perc >= 0)
            {
                if (!action2Reapeat)
                {
                    action2Reapeat = true;
                    m_creature->InterruptNonMeleeSpells(false);
                    DoCast(m_creature, SPELL_FRENZY);
                    DoScriptText(SAY2, m_creature);
                }
            }

            eventTimer = 500;
        }
        else
            eventTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_glutton(Creature *_Creature)
{
    return new boss_gluttonAI (_Creature);
}

void AddSC_boss_glutton()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name="boss_glutton";
    newscript->GetAI = &GetAI_glutton;
    newscript->RegisterSelf();
}
