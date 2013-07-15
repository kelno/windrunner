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
#include "def_molten_core.h"

//#define EMOTE_FRENZY                -1409001

enum {
    SPELL_FRENZY                = 19451,
    SPELL_MAGMASPIT             = 19449,
    SPELL_LAVABREATH            = 19272,
    SPELL_PANIC                 = 19408,
    SPELL_LAVABOMB              = 19411,                   //This calls a dummy server side effect that isn't implemented yet
    SPELL_LAVABOMB_ALT          = 19428                   //This is the spell that the lava bomb casts
};

class Boss_Magmadar : public CreatureScript
{
public:
    Boss_Magmadar() : CreatureScript("boss_magmadar") {}
    
    class Boss_MagmadarAI : public CreatureAINew
    {
    public:
        enum event {
            EV_LAVABREATH       = 0,
            EV_PANIC            = 1,
            EV_LAVABOMB         = 2,
            EV_FRENZY           = 3
        };
        
        Boss_MagmadarAI(Creature* creature) : CreatureAINew(creature)
        {
            _instance = ((ScriptedInstance*)creature->GetInstanceData());
        }
        
        void onReset(bool onSpawn)
        {
            if (onSpawn) {
                addEvent(EV_LAVABREATH, 25000, 30000);
                addEvent(EV_PANIC, 25000, 30000);
                addEvent(EV_LAVABOMB, 8000, 12000);
                addEvent(EV_FRENZY, 19000, 21000);
            }
            else {
                scheduleEvent(EV_LAVABREATH, 25000, 30000);
                scheduleEvent(EV_PANIC, 25000, 30000);
                scheduleEvent(EV_LAVABOMB, 8000, 12000);
                scheduleEvent(EV_FRENZY, 19000, 21000);
            }
            
            if (_instance)
                _instance->SetData(DATA_MAGMADAR, NOT_STARTED);
            
            doCast(me, SPELL_MAGMASPIT, true);
        }
        
        void onCombatStart(Unit* /*victim*/)
        {
            if (_instance)
                _instance->SetData(DATA_MAGMADAR, IN_PROGRESS);
        }
        
        void onDeath(Unit* /*killer*/)
        {
            if (_instance)
                _instance->SetData(DATA_MAGMADAR, DONE);
        }
        
        void update(uint32 const diff)
        {
            if (!updateVictim())
                return;
            
            updateEvents(diff);
            
            while (executeEvent(diff, m_currEvent)) {
                switch (m_currEvent) {
                case EV_LAVABREATH:
                    doCast(me->getVictim(), SPELL_LAVABREATH);
                    scheduleEvent(EV_LAVABREATH, 25000, 30000);
                    break;
                case EV_PANIC:
                    doCast(selectUnit(SELECT_TARGET_RANDOM, 0), SPELL_PANIC);
                    scheduleEvent(EV_PANIC, 25000, 30000);
                    break;
                case EV_LAVABOMB: // FIXME
                    doCast(selectUnit(SELECT_TARGET_RANDOM, 0), SPELL_LAVABOMB);
                    scheduleEvent(EV_LAVABOMB, 8000, 12000);
                    break;
                case EV_FRENZY:
                    doCast(me, SPELL_FRENZY);
                    scheduleEvent(EV_FRENZY, 19000, 21000);
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
        return new Boss_MagmadarAI(creature);
    }
};

void AddSC_boss_magmadar()
{
    sScriptMgr.addScript(new Boss_Magmadar());
}

