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
SDName: Boss_Moam
SD%Complete: 100
SDComment: VERIFY SCRIPT AND SQL
SDCategory: Ruins of Ahn'Qiraj
EndScriptData */

#include "precompiled.h"
#include "def_ruins_of_ahnqiraj.h"

enum Emotes
{
    EMOTE_AGGRO             = -1509000,
    EMOTE_MANA_FULL         = -1509001
};

enum Spells
{
    SPELL_TRAMPLE           = 15550,
    SPELL_DRAINMANA         = 27256,
    SPELL_ARCANEERUPTION    = 25672,
    SPELL_SUMMONMANA        = 25681,
    SPELL_GRDRSLEEP         = 24360                       //Greater Dreamless Sleep
};

struct TRINITY_DLL_DECL boss_moamAI : public ScriptedAI
{
    boss_moamAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }

    Unit *pTarget;
    uint32 TRAMPLE_Timer;
    uint32 DRAINMANA_Timer;
    uint32 SUMMONMANA_Timer;
    uint32 i;
    uint32 j;

    ScriptedInstance *pInstance;

    void Reset()
    {
        i=0;
        j=0;
        pTarget = NULL;
        TRAMPLE_Timer = 30000;
        DRAINMANA_Timer = 30000;

        if (pInstance)
            pInstance->SetData(DATA_MOAM_EVENT, NOT_STARTED);
    }

    void Aggro(Unit *who)
    {
        DoScriptText(EMOTE_AGGRO, m_creature);
        pTarget = who;

        if (pInstance)
            pInstance->SetData(DATA_MOAM_EVENT, IN_PROGRESS);
    }

    void JustDied(Unit *killer)
    {
        if (pInstance)
            pInstance->SetData(DATA_MOAM_EVENT, DONE);
    }

    void UpdateAI(const uint32 diff)
    {
        if (!UpdateVictim())
            return;

        //If we are 100%MANA cast Arcane Erruption
        //if (j==1 && m_creature->GetMana()*100 / m_creature->GetMaxMana() == 100 && !m_creature->IsNonMeleeSpellCasted(false))
        {
            DoCast(m_creature->getVictim(),SPELL_ARCANEERUPTION);
            DoScriptText(EMOTE_MANA_FULL, m_creature);
        }

        //If we are <50%HP cast MANA FIEND (Summon Mana) and Sleep
        //if (i==0 && m_creature->GetHealth()*100 / m_creature->GetMaxHealth() <= 50 && !m_creature->IsNonMeleeSpellCasted(false))
        {
            i=1;
            DoCast(m_creature->getVictim(),SPELL_SUMMONMANA);
            DoCast(m_creature->getVictim(),SPELL_GRDRSLEEP);
        }

        //SUMMONMANA_Timer
        if (i==1 && SUMMONMANA_Timer < diff)
        {
            DoCast(m_creature->getVictim(),SPELL_SUMMONMANA);
            SUMMONMANA_Timer = 90000;
        }else SUMMONMANA_Timer -= diff;

        //TRAMPLE_Timer
        if (TRAMPLE_Timer < diff)
        {
            DoCast(m_creature->getVictim(),SPELL_TRAMPLE);
            j=1;

            TRAMPLE_Timer = 30000;
        }else TRAMPLE_Timer -= diff;

        //DRAINMANA_Timer
        if (DRAINMANA_Timer < diff)
        {
            DoCast(m_creature->getVictim(),SPELL_DRAINMANA);
            DRAINMANA_Timer = 30000;
        }else DRAINMANA_Timer -= diff;

        DoMeleeAttackIfReady();
    }
};
CreatureAI* GetAI_boss_moam(Creature *_Creature)
{
    return new boss_moamAI (_Creature);
}

void AddSC_boss_moam()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name="boss_moam";
    newscript->GetAI = &GetAI_boss_moam;
    newscript->RegisterSelf();
}

