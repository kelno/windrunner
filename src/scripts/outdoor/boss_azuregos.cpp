/* Copyright (C) 2009 - 2011 Windrunner
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
#include "CreatureScript.h"
#include "CreatureAINew.h"

enum {
    SAY_TELEPORT            = 0,

    SPELL_MARK_OF_FROST     = 23182,
    SPELL_AURA_OF_FROST     = 23186,
    SPELL_MANASTORM         = 21097,
    SPELL_CHILL             = 21098,
    SPELL_FROSTBREATH       = 21099,
    SPELL_REFLECT           = 22067,
    SPELL_CLEAVE            = 8255,                     // Perhaps not right ID
    SPELL_ENRAGE            = 23537
};

enum {
    EV_MANASTORM,
    EV_CHILL,
    EV_BREATH,
    EV_TELEPORT,
    EV_REFLECT,
    EV_CLEAVE
};

class Boss_azuregos : public CreatureScript
{
public:
    Boss_azuregos() : CreatureScript("boss_azuregos_new") {}

    class Boss_azuregos_newAI : public CreatureAINew
    {
    public:
        Boss_azuregos_newAI(Creature* creature) : CreatureAINew(creature) {}
        
        void onReset(bool onSpawn)
        {
            if (onSpawn) {
                addEvent(EV_MANASTORM, 5000, 17000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_CHILL, 10000, 30000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_BREATH, 2000, 8000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_TELEPORT, 30000, 30000);
                addEvent(EV_REFLECT, 15000, 30000, EVENT_FLAG_DELAY_IF_CASTING);
                addEvent(EV_CLEAVE, 7000, EVENT_FLAG_DELAY_IF_CASTING);
            }
            else {
                scheduleEvent(EV_MANASTORM, 5000, 17000);
                scheduleEvent(EV_CHILL, 10000, 30000);
                scheduleEvent(EV_BREATH, 2000, 8000);
                scheduleEvent(EV_TELEPORT, 30000);
                scheduleEvent(EV_REFLECT, 15000, 30000);
                scheduleEvent(EV_CLEAVE, 7000);
            }
            
            enraged = false;
        }
        
        void onKill(Unit* killed)
        {
            me->AddAura(SPELL_MARK_OF_FROST, killed);
        }
        
        void onThreatAdd(Unit* who, float& threat)
        {
            if (who->HasAura(SPELL_MARK_OF_FROST) && !who->HasAura(SPELL_AURA_OF_FROST))
                who->CastSpell(me, SPELL_AURA_OF_FROST, true);
        }

        void update(uint32 const diff)
        {
            if (!updateVictim())
                return;

            updateEvents(diff);
                
            if (me->IsBelowHPPercent(25.0f) && !enraged) {
                doCast(me, SPELL_ENRAGE, true);
                enraged = true;
            }

            while (executeEvent(diff, m_currEvent)) {
                switch (m_currEvent) {
                case EV_MANASTORM:
                {
                    doCast(selectUnit(SELECT_TARGET_RANDOM, 0), SPELL_MANASTORM);
                    scheduleEvent(EV_MANASTORM, 7500, 12500);
                    break;
                }
                case EV_CHILL:
                {
                    doCast(me->getVictim(), SPELL_CHILL);
                    scheduleEvent(EV_CHILL, 13000, 25000);
                    break;
                }
                case EV_BREATH:
                {
                    doCast(me->getVictim(), SPELL_FROSTBREATH);
                    scheduleEvent(EV_BREATH, 1000, 15000);
                    break;
                }
                case EV_TELEPORT:
                {
                    float x, y, z;
                    me->GetNearPoint(me, x, y, z, 5.0f, 0.0f);

                    std::list<Player*> players;
                    getAllPlayersInRange(players, 25.0f);
                    
                    for (std::list<Player*>::const_iterator itr = players.begin(); itr != players.end(); itr++) {
                        (*itr)->TeleportTo(me->GetMapId(), x, y, z, (*itr)->GetOrientation(), TELE_TO_NOT_LEAVE_COMBAT);
                        deleteFromThreatList((*itr)->GetGUID());
                    }
                    
                    talk(SAY_TELEPORT);
                    scheduleEvent(EV_TELEPORT, 30000);

                    break;
                }
                case EV_REFLECT:
                {
                    doCast(me, SPELL_REFLECT);
                    scheduleEvent(EV_REFLECT, 20000, 35000);
                    break;
                }
                case EV_CLEAVE:
                {
                    doCast(me->getVictim(), SPELL_CLEAVE);
                    scheduleEvent(EV_CLEAVE, 7000);
                    break;
                }
                }
            }
            
            doMeleeAttackIfReady();
        }
        
    private:
        bool enraged;
    };
    
    CreatureAINew* getAI(Creature* creature)
    {
        return new Boss_azuregos_newAI(creature);
    }
};

void AddSC_boss_azuregos()
{
    sScriptMgr.addScript(new Boss_azuregos());
}
