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

#define SAY_AGGRO          -163
#define SAY_COMBAT1        -164
#define SAY_COMBAT2        -165
#define SAY_COMBAT3        -166

enum Spells
{
    SPELL_FIREBALL                = 12466,
    SPELL_CURSE_OF_TUTEN_KASH     = 12255,
    SPELL_FIRE_NOVA               = 12470,
};

struct boss_mordresh_fire_eyeAI : public ScriptedAI
{
    boss_mordresh_fire_eyeAI(Creature* c) : ScriptedAI(c)
    {
        pInstance =(ScriptedInstance*)m_creature->GetInstanceData();
    }

    ScriptedInstance* pInstance;

    uint32 phase;
    uint32 fireballTimer;
    uint32 eventupdateTimer;
    uint32 perc;
    uint32 phase2Timer;
    uint32 firenovaTimer;
    uint32 yellTimer;
    bool action1Reapeat;
    bool action2Reapeat;
    bool action3Reapeat;
    bool action4Reapeat;

    void Reset()
    {
        phase = 0;
        fireballTimer = urand(2400, 3800);
        eventupdateTimer = 500;
        phase2Timer = 100;
        perc = 0;
        firenovaTimer = urand(8000, 12000);
        yellTimer = urand(30000, 40000);

        action1Reapeat = false;
        action2Reapeat = false;
        action3Reapeat = false;
        action4Reapeat = false;

        me->GetMotionMaster()->Clear();
        m_creature->GetMotionMaster()->MoveIdle();

        if (pInstance && pInstance->GetData(DATA_MORDRESH_FIRE_EYE) != DONE)
                pInstance->SetData(DATA_MORDRESH_FIRE_EYE, NOT_STARTED);
    }

    void EnterCombat(Unit *who)
    {
        if (pInstance)
            pInstance->SetData(DATA_MORDRESH_FIRE_EYE, IN_PROGRESS);

        DoScriptText(SAY_AGGRO, m_creature);

        DoCast(who, SPELL_FIREBALL);

        phase = 1;
    }

    void JustDied(Unit* Killer)
    {
        if(pInstance)
            pInstance->SetData(DATA_MORDRESH_FIRE_EYE, DONE);
    }

    void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim())
            return;

        if (firenovaTimer <= diff)
        {
            m_creature->InterruptNonMeleeSpells(false);
            DoCast(m_creature, SPELL_FIRE_NOVA);
            firenovaTimer = urand(11000, 16000);
        }
        else
            firenovaTimer -= diff;

        if (yellTimer <= diff)
        {
            switch( rand()%3 )
            {
                case 0:
                    DoScriptText(SAY_COMBAT1, m_creature);
                    break;
                case 2:
                    DoScriptText(SAY_COMBAT2, m_creature);
                    break;
                case 3:
                    DoScriptText(SAY_COMBAT3, m_creature);
                    break;
            }
            yellTimer = urand(30000, 40000);
        }
        else
            yellTimer -= diff;

        switch (phase)
        {
            case 1:
                if (fireballTimer <= diff)
                {
                    if (m_creature->IsWithinDistInMap(m_creature->GetVictim(), 40.0f))
                        DoCast(m_creature->GetVictim(), SPELL_FIREBALL);
                    fireballTimer = urand(2400, 3800);
                }
                else
                    fireballTimer -= diff;

                if (eventupdateTimer <= diff)
                {
                    if (m_creature->IsWithinDistInMap(m_creature->GetVictim(), 80.0f))
                    {
                        if (m_creature->GetDistance(m_creature->GetVictim()) >= 35.0f)
                        {
                            if (!action2Reapeat)
                            {
                                me->GetMotionMaster()->Clear();
                                m_creature->GetMotionMaster()->MoveChase(m_creature->GetVictim(), 0.0f, 0.0f);
                                action2Reapeat = true;
                            }
                        }
                    }

                    if (m_creature->IsWithinDistInMap(m_creature->GetVictim(), 15.0f))
                    {
                        if (m_creature->GetDistance(m_creature->GetVictim()) >= 5.0f)
                        {
                            if (!action3Reapeat)
                            {
                                me->GetMotionMaster()->Clear();
                                m_creature->GetMotionMaster()->MoveIdle();
                                action3Reapeat = true;
                            }
                        }
                    }

                    if (m_creature->IsWithinDistInMap(m_creature->GetVictim(), 5.0f))
                    {
                        if (m_creature->GetDistance(m_creature->GetVictim()) >= 0.0f)
                        {
                            if (!action4Reapeat)
                            {
                                me->GetMotionMaster()->Clear();
                                m_creature->GetMotionMaster()->MoveChase(m_creature->GetVictim(), 0.0f, 0.0f);
                                action4Reapeat = true;
                            }
                        }
                    }

                    perc = (m_creature->GetPower(POWER_MANA)*100) / m_creature->GetMaxPower(POWER_MANA);
                    if (perc <= 15.0f && perc >= 0.0f)
                    {
                        if (!action1Reapeat)
                        {
                            phase = 2;
                            me->GetMotionMaster()->Clear();
                            m_creature->GetMotionMaster()->MoveChase(m_creature->GetVictim(), 0.0f, 0.0f);
                            action1Reapeat = true;
                        }
                    }

                    eventupdateTimer = 500;
                }
                else
                    eventupdateTimer -= diff;
                break;
            case 2:
                if (phase2Timer <= diff)
                {
                    perc = (m_creature->GetPower(POWER_MANA)*100) / m_creature->GetMaxPower(POWER_MANA);
                    if (perc <= 100.0f && perc >= 30.0f)
                        phase = 1;

                    phase2Timer = 100;
                }
                else
                    phase2Timer -= diff;
                break;
        }
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_mordresh_fire_eye(Creature *_Creature)
{
    return new boss_mordresh_fire_eyeAI(_Creature);
}

void AddSC_boss_mordresh_fire_eye()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name="boss_mordresh_fire_eye";
    newscript->GetAI = &GetAI_boss_mordresh_fire_eye;
    newscript->RegisterSelf();
}
