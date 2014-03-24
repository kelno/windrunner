/* ScriptData
SDName: boss_the_lurker_below
SD%Complete: 80
SDComment: Coilfang Frenzy, find out how could we fishing in the strangepool
SDCategory: The Lurker Below
EndScriptData */

#include "precompiled.h"
#include "def_serpent_shrine.h"
#include "SimpleAI.h"

enum
{
    SPELL_SPOUT         = 37433,
    SPELL_SPOUT_ANIM    = 42835,
    SPELL_SPOUT_BREATH  = 37431,
    SPELL_KNOCKBACK     = 19813,
    SPELL_GEYSER        = 37478,
    SPELL_WHIRL         = 37660,
    SPELL_WATERBOLT     = 37138,
    SPELL_SUBMERGE      = 37550,
    SPELL_EMERGE        = 20568,

    SPOUT_DIST          = 100,

    MOB_COILFANG_GUARDIAN = 21873,
    MOB_COILFANG_AMBUSHER = 21865,

    //Ambusher spells
    SPELL_SPREAD_SHOT   = 37790,
    SPELL_SHOOT         = 37770,

    //Guardian spells
    SPELL_ARCINGSMASH   = 38761, // Wrong SpellId. Can't find the right one.
    SPELL_HAMSTRING     = 26211
};

#define EMOTE_SPOUT     "prend une profonde respiration."

float AddPos[9][3] =
{
    {0.17f  , -468.30f, -19.79f},   //MOVE_AMBUSHER_1 X, Y, Z
    {8.43f  , -471.48f, -19.79f},   //MOVE_AMBUSHER_2 X, Y, Z
    {56.75f , -466.73f, -19.79f},   //MOVE_AMBUSHER_3 X, Y, Z
    {63.88f , -464.75f, -19.79f},   //MOVE_AMBUSHER_4 X, Y, Z
    {65.88f , -374.74f, -19.79f},   //MOVE_AMBUSHER_5 X, Y, Z
    {78.52f , -381.99f, -19.72f},   //MOVE_AMBUSHER_6 X, Y, Z
    {42.88f , -391.15f, -18.97f},   //MOVE_GUARDIAN_1 X, Y, Z
    {13.63f , -430.81f, -19.46f},   //MOVE_GUARDIAN_2 X, Y, Z
    {62.66f , -413.97f, -19.27f}    //MOVE_GUARDIAN_3 X, Y, Z
};

enum Phases
{
    NONE,
    INTRO,
    SUBMERGED,
    EMERGED,
    IN_ROTATE
};

class Boss_Lurker_Below : public CreatureScript
{
    public:
        Boss_Lurker_Below() : CreatureScript("Boss_Lurker_Below") {}
    
    class Boss_Lurker_BelowAI : public Creature_NoMovementAINew
    {
        public:

            uint32 introState;
            uint32 introPhaseTimer;
            uint32 whirlTimer;
            uint32 geyserTimer;
            uint32 waterboltTimer;
            uint32 phaseTimer;
            uint32 spoutTimer;
            uint32 rotateState;
            uint32 rotateStateTimer;
            uint32 spoutAnimTimer;
            uint32 rotTimer;
            uint32 submergeState;
            float lastOrientation;

            Boss_Lurker_BelowAI(Creature* creature) : Creature_NoMovementAINew(creature), summons(me)
            {
                _instance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            bool canStart()
            {
                if(_instance)
                {
                    if(_instance->GetData(DATA_THELURKERBELOWEVENT) != DONE)
                    {
                        if(_instance->GetData(DATA_STRANGE_POOL) == IN_PROGRESS)
                            return true;
                    }
                }
                return false;
            }

            void onReset(bool onSpawn)
            {
                me->AddUnitMovementFlag(MOVEMENTFLAG_SWIMMING + MOVEMENTFLAG_LEVITATING);
                summons.DespawnAll();

                if (_instance)
                {
                    _instance->SetData(DATA_THELURKERBELOWEVENT, NOT_STARTED);
                    _instance->SetData(DATA_STRANGE_POOL, NOT_STARTED);
                }

                setPhase(INTRO);

                introState = 0;
                introPhaseTimer = 0;

                whirlTimer = 18000;
                geyserTimer = rand()%5000 + 15000;
                waterboltTimer = 15000;
                phaseTimer = 90000;
                spoutTimer = 45000;
                rotateState = 0;
                rotateStateTimer = 0;
                spoutAnimTimer = 0;
                rotTimer = 0;
                submergeState = 0;
                lastOrientation = 0.0f;
            }

            void attackStart(Unit *pTarget)
            {
                if (getPhase() == SUBMERGED || getPhase() == IN_ROTATE)
                    return;

                Creature_NoMovementAINew::attackStart(pTarget);
            }

            void onDeath(Unit* /*killer*/)
            {
                if (_instance)
                    _instance->SetData(DATA_THELURKERBELOWEVENT, DONE);

                summons.DespawnAll();
            }

            void onEnterPhase(uint32 newPhase)
            {
                switch(newPhase)
                {
                    case INTRO:
                        me->InterruptNonMeleeSpells(false);
                        me->RemoveAllAuras();
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
                        me->SetFlag(UNIT_NPC_EMOTESTATE, EMOTE_STATE_SUBMERGED);
                        doCast(me, SPELL_SUBMERGE);
                        me->SetVisibility(VISIBILITY_OFF);
                        me->setFaction(35);
                        me->SetReactState(REACT_PASSIVE);
                        break;
                    case EMERGED:
                        me->InterruptNonMeleeSpells(false);
                        me->RemoveAllAuras();
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
                        me->RemoveFlag(UNIT_NPC_EMOTESTATE, EMOTE_STATE_SUBMERGED);
                        doCast(me, SPELL_EMERGE, true);
                        submergeState = 0;
                        break;
                    case SUBMERGED:
                        me->InterruptNonMeleeSpells(false);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
                        me->SetFlag(UNIT_NPC_EMOTESTATE, EMOTE_STATE_SUBMERGED);
                        doCast(me, SPELL_SUBMERGE);
                        submergeState = 1;
                        break;
                    case IN_ROTATE:
                        me->InterruptNonMeleeSpells(false);
                        me->MonsterTextEmote(EMOTE_SPOUT, 0, true);
                        lastOrientation = me->GetOrientation();
                        doCast(me, SPELL_SPOUT_BREATH);
                        rotateState = 1;
                        rotateStateTimer = 3000;
                        break;
                }
            }

            void onSummon(Creature* summoned)
            {
                summons.Summon(summoned);

                if (summoned->getAI())
                    summoned->getAI()->setZoneInCombat(true);
            }
    
            void onSummonDespawn(Creature* unit)
            {
                summons.Despawn(unit);
            }

            void update(uint32 const diff)
            {
                if (getPhase() <= INTRO)
                {
                    if (canStart())
                    {
                        if (introPhaseTimer <= diff)
                        {
                            switch (introState)
                            {
                                case 0:
                                    if (_instance)
                                        _instance->SendScriptInTestNoLootMessageToAll();
                                    me->SetVisibility(VISIBILITY_ON);
                                    introState = 1;
                                    introPhaseTimer = 500;
                                    break;
                                case 1:
                                    me->RemoveAllAuras();
                                    me->RemoveFlag(UNIT_NPC_EMOTESTATE, EMOTE_STATE_SUBMERGED);
                                    doCast(me, SPELL_EMERGE);
                                    introState = 2;
                                    introPhaseTimer = 3000;
                                    break;
                                case 2:
                                    me->SetReactState(REACT_AGGRESSIVE);
                                    me->RestoreFaction();
                                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
                                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

                                    setZoneInCombat(true);
                                    if (Unit* target = selectUnit(SELECT_TARGET_RANDOM, 0))
                                        attackStart(target);

                                    if (_instance)
                                    {
                                        _instance->SetData(DATA_THELURKERBELOWEVENT, IN_PROGRESS);
                                        _instance->SetData(DATA_STRANGE_POOL, DONE);
                                    }
                                    m_phase = EMERGED;
                                    break;
                            }
                        }
                        else
                            introPhaseTimer -= diff;
                    }
                    return;
                }

                if (!updateCombat())
                    return;

                switch (getPhase())
                {
                    case EMERGED:
                        if (!updateVictim())
                            return;

                        if (spoutTimer < diff)
                        {
                            spoutTimer = 22000;
                            whirlTimer = 1000;//whirl directly after spout
                            rotTimer = 24500;
                            setPhase(IN_ROTATE);
                            return;
                        }
                        else
                            spoutTimer -= diff;

                        if (whirlTimer < diff)
                        {
                            whirlTimer = 18000;
                            doCast(me, SPELL_WHIRL);
                        }
                        else
                            whirlTimer -= diff;

                        if (geyserTimer < diff)
                        {
                            if (Unit* target = selectUnit(SELECT_TARGET_RANDOM, 0, 150.0, true, true))
                                doCast(target, SPELL_GEYSER);
                            else
                                doCast(me->GetVictim(), SPELL_GEYSER);

                            geyserTimer = rand()%5000 + 15000;
                        }
                        else
                            geyserTimer -= diff;

                        if (waterboltTimer < diff)
                        {
                            if (!isInMeleeRange())
                            {
                                if (Unit* target = selectUnit(SELECT_TARGET_RANDOM, 0))
                                    doCast(target, SPELL_WATERBOLT);

                                waterboltTimer = 3000;
                            }
                        }
                        else
                            waterboltTimer -= diff;

                        doMeleeAttackIfReady();
                        break;
                    case IN_ROTATE:
                        if(rotTimer < diff)
                        {
                            rotTimer = 0;
                            rotateState = 0;
                            rotateStateTimer = 0;
                            m_phase = EMERGED;

                            if(me->GetVictim())
                            {
                                me->SetUInt64Value(UNIT_FIELD_TARGET, me->GetVictim()->GetGUID());
                                me->SetInFront(me->GetVictim());
                                me->StopMoving();
                            }
                            return;
                        }
                        else
                            rotTimer -= diff;

                        if (rotateStateTimer <= diff)
                        {
                            switch (rotateState)
                            {
                                case 1:
                                    if(rand()%2)
                                        me->StartAutoRotate(CREATURE_ROTATE_LEFT, 20000, lastOrientation, false);
                                    else
                                        me->StartAutoRotate(CREATURE_ROTATE_RIGHT, 20000, lastOrientation, false);

                                    rotateState = 2;
                                    break;
                                case 2:
                                case 3:
                                    break;
                            }
                        }
                        else
                            rotateStateTimer -= diff;

                        me->SetUInt64Value(UNIT_FIELD_TARGET, 0);

                        if (rotateState == 1)
                        {
                            me->SetOrientation(lastOrientation);
                            me->StopMoving();
                        }
                        else if (rotateState == 2)
                        {
                            Map* pMap = me->GetMap();
                            Map::PlayerList const &PlayerList = pMap->GetPlayers();
                            for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                            {
                                if (i->getSource() && i->getSource()->IsAlive() && !i->getSource()->IsPet())
                                {
                                    if (me->HasInArc((double)diff/20000*(double)M_PI*2,i->getSource()))
                                    {
                                        if (me->IsWithinDistInMap(i->getSource(), SPOUT_DIST))
                                        {
                                            if (!i->getSource()->IsInWater())
                                                doCast(i->getSource(), SPELL_SPOUT, true);
                                        }
                                    }
                                }
                            }

                            if(spoutAnimTimer < diff)
                            {
                                if (rotTimer >= 2000)
                                    doCast(me, SPELL_SPOUT_ANIM, true);

                                spoutAnimTimer = 1000;
                            }
                            else
                                spoutAnimTimer -= diff;

                            if (!me->IsUnitRotating())
                            {
                                rotateState = 3;
                                rotateStateTimer = 1000;
                                lastOrientation = me->GetOrientation();
                            }
                        }
                        else if (rotateState == 3)
                        {
                            me->SetOrientation(lastOrientation);
                            me->StopMoving();
                        }
                        break;
                    case SUBMERGED:
                        if (submergeState == 1)
                        {
                            for (uint8 i = 0; i < 9; ++i)
                            {
                                if (i < 6)
                                    me->SummonCreature(MOB_COILFANG_AMBUSHER, AddPos[i][0], AddPos[i][1], AddPos[i][2], 0, TEMPSUMMON_CORPSE_DESPAWN, 0);
                                else
                                    me->SummonCreature(MOB_COILFANG_GUARDIAN, AddPos[i][0], AddPos[i][1], AddPos[i][2], 0, TEMPSUMMON_CORPSE_DESPAWN, 0);
                            }
                            submergeState = 2;
                        }
                }

                if (phaseTimer < diff)
                {
                    if (getPhase() == EMERGED)
                    {
                        phaseTimer = 60000;
                        setPhase(SUBMERGED);
                        return;
                    }
                    else if (getPhase() == SUBMERGED)
                    {
                        phaseTimer = 90000;
                        setPhase(EMERGED);
                        spoutTimer = 2000;
                        return;
                    }
                }
                else
                    phaseTimer -= diff;
            }
        private:
            ScriptedInstance* _instance;
            SummonList summons;
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new Boss_Lurker_BelowAI(creature);
    }
};

class Mob_Coilfang_Guardian : public CreatureScript
{
    public:
        Mob_Coilfang_Guardian() : CreatureScript("Mob_Coilfang_Guardian") {}

    class Mob_Coilfang_GuardianAI : public CreatureAINew
    {
        public:
            enum event
            {
                EV_ARCINGSMASH    = 0,
                EV_HAMSTRING      = 1
            };

            Mob_Coilfang_GuardianAI(Creature* creature) : CreatureAINew(creature)
            {
                _instance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onReset(bool onSpawn)
            {
                if (onSpawn)
                {
                    addEvent(EV_ARCINGSMASH, 5000, 5000);
                    addEvent(EV_HAMSTRING, 2000, 2000);
                }
                else
                {
                    scheduleEvent(EV_ARCINGSMASH, 5000, 5000);
                    scheduleEvent(EV_HAMSTRING, 2000, 2000);
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
                        case EV_ARCINGSMASH:
                            doCast(me->GetVictim(), SPELL_ARCINGSMASH);
                            scheduleEvent(EV_ARCINGSMASH, urand(10000, 15000));
                            break;
                        case EV_HAMSTRING:
                            doCast(me->GetVictim(), SPELL_HAMSTRING);
                            scheduleEvent(EV_HAMSTRING, urand(10000, 15000));
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
        return new Mob_Coilfang_GuardianAI(creature);
    }
};

class Mob_Coilfang_Ambusher : public CreatureScript
{
    public:
        Mob_Coilfang_Ambusher() : CreatureScript("Mob_Coilfang_Ambusher") {}

    class Mob_Coilfang_AmbusherAI : public Creature_NoMovementAINew
    {
        public:
            enum event
            {
                EV_MULTISHOT      = 0,
                EV_SHOOTBOW       = 1
            };

            Mob_Coilfang_AmbusherAI(Creature* creature) : Creature_NoMovementAINew(creature)
            {
                _instance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onReset(bool onSpawn)
            {
                if (onSpawn)
                {
                    addEvent(EV_MULTISHOT, 10000, 10000);
                    addEvent(EV_SHOOTBOW, 4000, 4000);
                }
                else
                {
                    scheduleEvent(EV_MULTISHOT, 10000, 10000);
                    scheduleEvent(EV_SHOOTBOW, 4000, 4000);
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
                        case EV_MULTISHOT:
                            doCast(me->GetVictim(), SPELL_SPREAD_SHOT);

                            scheduleEvent(EV_MULTISHOT, urand(5000, 15000));
                            break;
                        case EV_SHOOTBOW:
                            if (!me->HasUnitState(UNIT_STAT_CASTING))
                                doCast(me->GetVictim(), SPELL_SHOOT);

                            scheduleEvent(EV_SHOOTBOW, urand(2000, 5000));
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
        return new Mob_Coilfang_AmbusherAI(creature);
    }
};

void AddSC_boss_the_lurker_below()
{
    sScriptMgr.addScript(new Boss_Lurker_Below());
    sScriptMgr.addScript(new Mob_Coilfang_Guardian());
    sScriptMgr.addScript(new Mob_Coilfang_Ambusher());
}
