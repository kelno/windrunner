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
SD%Complete: 0
SDComment:
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
    YELL_KALECGOS   =       -1580043 //after felmyst's death spawned and say this
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
    EVENT_ENCAPSULATE   =   5,
    EVENT_FLIGHT        =   6,

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

struct boss_felmystAI : public ScriptedAI
{
    boss_felmystAI(Creature *c) : ScriptedAI(c){
        pInstance = ((ScriptedInstance*)c->GetInstanceData());

        // wait for core patch be accepted
        /*SpellEntry *TempSpell = (SpellEntry*)GetSpellStore()->LookupEntry(SPELL_ENCAPSULATE_EFFECT);
        if(TempSpell->SpellIconID == 2294)
            TempSpell->SpellIconID = 2295;
        TempSpell = (SpellEntry*)GetSpellStore()->LookupEntry(SPELL_VAPOR_TRIGGER);
        if((TempSpell->Attributes & SPELL_ATTR_PASSIVE) == 0)
            TempSpell->Attributes |= SPELL_ATTR_PASSIVE;
        TempSpell = (SpellEntry*)GetSpellStore()->LookupEntry(SPELL_FOG_CHARM2);
        if((TempSpell->Attributes & SPELL_ATTR_PASSIVE) == 0)
            TempSpell->Attributes |= SPELL_ATTR_PASSIVE;*/
    }

    ScriptedInstance *pInstance;
    PhaseFelmyst Phase;
    EventFelmyst Event;
    uint32 Timer[EVENT_FLIGHT + 1];

    uint32 FlightCount;
    uint32 BreathCount;

    float BreathX, BreathY;
    
    bool justPulled;

    void Reset()
    {
        Phase = PHASE_NULL;
        Event = EVENT_NULL;
        Timer[EVENT_BERSERK] = 600000;
        FlightCount = 0;
        
        justPulled = false;

        m_creature->AddUnitMovementFlag(MOVEMENTFLAG_LEVITATING + MOVEMENTFLAG_ONTRANSPORT);
        m_creature->SetFloatValue(UNIT_FIELD_BOUNDINGRADIUS, 10);
        m_creature->SetFloatValue(UNIT_FIELD_COMBATREACH, 10);

        DespawnSummons(MOB_VAPOR_TRAIL);
        m_creature->setActive(false);
        
        m_creature->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
        m_creature->addFullTauntImmunity();

        if(pInstance)
            pInstance->SetData(DATA_FELMYST_EVENT, NOT_STARTED);
            
        WorldPacket data;                       //send update position to client
        m_creature->BuildHeartBeatMsg(&data);
        m_creature->SendMessageToSet(&data,true);
        
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
    }

    void Aggro(Unit *who)
    {
        m_creature->setActive(true);
        DoZoneInCombat();
        m_creature->CastSpell(m_creature, AURA_SUNWELL_RADIANCE, true);
        m_creature->CastSpell(m_creature, AURA_NOXIOUS_FUMES, true);
        EnterPhase(PHASE_GROUND);
        justPulled = true;

        if(pInstance)
            pInstance->SetData(DATA_FELMYST_EVENT, IN_PROGRESS);
    }

    void AttackStart(Unit *who)
    {
        if(Phase != PHASE_FLIGHT)
            ScriptedAI::AttackStart(who);
    }

    void MoveInLineOfSight(Unit *who)
    {
        if(Phase != PHASE_FLIGHT)
            ScriptedAI::MoveInLineOfSight(who);
    }

    void KilledUnit(Unit* victim)
    {
        DoScriptText(RAND(YELL_KILL1, YELL_KILL2), m_creature);
    }

    void JustRespawned()
    {
        DoScriptText(YELL_BIRTH, m_creature);
    }

    void JustDied(Unit* Killer)
    {
        DoScriptText(YELL_DEATH, m_creature);

        if(pInstance)
            pInstance->SetData(DATA_FELMYST_EVENT, DONE);
    }

    void SpellHit(Unit *caster, const SpellEntry *spell)
    {
        // workaround for linked aura
        /*if(spell->Id == SPELL_VAPOR_FORCE)
        {
            caster->CastSpell(caster, SPELL_VAPOR_TRIGGER, true);
        }*/
        // workaround for mind control
        if(spell->Id == SPELL_FOG_INFORM)
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
        Timer[EVENT_FLIGHT_SEQUENCE] = 1;
    }

    void DamageTaken(Unit*, uint32 &damage)
    {
        if(Phase != PHASE_GROUND && damage >= m_creature->GetHealth())
            damage = 0;
    }

    void EnterPhase(PhaseFelmyst NextPhase)
    {
        switch(NextPhase)
        {
        case PHASE_GROUND:
            Timer[EVENT_CLEAVE] = 5000 + rand()%5 * 1000;
            Timer[EVENT_CORROSION] = 10000 + rand()%10 * 1000;
            Timer[EVENT_GAS_NOVA] = 15000 + rand()%5 * 1000;
            Timer[EVENT_ENCAPSULATE] = 20000 + rand()%5 * 1000;
            Timer[EVENT_FLIGHT] = 60000;
            break;
        case PHASE_FLIGHT:
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
            //m_creature->AttackStop();
            //error_log("prevent fly phase");
            m_creature->GetMotionMaster()->Clear(false);
            m_creature->HandleEmoteCommand(EMOTE_ONESHOT_LIFTOFF);
            m_creature->SetUnitMovementFlags(MOVEMENTFLAG_LEVITATING + MOVEMENTFLAG_ONTRANSPORT);
            m_creature->StopMoving();
            DoScriptText(YELL_TAKEOFF, m_creature);
            Timer[EVENT_FLIGHT_SEQUENCE] = 2000;
            break;
        case 1:
            m_creature->SetUInt64Value(UNIT_FIELD_TARGET, 0);
            //error_log("Move to Fly point");
            m_creature->GetMotionMaster()->MovePoint(0, m_creature->GetPositionX()+1, m_creature->GetPositionY(), m_creature->GetPositionZ()+10);
            Timer[EVENT_FLIGHT_SEQUENCE] = 0;
            break;
        case 2:{
            m_creature->SetUInt64Value(UNIT_FIELD_TARGET, 0);
            //error_log("Summon Vapor case 2");
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
            //error_log("Summon Vapor case3");
            //m_creature->CastSpell(m_creature, SPELL_VAPOR_SELECT); need core support
            Unit* target;
            target = SelectUnit(SELECT_TARGET_RANDOM, 0, 150, true);
            if(!target) target = Unit::GetUnit((*m_creature), pInstance->GetData64(DATA_PLAYER_GUID));
            if(target)
            {
                //target->CastSpell(target, SPELL_VAPOR_SUMMON, true); need core support
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
        case 5:{
            m_creature->SetUInt64Value(UNIT_FIELD_TARGET, 0);
            Unit* target;
            target = SelectUnit(SELECT_TARGET_RANDOM, 0, 150, true);
            if(!target) target = Unit::GetUnit((*m_creature), pInstance->GetData64(DATA_PLAYER_GUID));
            if(target)
            {
                BreathX = target->GetPositionX();
                BreathY = target->GetPositionY();
                float x, y, z;
                target->GetContactPoint(m_creature, x, y, z, 70);
                m_creature->GetMotionMaster()->MovePoint(0, x, y, z+10);
            }else
            {
                EnterEvadeMode();
                return;
            }
            Timer[EVENT_FLIGHT_SEQUENCE] = 0;
            break;}
        case 6:
            m_creature->SetUInt64Value(UNIT_FIELD_TARGET, 0);
            m_creature->SetOrientation(m_creature->GetAngle(BreathX, BreathY));
            m_creature->StopMoving();
            DoTextEmote("prend une profonde respiration.", NULL);
            Timer[EVENT_FLIGHT_SEQUENCE] = 10000;
            break;
        case 7:
            m_creature->SetUInt64Value(UNIT_FIELD_TARGET, 0);
            m_creature->CastSpell(m_creature, SPELL_FOG_BREATH, true);
            {
                float x, y, z;
                m_creature->GetPosition(x, y, z);
                x = 2 * BreathX - x;
                y = 2 * BreathY - y;
                m_creature->GetMotionMaster()->MovePoint(0, x, y, z);
            }
            Timer[EVENT_SUMMON_FOG] = 1;
            Timer[EVENT_FLIGHT_SEQUENCE] = 0;
            break;
        case 8:
            m_creature->SetUInt64Value(UNIT_FIELD_TARGET, 0);
            m_creature->RemoveAurasDueToSpell(SPELL_FOG_BREATH);
            BreathCount++;
            Timer[EVENT_SUMMON_FOG] = 0;
            Timer[EVENT_FLIGHT_SEQUENCE] = 1;
            if(BreathCount < 3) FlightCount = 4;
            break;
        case 9:
            if(Unit* target = SelectUnit(SELECT_TARGET_TOPAGGRO, 0))
            {
                m_creature->SetUInt64Value(UNIT_FIELD_TARGET, target->GetGUID());
                float x, y, z;
                target->GetContactPoint(m_creature, x, y, z);
                m_creature->GetMotionMaster()->MovePoint(0, x, y, z);
            }
            else
            {
                EnterEvadeMode();
                return;
            }
            Timer[EVENT_FLIGHT_SEQUENCE] = 0;
            break;
        case 10:
        {
            m_creature->RemoveUnitMovementFlag(MOVEMENTFLAG_LEVITATING + MOVEMENTFLAG_ONTRANSPORT);
            m_creature->StopMoving();
            m_creature->HandleEmoteCommand(EMOTE_ONESHOT_LAND);
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

    void UpdateAI(const uint32 diff)
    {
        if (!UpdateVictim()) {
            if(Phase == PHASE_FLIGHT && !m_creature->IsInEvadeMode())
                EnterEvadeMode();
            return;
        }

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

        if(m_creature->IsNonMeleeSpellCasted(false))
            return;
            
        if (justPulled && m_creature->IsWithinMeleeRange(m_creature->getVictim())) {
            justPulled = false;
            //m_creature->HandleEmoteCommand(EMOTE_ONESHOT_LAND);
            m_creature->RemoveUnitMovementFlag(MOVEMENTFLAG_LEVITATING + MOVEMENTFLAG_ONTRANSPORT);
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
                if(Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0, 150.0f, true)) {
                    m_creature->CastSpell(target, SPELL_ENCAPSULATE_CHANNEL, false);
                    Timer[EVENT_ENCAPSULATE] = 25000 + rand()%5 * 1000;
                }
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
                    float x, y, z;
                    m_creature->GetPosition(x, y, z);
                    m_creature->UpdateGroundPositionZ(x, y, z);
                    if(Creature *Fog = m_creature->SummonCreature(MOB_VAPOR_TRAIL, x, y, z, 0, TEMPSUMMON_TIMED_DESPAWN, 10000))
                    {
                        Fog->RemoveAurasDueToSpell(SPELL_TRAIL_TRIGGER);
                        Fog->CastSpell(Fog, SPELL_FOG_TRIGGER, true);
                    }
                }
                Timer[EVENT_SUMMON_FOG] = 1000;
                break;
            default:
                break;
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
}
