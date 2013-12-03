/* Copyright (C) 2009 Trinity <http://www.trinitycore.org/>
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
#include "def_sunwell_plateau.h"

enum Quotes
{
    YELL_BIRTH      =       -1580036,
    YELL_KILL1      =       -1580037,
    YELL_KILL2      =       -1580038,
    YELL_BREATH     =       -1580039,
    YELL_TAKEOFF    =       -1580040,
    YELL_BERSERK    =       -1580041,
    YELL_DEATH      =       -1580042,
    YELL_KALECGOS   =       -1580043,   //after felmyst's death spawned and say this
    EMOTE_DEEP_BREATH   =   -1580110    // "Felmyst takes a deep breath"
};

enum Spells
{
    //Aura
    AURA_SUNWELL_RADIANCE       =   45769,
    AURA_NOXIOUS_FUMES          =   47002,

    //Land phase
    SPELL_CLEAVE                =   19983,
    SPELL_CORROSION             =   45866,
    SPELL_GAS_NOVA              =   45855,
    SPELL_ENCAPSULATE_CHANNEL   =   45661,
    // SPELL_ENCAPSULATE_EFFECT    =   45665,
    // SPELL_ENCAPSULATE_AOE       =   45662,

    //Flight phase
    SPELL_VAPOR_SELECT          =   45391,   // fel to player, force cast 45392, 50000y selete target
    SPELL_VAPOR_SUMMON          =   45392,   // player summon vapor, radius around caster, 5y,
    SPELL_VAPOR_FORCE           =   45388,   // vapor to fel, force cast 45389
    SPELL_VAPOR_CHANNEL         =   45389,   // fel to vapor, green beam channel
    SPELL_VAPOR_TRIGGER         =   45411,   // linked to 45389, vapor to self, trigger 45410 and 46931
    SPELL_VAPOR_DAMAGE          =   46931,   // vapor damage, 4000
    SPELL_TRAIL_SUMMON          =   45410,   // vapor summon trail
    SPELL_TRAIL_TRIGGER         =   45399,   // trail to self, trigger 45402
    SPELL_TRAIL_DAMAGE          =   45402,   // trail damage, 2000 + 2000 dot
    SPELL_DEAD_SUMMON           =   45400,   // summon blazing dead, 5min
    SPELL_DEAD_PASSIVE          =   45415,
    SPELL_FOG_BREATH            =   45495,   // fel to self, speed burst
    SPELL_FOG_TRIGGER           =   45582,   // fog to self, trigger 45782
    SPELL_FOG_FORCE             =   45782,   // fog to player, force cast 45714
    SPELL_FOG_INFORM            =   45714,   // player let fel cast 45717, script effect
    SPELL_FOG_CHARM             =   45717,   // fel to player
    SPELL_FOG_CHARM2            =   45726,   // link to 45717

    SPELL_TRANSFORM_TRIGGER     =   44885,   // madrigosa to self, trigger 46350
    SPELL_TRANSFORM_VISUAL      =   46350,   //46411stun?
    SPELL_TRANSFORM_FELMYST     =   45068,   // become fel
    SPELL_FELMYST_SUMMON        =   45069,

    //Other
    SPELL_BERSERK               =   45078,
    SPELL_CLOUD_VISUAL          =   45212,
    SPELL_CLOUD_SUMMON          =   45884,
    SPELL_SOUL_SEVER            =   45917   // Casted at reset on all players with Fog of Corruption aura
};

enum PhaseFelmyst
{
	PHASE_NULL   = 0,
    PHASE_INTRO  = 1,
    PHASE_RESET  = 2,
    PHASE_PULL   = 3,
    PHASE_GROUND = 4,
    PHASE_ENC    = 5,
    PHASE_FLIGHT = 6
};

#define ORIENTATION_LEFT    4.7984
#define ORIENTATION_RIGHT   1.3033

static float flightMobLeft[] = {1468.380005, 730.267029, 60.083302};
static float flightMobRight[] = {1458.170044, 501.295013, 60.083302};

static float lefts[3][3] = { {1494.760010, 705.000000, 60.083302},
                             {1469.939941, 704.239014, 60.083302},
                             {1446.540039, 702.570007, 60.083302} };

static float rights[3][3] = { {1492.819946, 515.668030, 60.083302},
                              {1467.219971, 516.318970, 60.083302},
                              {1441.640015, 520.520020, 60.083302} };

class boss_felmyst : public CreatureScript
{
public:
	boss_felmyst() : CreatureScript("boss_felmyst") {}

    class boss_felmystAI : public CreatureAINew
    {
        public:
    	boss_felmystAI(Creature* creature) : CreatureAINew(creature), Summons(me)
        {
            pInstance = ((ScriptedInstance*)creature->GetInstanceData());
        }

        ScriptedInstance* pInstance;
        SummonList Summons;

        enum events
        {
            EVENT_CLEAVE               = 0,
            EVENT_CORROSION            = 1,
            EVENT_GAS_NOVA             = 2,
            EVENT_ENCAPSULATE          = 3,
            EVENT_ENCAPS_WARN          = 4,
            EVENT_FOG_CORRUPTION       = 5,
            EVENT_DEMONIC_VAPOR        = 6,
            EVENT_BERSERK              = 7
        };

        uint32 flightPhaseTimer;
        uint32 flightPhase;
        uint32 introPhaseTimer;
        uint32 introPhase;
        uint32 BreathCount;
        uint32 demonicCount;
        bool origin;
        bool direction;
        bool inChaseOnFlight;

        Unit* encapsTarget;

        void onReset(bool onSpawn)
        {
            origin = false;
            direction = false;
            inChaseOnFlight = false;
            encapsTarget = NULL;
            flightPhaseTimer = 60000;
            flightPhase = 0;
            introPhaseTimer = 0;
            introPhase = 0;
            BreathCount = 0;
            demonicCount = 0;

            // Disable automatic update mouvement flag (sometimes ground is detect in fly :s)
            me->setUseUpdateMouvementFlag(false);

            setPhase(PHASE_NULL);
            if (onSpawn)
                setPhase(PHASE_INTRO);
            else
                setPhase(PHASE_RESET);

            if (onSpawn)
            {
                addEvent(EVENT_CLEAVE, 5000, 10000, EVENT_FLAG_NONE, true, phaseMaskForPhase(4));
                addEvent(EVENT_CORROSION, 10000, 20000, EVENT_FLAG_NONE, true, phaseMaskForPhase(4));
                addEvent(EVENT_GAS_NOVA, 21000, 26000, EVENT_FLAG_NONE, true, phaseMaskForPhase(4));
                addEvent(EVENT_ENCAPSULATE, 33000, 33000, EVENT_FLAG_NONE, true, phaseMaskForPhase(4) | phaseMaskForPhase(5));
                addEvent(EVENT_ENCAPS_WARN, 32000, 32000, EVENT_FLAG_NONE, true, phaseMaskForPhase(4));
                addEvent(EVENT_FOG_CORRUPTION, 500, 500, EVENT_FLAG_NONE, false, phaseMaskForPhase(6));
                addEvent(EVENT_DEMONIC_VAPOR, 500, 500, EVENT_FLAG_NONE, false, phaseMaskForPhase(6));
                addEvent(EVENT_BERSERK, 600000, 600000, EVENT_FLAG_NONE, true, phaseMaskForPhase(4) | phaseMaskForPhase(6));
            }
            else
            {
                resetEvent(EVENT_CLEAVE, 5000, 10000);
                resetEvent(EVENT_CORROSION, 10000, 20000);
                resetEvent(EVENT_GAS_NOVA, 21000);
                resetEvent(EVENT_ENCAPSULATE, 33000);
                resetEvent(EVENT_ENCAPS_WARN, 32000);
                resetEvent(EVENT_FOG_CORRUPTION, 500);
                resetEvent(EVENT_DEMONIC_VAPOR, 500);
                resetEvent(EVENT_BERSERK, 600000);
            }

            me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, 10);
            me->SetFloatValue(UNIT_FIELD_COMBATREACH, 10);

            me->addUnitState(UNIT_STAT_IGNORE_PATHFINDING);

            Summons.DespawnAll();

            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
            me->SetFullTauntImmunity(true);

            if(pInstance)
                pInstance->SetData(DATA_FELMYST_EVENT, NOT_STARTED);

            doCast((Unit*)NULL, SPELL_TRANSFORM_FELMYST, true);

            if (pInstance)
            {
                Map::PlayerList const& players = pInstance->instance->GetPlayers();
                if (!players.isEmpty())
                {
                    for(Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    {
                        if (Player* plr = itr->getSource())
                        {
                            if (plr->HasAura(SPELL_FOG_CHARM))
                                plr->CastSpell(plr, SPELL_SOUL_SEVER, true);
                        }
                    }
                }
            }

            me->RemoveAurasDueToSpell(AURA_SUNWELL_RADIANCE);
            doCast(me, AURA_SUNWELL_RADIANCE, true);
        }

        void onEnterPhase(uint32 newPhase)
        {
            switch (newPhase)
            {
                case PHASE_INTRO:
                    me->SetSpeed(MOVE_RUN, 1.3f, true);
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->SetReactState(REACT_PASSIVE);
                    break;
                case PHASE_RESET:
                    me->SetSpeed(MOVE_RUN, 1.3f, true);
                    me->StopMoving();
                    me->HandleEmoteCommand(EMOTE_ONESHOT_LIFTOFF);
                    me->SetDisableGravity(true);
                    me->SendMovementFlagUpdate();
                    me->GetMotionMaster()->MovePath(25038, true);
                    setPhase(PHASE_PULL);
                    break;
                case PHASE_GROUND:
                    me->SetSpeed(MOVE_RUN, 1.0f, true);
                    flightPhaseTimer = 60000;
                    scheduleEvent(EVENT_CLEAVE, 5000, 10000);
                    scheduleEvent(EVENT_CORROSION, 10000, 20000);
                    scheduleEvent(EVENT_GAS_NOVA, 20000, 25000);
                    scheduleEvent(EVENT_ENCAPSULATE, 30000);
                    scheduleEvent(EVENT_ENCAPS_WARN, 29000);
                    break;
                case PHASE_FLIGHT:
                    me->SetSpeed(MOVE_RUN, 1.3f, true);
                    switch (rand()%2)
                    {
                        case 0:
                            origin = true;
                            direction = true;
                            break;
                        case 1:
                            origin = false;
                            direction = false;
                            break;
                    }

                    demonicCount = 0;
                    scheduleEvent(EVENT_DEMONIC_VAPOR, 5000);
                    enableEvent(EVENT_DEMONIC_VAPOR);
                    flightPhaseTimer = 300;
                    flightPhase = 0;
                    BreathCount = 0;
                    break;
            }
        }

        void onCombatStart(Unit* /*who*/)
        {
            me->SetWalk(false);

            if (pInstance)
                pInstance->SetData(DATA_FELMYST_EVENT, IN_PROGRESS);

            me->CastSpell(me, AURA_NOXIOUS_FUMES, true);

            if (pInstance)
            {
                if (Creature* brutallus = Creature::GetCreature(*me, pInstance->GetData64(DATA_BRUTALLUS))) {
                    if (!brutallus->HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE))
                        brutallus->SetVisibility(VISIBILITY_OFF);
                }
            }
            inChaseOnFlight = true;
        }

        void attackStart(Unit *pTarget)
        {
            if (!pTarget)
                return;

            if (getPhase() != PHASE_PULL && getPhase() != PHASE_GROUND)
            	return;

            if (me->Attack(pTarget, true))
            {
            	if (!aiInCombat())
            	    me->GetMotionMaster()->Initialize();

                if (me->isPet())
                {
                    if (pTarget->getVictim() && pTarget->getVictim()->GetGUID() != me->GetGUID())
                        me->GetMotionMaster()->MoveChase(pTarget, CONTACT_DISTANCE, M_PI);
                    else
                        me->GetMotionMaster()->MoveChase(pTarget);
                }
                else
                    me->GetMotionMaster()->MoveChase(pTarget);

                if (!aiInCombat())
                {
                    setAICombat(true);
                    onCombatStart(pTarget);
                }
            }
        }

        void onDeath(Unit* /*killer*/)
        {
            DoScriptText(YELL_DEATH, me);

            if(pInstance)
                pInstance->SetData(DATA_FELMYST_EVENT, DONE);

            if (Creature* kalecgos = me->SummonCreature(24844, 1501.253174, 764.737061, 117.972687, 4.626863, TEMPSUMMON_MANUAL_DESPAWN, 0))
                kalecgos->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        }

        void onKill(Unit* /*victim*/)
        {
            DoScriptText(RAND(YELL_KILL1, YELL_KILL2), me);
        }

        void onSummon(Creature* summoned)
        {
            Summons.Summon(summoned);
            if(summoned->GetEntry() == MOB_DEAD)
            {
            	summoned->AI()->AttackStart(selectUnit(SELECT_TARGET_RANDOM, 0));
            	summoned->CastSpell(summoned, SPELL_DEAD_PASSIVE, true);
            }
            else if (summoned->GetEntry() == MOB_VAPOR)
            	me->SetTarget(summoned->GetGUID());
        }

        void onSummonDespawn(Creature* unit)
        {
            if (unit->GetEntry() == MOB_VAPOR)
                me->SetTarget(0);

            Summons.Despawn(unit);
        }

        void onMovementInform(uint32 type, uint32 id)
        {
            if (type != POINT_MOTION_TYPE)
                return;

            switch (id)
            {
                case 1:
                    flightPhase++;
                    break;
                case 2:
                    me->RemoveAurasDueToSpell(SPELL_FOG_BREATH);
                    disableEvent(EVENT_FOG_CORRUPTION);

                    if (BreathCount < 3)
                    {
                        if (!direction)
                            me->GetMotionMaster()->MovePoint(0, rights[BreathCount][0], rights[BreathCount][1], rights[BreathCount][2]-10, true, 500);
                        else
                            me->GetMotionMaster()->MovePoint(0, lefts[BreathCount][0], lefts[BreathCount][1], lefts[BreathCount][2]-10, true, 500);
                    }
                    break;
                case 3:
                    me->StopMoving();

                    me->HandleEmoteCommand(EMOTE_ONESHOT_LAND);
                    me->SetDisableGravity(false);

                    float x, y, z;
                    me->GetPosition(x, y, z);
                    me->UpdateGroundPositionZ(x, y, z);
                    me->Relocate(x, y, z);

                    me->SendMovementFlagUpdate();

                    setPhase(PHASE_PULL);
                    if (Unit *target = selectUnit(SELECT_TARGET_TOPAGGRO, 0))
                    {
                        attackStart(target);
                        me->GetMotionMaster()->MoveChase(target);
                    }
                    else
                        evade();
                    break;
            }
        }

        void onDamageTaken(Unit* attacker, uint32& damage)
        {
            if(getPhase() != PHASE_GROUND && getPhase() != PHASE_ENC && damage >= me->GetHealth())
                damage = 0;
        }

        void onMoveInLoS(Unit* who)
        {
            if (!me->isInCombat())
                return;

            if(getPhase() != PHASE_FLIGHT && getPhase() != PHASE_INTRO)
                CreatureAINew::onMoveInLoS(who);
        }

        void handleIntro(uint32 const diff)
        {
            if (introPhaseTimer <= diff)
            {
                switch (introPhase)
                {
                    case 0:
                        me->SetStandState(UNIT_STAND_STATE_STAND);
                        DoScriptText(YELL_BIRTH, me);
                        introPhaseTimer = 4000;
                        introPhase++;
                        break;
                    case 1:
                        me->StopMoving();
                        me->HandleEmoteCommand(EMOTE_ONESHOT_LIFTOFF);
                        me->SetDisableGravity(true);
                        me->SendMovementFlagUpdate();
                        introPhaseTimer = 500;
                        introPhase++;
                        break;
                    case 2:
                        me->SetHomePosition(1464.726440, 606.836243, 72.818344, 0);
                        me->GetMotionMaster()->MovePoint(0, 1464.726440, 606.836243, 72.818344);
                        introPhaseTimer = 6000;
                        introPhase++;
                        break;
                    case 3:
                        me->GetMotionMaster()->MovePath(25038, true);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        me->SetReactState(REACT_AGGRESSIVE);
                        setPhase(PHASE_PULL);
                        break;
                }
            }
            else
                introPhaseTimer -= diff;
        }

        void handleFlight(uint32 const diff)
        {
            if (flightPhaseTimer <= diff)
            {
                switch(flightPhase)
                {
                    case 0:
                        me->StopMoving();
                        me->HandleEmoteCommand(EMOTE_ONESHOT_LIFTOFF);
                        me->SetDisableGravity(true);
                        me->SendMovementFlagUpdate();
                        DoScriptText(YELL_TAKEOFF, me);
                        flightPhaseTimer = 2000;
                        flightPhase++;
                        break;
                    case 1:
                        me->GetMotionMaster()->MovePoint(0, me->GetPositionX()+1, me->GetPositionY(), me->GetPositionZ() + 15.0f);
                        flightPhaseTimer = 34000;
                        flightPhase++;
                        break;
                    case 2:
                        if (!direction)
                            me->GetMotionMaster()->MovePoint(1, flightMobRight[0], flightMobRight[1], flightMobRight[2]);
                        else
                            me->GetMotionMaster()->MovePoint(1, flightMobLeft[0], flightMobLeft[1], flightMobLeft[2]);

                        flightPhase++;
                        break;
                    case 3:
                        // Wait mouvement inform
                        break;
                    case 4:
                        if (!direction)
                            me->GetMotionMaster()->MovePoint(0, lefts[BreathCount][0], lefts[BreathCount][1], lefts[BreathCount][2]-10);
                        else
                            me->GetMotionMaster()->MovePoint(0, rights[BreathCount][0], rights[BreathCount][1], rights[BreathCount][2]-10);

                        DoScriptText(EMOTE_DEEP_BREATH, me);
                        flightPhaseTimer = 2500;
                        flightPhase++;
                        break;
                    case 5:
                        doCast(me, SPELL_FOG_BREATH, false);
                        if (!direction)
                            me->GetMotionMaster()->MovePoint(2, lefts[BreathCount][0], lefts[BreathCount][1], lefts[BreathCount][2]-10);
                        else
                            me->GetMotionMaster()->MovePoint(2, rights[BreathCount][0], rights[BreathCount][1], rights[BreathCount][2]-10);

                        scheduleEvent(EVENT_FOG_CORRUPTION, 500);
                        enableEvent(EVENT_FOG_CORRUPTION);
                        direction = !direction;
                        BreathCount++;

                        flightPhaseTimer = 16000;
                        if (BreathCount >= 3)
                            flightPhaseTimer = 9000;

                        flightPhase++;
                        break;
                    case 6:
                        flightPhase++;
                        if(BreathCount < 3)
                            flightPhase = 4;
                        break;
                    case 7:
                        me->SetSpeed(MOVE_RUN, 1.3f, true);
                        if (!origin)
                            me->GetMotionMaster()->MovePoint(3, 1482.709961, 649.406006, 21.081100);
                        else
                            me->GetMotionMaster()->MovePoint(3, 1491.119995, 553.672974, 24.921900);

                        flightPhase++;
                        break;
                    default:
                        break;
                }
            }
            else
                flightPhaseTimer -= diff;
        }

        void update(const uint32 diff)
        {
            if (getPhase() == PHASE_INTRO)
            {
                handleIntro(diff);
                return;
            }

            if (!updateVictim())
                return;

            switch (getPhase())
            {
                case PHASE_PULL:
                    if (me->IsWithinMeleeRange(me->getVictim()))
                    {
                        if (inChaseOnFlight)
                        {
                            me->StopMoving();
                            inChaseOnFlight = false;
                            me->HandleEmoteCommand(EMOTE_ONESHOT_LAND);
                            me->SetDisableGravity(false);

                            float x, y, z;
                            me->GetPosition(x, y, z);
                            me->UpdateGroundPositionZ(x, y, z);
                            me->Relocate(x, y, z);

                            me->SendMovementFlagUpdate();
                        }
                        setPhase(PHASE_GROUND);
                    }
                    break;
                case PHASE_FLIGHT:
                    if (flightPhase >= 2)
                        me->SetTarget(0);

                    handleFlight(diff);
                    break;
                case PHASE_GROUND:
                    if (flightPhaseTimer)
                    {
                        if (flightPhaseTimer <= diff)
                            setPhase(PHASE_FLIGHT);
                        else
                            flightPhaseTimer -= diff;
                    }

                    doMeleeAttackIfReady();
                    break;
                case PHASE_ENC:
                    if (encapsTarget)
                        me->SetTarget(encapsTarget->GetGUID());
                    break;
            }

            updateEvents(diff);

            while (executeEvent(diff, m_currEvent))
            {
                switch (m_currEvent)
                {
                    case EVENT_CLEAVE:
                        doCast(me->getVictim(), SPELL_CLEAVE, false);
                        scheduleEvent(EVENT_CLEAVE, 5000, 10000);
                        break;
                    case EVENT_CORROSION:
                        doCast(me->getVictim(), SPELL_CORROSION, false);
                        scheduleEvent(EVENT_CORROSION, 20000, 30000);
                        break;
                    case EVENT_GAS_NOVA:
                        doCast(me, SPELL_GAS_NOVA, false);
                        scheduleEvent(EVENT_GAS_NOVA, 21000, 26000);
                        break;
                    case EVENT_ENCAPSULATE:
                        if(encapsTarget)
                            doCast(encapsTarget, SPELL_ENCAPSULATE_CHANNEL, false);

                        m_phase = PHASE_GROUND;

                        if (Unit* tank = selectUnit(SELECT_TARGET_TOPAGGRO, 0, 150.0f, true))
                            me->SetTarget(tank->GetGUID());

                        scheduleEvent(EVENT_ENCAPSULATE, 33000);
                        scheduleEvent(EVENT_ENCAPS_WARN, 32000);
                        break;
                    case EVENT_ENCAPS_WARN:
                        if (encapsTarget = selectUnit(SELECT_TARGET_RANDOM, 0, 150.0f, true))
                            me->SetTarget(encapsTarget->GetGUID());

                        setPhase(PHASE_ENC);

                        scheduleEvent(EVENT_ENCAPS_WARN, 32000);
                        break;
                    case EVENT_BERSERK:
                        if (!me->HasAura(SPELL_BERSERK))
                        {
                            DoScriptText(YELL_BERSERK, me);
                            doCast(me, SPELL_BERSERK, true);
                        }
                        scheduleEvent(EVENT_BERSERK, 10000);
                        break;
                    case EVENT_DEMONIC_VAPOR:
                        if (Unit* target = selectUnit(SELECT_TARGET_RANDOM, 0, 150, true))
                            doCast(me, SPELL_VAPOR_SELECT, true);

                        demonicCount++;
                        if (demonicCount >= 2)
                            disableEvent(EVENT_DEMONIC_VAPOR);

                        scheduleEvent(EVENT_DEMONIC_VAPOR, 15000);
                        break;
                    case EVENT_FOG_CORRUPTION:
                    {
                        if (pInstance)
                        {
                            switch (BreathCount)
                            {
                                case 0:
                                    pInstance->SetData((direction ? DATA_ACTIVATE_NORTH_TO_LEFT : DATA_ACTIVATE_NORTH_TO_RIGHT), (uint32) me->GetPositionY());
                                    break;
                                case 1:
                                    pInstance->SetData((direction ? DATA_ACTIVATE_CENTER_TO_LEFT : DATA_ACTIVATE_CENTER_TO_RIGHT), (uint32) me->GetPositionY());
                                    break;
                                case 2:
                                    pInstance->SetData((direction ? DATA_ACTIVATE_SOUTH_TO_LEFT : DATA_ACTIVATE_SOUTH_TO_RIGHT), (uint32) me->GetPositionY());
                                    break;
                            }
                        }

                        float x, y, z;
                        me->GetPosition(x, y, z);
                        me->UpdateGroundPositionZ(x, y, z);
                        if(Creature *Fog = me->SummonCreature(MOB_VAPOR_TRAIL, x, y, z, 0, TEMPSUMMON_TIMED_DESPAWN, 10000))
                        {
                            Fog->RemoveAurasDueToSpell(SPELL_TRAIL_TRIGGER);
                            Fog->CastSpell(Fog, SPELL_FOG_TRIGGER, true);
                            Fog->getAI()->message(1, 1);
                        }

                        scheduleEvent(EVENT_FOG_CORRUPTION, 500);
                        break;
                    }
                }
            }
        }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new boss_felmystAI(creature);
    }
};

class mob_felmyst_vapor : public CreatureScript
{
public:
	mob_felmyst_vapor() : CreatureScript("mob_felmyst_vapor") {}

    class mob_felmyst_vaporAI : public CreatureAINew
    {
        public:
    	mob_felmyst_vaporAI(Creature* creature) : CreatureAINew(creature)
        {
            pInstance = ((ScriptedInstance*)creature->GetInstanceData());
        }

        ScriptedInstance* pInstance;
        bool startFollow;

        void onReset(bool onSpawn)
        {
            startFollow = false;
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetWalk(false);
            me->SetSpeed(MOVE_RUN, 1.0f);
        }

        void onCombatStart(Unit* /*who*/)
        {
            startFollow = true;
            setZoneInCombat(true);
            doCast((Unit*)NULL, SPELL_VAPOR_FORCE, true);
            doCast(me, SPELL_VAPOR_TRIGGER, true);
        }

        void update(uint32 const /*diff*/)
        {
            if (startFollow)
            {
                if(!me->getVictim())
                    attackStart(selectUnit(SELECT_TARGET_RANDOM, 0));
            }
        }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new mob_felmyst_vaporAI(creature);
    }
};

class mob_felmyst_trail : public CreatureScript
{
public:
	mob_felmyst_trail() : CreatureScript("mob_felmyst_trail") {}

    class mob_felmyst_trailAI : public Creature_NoMovementAINew
    {
        public:
    	mob_felmyst_trailAI(Creature* creature) : Creature_NoMovementAINew(creature)
        {
            pInstance = ((ScriptedInstance*)creature->GetInstanceData());
        }

        ScriptedInstance* pInstance;

        enum events
        {
            EVENT_DEAD            = 0
        };

        void onReset(bool onSpawn)
        {
            if (onSpawn)
                addEvent(EVENT_DEAD, 7000, 8000);
            else
                resetEvent(EVENT_DEAD, 7000, 8000);

            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            doCast((Unit*)NULL, SPELL_TRAIL_TRIGGER, true);
            me->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, 0.01); // core bug
        }

        void message(uint32 id, uint32 data)
        {
            if (id == 1)
                disableEvent(EVENT_DEAD);
        }
 
        void attackStart(Unit* /*victim*/) {}

        void update(uint32 const diff)
        {
            updateEvents(diff);

            while (executeEvent(diff, m_currEvent))
            {
                switch (m_currEvent)
                {
                    case EVENT_DEAD:
                        disableEvent(EVENT_DEAD);
                        doCast((Unit*)NULL, SPELL_DEAD_SUMMON, true);
                        me->DisappearAndDie();
                        break;
                }
            }
        }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new mob_felmyst_trailAI(creature);
    }
};

class mob_unyielding_dead : public CreatureScript
{
public:
	mob_unyielding_dead() : CreatureScript("mob_unyielding_dead") {}

    class mob_unyielding_deadAI : public CreatureAINew
    {
        public:
    	mob_unyielding_deadAI(Creature* creature) : CreatureAINew(creature)
        {
            pInstance = ((ScriptedInstance*)creature->GetInstanceData());
        }

        ScriptedInstance* pInstance;

        void onReset(bool onSpawn)
        {
            setZoneInCombat(true);
            attackStart(selectUnit(SELECT_TARGET_RANDOM, 0));
            doCast((Unit*)NULL, SPELL_DEAD_PASSIVE, true);
        }

        void updateEM(uint32 const diff)
        {
            if (pInstance->GetData(DATA_FELMYST_EVENT) != IN_PROGRESS)
                me->DisappearAndDie();
        }

        void update(const uint32 diff)
        {
            if (pInstance->GetData(DATA_FELMYST_EVENT) != IN_PROGRESS)
                me->DisappearAndDie();

            if (!updateVictim())
                return;

            doMeleeAttackIfReady();
        }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new mob_unyielding_deadAI(creature);
    }
};

struct trigger_felmyst_fogAI : public ScriptedAI
{
    trigger_felmyst_fogAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }
    
    ScriptedInstance *pInstance;
    
    void Reset()
    {
    }
    
    void DamageTaken(Unit *done_by, uint32 &damage)
    {
        damage = 0;
    }
    
    void SpellHit(Unit *caster, const SpellEntry *spell)
    {
        sLog.outString("Hit by spell %u", spell->Id);
    }
    
    void Aggro(Unit *pWho) {}
    
    void UpdateAI(uint32 const diff)
    {
    }
};

CreatureAI* GetAI_trigger_felmyst_fog(Creature *pCreature)
{
    return new trigger_felmyst_fogAI(pCreature);
}

void AddSC_boss_felmyst()
{
    Script *newscript;

    sScriptMgr.addScript(new boss_felmyst());
    sScriptMgr.addScript(new mob_felmyst_vapor());
    sScriptMgr.addScript(new mob_felmyst_trail());
    sScriptMgr.addScript(new mob_unyielding_dead());
    
    newscript = new Script;
    newscript->Name="trigger_felmyst_fog";
    newscript->GetAI = &GetAI_trigger_felmyst_fog;
    newscript->RegisterSelf();
}
