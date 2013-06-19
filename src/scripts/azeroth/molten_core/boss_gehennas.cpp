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

enum
{
    SPELL_SHADOWBOLT = 19728,
    SPELL_RAINOFFIRE = 19717,
    SPELL_GEHENNASCURSE = 19716
};

class Boss_Gehennas : public CreatureScript
{
public:

    Boss_Gehennas() : CreatureScript("boss_gehennas")
    {
    }

    class Boss_GehennasAI : public CreatureAINew
    {
    public:

        enum event
        {
            EV_SHADOWBOLT = 0,
            EV_RAINOFFIRE,
            EV_CURSE
        };

        Boss_GehennasAI(Creature* creature) : CreatureAINew(creature)
        {
            _instance = ((ScriptedInstance*) creature->GetInstanceData());
        }

        void onReset(bool onSpawn)
        {
            if (onSpawn) {
                addEvent(EV_SHADOWBOLT, 6000, 6000);
                addEvent(EV_RAINOFFIRE, 8000, 8000);
                addEvent(EV_CURSE, 15000, 15000);
            }
            else {
                scheduleEvent(EV_SHADOWBOLT, 6000, 6000);
                scheduleEvent(EV_RAINOFFIRE, 8000, 8000);
                scheduleEvent(EV_CURSE, 15000, 15000);
            }

            if (_instance)
                _instance->SetData(DATA_GEHENNAS, NOT_STARTED);

            // Respawn the adds if needed
            std::list<Creature*> adds;
            me->GetCreatureListWithEntryInGrid(adds, 11661, 50.0f);
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
                _instance->SetData(DATA_GEHENNAS, IN_PROGRESS);
        }

        void onDeath(Unit* /*killer*/)
        {
            if (_instance)
                _instance->SetData(DATA_GEHENNAS, DONE);
        }

        void update(uint32 const diff)
        {
            if (!updateVictim())
                return;

            updateEvents(diff);

            while (executeEvent(diff, m_currEvent)) {
                switch (m_currEvent) {
                case EV_SHADOWBOLT:
                    doCast(selectUnit(SELECT_TARGET_RANDOM, 0), SPELL_SHADOWBOLT);
                    scheduleEvent(EV_SHADOWBOLT, 6000);
                    break;
                case EV_RAINOFFIRE:
                    doCast(selectUnit(SELECT_TARGET_RANDOM, 0), SPELL_RAINOFFIRE);
                    scheduleEvent(EV_RAINOFFIRE, urand(5000, 6000));
                    break;
                case EV_CURSE:
                    doCast(selectUnit(SELECT_TARGET_RANDOM, 0), SPELL_GEHENNASCURSE);
                    scheduleEvent(EV_CURSE, urand(28000, 32000));
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
        return new Boss_GehennasAI(creature);
    }
};

void AddSC_boss_gehennas()
{
    sScriptMgr.addScript(new Boss_Gehennas());
}

