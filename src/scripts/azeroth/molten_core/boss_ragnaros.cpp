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
SDName: Boss_Ragnaros
SD%Complete: 75
SDComment: Intro Dialog and event NYI
SDCategory: Molten Core
EndScriptData */

#include "precompiled.h"
#include "def_molten_core.h"

enum
{
    SAY_SUMMON_DOMO             = -1409008,
    SAY_ARRIVAL1_RAG            = -1409009,
    SAY_ARRIVAL2_DOMO           = -1409010,
    SAY_ARRIVAL3_RAG            = -1409011,
    SAY_ARRIVAL4_RAG            = -1409012,

    SAY_REINFORCEMENTS1         = -1409013,
    SAY_REINFORCEMENTS2         = -1409014,
    SAY_HAND                    = -1409015,
    SAY_WRATH                   = -1409016,
    SAY_KILL                    = -1409017,
    SAY_MAGMABURST              = -1409018,

    SPELL_HANDOFRAGNAROS        = 19780,
    SPELL_WRATHOFRAGNAROS       = 20566,
    SPELL_LAVABURST             = 21158,
    SPELL_MAGMABURST            = 20565,

    SPELL_SONSOFFLAME_DUMMY     = 21108,                   //Server side effect
    SPELL_RAGSUBMERGE           = 21107,                   //Stealth aura
    SPELL_RAGEMERGE             = 20568,
    SPELL_MELTWEAPON            = 21388,
    SPELL_ELEMENTALFIRE         = 20564,
    SPELL_ERRUPTION             = 17731,

    SPELL_SUMMON_RAGNAROS       = 19774,                   //for Domo
};

struct Locations
{
    float x, y, z, o;
};

static Locations DomoNewLocation =
{
    851.106262, -814.688660, -229.283966, 4.641055
};

static Locations DomoInvocLocation =
{
    829.947754, -814.614807, -228.950043, 5.6
};

static Locations AddLocations[] =
{
    { 848.740356, -816.103455, -229.74327, 2.615287 },
    { 852.560791, -849.861511, -228.560974, 2.836073 },
    { 808.710632, -852.845764, -227.914963, 0.964207 },
    { 786.597107, -821.132874, -226.350128, 0.949377 },
    { 796.219116, -800.948059, -226.010361, 0.560603 },
    { 821.602539, -782.744109, -226.023575, 6.157440 },
    { 844.924744, -769.453735, -225.521698, 4.4539958 },
    { 839.823364, -810.869385, -229.683182, 4.693108 }
};

enum
{
    PHASE_INTRO,
    PHASE_NORMAL,
    PHASE_SUBMERGE
};

class Boss_Ragnaros : public CreatureScript
{
    public:
        Boss_Ragnaros() : CreatureScript("Boss_Ragnaros") {}

    class Boss_RagnarosAI : public CreatureAINew
    {
        public:
            enum event
            {
                EV_INTRO            = 0,
                EV_WRATHOFRAGNAROS  = 1,
                EV_HANDOFRAGNAROS   = 2,
                EV_LAVABURST        = 3,
                EV_ERUPTION         = 4,
                EV_ELEMENTALFIRE    = 5,
                EV_SUBMERGE         = 6,
                EV_EMERGE           = 7
            };
            uint8 Intro_Phase;
            Creature* Domo;
            uint32 MagmaBurst_Timer;
            bool HasYelledMagmaBurst;
            bool HasSubmergedOnce;

            Boss_RagnarosAI(Creature* creature) : CreatureAINew(creature), Summons(me)
            {
                _instance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onReset(bool onSpawn)
            {
                MagmaBurst_Timer = 2000;
                HasYelledMagmaBurst = false;
                HasSubmergedOnce = false;
                Intro_Phase = 1;
                Domo = NULL;
                Summons.DespawnAll();
                if (onSpawn)
                {
                    addEvent(EV_INTRO, 1, 1, EVENT_FLAG_NONE, true, phaseMaskForPhase(PHASE_INTRO));
                    addEvent(EV_WRATHOFRAGNAROS, 30000, 30000, EVENT_FLAG_NONE, true, phaseMaskForPhase(PHASE_NORMAL));
                    addEvent(EV_HANDOFRAGNAROS, 25000, 25000, EVENT_FLAG_NONE, true, phaseMaskForPhase(PHASE_NORMAL));
                    addEvent(EV_LAVABURST, 10000, 10000, EVENT_FLAG_NONE, true, phaseMaskForPhase(PHASE_NORMAL));
                    addEvent(EV_ERUPTION, 15000, 15000, EVENT_FLAG_NONE, true, phaseMaskForPhase(PHASE_NORMAL));
                    addEvent(EV_ELEMENTALFIRE, 3000, 3000, EVENT_FLAG_NONE, true, phaseMaskForPhase(PHASE_NORMAL));
                    addEvent(EV_SUBMERGE, 90000, 90000, EVENT_FLAG_NONE, true, phaseMaskForPhase(PHASE_NORMAL));
                    addEvent(EV_EMERGE, 90000, 90000, EVENT_FLAG_NONE, true, phaseMaskForPhase(PHASE_SUBMERGE));
                }
                else
                {
                    scheduleEvent(EV_INTRO, 1, 1);
                    scheduleEvent(EV_WRATHOFRAGNAROS, 30000, 30000);
                    scheduleEvent(EV_HANDOFRAGNAROS, 25000, 25000);
                    scheduleEvent(EV_LAVABURST, 10000, 10000);
                    scheduleEvent(EV_ERUPTION, 15000, 15000);
                    scheduleEvent(EV_ELEMENTALFIRE, 3000, 3000);
                    scheduleEvent(EV_SUBMERGE, 90000, 90000);
                    scheduleEvent(EV_EMERGE, 90000, 90000);
                }
            
                if (_instance)
                    _instance->SetData(DATA_RAGNAROS, NOT_STARTED);

                setPhase(PHASE_INTRO);

                me->SetVisibility(VISIBILITY_OFF);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->setFaction(35);
                doCast(me, SPELL_MELTWEAPON, true);
            }

            void onSummon(Creature* summoned)
            {
                Summons.Summon(summoned);
            }
	
            void onSummonDespawn(Creature* unit)
            {
                Summons.Despawn(unit);
            }

            void onEnterPhase(uint32 newPhase)
            {
            }

            void onKill(Unit* /*victim*/)
            {
                if (rand()%5)
                    return;

                DoScriptText(SAY_KILL, me);
            }

            void onDeath(Unit* /*killer*/)
            {
                if(_instance)
                    _instance->SetData(DATA_RAGNAROS, DONE);
            }

            void update(uint32 const diff)
            {
                if(_instance)
                {   
                    if (!Domo && _instance->GetData(DATA_MAJORDOMO) == DONE && Intro_Phase == 1)
                    {
                        Domo = me->SummonCreature(12018, DomoNewLocation.x, DomoNewLocation.y, DomoNewLocation.z, DomoNewLocation.o, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000);
                        if (Domo)
                            Domo->SetOwnerGUID(me->GetGUID());
                    }
                }

                updateEvents(diff, 129);
            
                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EV_INTRO:
                            if (_instance->GetData(DATA_RAGNAROS) == SPECIAL && Domo)
                            {
                                switch(Intro_Phase)
                                {
                                    case 1:
                                        Domo->CastSpell(Domo,SPELL_SUMMON_RAGNAROS,false); //visual
                                        Domo->GetMotionMaster()->MovePoint(0, DomoInvocLocation.x, DomoInvocLocation.y, DomoInvocLocation.z);
                                        DoScriptText(SAY_SUMMON_DOMO, Domo);
                                        scheduleEvent(EV_INTRO, 10000);
                                        break;
                                    case 2:
                                        Domo->SetInFront(me);
                                        Domo->StopMoving();
                                        scheduleEvent(EV_INTRO, 4000);
                                        break;
                                    case 3:
                                        me->SetVisibility(VISIBILITY_ON);
                                        doCast(me, SPELL_RAGEMERGE);
                                        scheduleEvent(EV_INTRO, 3000);
                                        break;
                                    case 4:
                                        DoScriptText(SAY_ARRIVAL1_RAG, me);
                                        scheduleEvent(EV_INTRO, 14000);
                                        break;
                                    case 5:
                                        DoScriptText(SAY_ARRIVAL2_DOMO, Domo);
                                        scheduleEvent(EV_INTRO, 8000);
                                        break;
                                    case 6:
                                        DoScriptText(SAY_ARRIVAL3_RAG, me);
                                        scheduleEvent(EV_INTRO, 17000);
                                        break;
                                    case 7:
                                        doCast(Domo, SPELL_MAGMABURST);
                                        scheduleEvent(EV_INTRO, 1000);
                                        break;
                                    case 8:
                                        Domo->setDeathState(JUST_DIED);
                                        scheduleEvent(EV_INTRO, 3000);
                                        break;        
                                    case 9:
                                        DoScriptText(SAY_ARRIVAL4_RAG, me);
                                        scheduleEvent(EV_INTRO, 8000);
                                        break;
                                    case 10:
                                        me->setFaction(91);
                                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                        if(_instance)
                                            _instance->SetData(DATA_RAGNAROS, IN_PROGRESS);
                                        setPhase(PHASE_NORMAL);
                                        setZoneInCombat(true);
                                        break;
                                }
                                Intro_Phase++;
                            }
                            break;
                        case EV_WRATHOFRAGNAROS:
                            doCast(me->getVictim(), SPELL_WRATHOFRAGNAROS);

                            if (rand()%2 == 0)
                                DoScriptText(SAY_WRATH, me);
                            scheduleEvent(EV_WRATHOFRAGNAROS, 30000);
                            break;
                        case EV_HANDOFRAGNAROS:
                            doCast(me, SPELL_HANDOFRAGNAROS);

                            if (rand()%2==0)
                                DoScriptText(SAY_HAND, me);
                            scheduleEvent(EV_HANDOFRAGNAROS, 25000);
                            break;
                        case EV_LAVABURST:
                            doCast(me->getVictim(), SPELL_LAVABURST);
                            scheduleEvent(EV_LAVABURST, 10000);
                            break;
                        case EV_ERUPTION:
                            doCast(me->getVictim(), SPELL_ERRUPTION);
                            scheduleEvent(EV_ERUPTION, 20000, 45000);
                            break;
                        case EV_ELEMENTALFIRE:
                            doCast(me->getVictim(), SPELL_ELEMENTALFIRE);
                            scheduleEvent(EV_ELEMENTALFIRE, 10000, 14000);
                            break;
                        case EV_SUBMERGE:
                            me->InterruptNonMeleeSpells(false);
                            //Root self
                            doCast(me, 23973);
                            me->setFaction(35);
                            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            me->HandleEmoteCommand(EMOTE_ONESHOT_SUBMERGE);
                            if (!HasSubmergedOnce)
                            {
                                DoScriptText(SAY_REINFORCEMENTS1, me);

                                // summon 10 elementals
                                Unit* target = NULL;
                                for(int i = 0; i < 9;i++)
                                {
                                    Creature* summoned = NULL;
                                    if (target = selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                    {
                                        summoned = me->SummonCreature(12143,target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(),0,TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN,900000);
                                        if(summoned)
                                            ((CreatureAI*)summoned->AI())->AttackStart(target);
                                    }
                                }

                                HasSubmergedOnce = true;
                            }
                            else
                            {
                                DoScriptText(SAY_REINFORCEMENTS2, me);

                                Unit* target = NULL;
                                for(int i = 0; i < 9;i++)
                                {
                                    Creature* summoned = NULL;
                                    if (target = selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                    {
                                        summoned = me->SummonCreature(12143,target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(),0,TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN,900000);
                                        if(summoned)
                                            ((CreatureAI*)summoned->AI())->AttackStart(target);
                                    }
                                }
                            }

                            doCast(me, SPELL_RAGSUBMERGE);
                            scheduleEvent(EV_SUBMERGE, 90000);
                            setPhase(PHASE_SUBMERGE);
                            break;
                        case EV_EMERGE:
                            me->setFaction(14);
                            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            doCast(me, SPELL_RAGEMERGE);
                            scheduleEvent(EV_EMERGE, 90000);
                            setPhase(PHASE_NORMAL);
                            break;
                    }
                }

                if (!updateVictim())
                    return;

                updateEvents(diff, 126);

                if (getPhase() != PHASE_NORMAL)
                {
                    //If we are within range melee the target
                    if(me->IsWithinMeleeRange(me->getVictim()))
                    {
                        //Make sure our attack is ready and we arn't currently casting
                        if(me->isAttackReady() && !me->IsNonMeleeSpellCasted(false))
                        {
                            me->AttackerStateUpdate(me->getVictim());
                            me->resetAttackTimer();
                        }
                    }
                    else
                    {
                        //MagmaBurst_Timer
                        if (MagmaBurst_Timer < diff)
                        {
                            doCast(me->getVictim(), SPELL_MAGMABURST);

                            if (!HasYelledMagmaBurst)
                            {
                                //Say our dialog
                                DoScriptText(SAY_MAGMABURST, me);
                                HasYelledMagmaBurst = true;
                            }

                            MagmaBurst_Timer = 2500;
                        }
                        else 
                            MagmaBurst_Timer -= diff;
                    }
                }
            }

        private:
            ScriptedInstance* _instance;
            SummonList Summons;
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new Boss_RagnarosAI(creature);
    }
};

void AddSC_boss_ragnaros()
{
    sScriptMgr.addScript(new Boss_Ragnaros());
}

