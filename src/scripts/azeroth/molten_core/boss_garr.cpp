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
SDName: Boss_Garr
SD%Complete: 50
SDComment: Adds NYI
SDCategory: Molten Core
EndScriptData */

#include "precompiled.h"
#include "def_molten_core.h"

enum
{
    // Garr spells
    SPELL_ANTIMAGICPULSE     = 19492,
    SPELL_MAGMASHACKLES      = 19496,
    SPELL_FRENZY             = 19516,

    //Add spells
    SPELL_ERUPTION           = 19497,
    SPELL_IMMOLATE           = 20294,

    NPC_FIRE_WORN            = 12099
};

class Boss_Garr : public CreatureScript
{
    public:
        Boss_Garr() : CreatureScript("Boss_Garr") {}
    
    class Boss_GarrAI : public CreatureAINew
    {
        public:
            enum event
            {
                EV_ANTIMAGICPULSE   = 0,
                EV_MAGMASHACKLES    = 1
            };

            Boss_GarrAI(Creature* creature) : CreatureAINew(creature)
            {
                _instance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onReset(bool onSpawn)
            {
                if (onSpawn)
                {
                    addEvent(EV_ANTIMAGICPULSE, 25000, 25000);
                    addEvent(EV_MAGMASHACKLES, 15000, 15000);
                }
                else
                {
                    scheduleEvent(EV_ANTIMAGICPULSE, 25000, 25000);
                    scheduleEvent(EV_MAGMASHACKLES, 15000, 15000);
                }
            
                if (_instance)
                    _instance->SetData(DATA_GARR, NOT_STARTED);
            
                // Respawn the adds if needed
                std::list<Creature*> adds;
                me->GetCreatureListWithEntryInGrid(adds, NPC_FIRE_WORN, 100.0f);
                for (std::list<Creature*>::iterator it = adds.begin(); it != adds.end(); it++)
                {
                    if ((*it)->isDead())
                    {
                        (*it)->DisappearAndDie();
                        (*it)->Respawn();
                    }
                }
            }

            void onCombatStart(Unit* /*victim*/)
            {
                if (_instance)
                    _instance->SetData(DATA_GARR, IN_PROGRESS);
            }
        
            void onDeath(Unit* /*killer*/)
            {
                if (_instance)
                    _instance->SetData(DATA_GARR, DONE);
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
                        case EV_ANTIMAGICPULSE:
                            doCast(me, SPELL_ANTIMAGICPULSE);
                            scheduleEvent(EV_ANTIMAGICPULSE, urand(10000, 15000));
                            break;
                        case EV_MAGMASHACKLES:
                            doCast(me, SPELL_MAGMASHACKLES);
                            scheduleEvent(EV_MAGMASHACKLES, urand(8000, 12000));
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
        return new Boss_GarrAI(creature);
    }
};

class Mob_FiresWorn : public CreatureScript
{
    public:
        Mob_FiresWorn() : CreatureScript("Mob_FiresWorn") {}
    
    class Mob_FiresWornAI : public CreatureAINew
    {
        public:
            enum event
            {
                EV_ERUPTION         = 0,
                EV_IMMOLATE         = 1
            };

            Mob_FiresWornAI(Creature* creature) : CreatureAINew(creature)
            {
                _instance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onReset(bool onSpawn)
            {
                if (onSpawn)
                {
                    addEvent(EV_ERUPTION, 200, 200, EVENT_FLAG_NONE, false);
                    addEvent(EV_IMMOLATE, 4000, 4000);
                }
                else
                {
                    scheduleEvent(EV_ERUPTION, 200);
                    scheduleEvent(EV_IMMOLATE, 4000);
                }
            }

            void onCombatStart(Unit* victim)
            {
                if (Creature* garr = _instance->instance->GetCreature(_instance->GetData64(DATA_GARR)))
                    if (!garr->GetVictim())
                        garr->getAI()->attackStart(victim);
            }

            void onDeath(Unit* /*killer*/)
            {
                if (Creature* garr = _instance->instance->GetCreature(_instance->GetData64(DATA_GARR)))
                    if (garr->IsAlive())
                        garr->CastSpell(garr, SPELL_FRENZY, false);
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
                        case EV_ERUPTION:
                            doCast(me->GetVictim(), SPELL_ERUPTION);
                            disableEvent(EV_ERUPTION);
                            me->DisappearAndDie();
                            break;
                        case EV_IMMOLATE:
                            doCast(selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true), SPELL_IMMOLATE);
                            scheduleEvent(EV_IMMOLATE, urand(5000, 10000));
                            break;
                    }
                }

                if (me->GetHealth() <= me->GetMaxHealth() * 0.10)
                {
                    enableEvent(EV_ERUPTION);
                }

                doMeleeAttackIfReady();
            }

        private:
            ScriptedInstance* _instance;
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new Mob_FiresWornAI(creature);
    }
};

void AddSC_boss_garr()
{
    sScriptMgr.addScript(new Boss_Garr());
    sScriptMgr.addScript(new Mob_FiresWorn());
}

