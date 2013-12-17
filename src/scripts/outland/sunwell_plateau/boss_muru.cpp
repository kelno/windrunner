/*
 * Copyright (C) 2010-2012 OregonCore <http://www.oregoncore.com/>
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2012 ScriptDev2 <http://www.scriptdev2.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* ScriptData
SDName: Boss_Muru
SD%Complete: 80
SDComment: all sounds, black hole effect triggers to often (46228)
*/

#include "precompiled.h"
#include "CreatureScript.h"
#include "CreatureAINew.h"
#include "def_sunwell_plateau.h"
#include "ObjectMgr.h"

// Muru & Entropius's spells
enum Spells
{
    SPELL_ENRAGE                = 26662,

    // Muru's spells
    SPELL_NEGATIVE_ENERGY       = 46009, //(this trigger 46008)
    SPELL_DARKNESS_P1           = 45999, //pre effect, trigger 45996
    SPELL_OPEN_ALL_PORTALS      = 46177,
    SPELL_OPEN_PORTAL           = 45977,
    SPELL_OPEN_PORTAL_2         = 45976,
    SPELL_SUMMON_BERSERKER      = 46037,
    SPELL_SUMNON_FURY_MAGE      = 46038,
    SPELL_SUMMON_VOID_SENTINEL  = 45988,
    SPELL_SUMMON_VOID_SENTINEL_VISUAL  = 45989,
    SPELL_SUMMON_ENTROPIUS      = 46217,

    // Entropius's spells
    SPELL_DARKNESS_P2           = 46268,
    SPELL_BLACKHOLE             = 46282,
    SPELL_NEGATIVE_ENERGY_E     = 46284,
    SPELL_ENTROPIUS_SPAWN       = 46223,

    // Shadowsword Berserker's spells
    SPELL_FLURRY                = 46160,
    SPELL_DUAL_WIELD            = 29651,

    // Shadowsword Fury Mage's spells
    SPELL_FEL_FIREBALL          = 46101,
    SPELL_SPELL_FURY            = 46102,

    // Void Sentinel's spells
    SPELL_SHADOW_PULSE          = 46087,
    SPELL_VOID_BLAST            = 46161,
    SPELL_SUMMON_VOID_SPAWN     = 46071,

    // Void Spawn's spells
    SPELL_SHADOW_BOLT_VOLLEY    = 46082,

    //Dark Fiend Spells
    SPELL_DARKFIEND_AOE         = 45944,
    SPELL_DARKFIEND_DEATH_VISUAL= 45936,
    SPELL_DARKFIEND_SKIN        = 45934,

    //Black Hole Spells
    SPELL_BLACKHOLE_SPAWN       = 46242,
    SPELL_BLACKHOLE_SPAWN2      = 46247,
    SPELL_BLACKHOLE_VISUAL2     = 46235,
    SPELL_BLACKHOLE_GROW        = 46228,
    SPELL_BLACK_HOLE_EFFECT     = 46230,
    SPELL_SINGULARITY           = 46238,

    //Darkness (aka Void Zone) Spells (P2)
    SPELL_VOID_ZONE_PERIODIC    = 46262,
};

enum BossTimers{
    TIMER_DARKNESS              = 0,
    TIMER_HUMANOIDES            = 1,
    TIMER_PHASE                 = 2,
    TIMER_SENTINEL              = 3
};

float DarkFiends[8][2] =
{
    {69.74f,    1.94f},
    {69.73f,    2.61f},
    {69.74f,    5.71f},
    {69.73f,    3.52f},
    {69.74f,    0.22f},
    {69.73f,    1.02f},
    {69.74f,    4.12f},
    {69.73f,    4.97f}
};

float Humanoides[6][5] =
{
    {CREATURE_FURY_MAGE, 1724.64f,    702.93f,    71.19f,    5.21f},
    {CREATURE_BERSERKER, 1724.64f,    702.93f,    71.19f,    5.28f},
    {CREATURE_BERSERKER, 1724.64f,    702.93f,    71.19f,    4.51f},
    {CREATURE_BERSERKER, 1900.85f,    555.99f,    71.30f,    2.43f},
    {CREATURE_BERSERKER, 1900.85f,    555.99f,    71.30f,    2.44f},
    {CREATURE_FURY_MAGE, 1900.85f,    555.99f,    71.30f,    2.57f}
};

typedef std::map<uint64, uint32> GuidMapCD;
typedef std::set<uint64> GuidSet;

class npc_blackhole : public CreatureScript
{
public:
    npc_blackhole() : CreatureScript("npc_blackhole") {}

    class npc_blackholeAI : public CreatureAINew
    {
        public:
        npc_blackholeAI(Creature* creature) : CreatureAINew(creature)
        {
            pInstance = ((ScriptedInstance*)creature->GetInstanceData());
        }

        ScriptedInstance* pInstance;

        uint32 DespawnTimer;
        uint32 SpellTimer;
        uint32 SingularityTimer;
        uint32 blackHoleTimer;
        uint32 phase;
        uint32 phaseTimer;
        bool Visual2;
        GuidMapCD guidPlayerCD;

        void onReset(bool onSpawn)
        {
            phase = 0;
            phaseTimer = 7000; //inactivity before continuing
            DespawnTimer = 15000;
            SpellTimer = 300;
            SingularityTimer = 300;
            blackHoleTimer = 300;
            Visual2 = false;

            doCast((Unit*)NULL, SPELL_BLACKHOLE_SPAWN, true);
            doCast((Unit*)NULL, SPELL_BLACKHOLE_SPAWN2, true);
            me->addUnitState(UNIT_STAT_STUNNED);
            me->SetReactState(REACT_AGGRESSIVE);

            guidPlayerCD.clear();
            if(!pInstance)
                return;

            Map::PlayerList const& players = pInstance->instance->GetPlayers();
            if (!players.isEmpty())
            {
                for(Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                {
                    Player* plr = itr->getSource();
                    if (plr)
                        guidPlayerCD[plr->GetGUID()] = 0;
                }
            }
        }

        void onDeath(Unit* /*killer*/)
        {
            guidPlayerCD.clear();
        }

        bool getMessage(uint32 id, uint64 data)
        {
            uint32 time = 0;
            switch (id)
            {
                case 1:
                    time = guidPlayerCD[data];
                    if (time > 0)
                        return true;
                    break;
            }
            return false;
        }

        void update(const uint32 diff)
        {
            if (DespawnTimer <= diff)
                me->DisappearAndDie();
            else
                DespawnTimer -= diff;

            if (phaseTimer <= diff)
            {
                switch (phase)
                {
                    case 0:
                        me->RemoveAura(SPELL_BLACKHOLE_SPAWN2, 1);
                        doCast((Unit*)NULL, SPELL_BLACKHOLE_VISUAL2, true);
                        me->clearUnitState(UNIT_STAT_STUNNED);
                        setZoneInCombat(true);
                        phaseTimer = 300;
                        phase = 1;
                        break;
                    case 1:
                    {
                        Unit* Victim = selectUnit(SELECT_TARGET_RANDOM, 0, -15.0f, true);
                        if (Victim)
                        {
                            attackStart(Victim);
                            doModifyThreat(Victim, 1000000.0f);
                        }
                        else
                        {
                            Victim = selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true);
                            if (Victim)
                            {
                                attackStart(Victim);
                                doModifyThreat(Victim, 1000000.0f);
                            }
                        }
                        phaseTimer = 300;
                        phase = 2;
                        break;
                    }
                    case 2:
                        if (SpellTimer <= diff)
                        {
                            Map::PlayerList const& players = pInstance->instance->GetPlayers();
                            if (!players.isEmpty())
                            {
                                for(Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                                {
                                    Player* plr = itr->getSource();
                                    if (plr && me->GetDistance(plr) <= 7.0f)
                                    {
                                        if (guidPlayerCD[plr->GetGUID()] == 0)
                                        {
                                            doCast(plr, SPELL_BLACK_HOLE_EFFECT, false);
                                            guidPlayerCD[plr->GetGUID()] = 4000;
                                        }
                                    }
                                }
                            }
                            SpellTimer = 500;
                        }
                        else
                            SpellTimer -= diff;

                        if (SingularityTimer <= diff)
                        {
                            Map::PlayerList const& players = pInstance->instance->GetPlayers();
                            if (!players.isEmpty())
                            {
                                for(Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                                {
                                    Player* plr = itr->getSource();
                                    if (plr && me->GetDistance(plr) <= 5.0f)
                                        doCast(plr, SPELL_SINGULARITY, true);
                                }
                            }
                            SingularityTimer = 1000;
                        }
                        else
                            SingularityTimer -= diff;

                        if (!guidPlayerCD.empty())
                        {
                            for (GuidMapCD::const_iterator i = guidPlayerCD.begin(); i != guidPlayerCD.end(); ++i)
                            {
                            	if ((*i).second > 0)
                            	{
                                    if ((*i).second > diff)
                                        guidPlayerCD[(*i).first] = (*i).second - diff;
                                    else
                                    {
                                        guidPlayerCD[(*i).first] = 0;
                                        if (Player *plr = objmgr.GetPlayer((*i).first))
                                            plr->RemoveAurasDueToSpell(SPELL_BLACK_HOLE_EFFECT);
                                    }
                            	}
                            }
                        }

                        if (blackHoleTimer <= diff)
                        {
                            if (!guidPlayerCD.empty())
                            {
                                for (GuidMapCD::const_iterator i = guidPlayerCD.begin(); i != guidPlayerCD.end(); ++i)
                                {
                                    if ((*i).second > 0)
                                    {
                                        Player *plr = objmgr.GetPlayer((*i).first);
                                        float vcos, vsin;
                                        float angle = me->GetMap()->rand_norm()*2*M_PI;
                                        vcos = cos(angle);
                                        vsin = sin(angle);

                                        WorldPacket data(SMSG_MOVE_KNOCK_BACK, (8+4+4+4+4+4));
                                        data.append(plr->GetPackGUID());
                                        data << uint32(0);                                      // Sequence
                                        data << float(vcos);                                    // x direction
                                        data << float(vsin);                                    // y direction
                                        data << float(12.0f);                                   // Horizontal speed
                                        data << float(-9.0f);                                   // Z Movement speed

                                        plr->GetSession()->SendPacket(&data);
                                    }
                                }
                            }
                            blackHoleTimer = 800;
                        }
                        else
                            blackHoleTimer -= diff;

                        break;
                }
            }
            else
                phaseTimer -= diff;
        }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new npc_blackholeAI(creature);
    }
};

class boss_entropius : public CreatureScript
{
public:
    boss_entropius() : CreatureScript("boss_entropius") {}
	
    class boss_entropiusAI : public CreatureAINew
    {
        public:
        boss_entropiusAI(Creature* creature) : CreatureAINew(creature), Summons(me)
        {
            pInstance = ((ScriptedInstance*)creature->GetInstanceData());
        }

        ScriptedInstance* pInstance;
        SummonList Summons;

        uint32 BlackHoleSummonTimer;
        uint32 EnrageTimer;
        uint32 phaseTimer;
        uint32 phase;

        void onReset(bool onSpawn)
        {
            BlackHoleSummonTimer = 16000;
            EnrageTimer = 600000;
            phaseTimer = 1000;
            phase = 0;

            Summons.DespawnAll();
            if (!onSpawn)
            {
                if (Creature* muru = pInstance->instance->GetCreature(pInstance->GetData64(DATA_MURU)))
                    muru->AttackStop();

                if (pInstance)
                {
                    pInstance->SetData(DATA_MURU_EVENT, NOT_STARTED);
                    pInstance->SetData(DATA_MURU_TO_ENTROPIUS, NOT_STARTED);
                }
            }
            me->SetFullTauntImmunity(true);
            me->addUnitState(UNIT_STAT_STUNNED);
        }

        void onCombatStart(Unit * /*who*/)
        {
            if (pInstance)
                pInstance->SetData(DATA_MURU_EVENT, IN_PROGRESS);
        }
        
        void message(uint32 id, uint32 data)
        {
            if (id == 1)
                EnrageTimer = data;
        }

        void onSummon(Creature* summoned)
        {
            /* Need to get correct visual
            switch(summoned->GetEntry())
            {
                case CREATURE_DARK_FIENDS:
                    summoned->CastSpell(summoned,SPELL_DARKFIEND_VISUAL,false);
                    break;
            }
            */
            Summons.Summon(summoned);
        }

        void onSummonDespawn(Creature* unit)
        {
            Summons.Despawn(unit);
        }

        void onDeath(Unit* /*killer*/)
        {
            Summons.DespawnAll();

            if (pInstance)
                pInstance->SetData(DATA_MURU_EVENT, DONE);
        }

        void update(const uint32 diff)
        {
            if (phaseTimer <= diff)
            {
                switch (phase)
                {
                    case 0:
                        doCast(me, SPELL_ENTROPIUS_SPAWN, false);
                        phase = 1;
                        phaseTimer = 3000;
                        break;
                    case 1:
                        me->clearUnitState(UNIT_STAT_STUNNED);
                        setZoneInCombat(true);
                        attackStart(selectUnit(SELECT_TARGET_NEAREST, 0, 100.0f, true));
                        doCast((Unit*)NULL, SPELL_NEGATIVE_ENERGY_E, true);
                        phase = 2;
                        break;
                }
            }
            else
                phaseTimer -= diff;

            if (phase != 2)
                return;

            if (!updateVictim())
                return;

            if (EnrageTimer <= diff && !me->HasAura(SPELL_ENRAGE, 0))
            {
                doCast(me, SPELL_ENRAGE, false);
            } else EnrageTimer -= diff;

            if (BlackHoleSummonTimer <= diff)
            {
                BlackHoleSummonTimer = 15000;
                float px, py;
                float angleDegre = rand() % 360;
                float angle = angleDegre * (2*M_PI) / 360;
                float rayon = rand() % 25;
                px = 1816.25f + cos(angle) * rayon;
                py = 625.484f + sin(angle) * rayon;
                me->CastSpell(px, py, 71.0f, SPELL_DARKNESS_P2, false);

                Unit* random = selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true);
                if (!random)
                    return;
                doCast(random, SPELL_BLACKHOLE, false);
            } else BlackHoleSummonTimer -= diff;

            doMeleeAttackIfReady();
        }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new boss_entropiusAI(creature);
    }
};

class boss_muru : public CreatureScript
{
public:
    boss_muru() : CreatureScript("boss_muru") {}
	
    class boss_muruAI : public Creature_NoMovementAINew
    {
        public:
	boss_muruAI(Creature* creature) : Creature_NoMovementAINew(creature), Summons(me)
	{
	    pInstance = ((ScriptedInstance*)creature->GetInstanceData());
	}

        ScriptedInstance* pInstance;
        SummonList Summons;

        uint8 Phase;
        uint32 DarknessTimer;
        uint32 HumanoidesTimer;
        uint32 PhaseTimer;
        uint32 SentinelTimer;
        uint32 EnrageTimer;
        uint32 RespawnTimer;
        uint32 GateTimer;

        bool DarkFiend;
        bool Gate;
        bool EnrageTimerTransmitted;

        void onReset(bool onSpawn)
        {
            DarkFiend = false;
            Gate = false;
            EnrageTimerTransmitted = false;
            Phase = 0;

            if (onSpawn)
                RespawnTimer = 0;

            EnrageTimer = 600000;
            DarknessTimer = 45000;
            HumanoidesTimer = 10000;
            GateTimer = 5000;
            PhaseTimer = 0;
            SentinelTimer = 31500;

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            me->SetVisibility(VISIBILITY_ON);

            Summons.DespawnAll();

            if (pInstance)
            {
                if (pInstance->GetData(DATA_MURU_EVENT) != DONE)
                    pInstance->SetData(DATA_MURU_EVENT, NOT_STARTED);

                pInstance->SetData(DATA_MURU_TO_ENTROPIUS, NOT_STARTED);
            }

            if (Creature* entropius = pInstance->instance->GetCreature(pInstance->GetData64(DATA_ENTROPIUS)))
                entropius->DisappearAndDie();
        }

        void onCombatStart(Unit* /*who*/)
        {
            doCast((Unit*)NULL, SPELL_NEGATIVE_ENERGY, false);

            if (pInstance)
                pInstance->SetData(DATA_MURU_EVENT, IN_PROGRESS);
        }

        void onDamageTaken(Unit * /*done_by*/, uint32 &damage)
        {
            if (damage >= me->GetHealth() && Phase == 0)
            {
                damage = 0;
                Phase = 1;
            }

            if (Phase >= 1 && Phase < 7)
                damage = 0;
        }

        void onSummon(Creature* summoned)
        {
            /* Need to get correct visual
            switch(summoned->GetEntry())
            {
                case CREATURE_DARK_FIENDS:
                    summoned->CastSpell(summoned, SPELL_DARKFIEND_VISUAL, false);
                    break;
            }
            */
            Summons.Summon(summoned);
        }
	
        void onSummonDespawn(Creature* unit)
        {
            Summons.Despawn(unit);
        }

        void evade()
        {
            me->SetVisibility(VISIBILITY_OFF);
            RespawnTimer = 30000;
            me->SetReactState(REACT_PASSIVE);
            CreatureAINew::evade();
        }

        void update(const uint32 diff)
        {
            if (RespawnTimer)
            {
                if (RespawnTimer <= diff)
                {
                    me->SetVisibility(VISIBILITY_ON);
                    me->SetReactState(REACT_AGGRESSIVE);
                    RespawnTimer = 0;
                }
                else
                    RespawnTimer -= diff;
            }

            if (!updateVictim())
                return;

            if (pInstance->GetData(DATA_EREDAR_TWINS_EVENT) != DONE)
            {
                evade();
                return;
            }
                
            me->SetUInt64Value(UNIT_FIELD_TARGET, 0);

            if (me->hasUnitState(UNIT_STAT_CASTING))
                return;

            if (PhaseTimer <= diff)
            {
                switch (Phase)
                {
                    case 1:
                        me->RemoveAllAuras();
                        doCast(me, SPELL_OPEN_ALL_PORTALS, false);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        Phase = 2;
                        PhaseTimer = 3000;
                        break;
                    case 2:
                        me->RemoveAllAuras();
                        if (pInstance)
                            pInstance->SetData(DATA_MURU_TO_ENTROPIUS, IN_PROGRESS);
                        Phase = 3;
                        PhaseTimer = 3000;
                        break;
                    case 3:
                        doCast(me, SPELL_SUMMON_ENTROPIUS, false);
                        Phase = 4;
                        PhaseTimer = 2000;
                        break;
                    case 4:
                        me->SetVisibility(VISIBILITY_OFF);
                        Phase = 5;
                        PhaseTimer = 1000;
                        break;
                    case 5:
                        pInstance->SetData(DATA_MURU_TO_ENTROPIUS, NOT_STARTED);
                        Phase = 6;
                        PhaseTimer = 1000;
                        break;
                    case 6:
                        if (!pInstance)
                            return;

                        switch(pInstance->GetData(DATA_MURU_EVENT))
                        {
                            case NOT_STARTED:
                                onReset(false);
                                break;
                            case IN_PROGRESS:
                                if (!EnrageTimerTransmitted) {
                                    if (Creature* entropius = pInstance->instance->GetCreature(pInstance->GetData64(DATA_ENTROPIUS)))
                                    {
                                        entropius->getAI()->message(1, EnrageTimer);
                                        EnrageTimerTransmitted = true;
                                    }
                                }
                                break;
                            case DONE:
                                Phase = 7;
                                Summons.DespawnAll(true);
                                me->DisappearAndDie();
                                break;
                        }

                        PhaseTimer = 1000;                            
                        break;
                }
            }
            else
                PhaseTimer -= diff;

            if (EnrageTimer <= diff && !me->HasAura(SPELL_ENRAGE, 0))
                doCast(me, SPELL_ENRAGE, false);
            else
                EnrageTimer -= diff;

            if (Phase == 0)
            {
                if (DarknessTimer <= diff)
                {
                    if (!DarkFiend)
                    {
                        doCast((Unit*)NULL, SPELL_DARKNESS_P1, false);
                        DarknessTimer = 3000;
                        DarkFiend = true;
                    }
                    else
                    {
                        uint16 angleDegre = 0;
                        for (uint8 i = 0; i < 8; i++)
                        {
                            float px, py;
                            float angle = angleDegre * (2*M_PI) / 360;
                            float rayon = 12.0f;
                            px = me->GetPositionX() + cos(angle) * rayon;
                            py = me->GetPositionY() + sin(angle) * rayon;
                            Creature* crea = me->SummonCreature(CREATURE_DARK_FIENDS, px, py, DarkFiends[i][0], DarkFiends[i][1], TEMPSUMMON_MANUAL_DESPAWN, 0);
                            angleDegre = angleDegre + 45;
                        }
                        DarkFiend = false;
                        DarknessTimer = 42000;
                    }
                }
                else
                    DarknessTimer -= diff;

                if (HumanoidesTimer <= diff)
                {
                    for (uint8 i = 0; i < 3; ++i)
                        if (Creature* summon = me->SummonCreature(Humanoides[i][0],Humanoides[i][1] + ((2 * rand()%1000) / 1000.0f),Humanoides[i][2] + ((2 * rand()%1000) / 1000.0f),Humanoides[i][3], Humanoides[i][4], TEMPSUMMON_CORPSE_DESPAWN, 0))
                            summon->GetMotionMaster()->MovePoint(0, 1785.72f, 653.95f, 71.21f);

                    for (uint8 i = 3; i < 6; ++i)
                        if (Creature* summon = me->SummonCreature(Humanoides[i][0],Humanoides[i][1] + ((2 * rand()%1000) / 1000.0f),Humanoides[i][2] + ((2 * rand()%1000) / 1000.0f),Humanoides[i][3], Humanoides[i][4], TEMPSUMMON_CORPSE_DESPAWN, 0))
                            summon->GetMotionMaster()->MovePoint(0, 1844.83f, 601.82f, 71.30f);

                    HumanoidesTimer = 60000;
                }
                else
                    HumanoidesTimer -= diff;

                if (GateTimer <= diff)
                {
                    if (!Gate)
                    {
                        Gate = true;
                        if (pInstance)
                            pInstance->SetData(DATA_MURU_GATE_EVENT, 0);
                    }
                }
                else
                    GateTimer -= diff;

                if (SentinelTimer <= diff)
                {
                    doCast((Unit*)NULL, SPELL_OPEN_PORTAL_2, false);
                    SentinelTimer = 30000;
                }
                else
                    SentinelTimer -= diff;
            }
        }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new boss_muruAI(creature);
    }
};

class npc_muru_portal : public CreatureScript
{
public:
    npc_muru_portal() : CreatureScript("npc_muru_portal") {}
	
    class npc_muru_portalAI : public Creature_NoMovementAINew
    {
        public:
	npc_muru_portalAI(Creature* creature) : Creature_NoMovementAINew(creature)
	{
	    pInstance = ((ScriptedInstance*)creature->GetInstanceData());
	}

        ScriptedInstance* pInstance;

        bool SummonSentinel;
        bool InAction;
        bool switchState;

        uint32 SummonTimer;
        uint32 switchTimer;

        void onReset(bool onSpawn)
        {
            SummonTimer = 5000;
            switchTimer = 0;

            InAction = false;
            SummonSentinel = false;
            switchState = false;

            me->SetDisableGravity(true);
            me->addUnitState(UNIT_STAT_STUNNED);
        }

        void onHitBySpell(Unit* /*caster*/, const SpellEntry* Spell)
        {
            float x,y,z,o;
            me->GetHomePosition(x,y,z,o);
            doTeleportTo(x,y,z);
            InAction = true;
            switch(Spell->Id)
            {
                case SPELL_OPEN_ALL_PORTALS:
                    doCast((Unit*)NULL, SPELL_OPEN_PORTAL, false);
                    break;
                case SPELL_OPEN_PORTAL_2:
                    doCast((Unit*)NULL, SPELL_OPEN_PORTAL, false);
                    SummonSentinel = true;
                    break;
            }
        }

        void update(const uint32 diff)
        {
            if (!SummonSentinel)
            {
                if (!pInstance)
                    return;

                if (InAction && pInstance->GetData(DATA_MURU_EVENT) == NOT_STARTED)
                    onReset(false);

                if (pInstance->GetData(DATA_MURU_TO_ENTROPIUS) == IN_PROGRESS)
                {
                    if (switchTimer <= diff)
                    {
                        if (Creature* muru = pInstance->instance->GetCreature(pInstance->GetData64(DATA_MURU)))
                        {
                            doCast(muru, switchState ? 46178 : 46208, true);
                            switchState = 1 - switchState;
                        }
                        switchTimer = 500;
                   }
                    else
                        switchTimer -= diff;
                }

                return;
            }

            if (SummonTimer <= diff)
            {
                if (Creature* summoner = me->FindNearestCreature(CREATURE_SENTINAL_SUMMONER, 100.0f, true))
                    doCast(summoner, SPELL_SUMMON_VOID_SENTINEL_VISUAL, false);

                SummonTimer = 5000;
                SummonSentinel = false;
            }
            else
                SummonTimer -= diff;
        }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new npc_muru_portalAI(creature);
    }
};

class npc_sentinal_summoner : public CreatureScript
{
public:
    npc_sentinal_summoner() : CreatureScript("npc_sentinal_summoner") {}

    class npc_sentinal_summonerAI : public CreatureAINew
    {
        public:
        npc_sentinal_summonerAI(Creature* creature) : CreatureAINew(creature), Summons(me)
        {
            pInstance = ((ScriptedInstance*)creature->GetInstanceData());
        }

        ScriptedInstance* pInstance;
        SummonList Summons;

        void onReset(bool onSpawn)
        {
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

        void onHitBySpell(Unit* /*caster*/, const SpellEntry* Spell)
        {
            switch(Spell->Id)
            {
                case SPELL_SUMMON_VOID_SENTINEL_VISUAL:
                    doCast(me, SPELL_SUMMON_VOID_SENTINEL, false);
                    break;
            }
        }

        void update(const uint32 diff)
        {
            me->SetReactState(REACT_PASSIVE);

            if (pInstance && pInstance->GetData(DATA_MURU_EVENT) == NOT_STARTED)
                if (!Summons.IsEmpty())
                    Summons.DespawnAll();
            else if (pInstance && pInstance->GetData(DATA_MURU_EVENT) == DONE)
                if (!Summons.IsEmpty())
                    Summons.DespawnAll();
        }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new npc_sentinal_summonerAI(creature);
    }
};

//P2 darkness summoned npc
class npc_darkness : public CreatureScript
{
public:
    npc_darkness() : CreatureScript("npc_darkness") {}

    class npc_darknessAI : public CreatureAINew
    {
        public:
        npc_darknessAI(Creature* creature) : CreatureAINew(creature)
        {
            pInstance = ((ScriptedInstance*)creature->GetInstanceData());
            me->SetFloatValue(UNIT_FIELD_COMBATREACH, 0 ); //fix aoe radius
        }

        ScriptedInstance* pInstance;

        uint32 WaitTimer;
        uint32 DarknessTimer;
        bool Spawned;

        void onReset(bool onSpawn)
        {
            WaitTimer = 3000;
            //DarknessTimer = 3000;
            bool Spawned = false;
            me->addUnitState(UNIT_STAT_STUNNED);
            //doCast(me,SPELL_VOID_ZONE_PERIODIC,true); //already done via db
            
            //setZoneInCombat(true);
        }

        void update(const uint32 diff)
        {
            /*
            if (DarknessTimer <= diff)
            {
                std::list<Unit*> players;
                players.clear();
                selectUnitList(players, 25, SELECT_TARGET_RANDOM, 3.0f, true);
                for (std::list<Unit*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                {
                    Player* plr = (*itr)->ToPlayer();
                    if (plr && !plr->HasAura(45996))
                    {
                        bool isBumping = false;
                        if (Creature* blackHole = pInstance->instance->GetCreature(pInstance->GetData64(DATA_BLACK_HOLE)))
                            if (blackHole->getAI())
                                if (blackHole->getAI()->getMessage(1, plr->GetGUID()))
                                    isBumping = true;

                        if (!isBumping)
                        {
                            SpellEntry const *spellInfo = spellmgr.LookupSpell(45996);
                            if (spellInfo)
                            {
                                for (uint8 i = 0; i < 3 ; ++i)
                                {
                                    uint8 eff = spellInfo->Effect[i];
                                    if (eff>=TOTAL_SPELL_EFFECTS)
                                        continue;

                                    if (IsAreaAuraEffect(eff)
                                        || eff == SPELL_EFFECT_APPLY_AURA
                                        || eff == SPELL_EFFECT_PERSISTENT_AREA_AURA)
                                    {
                                        Aura* Aur = CreateAura(spellInfo, i, NULL, plr);
                                        plr->AddAura(Aur);
                                    }
                                }
                            }
                        }
                    }
                }
                DarknessTimer = 500;
            }
            else
                DarknessTimer -= diff;
            */
            if (!Spawned)
            {
                if (WaitTimer <= diff)
                {
                    if (Creature* entropius = pInstance->instance->GetCreature(pInstance->GetData64(DATA_ENTROPIUS)))
                        entropius->SummonCreature(CREATURE_DARK_FIENDS, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_CORPSE_DESPAWN, 0);

                    Spawned = true;
                }
                else
                    WaitTimer -= diff;
            }
        }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new npc_darknessAI(creature);
    }
};
                
class npc_dark_fiend : public CreatureScript
{
public:
    npc_dark_fiend() : CreatureScript("npc_dark_fiend") {}
	
    class npc_dark_fiendAI : public CreatureAINew
    {
        public:
        npc_dark_fiendAI(Creature* creature) : CreatureAINew(creature) {}

        uint32 phase;
        uint32 phaseTimer;

        void onDeath(Unit* killer)
        {
            doCast(me,SPELL_DARKFIEND_DEATH_VISUAL,true); //Visuel effect on death
        }

        void onReset(bool /*onSpawn*/)
        {
            phase = 0;
            phaseTimer = 2000;

            me->addUnitState(UNIT_STAT_STUNNED);
        }

        void onHitBySpell(Unit* /*caster*/, const SpellEntry* Spell)
        {
            for (uint8 i = 0; i < 3; ++i)
                if (Spell->Effect[i] == 38) {
                    me->DisappearAndDie();
                }
        }

        void update(const uint32 diff)
        {
            if (phaseTimer <= diff)
            {
                switch (phase)
                {
                    case 0:
                        me->clearUnitState(UNIT_STAT_STUNNED);
                        doCast((Unit*)NULL, SPELL_DARKFIEND_SKIN, false);
                        setZoneInCombat(true);
                        attackStart(selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true));
                        phase = 1;
                        phaseTimer = 500;
                        break;
                    case 1:
                        phaseTimer = 500;

                        if (!updateVictim())
                            return;

                        if (me->GetDistance(me->getVictim()) < 5)
                        {
                            if (Creature* trigger = me->SummonCreature(WORLD_TRIGGER, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 12000))
                            {
                                trigger->setFaction(16);
                                trigger->SetName("Sombre fiel");
                                trigger->CastSpell(trigger, SPELL_DARKFIEND_AOE, false);
                            }
                            me->DisappearAndDie();
                        }
                        break;
                }
            }
            else
                phaseTimer -= diff;
        }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new npc_dark_fiendAI(creature);
    }
};

class npc_void_sentinel : public CreatureScript
{
    public:
    npc_void_sentinel() : CreatureScript("npc_void_sentinel") {}

    class npc_void_sentinelAI : public CreatureAINew
    {
        public:
	npc_void_sentinelAI(Creature* creature) : CreatureAINew(creature)
        {
            pInstance = ((ScriptedInstance*)creature->GetInstanceData());
        }

        ScriptedInstance* pInstance;

        uint32 PulseTimer;
        uint32 VoidBlastTimer;
        uint32 phase;
        uint32 phaseTimer;

        void onReset(bool /*onSpawn*/)
        {
            phase = 0;
            phaseTimer = 3000;
            PulseTimer = 3000;
            VoidBlastTimer = 10000 + rand()%10000; //is this a correct timer?

            me->SetFullTauntImmunity(true);

            me->addUnitState(UNIT_STAT_STUNNED);
            me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_HASTE_SPELLS, true);
        }

        void onDeath(Unit* killer)
        {
            for (uint8 i = 0; i < 6; ++i)
            {
                me->CastSpell(me->GetPositionX() + ((2 * rand()%1000) / 1000.0f), me->GetPositionY() + ((2 * rand()%1000) / 1000.0f), me->GetPositionZ(), SPELL_SUMMON_VOID_SPAWN, false);
            }
        }

        void update(const uint32 diff)
        {
            if (phaseTimer <= diff)
            {
                switch (phase)
                {
                    case 0:
                        me->clearUnitState(UNIT_STAT_STUNNED);
                        setZoneInCombat(true);
                        attackStart(selectUnit(SELECT_TARGET_NEAREST, 0, 100.0f, true));
                        phase = 1;
                        break;
                }
            }
            else
                phaseTimer -= diff;

            if (phase != 1)
                return;

            if (!updateVictim())
                return;

            if (PulseTimer <= diff)
            {
                doCast((Unit*)NULL, SPELL_SHADOW_PULSE, true);
                PulseTimer = 3000;
            }
            else
                PulseTimer -= diff;

            if (VoidBlastTimer <= diff)
            {
                doCast(me->getVictim(), SPELL_VOID_BLAST, false);
                VoidBlastTimer = 20000;
            }
            else
                VoidBlastTimer -= diff;

            doMeleeAttackIfReady();
        }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new npc_void_sentinelAI(creature);
    }
};

class npc_void_spawn : public CreatureScript
{
    public:
    npc_void_spawn() : CreatureScript("npc_void_spawn") {}

    class npc_void_spawnAI : public CreatureAINew
    {
        public:
	npc_void_spawnAI(Creature* creature) : CreatureAINew(creature)
        {
            pInstance = ((ScriptedInstance*)creature->GetInstanceData());
        }

        ScriptedInstance* pInstance;

        uint32 ShadowBoltVolleyTimer;
        uint32 phase;
        uint32 phaseTimer;

        void onReset(bool /*onSpawn*/)
        {
            phase = 0;
            phaseTimer = 2000;
            ShadowBoltVolleyTimer = urand(2000, 10000);
            me->addUnitState(UNIT_STAT_STUNNED);
        }

        void updateEM(const uint32 diff)
        {
            if (pInstance->GetData(DATA_MURU_EVENT) == NOT_STARTED)
                me->DisappearAndDie();
        }

        void update(const uint32 diff)
        {
            if (pInstance->GetData(DATA_MURU_EVENT) == NOT_STARTED)
                me->DisappearAndDie();

            if (phaseTimer <= diff)
            {
                switch (phase)
                {
                    case 0:
                        me->clearUnitState(UNIT_STAT_STUNNED);
                        setZoneInCombat(true);
                        attackStart(selectUnit(SELECT_TARGET_NEAREST, 0, 100.0f, true));
                        phase = 1;
                        break;
                }
            }
            else
                phaseTimer -= diff;

            if (phase != 1)
                return;

            if (!updateVictim())
                return;

            if (ShadowBoltVolleyTimer <= diff)
            {
                doCast((Unit*)NULL, SPELL_SHADOW_BOLT_VOLLEY, false);
                ShadowBoltVolleyTimer = 5000;
            }
            else
                ShadowBoltVolleyTimer -= diff;

            doMeleeAttackIfReady();
        }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new npc_void_spawnAI(creature);
    }
};

class npc_berserker : public CreatureScript
{
    public:
    npc_berserker() : CreatureScript("npc_berserker") {}

    class npc_berserkerAI : public CreatureAINew
    {
        public:
	npc_berserkerAI(Creature* creature) : CreatureAINew(creature)
        {
            pInstance = ((ScriptedInstance*)creature->GetInstanceData());
        }

        ScriptedInstance* pInstance;

        uint32 FuryTimer;
        uint32 tempTimer;

        void onReset(bool /*onSpawn*/)
        {
            FuryTimer = 20000;
            tempTimer = 11000;
            me->RemoveUnitMovementFlag(0x00000100/*MOVEMENTFLAG_WALKING*/);
        }

        void onMovementInform(uint32 type, uint32 id)
        {
            if (type == POINT_MOTION_TYPE)
            {
                switch (id)
                {
                    case 0:
                        setZoneInCombat(true);
                        attackStart(selectUnit(SELECT_TARGET_NEAREST, 0, 100.0f, true));
                        break;
                }
            }
        }

        void update(const uint32 diff)
        {
            if (tempTimer)
            {
                if (tempTimer <= diff)
                {
                    setZoneInCombat(true);
                    if (Unit* target = selectUnit(SELECT_TARGET_NEAREST, 0, 100.0f, true))
                    {
                        attackStart(target);
                        tempTimer = 0;
                    }
                    else
                        tempTimer = 500;
                }
                else
                    tempTimer -= diff;
            }

            if (!updateVictim())
                return;

            if (FuryTimer <= diff)
            {
                if (!me->HasAura(SPELL_FLURRY))
                {
                    me->InterruptNonMeleeSpells(false);
                    doCast(me, SPELL_FLURRY, false);
                }

                FuryTimer = urand(20000, 35000);
            }
            else
                FuryTimer -= diff;

            doMeleeAttackIfReady();
        }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new npc_berserkerAI(creature);
    }
};

class npc_mage : public CreatureScript
{
    public:
    npc_mage() : CreatureScript("npc_mage") {}

    class npc_mageAI : public CreatureAINew
    {
        public:
	npc_mageAI(Creature* creature) : CreatureAINew(creature)
        {
            pInstance = ((ScriptedInstance*)creature->GetInstanceData());
        }

        ScriptedInstance* pInstance;

        uint32 FuryTimer;
        uint32 FelFireballTimer;
        uint32 tempTimer;

        void onReset(bool /*onSpawn*/)
        {
            FuryTimer = 25000;
            FelFireballTimer = urand(2000, 3000);
            tempTimer = 11000;
            me->RemoveUnitMovementFlag(0x00000100/*MOVEMENTFLAG_WALKING*/);
        }

        void onMovementInform(uint32 type, uint32 id)
        {
            if (type == POINT_MOTION_TYPE)
            {
                switch (id)
                {
                    case 0:
                        setZoneInCombat(true);
                        attackStart(selectUnit(SELECT_TARGET_NEAREST, 0, 100.0f, true));
                        break;
                }
            }
        }

        void update(const uint32 diff)
        {
            if (tempTimer)
            {
                if (tempTimer <= diff)
                {
                    setZoneInCombat(true);
                    if (Unit* target = selectUnit(SELECT_TARGET_NEAREST, 0, 100.0f, true))
                    {
                        attackStart(target);
                        tempTimer = 0;
                    }
                    else
                        tempTimer = 500;
                }
                else
                    tempTimer -= diff;
            }
                    
            if (!updateVictim())
               return;

            if (FuryTimer <= diff)
            {
                if (!me->HasAura(SPELL_SPELL_FURY))
                {
                    me->InterruptNonMeleeSpells(false);
                    doCast(me, SPELL_SPELL_FURY, false);
                }

                FuryTimer = urand(45000, 55000);
            }
            else
                FuryTimer -= diff;

            if (FelFireballTimer <= diff)
            {
                doCast(me->getVictim(), SPELL_FEL_FIREBALL, false);

                FelFireballTimer = urand(2000, 3000);
            }
            else
                FelFireballTimer -= diff;

            doMeleeAttackIfReady();
        }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new npc_mageAI(creature);
    }
};

void AddSC_boss_muru()
{
    sScriptMgr.addScript(new boss_entropius());
    sScriptMgr.addScript(new boss_muru());
    sScriptMgr.addScript(new npc_muru_portal());
    sScriptMgr.addScript(new npc_sentinal_summoner());
    sScriptMgr.addScript(new npc_dark_fiend());
    sScriptMgr.addScript(new npc_darkness());
    sScriptMgr.addScript(new npc_void_sentinel());
    sScriptMgr.addScript(new npc_void_spawn());
    sScriptMgr.addScript(new npc_blackhole());
    sScriptMgr.addScript(new npc_berserker());
    sScriptMgr.addScript(new npc_mage());
}
