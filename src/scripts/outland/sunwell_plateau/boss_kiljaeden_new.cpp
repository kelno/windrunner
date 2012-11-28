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
#include <math.h>

/*** Spells used during the encounter ***/
enum SpellIds
{
    /* Hand of the Deceiver's spells and cosmetics */
    SPELL_SHADOW_BOLT_VOLLEY                            = 45770, // ~30 yard range Shadow Bolt Volley for ~2k(?) damage
    SPELL_SHADOW_INFUSION                               = 45772, // They gain this at 20% - Immunity to Stun/Silence and makes them look angry!
    SPELL_FELFIRE_PORTAL                                = 46875, // Creates a portal that spawns Felfire Fiends (LIVE FOR THE SWARM!1 FOR THE OVERMIND!)
    SPELL_SHADOW_CHANNELING                             = 46757, // Channeling animation out of combat
    /* Felfire Portal */
    SPELL_SUMMON_FELFIRE_FIEND                          = 46464,
    
    /* Volatile Felfire Fiend's spells */
    SPELL_FELFIRE_FISSION                               = 45779, // Felfire Fiends explode when they die or get close to target.

    /* Kil'Jaeden's spells and cosmetics */
    SPELL_TRANS                                         = 23188, // Surprisingly, this seems to be the right spell.. (Where is it used?)
    SPELL_REBIRTH                                       = 44200, // Emerge from the Sunwell
    SPELL_SOUL_FLAY                                     = 45442, // 9k Shadow damage over 3 seconds. Spammed throughout all the fight.
    SPELL_SOUL_FLAY_SLOW                                = 47106,
    SPELL_LEGION_LIGHTNING                              = 45664, // Chain Lightning, 4 targets, ~3k Shadow damage, 1.5k mana burn
    SPELL_FIRE_BLOOM                                    = 45641, // Places a debuff on 5 raid members, which causes them to deal 2k Fire damage to nearby allies and selves. MIGHT NOT WORK

    SPELL_SINISTER_REFLECTION                           = 45785, // Summon shadow copies of 5 raid members that fight against KJ's enemies
    SPELL_COPY_WEAPON                                   = 41055, // }
    SPELL_COPY_WEAPON2                                  = 41054, // }
    SPELL_COPY_OFFHAND                                  = 45206, // }- Spells used in Sinister Reflection creation
    SPELL_COPY_OFFHAND_WEAPON                           = 45205, // }

    SPELL_SHADOW_SPIKE                                  = 46680, // Bombard random raid members with Shadow Spikes (Very similar to Void Reaver orbs)
    SPELL_FLAME_DART                                    = 45737, // Bombards the raid with flames every 3(?) seconds
    SPELL_DARKNESS_OF_A_THOUSAND_SOULS                  = 46605, // Begins a 8-second channeling, after which he will deal 50'000 damage to the raid
    SPELL_DARKNESS_OF_A_THOUSAND_SOULS_DAMAGE           = 45657,

    /* Armageddon spells wrong visual */
    SPELL_ARMAGEDDON_TRIGGER                            = 45909, // Meteor spell trigger missile should cast creature on himself
    SPELL_ARMAGEDDON_VISUAL                             = 45911, // Does the hellfire visual to indicate where the meteor missle lands
    SPELL_ARMAGEDDON_VISUAL2                            = 45914, // Does the light visual to indicate where the meteor missle lands
    SPELL_ARMAGEDDON_VISUAL3                            = 24207, // This shouldn't correct but same as seen on the movie
    SPELL_ARMAGEDDON_SUMMON_TRIGGER                     = 45921, // Summons the triggers that cast the spells on himself need random target select
    SPELL_ARMAGEDDON_DAMAGE                             = 45915, // This does the area damage

    /* Shield Orb Spells*/
    SPELL_SHADOW_BOLT                                   = 45680, //45679 would be correct but triggers to often //TODO fix console error


    /* Anveena's spells and cosmetics (Or, generally, everything that has "Anveena" in name) */
    SPELL_ANVEENA_PRISON                                = 46367, // She hovers locked within a bubble
    SPELL_ANVEENA_ENERGY_DRAIN                          = 46410, // Sunwell energy glow animation (Control mob uses this)
    SPELL_SACRIFICE_OF_ANVEENA                          = 46474, // This is cast on Kil'Jaeden when Anveena sacrifices herself into the Sunwell

    /* Sinister Reflection Spells */
    SPELL_SR_CURSE_OF_AGONY                             = 46190,
    SPELL_SR_SHADOW_BOLT                                = 47076,

    SPELL_SR_EARTH_SHOCK                                = 47071,

    SPELL_SR_FIREBALL                                   = 47074,

    SPELL_SR_HEMORRHAGE                                 = 45897,

    SPELL_SR_HOLY_SHOCK                                 = 38921,
    SPELL_SR_HAMMER_OF_JUSTICE                          = 37369,

    SPELL_SR_HOLY_SMITE                                 = 47077,
    SPELL_SR_RENEW                                      = 47079,

    SPELL_SR_SHOOT                                      = 16496,
    SPELL_SR_MULTI_SHOT                                 = 48098,
    SPELL_SR_WING_CLIP                                  = 40652,

    SPELL_SR_WHIRLWIND                                  = 17207,

    SPELL_SR_MOONFIRE                                   = 47072,

    /*** Other Spells (used by players, etc) ***/
    SPELL_VENGEANCE_OF_THE_BLUE_FLIGHT                  = 45839, // Possess the blue dragon from the orb to help the raid.
    SPELL_ENTROPIUS_BODY                                = 46819, // Visual for Entropius at the Epilogue
    SPELL_RING_OF_BLUE_FLAMES                           = 45825  //Cast this spell when the go is activated
};

/*** GameObjects ***/
#define GAMEOBJECT_ORB_OF_THE_BLUE_DRAGONFLIGHT 188415

/*** Others ***/
#define FLOOR_Z 28.050388
#define SHIELD_ORB_Z 45.000

enum Phase
{
    PHASE_DECEIVERS     = 1, // Fight 3 adds
    PHASE_NORMAL        = 2, // Kil'Jaeden emerges from the sunwell
    PHASE_DARKNESS      = 3, // At 85%, he gains few abilities; Kalecgos joins the fight
    PHASE_ARMAGEDDON    = 4, // At 55%, he gains even more abilities
    PHASE_SACRIFICE     = 5, // At 25%, Anveena sacrifices herself into the Sunwell; at this point he becomes enraged and has *significally* shorter cooldowns.
};

//Events
enum KilJaedenEvents
{
    EVENT_KALEC_JOIN            = 0,

    //Phase 2
    EVENT_SOUL_FLAY             = 1,
    EVENT_LEGION_LIGHTNING      = 2,
    EVENT_FIRE_BLOOM            = 3,
    EVENT_SUMMON_SHILEDORB      = 4,

    //Phase 3
    EVENT_SHADOW_SPIKE          = 5,
    EVENT_FLAME_DART            = 6,
    EVENT_DARKNESS              = 7,
    EVENT_ORBS_EMPOWER          = 8,

    //Phase 4
    EVENT_ARMAGEDDON            = 9
};

enum KilJaedenControllerEvents
{
    EVENT_SAY                   = 0
};

enum HandOfTheDeceiverEvents
{
    EVENT_SHADOWBOLT            = 0,
    EVENT_FELFIRE               = 1
};

enum FelfirePortal
{
    EVENT_SPAWNFIEND            = 0
};

enum FelfireFiend
{
    EVENT_STUN                  = 0,
    EVENT_EXPLODE               = 1,
    EVENT_DIE_F                 = 2
};

enum Armageddontarget
{
    EVENT_VISUAL1               = 0,
    EVENT_VISUAL2               = 1,
    EVENT_TRIGGER               = 2,
    EVENT_DIE                   = 3
};

enum ShieldOrb
{
    EVENT_SHADOWBOLT_S          = 0
};

// Locations of the Hand of Deceiver adds
float DeceiverLocations[3][3]=
{
    {1682.045, 631.299, 5.936},
    {1684.099, 618.848, 0.589},
    {1694.170, 612.272, 1.416},
};

// Locations, where Shield Orbs will spawn
float ShieldOrbLocations[4][2]=
{
    {1698.900, 627.870},    //middle pont of Sunwell
    {12, 3.14},             // First one spawns northeast of KJ
    {12, 3.14/0.7},         // Second one spawns southeast
    {12, 3.14*3.8}          // Third one spawns (?)
};

float OrbLocations[4][5] = {
    (1694.48, 674.29,  28.0502, 4.86985),
    (1745.68, 621.823, 28.0505, 2.93777),
    (1704.14, 583.591, 28.1696, 1.59003),
    (1653.12, 635.41,  28.0932, 0.0977725),
};

struct Speech
{
    int32 textid;
    uint32 creature, timer;
};

enum
{
    YELL_AGGRO  = 0,
    YELL_SLAY,
    YELL_DEATH,
    YELL_KALEC,
    YELL_REFLECTION,
    YELL_PHASE3,
    YELL_PHASE4,
    YELL_KJ_OFFCOMBAT,
    YELL_KALEC_ORB_READY1,
    YELL_KALEC_ORB_READY2,
    YELL_KALEC_ORB_READY3,
    YELL_KALEC_ORB_READY4,
    YELL_EMERGE,
    YELL_DARKNESS1,
    YELL_DARKNESS2,
};

class AllOrbsInGrid
{
    public:
        AllOrbsInGrid() {}
        bool operator() (GameObject* go)
        {
            if(go->GetEntry() == GAMEOBJECT_ORB_OF_THE_BLUE_DRAGONFLIGHT)
                return true;
            return false;
        }
};

bool GOHello_go_orb_of_the_blue_flight(Player *plr, GameObject* go)
{
    if (go->GetUInt32Value(GAMEOBJECT_FACTION) == 35) {
        ScriptedInstance* pInstance = ((ScriptedInstance*)go->GetInstanceData());

        float x,y,z, dx,dy,dz;
        go->SummonCreature(CREATURE_POWER_OF_THE_BLUE_DRAGONFLIGHT, plr->GetPositionX(), plr->GetPositionY(), plr->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 121000);
        plr->CastSpell(plr, SPELL_VENGEANCE_OF_THE_BLUE_FLIGHT, true);
        go->SetUInt32Value(GAMEOBJECT_FACTION, 0);

        Creature* Kalec = (Creature*)(Unit::GetUnit(*plr, pInstance->GetData64(DATA_KALECGOS_KJ)));

        go->GetPosition(x,y,z);
        for (uint8 i = 0; i < 4; ++i) {
            DynamicObject* Dyn = Kalec->GetDynObject(SPELL_RING_OF_BLUE_FLAMES);
            if (Dyn) {
                Dyn->GetPosition(dx,dy,dz);
                if (x == dx && dy == y && dz == z) {
                    Dyn->RemoveFromWorld();
                    break;
                }
            }
        }
        
        go->Refresh();
    }

    return true;
}

//AI for Kalecgos
class boss_kalecgos_kj : public CreatureScript
{
public:
    boss_kalecgos_kj() : CreatureScript("boss_kalecgos_kj") {}
	
    class boss_kalecgos_kjAI : public CreatureAINew
    {
        public:
	    boss_kalecgos_kjAI(Creature* creature) : CreatureAINew(creature)
	    {
	        pInstance = ((ScriptedInstance*)creature->GetInstanceData());
	    }

            void onReset(bool /*onSpawn*/)
            {
                for (uint8 i = 0; i < 4; ++i)
                    Orb[i] = 0;

                FindOrbs();
                OrbsEmpowered = 0;
                EmpowerCount = 0;
                me->AddUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT + MOVEMENTFLAG_LEVITATING);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->setActive(true);
                Searched = false;
                me->SetVisibility(VISIBILITY_OFF);
                FindOrbs();     // FIXME: Is this really useful?
            }

            void FindOrbs()
            {
                CellPair pair(Trinity::ComputeCellPair(me->GetPositionX(), me->GetPositionY()));
                Cell cell(pair);
                cell.data.Part.reserved = ALL_DISTRICT;
                cell.SetNoCreate();
                std::list<GameObject*> orbList;
                AllOrbsInGrid check;
                Trinity::GameObjectListSearcher<AllOrbsInGrid> searcher(orbList, check);
                TypeContainerVisitor<Trinity::GameObjectListSearcher<AllOrbsInGrid>, GridTypeMapContainer> visitor(searcher);
                cell.Visit(pair, visitor, *(me->GetMap()));

                if (orbList.empty())
                    return;

                uint8 i = 0;
                for (std::list<GameObject*>::iterator itr = orbList.begin(); itr != orbList.end(); ++itr, ++i)
                    Orb[i] = (*itr)->GetGUID();
            }

            void ResetOrbs()
            {
                me->RemoveDynObject(SPELL_RING_OF_BLUE_FLAMES);
        
                for (uint8 i = 0; i < 4; ++i)
                {
                    if(GameObject *orb = pInstance->instance->GetGameObjectInMap(Orb[i]))
                        orb->SetUInt32Value(GAMEOBJECT_FACTION, 0);
                }
            }

            void EmpowerOrb(bool all)
            {
                GameObject *orb = pInstance->instance->GetGameObjectInMap(Orb[OrbsEmpowered]);
                if (!orb)
                    return;

                uint8 random = rand()%3;
                if (all)
                {
                    me->RemoveDynObject(SPELL_RING_OF_BLUE_FLAMES);
            
                    for (uint8 i = 0; i < 4; ++i)
                    {
                        if (orb = pInstance->instance->GetGameObjectInMap(Orb[i]))
                        {
                            orb->CastSpell(me, SPELL_RING_OF_BLUE_FLAMES);
                            orb->SetUInt32Value(GAMEOBJECT_FACTION, 35);
                            orb->setActive(true);
                            orb->Refresh();
                        }
                    }
                }
                else
                {
                    float x,y,z, dx,dy,dz;
                    orb = pInstance->instance->GetGameObjectInMap(Orb[random]);
                    if (!orb)
                        return;

                    orb->GetPosition(x,y,z);

                    for (uint8 i = 0; i < 4; ++i)
                    {
                        if (DynamicObject* Dyn = me->GetDynObject(SPELL_RING_OF_BLUE_FLAMES))
                        {
                            Dyn->GetPosition(dx,dy,dz);
                            if (x == dx && dy == y && dz == z)
                            {
                                Dyn->RemoveFromWorld();
                                break;
                            }
                        }
                    }

                    orb->CastSpell(me, SPELL_RING_OF_BLUE_FLAMES);
                    orb->SetUInt32Value(GAMEOBJECT_FACTION, 35);
                    orb->setActive(true);
                    orb->Refresh();
            
                    ++OrbsEmpowered;
                }
        
                ++EmpowerCount;

                switch (EmpowerCount)
                {
                    case 1:
                        talk(YELL_KALEC_ORB_READY1);
                        break;
                    case 2:
                        talk(YELL_KALEC_ORB_READY2);
                        break;
                    case 3:
                        talk(YELL_KALEC_ORB_READY3);
                        break;
                    case 4:
                        talk(YELL_KALEC_ORB_READY4);
                        break;
                }
            }

            void update(uint32 const diff)
            {
                if (!Searched)
                {
                    FindOrbs();
                    Searched = true;
                }

                if(OrbsEmpowered == 4)
                    OrbsEmpowered = 0;
            }
        private:
            uint64 Orb[4];
    
            ScriptedInstance* pInstance;
    
            uint8 OrbsEmpowered;
            uint8 EmpowerCount;

            bool Searched;
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new boss_kalecgos_kjAI(creature);
    }
};

//AI for Kil'jaeden
class boss_kiljaeden : public CreatureScript
{
public:
    boss_kiljaeden() : CreatureScript("boss_kiljaeden") {}
	
    class boss_kiljaedenAI : public Creature_NoMovementAINew
    {
        private:
            ScriptedInstance* pInstance;
    
            SummonList Summons;

            uint32 WaitTimer;
            bool IsInDarkness;
            bool shadowSpike;
            bool OrbActivated;
            bool IsWaiting;
        public:
	    boss_kiljaedenAI(Creature* creature) : Creature_NoMovementAINew(creature), Summons(me)
	    {
	        pInstance = ((ScriptedInstance*)creature->GetInstanceData());
	    }

            void onReset(bool onSpawn)
            {
                if (onSpawn)
                {
                    addEvent(EVENT_KALEC_JOIN, 26000, 26000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(2) | phaseMaskForPhase(3) | phaseMaskForPhase(4) | phaseMaskForPhase(5));
                    // Phase 2
                    addEvent(EVENT_SOUL_FLAY, 20000, 20000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(2) | phaseMaskForPhase(3) | phaseMaskForPhase(4) | phaseMaskForPhase(5));
                    addEvent(EVENT_LEGION_LIGHTNING, 40000, 40000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(2) | phaseMaskForPhase(3) | phaseMaskForPhase(4) | phaseMaskForPhase(5));
                    addEvent(EVENT_FIRE_BLOOM, 30000, 30000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(2) | phaseMaskForPhase(3) | phaseMaskForPhase(4) | phaseMaskForPhase(5));
                    addEvent(EVENT_SUMMON_SHILEDORB, 45000, 45000, EVENT_FLAG_NONE, true, phaseMaskForPhase(2) | phaseMaskForPhase(3) | phaseMaskForPhase(4) | phaseMaskForPhase(5));
                    // Phase 3
                    addEvent(EVENT_SHADOW_SPIKE, 4000, 4000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(3) | phaseMaskForPhase(4) | phaseMaskForPhase(5));
                    addEvent(EVENT_FLAME_DART, 3000, 3000, EVENT_FLAG_NONE, true, phaseMaskForPhase(3) | phaseMaskForPhase(4) | phaseMaskForPhase(5));
                    addEvent(EVENT_DARKNESS, 45000, 45000, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(3) | phaseMaskForPhase(4) | phaseMaskForPhase(5));
                    addEvent(EVENT_ORBS_EMPOWER, 35000, 35000, EVENT_FLAG_NONE, true, phaseMaskForPhase(3) | phaseMaskForPhase(4) | phaseMaskForPhase(5));
                    // Phase 4
                    addEvent(EVENT_ARMAGEDDON, 2000, 2000, EVENT_FLAG_NONE, true, phaseMaskForPhase(4) | phaseMaskForPhase(5));
                }
                else
                {
                    resetEvent(EVENT_KALEC_JOIN, 26000);
                    // Phase 2
                    resetEvent(EVENT_SOUL_FLAY, 20000);
                    resetEvent(EVENT_LEGION_LIGHTNING, 40000);
                    resetEvent(EVENT_FIRE_BLOOM, 30000);
                    resetEvent(EVENT_SUMMON_SHILEDORB, 45000);
                    // Phase 3
                    resetEvent(EVENT_SHADOW_SPIKE, 4000);
                    resetEvent(EVENT_FLAME_DART, 3000);
                    resetEvent(EVENT_DARKNESS, 45000);
                    resetEvent(EVENT_ORBS_EMPOWER, 35000);
                    // Phase 4
                    resetEvent(EVENT_ARMAGEDDON, 2000);
                }

                setPhase(PHASE_DECEIVERS);
                IsInDarkness  = false;
                shadowSpike = false;
                OrbActivated = false;
                IsWaiting = false;
                me->SetFullTauntImmunity(true);
            }

            void onSummon(Creature* summoned)
            {
                Summons.Summon(summoned);
                switch (summoned->GetEntry())
                {
                    case CREATURE_ARMAGEDDON_TARGET:
                        summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        break;
                    default:
                        summoned->SetLevel(me->getLevel());
                        break;
                }
                summoned->setFaction(me->getFaction());
            }
	
            void onSummonDespawn(Creature* unit)
            {
                Summons.Despawn(unit);
            }

            void onDeath(Unit* /*killer*/)
            {
                talk(YELL_DEATH);

                if (pInstance)
                    pInstance->SetData(DATA_KILJAEDEN_EVENT, DONE);
            }

            void onKill(Unit* /*victim*/)
            {
                talk(YELL_SLAY);
            }

            void evade()
            {
                CreatureAINew::evade();

                Summons.DespawnAll();

                // Reset the controller
                if (pInstance)
                {
                    if (Creature* Control = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_KILJAEDEN_CONTROLLER)))
                        Control->getAI()->onReset(false);
                }
            }

            void onCombatStart(Unit* /*who*/)
            {
                setZoneInCombat();
                talk(YELL_EMERGE);
            }

            void ChangeTimers (bool status, uint32 WTimer)
            {
                if (status)
                {
                    disableEvent(EVENT_SOUL_FLAY);
                    disableEvent(EVENT_LEGION_LIGHTNING);
                    disableEvent(EVENT_FIRE_BLOOM);
                    disableEvent(EVENT_SUMMON_SHILEDORB);
                    disableEvent(EVENT_SHADOW_SPIKE);
                    disableEvent(EVENT_FLAME_DART);
                    disableEvent(EVENT_DARKNESS);
                    disableEvent(EVENT_ORBS_EMPOWER);
                    disableEvent(EVENT_ARMAGEDDON);
                }
                else
                {
                    enableEvent(EVENT_SOUL_FLAY);
                    enableEvent(EVENT_LEGION_LIGHTNING);
                    enableEvent(EVENT_FIRE_BLOOM);
                    enableEvent(EVENT_FLAME_DART);
                    enableEvent(EVENT_DARKNESS);
                    enableEvent(EVENT_ARMAGEDDON);

                    if (getPhase() != PHASE_SACRIFICE)
                        enableEvent(EVENT_SUMMON_SHILEDORB);
                    if (!shadowSpike)
                        enableEvent(EVENT_SHADOW_SPIKE);
                    if (!OrbActivated)
                        enableEvent(EVENT_ORBS_EMPOWER);
                }

                if (WTimer > 0)
                {
                    IsWaiting = true;
                    WaitTimer = WTimer;
                }
            }

            void CastSinisterReflection()
            {
                talk(YELL_REFLECTION);
                doCast(me, SPELL_SINISTER_REFLECTION, true);
        
                for (uint8 i = 0; i < 4; i++)
                {
                    float x,y,z;
                    Unit* target;
                    for (uint8 z = 0; z < 6; ++z)
                    {
                        target = NULL;
                        if (target = selectUnit(SELECT_TARGET_RANDOM, 0, 100, true))
                            if (!target->HasAura(SPELL_VENGEANCE_OF_THE_BLUE_FLIGHT))
                                break;
                    }

                    if (target)
                    {
                        target->GetPosition(x,y,z);
            
                        if(Creature* SinisterReflection = me->SummonCreature(CREATURE_SINISTER_REFLECTION, x,y,z,0, TEMPSUMMON_CORPSE_DESPAWN, 0))
                            SinisterReflection->getAI()->attackStart(target);
                    }
                }
            }

            void update(const uint32 diff)
            {
                if (!updateVictim())
                    return;

                if (IsWaiting)
                {
                    if (WaitTimer <= diff)
                    {
                        IsWaiting = false;
                        ChangeTimers(false, 0);
                    }
                    else
                        WaitTimer -= diff;
                }

                updateEvents(diff);
            
                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EVENT_KALEC_JOIN:
                            disableEvent(EVENT_KALEC_JOIN);
                            if (Creature* kalec = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_KALECGOS_KJ)))
                                kalec->getAI()->talk(YELL_KALEC);
                            break;
                        case EVENT_SOUL_FLAY:
                            doCast(me, SPELL_SOUL_FLAY);
                            me->getVictim()->CastSpell(me->getVictim(), SPELL_SOUL_FLAY_SLOW, true);
                            scheduleEvent(EVENT_SOUL_FLAY, 3500);
                            break;
                        case EVENT_LEGION_LIGHTNING:
                            me->RemoveAurasDueToSpell(SPELL_SOUL_FLAY);
                            for(uint8 i = 0; i < 6; ++i)
                            {
                                if (Unit *randomPlayer = selectUnit(SELECT_TARGET_RANDOM, 0, 100, true))
                                {
                                    if (!randomPlayer->HasAura(SPELL_VENGEANCE_OF_THE_BLUE_FLIGHT))
                                        break;

                                    doCast(randomPlayer, SPELL_LEGION_LIGHTNING);
                                    scheduleEvent(EVENT_LEGION_LIGHTNING, (getPhase() == PHASE_SACRIFICE) ? 18000 : 30000, (getPhase() == PHASE_SACRIFICE) ? 18000 : 30000);
                                    scheduleEvent(EVENT_SOUL_FLAY, 2500);
                                }
                            }
                            break;
                        case EVENT_FIRE_BLOOM:
                            me->RemoveAurasDueToSpell(SPELL_SOUL_FLAY);
                            doCast(NULL, SPELL_FIRE_BLOOM);
                            scheduleEvent(EVENT_FIRE_BLOOM, (getPhase() == PHASE_SACRIFICE) ? 25000 : 40000);
                            scheduleEvent(EVENT_SOUL_FLAY, 1000);
                            break;
                        case EVENT_SUMMON_SHILEDORB:
                            for (uint8 i = 1; i < getPhase(); ++i)
                            {
                                float sx, sy;
                                sx = ShieldOrbLocations[0][0] + sin(ShieldOrbLocations[i][0]);
                                sy = ShieldOrbLocations[0][1] + sin(ShieldOrbLocations[i][1]);
                                me->SummonCreature(CREATURE_SHIELD_ORB, sx, sy, SHIELD_ORB_Z, 0, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 45000);
                            }

                            scheduleEvent(EVENT_SUMMON_SHILEDORB, 30000, 60000);
                            scheduleEvent(EVENT_SOUL_FLAY, 2000);
                            break;
                        case EVENT_SHADOW_SPIKE:
                            doCast(NULL, SPELL_SHADOW_SPIKE);
                            shadowSpike = true;
                            disableEvent(EVENT_SHADOW_SPIKE);
                            ChangeTimers(true, 30000);
                            break;
                        case EVENT_FLAME_DART:
                            doCast(NULL, SPELL_FLAME_DART);
                            scheduleEvent(EVENT_SOUL_FLAY, 3000);
                            break;
                        case EVENT_DARKNESS:
                            if (!IsInDarkness)
                            {
                                ChangeTimers(true, 9000);
                                talk(YELL_DARKNESS1);
                                doCast(NULL, SPELL_DARKNESS_OF_A_THOUSAND_SOULS);
                                scheduleEvent(EVENT_DARKNESS, 8750);
                                enableEvent(EVENT_DARKNESS);
                                if (getPhase() == PHASE_SACRIFICE)
                                    enableEvent(EVENT_ARMAGEDDON);
                                IsInDarkness = true;
                            }
                            else
                            {
                                scheduleEvent(EVENT_DARKNESS, (getPhase() == PHASE_SACRIFICE) ? urand(20000, 35000) : urand(40000, 70000));
                                IsInDarkness = false;
                                doCast(NULL, SPELL_DARKNESS_OF_A_THOUSAND_SOULS_DAMAGE);
                                talk(YELL_DARKNESS2);
                            }
                            scheduleEvent(EVENT_SOUL_FLAY, 9000);
                            break;
                        case EVENT_ORBS_EMPOWER:
                            if (getPhase() == PHASE_SACRIFICE)
                            {
                                if (Creature* kalec = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_KALECGOS_KJ)))
                                    ((boss_kalecgos_kj::boss_kalecgos_kjAI*)kalec->getAI())->EmpowerOrb(true);
                            }
                            else
                            {
                                if (Creature* kalec = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_KALECGOS_KJ)))
                                    ((boss_kalecgos_kj::boss_kalecgos_kjAI*)kalec->getAI())->EmpowerOrb(false);

                                scheduleEvent(EVENT_ORBS_EMPOWER, (getPhase() == PHASE_SACRIFICE) ? 45000 : 35000);
                            }
                            OrbActivated = true;
                            disableEvent(EVENT_ORBS_EMPOWER);
                            break;
                        case EVENT_ARMAGEDDON:
                            Unit *target;
                            for (uint8 i = 0; i < 6; ++i)
                            {
                                target = NULL;
                                if (target = selectUnit(SELECT_TARGET_RANDOM, 0, 100, true))
                                    if (!target->HasAura(SPELL_VENGEANCE_OF_THE_BLUE_FLIGHT))
                                        break;
                            }

                            if (target)
                            {
                                float x, y, z;
                                target->GetPosition(x, y, z);
                                me->SummonCreature(CREATURE_ARMAGEDDON_TARGET, x,y,z,0, TEMPSUMMON_TIMED_DESPAWN,15000);
                            }
                            scheduleEvent(EVENT_ARMAGEDDON, 2000);
                            break;
                    }
                }

                //Phase 3
                if (getPhase() <= PHASE_NORMAL)
                {
                    if (getPhase() == PHASE_NORMAL && me->IsBelowHPPercent(85))
                    {
                        CastSinisterReflection();
                        talk(YELL_PHASE3);
                        OrbActivated = false;
                        setPhase(PHASE_DARKNESS);
                    }
                    else
                        return;
                }

                //Phase 4
                if (getPhase() <= PHASE_DARKNESS)
                {
                    if (getPhase() == PHASE_DARKNESS && me->IsBelowHPPercent(55))
                    {
                        talk(YELL_PHASE4);
                        setPhase(PHASE_ARMAGEDDON);
                        OrbActivated = false;
                    }
                    else
                        return;
                }

                //Phase 5
                if (getPhase() <= PHASE_ARMAGEDDON)
                {
                    if (getPhase() == PHASE_ARMAGEDDON && me->IsBelowHPPercent(25))
                    {
                        setPhase(PHASE_SACRIFICE);

                        if (Creature* Anveena = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_ANVEENA)))
                            Anveena->CastSpell(me, SPELL_SACRIFICE_OF_ANVEENA, false);
                        OrbActivated = false;
                        ChangeTimers(true, 10000);
                    }
                    else
                        return;
                }
            }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new boss_kiljaedenAI(creature);
    }
};

//AI for Kil'Jaeden Event Controller
class mob_kiljaeden_controller : public CreatureScript
{
public:
    mob_kiljaeden_controller() : CreatureScript("mob_kiljaeden_controller") {}
	
    class mob_kiljaeden_controllerAI : public Creature_NoMovementAINew
    {
        private:
            ScriptedInstance* pInstance;

            SummonList Summons;

            bool KiljaedenDeath;
        public:
	    mob_kiljaeden_controllerAI(Creature* creature) : Creature_NoMovementAINew(creature), Summons(me)
	    {
	        pInstance = ((ScriptedInstance*)creature->GetInstanceData());
	    }

            uint8 DeceiverDeathCount;

            void onSummon(Creature* summoned)
            {
                Summons.Summon(summoned);
                switch(summoned->GetEntry())
                {
                    case CREATURE_HAND_OF_THE_DECEIVER:
                        summoned->CastSpell(summoned, SPELL_SHADOW_CHANNELING, false);
                        break;
                    case CREATURE_ANVEENA:
                    {
                        summoned->AddUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT + MOVEMENTFLAG_LEVITATING);
                        WorldPacket data;
                        summoned->BuildHeartBeatMsg(&data);
                        summoned->SendMessageToSet(&data, true);
                        summoned->CastSpell(summoned, SPELL_ANVEENA_PRISON, true);
                        summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        break;
                    }
                    case CREATURE_KILJAEDEN:
                        summoned->CastSpell(summoned, SPELL_REBIRTH, false);
                        summoned->getAI()->setPhase(PHASE_NORMAL);
                        summoned->AddThreat(me->getVictim(), 1.0f);
                        break;
                }
            }

            void onSummonDespawn(Creature* unit)
            {
                Summons.Despawn(unit);
            }

            void onReset(bool onSpawn)
            {
                if (onSpawn)
                {
                    addEvent(EVENT_SAY, 45000, 75000, EVENT_FLAG_DELAY_IF_CASTING);
                }
                else
                {
                    resetEvent(EVENT_SAY, 45000, 75000);
                }

                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);

                setPhase(PHASE_DECEIVERS);

                if (Creature *KalecKJ = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_KALECGOS_KJ)))
                    ((boss_kalecgos_kj::boss_kalecgos_kjAI*)KalecKJ->getAI())->ResetOrbs();

                DeceiverDeathCount = 0;
                KiljaedenDeath = false;

                Summons.DespawnAll();

                for(uint8 i = 0; i < 3; ++i)
                    me->SummonCreature(CREATURE_HAND_OF_THE_DECEIVER, DeceiverLocations[i][0], DeceiverLocations[i][1], FLOOR_Z, DeceiverLocations[i][2], TEMPSUMMON_DEAD_DESPAWN, 0);

                me->SummonCreature(CREATURE_ANVEENA,  0, 0, 40, 0, TEMPSUMMON_DEAD_DESPAWN, 0);
                doCast(me, SPELL_ANVEENA_ENERGY_DRAIN);
            }

            void update(uint32 const diff)
            {
                updateEvents(diff);
            
                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EVENT_SAY:
                            if (pInstance->GetData(DATA_MURU_EVENT) != DONE && pInstance->GetData(DATA_KILJAEDEN_EVENT) == NOT_STARTED)
                                talk(YELL_KJ_OFFCOMBAT);

                            scheduleEvent(EVENT_SAY, 45000, 75000);
                            break;
                    }
                }

                if (DeceiverDeathCount > 2 && getPhase() == PHASE_DECEIVERS)
                {
                    me->RemoveAurasDueToSpell(SPELL_ANVEENA_ENERGY_DRAIN);
                    setPhase(PHASE_NORMAL);
                    me->SummonCreature(CREATURE_KILJAEDEN, 0, 0, 0, 3.699289, TEMPSUMMON_MANUAL_DESPAWN, 0);
                }
            }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new mob_kiljaeden_controllerAI(creature);
    }
};

//AI for Hand of the Deceiver
class mob_hand_of_the_deceiver : public CreatureScript
{
public:
    mob_hand_of_the_deceiver() : CreatureScript("mob_hand_of_the_deceiver") {}

    class mob_hand_of_the_deceiverAI : public CreatureAINew
    {
        private:
            ScriptedInstance* pInstance;

            SummonList Summons;
        public:
            mob_hand_of_the_deceiverAI(Creature* creature) : CreatureAINew(creature), Summons(me)
            {
                pInstance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onSummon(Creature* summoned)
            {
                Summons.Summon(summoned);
            }

            void onSummonDespawn(Creature* unit)
            {
                Summons.Despawn(unit);
            }

            void onReset(bool onSpawn)
            {
                if (onSpawn)
                {
                    addEvent(EVENT_SHADOWBOLT, 2000, 3000, EVENT_FLAG_DELAY_IF_CASTING);
                    addEvent(EVENT_FELFIRE, 20000, 20000, EVENT_FLAG_DELAY_IF_CASTING);
                }
                else
                {
                    resetEvent(EVENT_SHADOWBOLT, 2000, 3000);
                    resetEvent(EVENT_FELFIRE, 20000);
                }

                if (pInstance)
                    pInstance->SetData(DATA_KILJAEDEN_EVENT, NOT_STARTED);

                Summons.DespawnAll();
            }

            void onCombatStart(Unit* victim)
            {
                if(pInstance)
                {
                    pInstance->SetData(DATA_KILJAEDEN_EVENT, IN_PROGRESS);
                    if (Creature* Control = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_KILJAEDEN_CONTROLLER)))
                        Control->AddThreat(victim, 1.0f);
                }

                me->InterruptNonMeleeSpells(true);

                if (Creature *controller = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_KILJAEDEN_CONTROLLER)))
                    if (!controller->getAI()->aiInCombat())
                        controller->getAI()->attackStart(victim);
            }

            void onDeath(Unit* /*killer*/)
            {
                Summons.DespawnAll();

                if(pInstance)
                    if (Creature* Control = pInstance->instance->GetCreatureInMap(pInstance->GetData64(DATA_KILJAEDEN_CONTROLLER)))
                        ((mob_kiljaeden_controller::mob_kiljaeden_controllerAI*)Control->getAI())->DeceiverDeathCount++;
            }

            void update(uint32 const diff)
            {
                if (!me->isInCombat())
                    doCast(me, SPELL_SHADOW_CHANNELING);

                if (!updateVictim())
                    return;

                // Gain Shadow Infusion
                if (me->IsBetweenHPPercent(20, 25) && !me->HasAura(SPELL_SHADOW_INFUSION))
                    if (rand()%2)
                        doCast(me, SPELL_SHADOW_INFUSION, true);

                updateEvents(diff);
            
                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EVENT_SHADOWBOLT:
                            doCast(me, SPELL_SHADOW_BOLT_VOLLEY);
                            scheduleEvent(EVENT_SHADOWBOLT, 2000, 3000);
                            break;
                        case EVENT_FELFIRE:
                            doCast(me, SPELL_FELFIRE_PORTAL);
                            scheduleEvent(EVENT_FELFIRE, 20000);
                            break;
                    }
                }

                doMeleeAttackIfReady();
            }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new mob_hand_of_the_deceiverAI(creature);
    }
};

//AI for Felfire Portal
class mob_felfire_portal : public CreatureScript
{
public:
    mob_felfire_portal() : CreatureScript("mob_felfire_portal") {}
	
    class mob_felfire_portalAI : public Creature_NoMovementAINew
    {
        private:
            ScriptedInstance* pInstance;

            SummonList Summons;

        public:
	    mob_felfire_portalAI(Creature* creature) : Creature_NoMovementAINew(creature), Summons(me)
	    {
	        pInstance = ((ScriptedInstance*)creature->GetInstanceData());
	    }

            void onReset(bool onSpawn)
            {
                if (onSpawn)
                    addEvent(EVENT_SPAWNFIEND, 1000, 1000);
                else
                    resetEvent(EVENT_SPAWNFIEND, 1000);

                Summons.DespawnAll();
            }

            void onSummon(Creature* summoned)
            {
                Summons.Summon(summoned);
            }

            void onSummonDespawn(Creature* unit)
            {
                Summons.Despawn(unit);
            }

            void update(uint32 const diff)
            {
                updateEvents(diff);
            
                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EVENT_SPAWNFIEND:
                            doCast(me, SPELL_SUMMON_FELFIRE_FIEND, false);
                            disableEvent(EVENT_SPAWNFIEND);
                            break;
                    }
                }
            }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new mob_felfire_portalAI(creature);
    }
};

//AI for Felfire Fiend
class mob_volatile_felfire_fiend : public CreatureScript
{
public:
    mob_volatile_felfire_fiend() : CreatureScript("mob_volatile_felfire_fiend") {}

    class mob_volatile_felfire_fiendAI : public CreatureAINew
    {
        private:
            ScriptedInstance* pInstance;
        public:
            mob_volatile_felfire_fiendAI(Creature* creature) : CreatureAINew(creature)
            {
                pInstance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onReset(bool onSpawn)
            {
                if (onSpawn)
                {
                    addEvent(EVENT_STUN, 2000, 2000);
                    addEvent(EVENT_EXPLODE, 2000, 2000);
                    addEvent(EVENT_DIE_F, 500, 500, EVENT_FLAG_NONE, false);
                }
                else
                {
                    resetEvent(EVENT_STUN, 2000);
                    resetEvent(EVENT_EXPLODE, 2000);
                    resetEvent(EVENT_DIE_F, 500);
                }

                me->addUnitState(UNIT_STAT_STUNNED);
            }

            void onDamageTaken(Unit* /*attacker*/, uint32& damage)
            {
                doCast(me, SPELL_FELFIRE_FISSION);
                enableEvent(EVENT_DIE_F);
            }

            void onMoveInLoS(Unit* /*who*/)
            {
            }

            void updateEM(uint32 const diff)
            {
                if (pInstance->GetData(DATA_KILJAEDEN_EVENT) == NOT_STARTED)
                    me->DisappearAndDie();
            }

            void update(uint32 const diff)
            {
                if (pInstance->GetData(DATA_KILJAEDEN_EVENT) == NOT_STARTED)
                    me->DisappearAndDie();

                updateEvents(diff, 5);

                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EVENT_STUN:
                            me->clearUnitState(UNIT_STAT_STUNNED);
                            setZoneInCombat(true);
                            if (Unit *unit = selectUnit(SELECT_TARGET_NEAREST, 0, 1000.0f, false))
                            {
                                attackStart(unit);
                                doModifyThreat(unit, 10000000.0f);
                                disableEvent(EVENT_STUN);
                            }
                            scheduleEvent(EVENT_STUN, 500);
                            break;
                        case EVENT_EXPLODE:
                            if (me->IsWithinMeleeRange(me->getVictim()))
                            {
                                // Explode if it's close enough to it's target
                                doCast(me, SPELL_FELFIRE_FISSION);
                                disableEvent(EVENT_EXPLODE);
                                enableEvent(EVENT_DIE_F);
                            }
                            scheduleEvent(EVENT_EXPLODE, 2000);
                            break;
                        case EVENT_DIE_F:
                            disableEvent(EVENT_DIE_F);
                            me->DisappearAndDie();
                            break;
                    }
                }

                if (!updateVictim())
                    return;

                updateEvents(diff, 2);

                doMeleeAttackIfReady();
            }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new mob_volatile_felfire_fiendAI(creature);
    }
};

//AI for Armageddon target
class mob_armageddon : public CreatureScript
{
public:
    mob_armageddon() : CreatureScript("mob_armageddon") {}

    class mob_armageddonAI : public Creature_NoMovementAINew
    {
        private:
            ScriptedInstance* pInstance;
        public:
            mob_armageddonAI(Creature* creature) : Creature_NoMovementAINew(creature)
            {
                pInstance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onReset(bool onSpawn)
            {
                setPhase(1);
                if (onSpawn)
                {
                    addEvent(EVENT_VISUAL1, 0, 0, EVENT_FLAG_DELAY_IF_CASTING, true, phaseMaskForPhase(1));
                    addEvent(EVENT_VISUAL2, 0, 0, EVENT_FLAG_DELAY_IF_CASTING, false, phaseMaskForPhase(2));
                    addEvent(EVENT_TRIGGER, 9000, 9000, EVENT_FLAG_DELAY_IF_CASTING, false, phaseMaskForPhase(3));
                    addEvent(EVENT_DIE, 5000, 5000, EVENT_FLAG_DELAY_IF_CASTING, false, phaseMaskForPhase(4));
                }
                else
                {
                    resetEvent(EVENT_VISUAL1, 0);
                    resetEvent(EVENT_VISUAL2, 0);
                    resetEvent(EVENT_TRIGGER, 9000);
                    resetEvent(EVENT_DIE, 5000);
                }
            }

            void update(uint32 const diff)
            {
                updateEvents(diff);

                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EVENT_VISUAL1:
                            doCast(me, SPELL_ARMAGEDDON_VISUAL, true);
                            incrPhase();
                            enableEvent(EVENT_VISUAL2);
                            break;
                        case EVENT_VISUAL2:
                            doCast(me, SPELL_ARMAGEDDON_VISUAL2, true);
                            incrPhase();
                            enableEvent(EVENT_TRIGGER);
                            break;
                        case EVENT_TRIGGER:
                            doCast(me, SPELL_ARMAGEDDON_TRIGGER, true);
                            incrPhase();
                            enableEvent(EVENT_DIE);
                            break;
                        case EVENT_DIE:
                            incrPhase();
                            disableEvent(EVENT_DIE);
                            me->DisappearAndDie();
                            break;
                    }
                }
            }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new mob_armageddonAI(creature);
    }
};

//AI for Shield Orbs
class mob_shield_orb : public CreatureScript
{
public:
    mob_shield_orb() : CreatureScript("mob_shield_orb") {}

    class mob_shield_orbAI : public CreatureAINew
    {
        private:
            bool PointReached;
            bool Clockwise;
            uint32 CheckTimer;
    
            ScriptedInstance *pInstance;
    
            float x, y, r, c, mx, my;
        public:
            mob_shield_orbAI(Creature* creature) : CreatureAINew(creature)
            {
                pInstance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onReset(bool onSpawn)
            {
                me->AddUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT + MOVEMENTFLAG_LEVITATING);
        
                PointReached = true;

                CheckTimer = 1000;
        
                r = 17;
                c = 0;
        
                mx = ShieldOrbLocations[0][0];
                my = ShieldOrbLocations[0][1];
        
                if (rand()%2)
                    Clockwise = true;
                else
                    Clockwise = false;

                if (onSpawn)
                {
                    addEvent(EVENT_SHADOWBOLT_S, 500, 1000, EVENT_FLAG_DELAY_IF_CASTING);
                }
                else
                {
                    resetEvent(EVENT_SHADOWBOLT_S, 500, 1000);
                }
            }

            void onMovementInform(uint32 type, uint32 /*id*/)
            {
                if(type != POINT_MOTION_TYPE)
                    return;

                PointReached = true;
            }

            void update(uint32 const diff)
            {
                if (PointReached)
                {
                    if (Clockwise)
                    {
                        y = my - r * sin(c);
                        x = mx - r * cos(c);
                    }
                    else
                    {
                        y = my + r * sin(c);
                        x = mx + r * cos(c);
                    }
            
                    PointReached = false;
                    CheckTimer = 1000;
            
                    me->GetMotionMaster()->MovePoint(1,x, y, SHIELD_ORB_Z);
            
                    c += 3.1415926535/32;
                    if (c > 2*3.1415926535)
                        c = 0;
                }
                else
                {
                    if (CheckTimer <= diff)
                    {
                        doTeleportTo(x, y, SHIELD_ORB_Z);
                        PointReached = true;
                    }
                    else
                        CheckTimer -= diff;
                }

                updateEvents(diff);

                while (executeEvent(diff, m_currEvent))
                {
                    switch (m_currEvent)
                    {
                        case EVENT_SHADOWBOLT_S:
                            if (Unit* random = selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true))
                                doCast(random, SPELL_SHADOW_BOLT, false);
                            scheduleEvent(EVENT_SHADOWBOLT_S, 500, 1000);
                            break;
                    }
                }
            }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new mob_shield_orbAI(creature);
    }
};

//AI for Sinister Reflection
class mob_sinster_reflection : public CreatureScript
{
public:
    mob_sinster_reflection() : CreatureScript("mob_sinster_reflection") {}

    class mob_sinster_reflectionAI : public CreatureAINew
    {
        private:
            ScriptedInstance *pInstance;

            uint8 Class;
            uint32 Timer[3];
        public:
            mob_sinster_reflectionAI(Creature* creature) : CreatureAINew(creature)
            {
                pInstance = ((ScriptedInstance*)creature->GetInstanceData());
            }

            void onReset(bool /*onSpawn*/)
            {
                Timer[0] = 0;
                Timer[1] = 0;
                Timer[2] = 0;
                Class = 0;
            }

            void update(uint32 const diff)
            {
                if (Class == 0)
                {
                    Class = me->getVictim()->getClass();
                    switch(Class)
                    {
                        case CLASS_DRUID:
                        case CLASS_HUNTER:
                        case CLASS_MAGE:
                        case CLASS_WARLOCK:
                        case CLASS_PALADIN:
                        case CLASS_PRIEST:
                            break;
                        case CLASS_WARRIOR:
                        case CLASS_SHAMAN:
                        case CLASS_ROGUE:
                            me->SetCanDualWield(true);
                            break;
                    }
                }

                switch (Class)
                {
                    case CLASS_DRUID:
                        if (Timer[1] <= diff)
                        {
                            doCast(me->getVictim(), SPELL_SR_MOONFIRE, false);
                            Timer[1] = 3000;
                        }

                        doMeleeAttackIfReady();
                        break;
                    case CLASS_HUNTER:
                        if (Timer[1] <= diff)
                        {
                            doCast(me->getVictim(), SPELL_SR_MULTI_SHOT, false);
                            Timer[1] = 9000;
                        }

                        if (Timer[2] <= diff)
                        {
                            doCast(me->getVictim(), SPELL_SR_SHOOT, false);
                            Timer[2] = 5000;
                        }

                        if (me->IsWithinMeleeRange(me->getVictim(), 6))
                        {
                            if (Timer[3] <= diff)
                            {
                                doCast(me->getVictim(), SPELL_SR_MULTI_SHOT, false);
                                Timer[3] = 7000;
                            }
                
                            doMeleeAttackIfReady();
                        }
                        break;
                    case CLASS_MAGE:
                        if (Timer[1] <= diff)
                        {
                            doCast(me->getVictim(), SPELL_SR_FIREBALL, false);
                            Timer[1] = 3000;
                        }

                        doMeleeAttackIfReady();
                        break;
                    case CLASS_WARLOCK:
                        if (Timer[1] <= diff)
                        {
                            doCast(me->getVictim(), SPELL_SR_SHADOW_BOLT, false);
                            Timer[1] = 4000;
                        }

                        if (Timer[2] <= diff)
                        {
                            doCast(selectUnit(SELECT_TARGET_RANDOM, 0, 100, true), SPELL_SR_CURSE_OF_AGONY, true);
                            Timer[2] = 3000;
                        }

                        doMeleeAttackIfReady();
                        break;
                    case CLASS_WARRIOR:
                        if (Timer[1] <= diff)
                        {
                            doCast(me->getVictim(), SPELL_SR_WHIRLWIND, false);
                            Timer[1] = 10000;
                        }

                        doMeleeAttackIfReady();
                        break;
                    case CLASS_PALADIN:
                        if (Timer[1] <= diff)
                        {
                            doCast(me->getVictim(), SPELL_SR_HAMMER_OF_JUSTICE, false);
                            Timer[1] = 7000;
                        }

                        if (Timer[2] <= diff)
                        {
                            doCast(me->getVictim(), SPELL_SR_HOLY_SHOCK, false);
                            Timer[2] = 3000;
                        }

                        doMeleeAttackIfReady();
                        break;
                    case CLASS_PRIEST:
                        if (Timer[1] <= diff)
                        {
                            doCast(me->getVictim(), SPELL_SR_HOLY_SMITE, false);
                            Timer[1] = 5000;
                        }

                        if (Timer[2] <= diff)
                        {
                            doCast(me,  SPELL_SR_RENEW, false);
                            Timer[2] = 7000;
                        }

                        doMeleeAttackIfReady();
                        break;
                    case CLASS_SHAMAN:
                        if (Timer[1] <= diff)
                        {
                            doCast(me->getVictim(), SPELL_SR_EARTH_SHOCK, false);
                            Timer[1] = 5000;
                        }

                        doMeleeAttackIfReady();
                        break;
                    case CLASS_ROGUE:
                        if (Timer[1] <= diff)
                        {
                            doCast(me->getVictim(), SPELL_SR_HEMORRHAGE, true);
                            Timer[1] = 5000;
                        }

                        doMeleeAttackIfReady();
                        break;
                }
                for (uint8 i = 0; i < 3; ++i)
                    Timer[i] -= diff;
            }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new mob_sinster_reflectionAI(creature);
    }
};

void AddSC_boss_kiljaeden_new()
{
    Script* newscript;

    newscript = new Script;
    newscript->Name = "go_orb_of_the_blue_flight";
    newscript->pGOHello = &GOHello_go_orb_of_the_blue_flight;
    newscript->RegisterSelf();

    sScriptMgr.addScript(new boss_kalecgos_kj());
    sScriptMgr.addScript(new boss_kiljaeden());
    sScriptMgr.addScript(new mob_kiljaeden_controller());
    sScriptMgr.addScript(new mob_hand_of_the_deceiver());
    sScriptMgr.addScript(new mob_felfire_portal());
    sScriptMgr.addScript(new mob_volatile_felfire_fiend());
    sScriptMgr.addScript(new mob_armageddon());
    sScriptMgr.addScript(new mob_shield_orb());
    sScriptMgr.addScript(new mob_sinster_reflection());
}
