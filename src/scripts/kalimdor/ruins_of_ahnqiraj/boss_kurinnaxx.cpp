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
SDName: Boss_Kurinnaxx
SD%Complete: 100
SDComment: VERIFY SCRIPT AND SQL
SDCategory: Ruins of Ahn'Qiraj
EndScriptData */

#include "precompiled.h"
#include "def_ruins_of_ahnqiraj.h"
#include "CreatureScript.h"
#include "CreatureAINew.h"

enum Spells
{
    SPELL_ENRAGE                 = 28798,
    SPELL_MORTAL_WOUND           = 25646,
    SPELL_SANDTRAP               = 25648,
    SPELL_SANDTRAP_DAMAGE        = 25656,
    SPELL_WIDE_SLASH             = 25814,
    SPELL_SUMMON_PLAYER          = 26446,
    SPELL_TRASH                  =  3391
};

enum Events
{
    EV_MORTAL_WOUND,
    EV_SANDTRAP,
    EV_WIDE_SLASH,
    EV_SUMMON_PLAYER,
    EV_TRASH
};

class Boss_kurinaxx : public CreatureScript
{
public:
    Boss_kurinaxx() : CreatureScript("boss_kurinaxx_new") {}
    
    class Boss_kurinaxx_newAI : public CreatureAINew
    {
    public:
        Boss_kurinaxx_newAI(Creature* creature) : CreatureAINew(creature)
        {
            pInstance = ((ScriptedInstance*)creature->GetInstanceData());
        }
        
        void onReset(bool onSpawn)
        {
            if (onSpawn) {
                addEvent(EV_MORTAL_WOUND, 2000, 7000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_SANDTRAP, 10000, 15000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_WIDE_SLASH, 10000, 15000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_TRASH, 20000, 25000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_SUMMON_PLAYER, 30000, 40000, EVENT_FLAG_DELAY_IF_CASTING);
            }
            else {
                scheduleEvent(EV_MORTAL_WOUND, 2000, 7000);
                scheduleEvent(EV_SANDTRAP, 10000, 15000);
                scheduleEvent(EV_WIDE_SLASH, 10000, 15000);
                scheduleEvent(EV_TRASH, 20000, 25000);
                scheduleEvent(EV_SUMMON_PLAYER, 30000, 40000);
            }
            
            enraged = false;
            
            if (pInstance)
                pInstance->SetData(DATA_KURINNAXX_EVENT, NOT_STARTED);
        }
        
        void onDeath(Unit* killed)
        {
            if (pInstance)
                pInstance->SetData(DATA_KURINNAXX_EVENT, DONE);
        }
        
        void onCombatStart(Unit* victim)
        {
            if (pInstance)
                pInstance->SetData(DATA_KURINNAXX_EVENT, IN_PROGRESS);
        }
        
        void update(uint32 const diff)
        {
            if (!updateVictim())
                return;

            updateEvents(diff);
            
            if (me->IsBelowHPPercent(30.0f) && !enraged) {
                doCast(me, SPELL_ENRAGE, true);
                enraged = true;
            }
            
            while (executeEvent(diff, m_currEvent)) {
                switch (m_currEvent) {
                case EV_MORTAL_WOUND:
                    doCast(me->GetVictim(), SPELL_MORTAL_WOUND);
                    scheduleEvent(EV_MORTAL_WOUND, 2000, 7000);
                    break;
                case EV_SANDTRAP:
                    doCast(selectUnit(SELECT_TARGET_RANDOM, 3, 200.0f, true), SPELL_SANDTRAP, true);
                    scheduleEvent(EV_SANDTRAP, 20000);
                    break;
                case EV_WIDE_SLASH:
                    doCast(me->GetVictim(), SPELL_WIDE_SLASH);
                    scheduleEvent(EV_WIDE_SLASH, 10000, 15000);
                    break;
                case EV_TRASH:
                    doCast(me, SPELL_TRASH);
                    scheduleEvent(EV_TRASH, 20000, 25000);
                    break;
                case EV_SUMMON_PLAYER:
                    doCast(selectUnit(SELECT_TARGET_RANDOM, 3, 150.0f, true), SPELL_SUMMON_PLAYER);
                    scheduleEvent(EV_SUMMON_PLAYER, 30000, 40000);
                    break;
                }
            }

            doMeleeAttackIfReady();
        }
        
    private:
        bool enraged;
        
        ScriptedInstance* pInstance;
    };
    
    CreatureAINew* getAI(Creature* creature)
    {
        return new Boss_kurinaxx_newAI(creature);
    }
};

void AddSC_boss_kurinnaxx()
{
    sScriptMgr.addScript(new Boss_kurinaxx());
}

