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

/* ScriptData
SDName: Boss_Felmyst
SD%Complete: 95%
SDComment: Flying movements may not be 100% correct. Kalecgos event on death. Flying path on spawn. Autoshoot & mind flay decast.
EndScriptData */

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
    PHASE_NULL  = 0,
    PHASE_GROUND = 1,
    PHASE_FLIGHT = 2,
};

enum EventFelmyst
{
    EVENT_NULL          =   0,
    EVENT_BERSERK       =   1,

    EVENT_CLEAVE        =   2,
    EVENT_CORROSION     =   3,
    EVENT_GAS_NOVA      =   4,
    EVENT_ENCAPS_WARN   =   5,
    EVENT_ENCAPSULATE   =   6,
    EVENT_FLIGHT        =   7,

    EVENT_FLIGHT_SEQUENCE   =   2,
    EVENT_SUMMON_DEAD       =   3,
    EVENT_SUMMON_FOG        =   4
};

static EventFelmyst MaxTimer[]=
{
    EVENT_NULL,
    EVENT_FLIGHT,
    EVENT_SUMMON_FOG,
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

struct boss_felmystAI : public ScriptedAI
{
    boss_felmystAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        justBorn = true;
        IntroTimer = 5000;
        IntroPhase = 1;
    }

    ScriptedInstance *pInstance;
    PhaseFelmyst Phase;
    EventFelmyst Event;
    uint32 Timer[EVENT_FLIGHT + 1];
    
    uint32 jumpFlagTimer;
    
    uint32 IntroTimer;
    uint8 IntroPhase;

    uint32 FlightCount;
    uint32 BreathCount;
    
    uint8 randomPoint;

    float BreathX, BreathY;
    
    bool justPulled;
    bool goingLeft;
    bool justBorn;
    
    Unit* encapsTarget;

    void Reset()
    {
        Phase = PHASE_NULL;
        Event = EVENT_NULL;
        Timer[EVENT_BERSERK] = 600000;
        FlightCount = 0;
        randomPoint = 0;
        
        jumpFlagTimer = 1000;
        
        justPulled = false;

        if (justBorn) {
            me->SetStandState(PLAYER_STATE_SLEEP);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetReactState(REACT_PASSIVE);
        }
        else {
            m_creature->SetHover(true);
            me->GetMotionMaster()->MovePath(25038, true);
            m_creature->SendMovementFlagUpdate();
        }

        m_creature->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, 10);
        m_creature->SetFloatValue(UNIT_FIELD_COMBATREACH, 10);
        
        m_creature->addUnitState(UNIT_STAT_IGNORE_PATHFINDING);

        DespawnSummons(MOB_VAPOR_TRAIL);
        m_creature->setActive(false);
        
        m_creature->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
        m_creature->SetFullTauntImmunity(true);

        if(pInstance)
            pInstance->SetData(DATA_FELMYST_EVENT, NOT_STARTED);
            
        WorldPacket data;                       //send update position to client
        m_creature->BuildHeartBeatMsg(&data);
        m_creature->SendMessageToSet(&data,true);
        
        goingLeft = false;
        
        encapsTarget = NULL;
        
        if (pInstance) {
            Map::PlayerList const& players = pInstance->instance->GetPlayers();
            if (!players.isEmpty()) {
                for(Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr) {
                    if (Player* plr = itr->getSource()) {
                        if (plr->HasAura(SPELL_FOG_CHARM))
                            plr->CastSpell(plr, SPELL_SOUL_SEVER, true);
                    }
                }
            }
        }
        
        me->RemoveAurasDueToSpell(45769);
        me->CastSpell(me, 45769, true);
    }

    void Aggro(Unit *who)
    {
        if (justBorn)
            return;
            
        m_creature->GetMotionMaster()->Initialize();
        //m_creature->GetMotionMaster()->MoveChase(who);

        m_creature->setActive(true);
        DoZoneInCombat();
        m_creature->CastSpell(m_creature, AURA_SUNWELL_RADIANCE, true);
        m_creature->CastSpell(m_creature, AURA_NOXIOUS_FUMES, true);
        EnterPhase(PHASE_GROUND, true);
        justPulled = true;

        if (pInstance) {
            pInstance->SetData(DATA_FELMYST_EVENT, IN_PROGRESS);
            if (Creature* brutallus = Creature::GetCreature(*me, pInstance->GetData64(DATA_BRUTALLUS))) {
                if (!brutallus->HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE))
                    brutallus->SetVisibility(VISIBILITY_OFF);
            }
        }
    }

    void AttackStart(Unit *who)
    {
        if(Phase != PHASE_FLIGHT)
            ScriptedAI::AttackStart(who);
    }

    void MoveInLineOfSight(Unit *who)
    {
        if (!me->isInCombat())
            return;

        if(Phase != PHASE_FLIGHT && !justBorn)
            ScriptedAI::MoveInLineOfSight(who);
    }

    void KilledUnit(Unit* victim)
    {
        DoScriptText(RAND(YELL_KILL1, YELL_KILL2), m_creature);
    }

    void JustDied(Unit* Killer)
    {
        DoScriptText(YELL_DEATH, m_creature);

        if(pInstance)
            pInstance->SetData(DATA_FELMYST_EVENT, DONE);
            
        if (Creature* kalecgos = me->SummonCreature(24844, 1501.253174, 764.737061, 117.972687, 4.626863, TEMPSUMMON_MANUAL_DESPAWN, 0)) {
            kalecgos->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            kalecgos->setActive(true);
        }
    }

    void SpellHit(Unit *caster, const SpellEntry *spell)
    {
        if (spell->Id == SPELL_FOG_INFORM)
        {
            float x, y, z;
            caster->GetPosition(x, y, z);
            Unit* summon = m_creature->SummonCreature(MOB_DEAD, x, y, z, 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000);
            if(summon)
            {
                summon->SetMaxHealth(caster->GetMaxHealth());
                summon->SetHealth(caster->GetMaxHealth());
                summon->CastSpell(summon, SPELL_FOG_CHARM, true);
                summon->CastSpell(summon, SPELL_FOG_CHARM2, true);
            }
            m_creature->DealDamage(caster, caster->GetHealth(), NULL, DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
        }
    }

    void JustSummoned(Creature *summon)
    {
        if(summon->GetEntry() == MOB_DEAD)
        {
            summon->AI()->AttackStart(SelectUnit(SELECT_TARGET_RANDOM, 0));
            DoZoneInCombat(summon);
            summon->CastSpell(summon, SPELL_DEAD_PASSIVE, true);
        }
    }

    void MovementInform(uint32, uint32)
    {
        if (FlightCount == 6 || FlightCount == 7) {
            if (!goingLeft)
                m_creature->SetOrientation(m_creature->GetAngle(lefts[randomPoint][0], lefts[randomPoint][1]));
            else
                m_creature->SetOrientation(m_creature->GetAngle(rights[randomPoint][0], rights[randomPoint][1]));
//            me->SendMovementFlagUpdate(300.f);
            Timer[EVENT_FLIGHT_SEQUENCE] = 1;
        }
        else
            Timer[EVENT_FLIGHT_SEQUENCE] = 1;
    }

    void DamageTaken(Unit*, uint32 &damage)
    {
        if(Phase != PHASE_GROUND && damage >= m_creature->GetHealth())
            damage = 0;
    }

    void EnterPhase(PhaseFelmyst NextPhase, bool pull = false)
    {
        switch(NextPhase)
        {
        case PHASE_GROUND:
            Timer[EVENT_CLEAVE] = 5000 + rand()%5 * 1000;
            Timer[EVENT_CORROSION] = 10000 + rand()%10 * 1000;
            Timer[EVENT_GAS_NOVA] = 20000 + rand()%5 * 1000;
            if (pull)
                Timer[EVENT_ENCAPSULATE] = 25000 + rand()%5 * 1000;
            else
                Timer[EVENT_ENCAPSULATE] = 25000 + rand()%5 * 1000;
            Timer[EVENT_ENCAPS_WARN] = Timer[EVENT_ENCAPSULATE] - 1000;
            Timer[EVENT_FLIGHT] = 60000;
            break;
        case PHASE_FLIGHT:
            m_creature->SetSpeed(MOVE_FLIGHT, 1.3f, true);
            Timer[EVENT_FLIGHT_SEQUENCE] = 1000;
            Timer[EVENT_SUMMON_DEAD] = 0;
            Timer[EVENT_SUMMON_FOG] = 0;
            FlightCount = 0;
            BreathCount = 0;
            break;
        default:
            break;
        }
        Phase = NextPhase;
    }

    void HandleFlightSequence()
    {
        switch(FlightCount)
        {
        case 0:
            m_creature->SetUInt64Value(UNIT_FIELD_TARGET, 0);
            m_creature->GetMotionMaster()->Clear(false);
            m_creature->HandleEmoteCommand(EMOTE_ONESHOT_LIFTOFF);
            m_creature->SetHover(true);
            m_creature->StopMoving();
            m_creature->SendMovementFlagUpdate();
            DoScriptText(YELL_TAKEOFF, m_creature);
            Timer[EVENT_FLIGHT_SEQUENCE] = 2000;
            break;
        case 1:
            m_creature->SetUInt64Value(UNIT_FIELD_TARGET, 0);
            m_creature->GetMotionMaster()->MovePoint(0, m_creature->GetPositionX()+1, m_creature->GetPositionY(), m_creature->GetPositionZ()+10);
            Timer[EVENT_FLIGHT_SEQUENCE] = 0;
            break;
        case 2:{
            m_creature->SetUInt64Value(UNIT_FIELD_TARGET, 0);
            Unit* target;
            target = SelectUnit(SELECT_TARGET_RANDOM, 0, 150.0f, true);
            if(!target) target = Unit::GetUnit((*m_creature), pInstance->GetData64(DATA_PLAYER_GUID));
            if(target)
            {
                Creature* Vapor = m_creature->SummonCreature(MOB_VAPOR, target->GetPositionX()-5+rand()%10, target->GetPositionY()-5+rand()%10, target->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 9000);
                if(Vapor)
                {
                    Vapor->AI()->AttackStart(target);
                    m_creature->InterruptNonMeleeSpells(false);
                    m_creature->CastSpell(Vapor, SPELL_VAPOR_CHANNEL, false); // core bug
                    Vapor->CastSpell(Vapor, SPELL_VAPOR_TRIGGER, true);
                }
            }
            else
            {
                EnterEvadeMode();
                return;
            }
            Timer[EVENT_FLIGHT_SEQUENCE] = 10000;
            break;}
        case 3: {
            m_creature->SetUInt64Value(UNIT_FIELD_TARGET, 0);
            DespawnSummons(MOB_VAPOR_TRAIL);
            Unit* target;
            target = SelectUnit(SELECT_TARGET_RANDOM, 0, 150, true);
            if(!target) target = Unit::GetUnit((*m_creature), pInstance->GetData64(DATA_PLAYER_GUID));
            if(target)
            {
                Creature* Vapor = m_creature->SummonCreature(MOB_VAPOR, target->GetPositionX()-5+rand()%10, target->GetPositionY()-5+rand()%10, target->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN, 9000);
                if(Vapor)
                {
                    Vapor->AI()->AttackStart(target);
                    m_creature->InterruptNonMeleeSpells(false);
                    m_creature->CastSpell(Vapor, SPELL_VAPOR_CHANNEL, false); // core bug
                    Vapor->CastSpell(Vapor, SPELL_VAPOR_TRIGGER, true);
                }
            }
            else
            {
                EnterEvadeMode();
                return;
            }
            Timer[EVENT_FLIGHT_SEQUENCE] = 10000;
            break;}
        case 4:
            m_creature->SetUInt64Value(UNIT_FIELD_TARGET, 0);
            DespawnSummons(MOB_VAPOR_TRAIL);
            Timer[EVENT_FLIGHT_SEQUENCE] = 1;
            break;
        case 5:
        {
            m_creature->SetUInt64Value(UNIT_FIELD_TARGET, 0);
            if (BreathCount == 0) {
                switch (rand()%2) {
                    case 0:
                        goingLeft = true;
                        break;
                    case 1:
                        goingLeft = false;
                        break;
                }
            }
            m_creature->RemoveAurasDueToSpell(AURA_NOXIOUS_FUMES);
            if (!goingLeft)     // Right
                m_creature->GetMotionMaster()->MovePoint(0, flightMobRight[0], flightMobRight[1], flightMobRight[2]);
            else                      // Left
                m_creature->GetMotionMaster()->MovePoint(0, flightMobLeft[0], flightMobLeft[1], flightMobLeft[2]);
            Timer[EVENT_FLIGHT_SEQUENCE] = 28000;
            break;
        }
        case 6:
        {
            randomPoint = rand()%3;
            if (!goingLeft)     // Right
                m_creature->GetMotionMaster()->MovePoint(1, rights[randomPoint][0], rights[randomPoint][1], rights[randomPoint][2]-10);
            else
                m_creature->GetMotionMaster()->MovePoint(1, lefts[randomPoint][0], lefts[randomPoint][1], lefts[randomPoint][2]-10);
//            me->SendMovementFlagUpdate(300.f);
            Timer[EVENT_FLIGHT_SEQUENCE] = 28000;
            break;
        }
        case 7:
            m_creature->SetUInt64Value(UNIT_FIELD_TARGET, 0);
            if (!goingLeft) {      // Right
                m_creature->SetOrientation(m_creature->GetAngle(lefts[randomPoint][0], lefts[randomPoint][1]));
                m_creature->GetMotionMaster()->MovePoint(0, m_creature->GetPositionX() - 0.5, m_creature->GetPositionY() + 1, m_creature->GetPositionZ());
            } else {
                m_creature->SetOrientation(m_creature->GetAngle(rights[randomPoint][0], rights[randomPoint][1]));
                m_creature->GetMotionMaster()->MovePoint(0, m_creature->GetPositionX() + 0.5, m_creature->GetPositionY() - 1, m_creature->GetPositionZ());
            }
            //m_creature->StopMoving();
            //me->SendMovementFlagUpdate(300.f);
            DoScriptText(EMOTE_DEEP_BREATH, m_creature);
            Timer[EVENT_FLIGHT_SEQUENCE] = 2500;
            break;
        case 8:
            m_creature->SetUInt64Value(UNIT_FIELD_TARGET, 0);
            m_creature->CastSpell(m_creature, SPELL_FOG_BREATH, true);
            {
                if (!goingLeft)   // Right
                    m_creature->GetMotionMaster()->MovePoint(2, lefts[randomPoint][0], lefts[randomPoint][1], lefts[randomPoint][2]-10);
                else
                    m_creature->GetMotionMaster()->MovePoint(2, rights[randomPoint][0], rights[randomPoint][1], rights[randomPoint][2]-10);
            }
            Timer[EVENT_SUMMON_FOG] = 1;
            Timer[EVENT_FLIGHT_SEQUENCE] = 0;
            break;
        case 9:
            m_creature->SetUInt64Value(UNIT_FIELD_TARGET, 0);
            m_creature->RemoveAurasDueToSpell(SPELL_FOG_BREATH);
            BreathCount++;
            Timer[EVENT_SUMMON_FOG] = 0;
            Timer[EVENT_FLIGHT_SEQUENCE] = 1;
            goingLeft = !goingLeft;
            if(BreathCount < 3) FlightCount = 4;
            break;
        case 10:
            m_creature->CastSpell(m_creature, AURA_NOXIOUS_FUMES, true);
            if(Unit* target = SelectUnit(SELECT_TARGET_TOPAGGRO, 0))
            {
                m_creature->SetUInt64Value(UNIT_FIELD_TARGET, target->GetGUID());
                float x, y, z;
                target->GetContactPoint(m_creature, x, y, z);
                if (goingLeft)
                    m_creature->GetMotionMaster()->MovePoint(4, 1482.709961, 649.406006, 21.081100);    // Left landing point
                else
                    m_creature->GetMotionMaster()->MovePoint(4, 1491.119995, 553.672974, 24.921900);    // Right landing point
            }
            else
            {
                EnterEvadeMode();
                return;
            }
            Timer[EVENT_FLIGHT_SEQUENCE] = 0;
            break;
        case 11:
        {
            m_creature->SetHover(false);
            m_creature->StopMoving();
            m_creature->HandleEmoteCommand(EMOTE_ONESHOT_LAND);
            m_creature->SendMovementFlagUpdate();
            EnterPhase(PHASE_GROUND);
            Unit *target = SelectUnit(SELECT_TARGET_TOPAGGRO, 0);
            m_creature->AI()->AttackStart(target);
            m_creature->GetMotionMaster()->MoveChase(target);
            break;
        }
        default:
            break;
        }
        FlightCount++;
    }
    
    void DeleteFromThreatList(uint64 TargetGUID)
    {
        for(std::list<HostilReference*>::iterator itr = m_creature->getThreatManager().getThreatList().begin(); itr != m_creature->getThreatManager().getThreatList().end(); ++itr)
        {
            if((*itr)->getUnitGuid() == TargetGUID)
            {
                (*itr)->removeReference();
                break;
            }
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (justBorn) {
            if (IntroTimer <= diff) {
                switch (IntroPhase) {
                case 1:
                    me->SetStandState(UNIT_STAND_STATE_STAND);
                    DoScriptText(YELL_BIRTH, me);
                    IntroTimer = 4000;
                    IntroPhase++;
                    break;
                case 2:
                    me->GetMotionMaster()->Clear(false);
                    me->HandleEmoteCommand(EMOTE_ONESHOT_LIFTOFF);
                    me->SetHover(true);
                    me->SendMovementFlagUpdate();
                    IntroTimer = 500;
                    IntroPhase++;
                    break;
                case 3:
                    me->GetMotionMaster()->MovePoint(0, 1464.726440, 606.836243, 72.818344, false);
                    me->SetHomePosition(1464.726440, 606.836243, 72.818344, 0);
                    m_creature->SetSpeed(MOVE_FLIGHT, 1.3f, true);
                    IntroTimer = 10000;
                    IntroPhase++;
                    break;
                case 4:
                    me->GetMotionMaster()->MovePath(25038, true);
                    IntroTimer = 0;
                    IntroPhase++;
                    me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                    me->SetReactState(REACT_AGGRESSIVE);
                    justBorn = false;
                }
            }
            else
                IntroTimer -= diff;

            return;
        }

        if (!UpdateVictim() && Phase != PHASE_FLIGHT)
            return;
            
        m_creature->addUnitState(UNIT_STAT_IGNORE_PATHFINDING);
        
        if (jumpFlagTimer <= diff) {
            Map::PlayerList const& players = pInstance->instance->GetPlayers();
            if (!players.isEmpty()) {
                for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr) {
                    if (Player* plr = itr->getSource())
                        plr->RemoveUnitMovementFlag(MOVEMENTFLAG_JUMPING);
                }
            }

            jumpFlagTimer = 1000;
        }
        else
            jumpFlagTimer -= diff;

        Event = EVENT_NULL;
        for(uint32 i = 1; i <= MaxTimer[Phase]; i++) {
            if(Timer[i]) {
                if(Timer[i] <= diff) {
                    if(!Event)
                        Event = (EventFelmyst)i;
                }
                else Timer[i] -= diff;
            }
        }
        
        if (m_creature->getVictim() && m_creature->getVictim()->HasAura(SPELL_FOG_CHARM))
            DeleteFromThreatList(m_creature->getVictim()->GetGUID());

        if(m_creature->IsNonMeleeSpellCasted(false))
            return;
            
        if (justPulled && m_creature->IsWithinMeleeRange(m_creature->getVictim())) {
            justPulled = false;
            m_creature->HandleEmoteCommand(EMOTE_ONESHOT_LAND);
            m_creature->SetHover(false);
            m_creature->SendMovementFlagUpdate();
            float x, y, z;
            m_creature->GetPosition(x, y, z);
            m_creature->UpdateGroundPositionZ(x, y, z);
            m_creature->Relocate(x, y, z);
            
            WorldPacket data;                       //send update position to client
            m_creature->BuildHeartBeatMsg(&data);
            m_creature->SendMessageToSet(&data,true);
        }

        if (Phase == PHASE_GROUND) {
            switch(Event)
            {
            case EVENT_BERSERK:
                DoScriptText(YELL_BERSERK, m_creature);
                m_creature->CastSpell(m_creature, SPELL_BERSERK, true);
                Timer[EVENT_BERSERK] = 10000;
                break;
            case EVENT_CLEAVE:
                m_creature->CastSpell(m_creature->getVictim(), SPELL_CLEAVE, false);
                Timer[EVENT_CLEAVE] = 5000 + rand()%5 * 1000;
                break;
            case EVENT_CORROSION:
                m_creature->CastSpell(m_creature->getVictim(), SPELL_CORROSION, false);
                Timer[EVENT_CORROSION] = 20000 + rand()%10 * 1000;
                break;
            case EVENT_GAS_NOVA:
                m_creature->CastSpell(m_creature, SPELL_GAS_NOVA, false);
                Timer[EVENT_GAS_NOVA] = 20000 + rand()%5 * 1000;
                break;
            case EVENT_ENCAPSULATE:
                if(encapsTarget) {
                    m_creature->CastSpell(encapsTarget, SPELL_ENCAPSULATE_CHANNEL, false);
                    Timer[EVENT_ENCAPSULATE] = 25000 + rand()%5 * 1000;
                    Timer[EVENT_ENCAPS_WARN] = Timer[EVENT_ENCAPSULATE] - 1000;
                    /*if (Unit* topTarget = SelectUnit(SELECT_TARGET_TOPAGGRO, 0))
                        m_creature->SetUInt64Value(UNIT_FIELD_TARGET, topTarget->GetGUID());*/
                }
                break;
            case EVENT_ENCAPS_WARN:
                if (Timer[EVENT_FLIGHT] < 8000) {
                    Timer[EVENT_ENCAPS_WARN] = 30000;
                    Timer[EVENT_ENCAPSULATE] = 30000;
                    break;
                }
                if (encapsTarget = SelectUnit(SELECT_TARGET_RANDOM, 0, 150.0f, true))
                    m_creature->SetUInt64Value(UNIT_FIELD_TARGET, encapsTarget->GetGUID());
                if (!encapsTarget)
                    break;
                while (encapsTarget->isDead()) {
                    encapsTarget = SelectUnit(SELECT_TARGET_RANDOM, 0, 150.0f, true);
                    if (!encapsTarget)
                        break;
                    m_creature->SetUInt64Value(UNIT_FIELD_TARGET, encapsTarget->GetGUID());
                }
                Timer[EVENT_ENCAPS_WARN] = 20000;
                break;
            case EVENT_FLIGHT:
                EnterPhase(PHASE_FLIGHT);
                break;
            default:
                DoMeleeAttackIfReady();
                break;
            }
        }

        if (Phase == PHASE_FLIGHT) {
            switch(Event) {
            case EVENT_BERSERK:
                DoScriptText(YELL_BERSERK, m_creature);
                m_creature->CastSpell(m_creature, SPELL_BERSERK, true);
                Timer[EVENT_BERSERK] = 0;
                break;
            case EVENT_FLIGHT_SEQUENCE:
                HandleFlightSequence();
                break;
            case EVENT_SUMMON_FOG:
                {
                    if (pInstance) {
                        switch (randomPoint) {
                        case 0: pInstance->SetData((goingLeft ? DATA_ACTIVATE_NORTH_TO_LEFT : DATA_ACTIVATE_NORTH_TO_RIGHT), (uint32) me->GetPositionY()); break;
                        case 1: pInstance->SetData((goingLeft ? DATA_ACTIVATE_CENTER_TO_LEFT : DATA_ACTIVATE_CENTER_TO_RIGHT), (uint32) me->GetPositionY()); break;
                        case 2: pInstance->SetData((goingLeft ? DATA_ACTIVATE_SOUTH_TO_LEFT : DATA_ACTIVATE_SOUTH_TO_RIGHT), (uint32) me->GetPositionY()); break;
                        }
                    }
                    float x, y, z;
                    m_creature->GetPosition(x, y, z);
                    m_creature->UpdateGroundPositionZ(x, y, z);
                    if(Creature *Fog = m_creature->SummonCreature(MOB_VAPOR_TRAIL, x, y, z, 0, TEMPSUMMON_TIMED_DESPAWN, 10000))
                    {
                        Fog->RemoveAurasDueToSpell(SPELL_TRAIL_TRIGGER);
                        Fog->CastSpell(Fog, SPELL_FOG_TRIGGER, true);
                        /*if (Creature* trigger = Fog->FindNearestCreature(23472, 10.0f))
                            trigger->CastSpell(trigger, SPELL_FOG_TRIGGER, true);*/
                    }
                }
                Timer[EVENT_SUMMON_FOG] = 500;
                break;
            default:
                break;
            }
        }
    }
    
    void OnSpellFinish(Unit* caster, uint32 spellId, Unit* target, bool ok)
    {
        if (spellId == SPELL_ENCAPSULATE_CHANNEL) {
            if (Unit* topTarget = SelectUnit(SELECT_TARGET_TOPAGGRO, 0))
                m_creature->SetUInt64Value(UNIT_FIELD_TARGET, topTarget->GetGUID());
        }
        
        Map::PlayerList const& players = pInstance->instance->GetPlayers();
        if (!players.isEmpty()) {
            for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr) {
                if (Player* plr = itr->getSource())
                    plr->RemoveUnitMovementFlag(MOVEMENTFLAG_JUMPING);
            }
        }
    }

    void DespawnSummons(uint32 entry)
    {
        std::list<Creature*> templist;
        float x, y, z;
        m_creature->GetPosition(x, y, z);

        {
            CellPair pair(Trinity::ComputeCellPair(x, y));
            Cell cell(pair);
            cell.data.Part.reserved = ALL_DISTRICT;
            cell.SetNoCreate();

            Trinity::AllCreaturesOfEntryInRange check(m_creature, entry, 100);
            Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(templist, check);

            TypeContainerVisitor<Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange>, GridTypeMapContainer> cSearcher(searcher);

            cell.Visit(pair, cSearcher, *(m_creature->GetMap()));
        }

        for(std::list<Creature*>::iterator i = templist.begin(); i != templist.end(); ++i)
        {
            if(entry == MOB_VAPOR_TRAIL && Phase == PHASE_FLIGHT)
            {
                float x, y, z;
                (*i)->GetPosition(x, y, z);
                m_creature->SummonCreature(MOB_DEAD, x, y, z, 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000);
            }
            (*i)->SetVisibility(VISIBILITY_OFF);
            (*i)->setDeathState(JUST_DIED);
            if((*i)->getDeathState() == CORPSE)
                (*i)->RemoveCorpse();
        }
    }
};

struct mob_felmyst_vaporAI : public ScriptedAI
{
    mob_felmyst_vaporAI(Creature *c) : ScriptedAI(c)
    {
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        m_creature->SetSpeed(MOVE_RUN, 1.0f);
    }
    void Reset() {}
    void Aggro(Unit* who)
    {
        DoZoneInCombat();
        //m_creature->CastSpell(m_creature, SPELL_VAPOR_FORCE, true); core bug
    }
    void UpdateAI(const uint32 diff)
    {
        if(!m_creature->getVictim())
            AttackStart(SelectUnit(SELECT_TARGET_RANDOM, 0));
        
        if (ScriptedInstance* instance = ((ScriptedInstance*)me->GetInstanceData())) {
            if (instance->GetData(DATA_FELMYST_EVENT) != IN_PROGRESS)
                me->DisappearAndDie();
        }
    }
};

struct mob_felmyst_trailAI : public ScriptedAI
{
    mob_felmyst_trailAI(Creature *c) : ScriptedAI(c)
    {
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        m_creature->CastSpell(m_creature, SPELL_TRAIL_TRIGGER, true);
        m_creature->SetUInt64Value(UNIT_FIELD_TARGET, m_creature->GetGUID());
        m_creature->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, 0.01); // core bug
    }
    void Reset() {}
    void Aggro(Unit* who) {}
    void AttackStart(Unit* who) {}
    void MoveInLineOfSight(Unit* who) {}
    void UpdateAI(const uint32 diff) {}
};

CreatureAI* GetAI_boss_felmyst(Creature *_Creature)
{
    return new boss_felmystAI(_Creature);
}

CreatureAI* GetAI_mob_felmyst_vapor(Creature *_Creature)
{
    return new mob_felmyst_vaporAI(_Creature);
}

CreatureAI* GetAI_mob_felmyst_trail(Creature *_Creature)
{
    return new mob_felmyst_trailAI(_Creature);
}

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
    newscript = new Script;
    newscript->Name="boss_felmyst";
    newscript->GetAI = &GetAI_boss_felmyst;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="mob_felmyst_vapor";
    newscript->GetAI = &GetAI_mob_felmyst_vapor;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="mob_felmyst_trail";
    newscript->GetAI = &GetAI_mob_felmyst_trail;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name="trigger_felmyst_fog";
    newscript->GetAI = &GetAI_trigger_felmyst_fog;
    newscript->RegisterSelf();
}
