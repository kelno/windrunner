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
SDName: Boss_Golemagg
SD%Complete: 90
SDComment:
SDCategory: Molten Core
EndScriptData */

#include "precompiled.h"
#include "def_molten_core.h"

enum
{
    EMOTE_AEGIS           = -1409002,

    SPELL_MAGMASPLASH     = 13879,
    SPELL_PYROBLAST       = 20228,
    SPELL_EARTHQUAKE      = 19798,
    SPELL_ENRAGE          = 19953,
    SPELL_BUFF            = 20553,

    //-- CoreRager Spells --
    SPELL_MANGLE          = 19820,
    SPELL_AEGIS           = 20620,               //This is self casted whenever we are below 50%

    NPC_CORE_RAGER        = 11672
};

class Boss_Golemagg : public CreatureScript
{
    public:
        Boss_Golemagg() : CreatureScript("Boss_Golemagg") {}

    class Boss_GolemaggAI : public CreatureAINew
    {
        public:
            enum event
            {
                EV_PYROBLAST     = 0,
                EV_EARTHQUAKE    = 1,
                EV_ENRAGE        = 2,
                EV_BUFF          = 3
            };

            Boss_GolemaggAI(Creature* creature) : CreatureAINew(creature)
            {
                _instance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onReset(bool onSpawn)
            {
                if (onSpawn)
                {
                    addEvent(EV_PYROBLAST, 4000, 4000);
                    addEvent(EV_EARTHQUAKE, 3000, 3000);
                    addEvent(EV_ENRAGE, 200, 200);
                    addEvent(EV_BUFF, 2500, 2500);
                }
                else
                {
                    scheduleEvent(EV_PYROBLAST, 4000, 4000);
                    scheduleEvent(EV_EARTHQUAKE, 3000, 3000);
                    scheduleEvent(EV_ENRAGE, 200, 200);
                    scheduleEvent(EV_BUFF, 2500, 2500);
                }
            
                if (_instance)
                    _instance->SetData(DATA_GOLEMAGG, NOT_STARTED);

                // Respawn the adds if needed
                std::list<Creature*> adds;
                me->GetCreatureListWithEntryInGrid(adds, NPC_CORE_RAGER, 100.0f);
                for (std::list<Creature*>::iterator it = adds.begin(); it != adds.end(); it++)
                {
                    if ((*it)->isDead())
                    {
                        (*it)->DisappearAndDie();
                        (*it)->Respawn();
                    }
                }

                doCast(me, SPELL_MAGMASPLASH, true);
            }

            void onCombatStart(Unit* /*victim*/)
            {
                if (_instance)
                    _instance->SetData(DATA_GOLEMAGG, IN_PROGRESS);
            }
        
            void onDeath(Unit* /*killer*/)
            {
                std::list<Creature*> adds;
                me->GetCreatureListWithEntryInGrid(adds, NPC_CORE_RAGER, 100.0f);
                for (std::list<Creature*>::iterator it = adds.begin(); it != adds.end(); it++)
                {
                    if ((*it)->isAlive())
                        (*it)->DisappearAndDie();
                }

                if (_instance)
                    _instance->SetData(DATA_GOLEMAGG, DONE);
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
                        case EV_PYROBLAST:
                            doCast(selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true), SPELL_PYROBLAST);
                            scheduleEvent(EV_PYROBLAST, urand(4000, 5000));
                            break;
                        case EV_EARTHQUAKE:
                            doCast(me->getVictim(), SPELL_EARTHQUAKE);
                            scheduleEvent(EV_EARTHQUAKE, urand(3000, 4000));
                            break;
                        case EV_ENRAGE:
                            if (me->GetHealth()*100 / me->GetMaxHealth() < 10.0f)
                                doCast(me, SPELL_ENRAGE);

                            scheduleEvent(EV_ENRAGE, 62000);
                            break;
                        case EV_BUFF:
                            doCast(me, SPELL_BUFF);
                            scheduleEvent(EV_BUFF, urand(2500, 3000));
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
        return new Boss_GolemaggAI(creature);
    }
};

class Mob_Core_Rager : public CreatureScript
{
    public:
        Mob_Core_Rager() : CreatureScript("Mob_Core_Rager") {}

    class Mob_Core_RagerAI : public CreatureAINew
    {
        public:
            enum event
            {
                EV_MANGLE     = 0,
                EV_AEGIS      = 1
            };

            Mob_Core_RagerAI(Creature* creature) : CreatureAINew(creature)
            {
                _instance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onReset(bool onSpawn)
            {
                if (onSpawn)
                {
                    addEvent(EV_MANGLE, 7000, 7000);
                    addEvent(EV_AEGIS, 200, 200, EVENT_FLAG_NONE, false);
                }
                else
                {
                    scheduleEvent(EV_MANGLE, 7000, 7000);
                    scheduleEvent(EV_AEGIS, 200, 200);
                }
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
                        case EV_MANGLE:
                            doCast(me->getVictim(), SPELL_MANGLE);
                            scheduleEvent(EV_MANGLE, urand(10000, 11000));
                            break;
                        case EV_AEGIS:
                            doCast(me, SPELL_AEGIS);
                            DoScriptText(EMOTE_AEGIS, me);
                            disableEvent(EV_AEGIS);
                            break;
                    }
                }

                if (me->GetHealth()*100 / me->GetMaxHealth() < 50.0f)
                {
                    scheduleEvent(EV_AEGIS, 200);
                    enableEvent(EV_AEGIS);
                }

                doMeleeAttackIfReady();
            }

        private:
            ScriptedInstance* _instance;
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new Mob_Core_RagerAI(creature);
    }
};

void AddSC_boss_golemagg()
{
    sScriptMgr.addScript(new Boss_Golemagg());
    sScriptMgr.addScript(new Mob_Core_Rager());
}

