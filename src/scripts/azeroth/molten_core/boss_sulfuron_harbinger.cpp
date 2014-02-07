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
SDName: Boss_Sulfuron_Harbringer
SD%Complete: 80
SDComment: Adds NYI
SDCategory: Molten Core
EndScriptData */

#include "precompiled.h"
#include "def_molten_core.h"

enum
{
    SPELL_THROW               = 19785,
    SPELL_DEMORALIZINGSHOUT   = 19778,
    SPELL_INSPIRE             = 19779,
    SPELL_KNOCKDOWN           = 19780,
    SPELL_FLAMESPEAR          = 19781,

    //Adds Spells
    SPELL_DARKSTRIKE          = 19777,
    SPELL_HEAL                = 19775,
    SPELL_SHADOWWORDPAIN      = 19776,
    SPELL_IMMOLATE            = 20294,

    NPC_FLAMEWAKER_PRIEST     = 11662
};

class Boss_Sulfuron : public CreatureScript
{
    public:
        Boss_Sulfuron() : CreatureScript("Boss_Sulfuron") {}
    
    class Boss_SulfuronAI : public CreatureAINew
    {
        public:
            enum event
            {
                EV_THROW              = 0,
                EV_DEMORALIZINGSHOUT  = 1,
                EV_INSPIRE            = 2,
                EV_KNOCKDOWN          = 3,
                EV_FLAMESPEAR         = 4
            };

            Boss_SulfuronAI(Creature* creature) : CreatureAINew(creature)
            {
                _instance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onReset(bool onSpawn)
            {
                if (onSpawn)
                {
                    addEvent(EV_THROW, 5000, 5000);
                    addEvent(EV_DEMORALIZINGSHOUT, 25000, 25000);
                    addEvent(EV_INSPIRE, 13000, 13000);
                    addEvent(EV_KNOCKDOWN, 6000, 6000);
                    addEvent(EV_FLAMESPEAR, 2000, 2000);
                }
                else
                {
                    scheduleEvent(EV_THROW, 5000, 5000);
                    scheduleEvent(EV_DEMORALIZINGSHOUT, 25000, 25000);
                    scheduleEvent(EV_INSPIRE, 13000, 13000);
                    scheduleEvent(EV_KNOCKDOWN, 6000, 6000);
                    scheduleEvent(EV_FLAMESPEAR, 2000, 2000);
                }

                if (_instance)
                    _instance->SetData(DATA_SULFURON, NOT_STARTED);

                // Respawn the adds if needed
                std::list<Creature*> adds;
                me->GetCreatureListWithEntryInGrid(adds, NPC_FLAMEWAKER_PRIEST, 100.0f);
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
                    _instance->SetData(DATA_SULFURON, IN_PROGRESS);
            }
        
            void onDeath(Unit* /*killer*/)
            {
                if (_instance)
                    _instance->SetData(DATA_SULFURON, DONE);
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
                        case EV_DEMORALIZINGSHOUT:
                            doCast(me->GetVictim(), SPELL_DEMORALIZINGSHOUT);
                            scheduleEvent(EV_DEMORALIZINGSHOUT, urand(25000, 26000));
                            break;
                        case EV_INSPIRE:
                        {
                            Creature* target = NULL;
                            std::list<Creature*> pList = doFindFriendlyMissingBuff(45.0f,SPELL_INSPIRE);
                            if (!pList.empty())
                            {
                                std::list<Creature*>::iterator i = pList.begin();
                                advance(i, (rand()%pList.size()));
                                target = (*i);
                            }

                            if (target)
                                doCast(target, SPELL_INSPIRE);

                            doCast(me, SPELL_INSPIRE);
                            scheduleEvent(EV_INSPIRE, urand(20000, 26000));
                            break;
                        }
                        case EV_KNOCKDOWN:
                            doCast(me->GetVictim(), SPELL_KNOCKDOWN);
                            scheduleEvent(EV_KNOCKDOWN, 12000, 15000);
                            break;
                        case EV_FLAMESPEAR:
                            doCast(selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true), SPELL_FLAMESPEAR);
                            scheduleEvent(EV_FLAMESPEAR, urand(12000, 16000));
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
        return new Boss_SulfuronAI(creature);
    }
};

class Mob_Flamewaker_Priest : public CreatureScript
{
    public:
        Mob_Flamewaker_Priest() : CreatureScript("Mob_Flamewaker_Priest") {}

    class Mob_Flamewaker_PriestAI : public CreatureAINew
    {
        public:
            enum event
            {
                EV_DARKSTRIKE         = 0,
                EV_HEAL               = 1,
                EV_SHADOWWORDPAIN     = 2,
                EV_IMMOLATE           = 3
            };

            Mob_Flamewaker_PriestAI(Creature* creature) : CreatureAINew(creature)
            {
                _instance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onReset(bool onSpawn)
            {
                if (onSpawn)
                {
                    addEvent(EV_DARKSTRIKE, 10000, 10000);
                    addEvent(EV_HEAL, 15000, 30000);
                    addEvent(EV_SHADOWWORDPAIN, 1000, 2000);
                    addEvent(EV_IMMOLATE, 5000, 6000);
                }
                else
                {
                    scheduleEvent(EV_DARKSTRIKE, 10000, 10000);
                    scheduleEvent(EV_HEAL, 15000, 30000);
                    scheduleEvent(EV_SHADOWWORDPAIN, 1000, 2000);
                    scheduleEvent(EV_IMMOLATE, 5000, 6000);
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
                        case EV_DARKSTRIKE:
                            doCast(me, SPELL_DARKSTRIKE);
                            scheduleEvent(EV_DARKSTRIKE, urand(15000, 18000));
                            break;
                        case EV_HEAL:
                        {
                            Unit* pUnit = doSelectLowestHpFriendly(60.0f, 1);
                            if (pUnit)
                                doCast(pUnit, SPELL_HEAL);

                            scheduleEvent(EV_HEAL, urand(15000, 20000));
                            break;
                        }
                        case EV_SHADOWWORDPAIN:
                            doCast(selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true), SPELL_SHADOWWORDPAIN);
                            scheduleEvent(EV_SHADOWWORDPAIN, 5000, 6000);
                            break;
                        case EV_IMMOLATE:
                            doCast(selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true), SPELL_IMMOLATE);
                            scheduleEvent(EV_IMMOLATE, urand(5000, 6000));
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
        return new Mob_Flamewaker_PriestAI(creature);
    }
};

void AddSC_boss_sulfuron()
{
    sScriptMgr.addScript(new Boss_Sulfuron());
    sScriptMgr.addScript(new Mob_Flamewaker_Priest());
}

