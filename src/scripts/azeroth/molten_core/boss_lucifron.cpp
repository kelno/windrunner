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
SDName: Boss_Lucifron
SD%Complete: 100
SDComment:
SDCategory: Molten Core
EndScriptData */

#include "precompiled.h"
#include "def_molten_core.h"

enum {
    SPELL_IMPENDINGDOOM = 19702,
    SPELL_LUCIFRONCURSE = 19703,
    SPELL_SHADOWSHOCK   = 20603,
    
    NPC_FLAMEWAKER_PROTECTOR    = 12119
};

class Boss_Lucifron : public CreatureScript
{
public:
    Boss_Lucifron() : CreatureScript("boss_lucifron") {}
    
    class Boss_LucifronAI : public CreatureAINew
    {
    public:
        enum event {
            EV_IMPENDINGDOOM    = 0,
            EV_LUCIFRONCURSE    = 1,
            EV_SHADOWSHOCK      = 2
        };
        
        Boss_LucifronAI(Creature* creature) : CreatureAINew(creature)
        {
            _instance = ((ScriptedInstance*)creature->GetInstanceData());
        }
        
        void onReset(bool onSpawn)
        {
            if (onSpawn) {
                addEvent(EV_IMPENDINGDOOM, 15000, 15000);
                addEvent(EV_LUCIFRONCURSE, 10000, 10000);
                addEvent(EV_SHADOWSHOCK, 6000, 6000);
            }
            else {
                scheduleEvent(EV_IMPENDINGDOOM, 15000, 15000);
                scheduleEvent(EV_LUCIFRONCURSE, 10000, 10000);
                scheduleEvent(EV_SHADOWSHOCK, 6000, 6000);
            }
            if (_instance)
                _instance->SetData(DATA_LUCIFRON, NOT_STARTED);
            
            // Respawn the adds if needed
            std::list<Creature*> adds;
            me->GetCreatureListWithEntryInGrid(adds, NPC_FLAMEWAKER_PROTECTOR, 100.0f);
            for (std::list<Creature*>::iterator it = adds.begin(); it != adds.end(); it++) {
                if ((*it)->isDead()) {
                    (*it)->DisappearAndDie();
                    (*it)->Respawn();
                }
            }
        }
        
        void onCombatStart(Unit* /*victim*/)
        {
            if (_instance)
                _instance->SetData(DATA_LUCIFRON, IN_PROGRESS);
        }
        
        void onDeath(Unit* /*killer*/)
        {
            if (_instance)
                _instance->SetData(DATA_LUCIFRON, DONE);
        }
        
        void update(uint32 const diff)
        {
            if (!updateVictim())
                return;
            
            updateEvents(diff);
            
            while (executeEvent(diff, m_currEvent)) {
                switch (m_currEvent) {
                case EV_IMPENDINGDOOM:
                    doCast(me->GetVictim(), SPELL_IMPENDINGDOOM);
                    scheduleEvent(EV_IMPENDINGDOOM, urand(15000, 20000));
                    break;
                case EV_LUCIFRONCURSE:
                    doCast(me->GetVictim(), SPELL_LUCIFRONCURSE);
                    scheduleEvent(EV_LUCIFRONCURSE, urand(15000, 20000));
                    break;
                case EV_SHADOWSHOCK:
                    doCast(me->GetVictim(), SPELL_SHADOWSHOCK);
                    scheduleEvent(EV_SHADOWSHOCK, 4000);
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
        return new Boss_LucifronAI(creature);
    }
};

enum {
    SPELL_DOMINATE_MIND = 20604,
    SPELL_CLEAVE        = 20605
};

class Add_FlamewakerProtector : public CreatureScript
{
public:
    Add_FlamewakerProtector() : CreatureScript("add_flamewakerprotector") {}
    
    class Add_FlamewakerProtectorAI : public CreatureAINew
    {
    public:
        enum event {
            EV_DOMINATE_MIND    = 0,
            EV_CLEAVE           = 1
        };
        
        Add_FlamewakerProtectorAI(Creature* creature) : CreatureAINew(creature) {}
        
        void onReset(bool onSpawn)
        {
            if (onSpawn) {
                addEvent(EV_DOMINATE_MIND, 15000, 15000);
                addEvent(EV_CLEAVE, 6000, 6000);
            }
            else {
                scheduleEvent(EV_DOMINATE_MIND, 15000, 15000);
                scheduleEvent(EV_CLEAVE, 6000, 6000);
            }
        }
        
        void update(uint32 const diff)
        {
            if (!updateVictim())
                return;
            
            updateEvents(diff);
            
            while (executeEvent(diff, m_currEvent)) {
                switch (m_currEvent) {
                case EV_DOMINATE_MIND:
                    doCast(me->GetVictim(), SPELL_DOMINATE_MIND);
                    scheduleEvent(EV_DOMINATE_MIND, 15000);
                    break;
                case EV_CLEAVE:
                    doCast(me->GetVictim(), SPELL_CLEAVE);
                    scheduleEvent(EV_CLEAVE, 6000);
                    break;
                }
            }
            
            doMeleeAttackIfReady();
        }
    };
    
    CreatureAINew* getAI(Creature* creature)
    {
        return new Add_FlamewakerProtectorAI(creature);
    }
};

void AddSC_boss_lucifron()
{
    sScriptMgr.addScript(new Boss_Lucifron());
    sScriptMgr.addScript(new Add_FlamewakerProtector());
}

