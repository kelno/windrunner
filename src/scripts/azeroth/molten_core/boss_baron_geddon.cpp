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
SDName: Boss_Baron_Geddon
SD%Complete: 100
SDComment:
SDCategory: Molten Core
EndScriptData */

#include "precompiled.h"
#include "def_molten_core.h"

enum
{
    EMOTE_SERVICE         = -1409000,

    SPELL_INFERNO         = 19695,
    SPELL_IGNITEMANA      = 19659,
    SPELL_LIVINGBOMB      = 20475,
    SPELL_ARMAGEDDOM      = 20479
};

class Boss_Baron_Geddon : public CreatureScript
{
    public:
        Boss_Baron_Geddon() : CreatureScript("Boss_Baron_Geddon") {}
    
    class Boss_Baron_GeddonAI : public CreatureAINew
    {
        public:
            enum event
            {
                EV_INFERNO        = 0,
                EV_IGNITEMANA     = 1,
                EV_LIVINGBOMB     = 2,
                EV_ARMAGEDDOM     = 3
            };

            Boss_Baron_GeddonAI(Creature* creature) : CreatureAINew(creature)
            {
                _instance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onReset(bool onSpawn)
            {
                if (onSpawn)
                {
                    addEvent(EV_INFERNO, 15000, 15000);
                    addEvent(EV_IGNITEMANA, 5000, 5000);
                    addEvent(EV_LIVINGBOMB, 30000, 30000);
                    addEvent(EV_ARMAGEDDOM, 200, 200, EVENT_FLAG_NONE, false);
                }
                else
                {
                    scheduleEvent(EV_INFERNO, 15000, 15000);
                    scheduleEvent(EV_IGNITEMANA, 5000, 5000);
                    scheduleEvent(EV_LIVINGBOMB, 30000, 30000);
                    scheduleEvent(EV_ARMAGEDDOM, 200, 200);
                }

                if (_instance)
                    _instance->SetData(DATA_GEDDON, NOT_STARTED);
            }

            void onCombatStart(Unit* /*victim*/)
            {
                if (_instance)
                    _instance->SetData(DATA_GEDDON, IN_PROGRESS);
            }
        
            void onDeath(Unit* /*killer*/)
            {
                if (_instance)
                    _instance->SetData(DATA_GEDDON, DONE);
            }
        
            void update(uint32 const diff)
            {
                if (!updateVictim())
                    return;

                updateEvents(diff);

                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EV_INFERNO:
                            doCast(me, SPELL_INFERNO);
                            scheduleEvent(EV_INFERNO, urand(15000, 16000));
                            break;
                        case EV_IGNITEMANA:
                            doCast(selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true), SPELL_IGNITEMANA);
                            scheduleEvent(EV_IGNITEMANA, urand(5000, 6000));
                            break;
                        case EV_LIVINGBOMB:
                            doCast(selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true), SPELL_LIVINGBOMB);
                            scheduleEvent(EV_LIVINGBOMB, 30000);
                            break;
                        case EV_ARMAGEDDOM:
                            me->InterruptNonMeleeSpells(true);
                            doCast(me, SPELL_ARMAGEDDOM);
                            DoScriptText(EMOTE_SERVICE, me);
                            disableEvent(EV_ARMAGEDDOM);
                            break;
                    }
                }

                if (me->GetHealth()*100 / me->GetMaxHealth() <= 2.5f)
                {
                    enableEvent(EV_ARMAGEDDOM);
                }

                doMeleeAttackIfReady();
            }

        private:
            ScriptedInstance* _instance;
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new Boss_Baron_GeddonAI(creature);
    }
};

void AddSC_boss_baron_geddon()
{
    sScriptMgr.addScript(new Boss_Baron_Geddon());
}

