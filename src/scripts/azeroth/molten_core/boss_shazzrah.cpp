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
SDName: Boss_Shazzrah
SD%Complete: 75
SDComment: Teleport NYI
SDCategory: Molten Core
EndScriptData */

#include "precompiled.h"
#include "def_molten_core.h"

enum
{
    SPELL_ARCANEEXPLOSION   = 19712,
    SPELL_SHAZZRAHCURSE     = 19713,
    SPELL_DEADENMAGIC       = 19714,
    SPELL_COUNTERSPELL      = 19715,
    SPELL_GATE_SHAZZRAH     = 23138
};

class Boss_Shazzrah : public CreatureScript
{
    public:
        Boss_Shazzrah() : CreatureScript("Boss_Shazzrah") {}
    
    class Boss_ShazzrahAI : public CreatureAINew
    {
        public:
            enum event
            {
                EV_ARCANEEXPLOSION   = 0,
                EV_SHAZZRAHCURSE     = 1,
                EV_DEADENMAGIC       = 2,
                EV_COUNTERSPELL      = 3,
                EV_GATE_SHAZZRAH     = 4
            };

            Boss_ShazzrahAI(Creature* creature) : CreatureAINew(creature)
            {
                _instance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onReset(bool onSpawn)
            {
                if (onSpawn)
                {
                    addEvent(EV_ARCANEEXPLOSION, 5000, 5000);
                    addEvent(EV_SHAZZRAHCURSE, 20000, 25000);
                    addEvent(EV_DEADENMAGIC, 24000, 24000);
                    addEvent(EV_COUNTERSPELL, 15000, 15000);
                    addEvent(EV_GATE_SHAZZRAH, 45000, 45000);
                }
                else
                {
                    scheduleEvent(EV_ARCANEEXPLOSION, 5000, 5000);
                    scheduleEvent(EV_SHAZZRAHCURSE, 20000, 25000);
                    scheduleEvent(EV_DEADENMAGIC, 24000, 24000);
                    scheduleEvent(EV_COUNTERSPELL, 15000, 15000);
                    scheduleEvent(EV_GATE_SHAZZRAH, 45000, 45000);
                }
            
                if (_instance)
                    _instance->SetData(DATA_SHAZZRAH, NOT_STARTED);
            }

            void onCombatStart(Unit* /*victim*/)
            {
                if (_instance)
                    _instance->SetData(DATA_SHAZZRAH, IN_PROGRESS);
            }
        
            void onDeath(Unit* /*killer*/)
            {
                if (_instance)
                    _instance->SetData(DATA_SHAZZRAH, DONE);
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
                        case EV_ARCANEEXPLOSION:
                            doCast(me->getVictim(), SPELL_ARCANEEXPLOSION);
                            scheduleEvent(EV_ARCANEEXPLOSION, urand(5000, 6000));
                            break;
                        case EV_SHAZZRAHCURSE:
                            doCast(selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true), SPELL_SHAZZRAHCURSE);
                            scheduleEvent(EV_SHAZZRAHCURSE, urand(20000, 25000));
                            break;
                        case EV_DEADENMAGIC:
                            doCast(me, SPELL_DEADENMAGIC);
                            scheduleEvent(EV_DEADENMAGIC, 35000);
                            break;
                        case EV_COUNTERSPELL:
                            doCast(me->getVictim(), SPELL_COUNTERSPELL);
                            scheduleEvent(EV_COUNTERSPELL, urand(16000, 20000));
                            break;
                        case EV_GATE_SHAZZRAH:
                            doCast(me, SPELL_GATE_SHAZZRAH);
                            scheduleEvent(EV_GATE_SHAZZRAH, 45000);
                            break;
                    }
                }
            
                doMeleeAttackIfReady();
            }

        private:
            ScriptedInstance* _instance;
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new Boss_ShazzrahAI(creature);
    }
};

void AddSC_boss_shazzrah()
{
    sScriptMgr.addScript(new Boss_Shazzrah());
}

