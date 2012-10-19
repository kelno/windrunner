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

// Muru & Entropius's spells
enum Spells
{
    SPELL_ENRAGE                = 26662,

    // Muru's spells
    SPELL_NEGATIVE_ENERGY       = 46009, //(this trigger 46008)
    SPELL_DARKNESS              = 45999,
    SPELL_OPEN_ALL_PORTALS      = 46177,
    SPELL_OPEN_PORTAL           = 45977,
    SPELL_OPEN_PORTAL_2         = 45976,
    SPELL_SUMMON_BERSERKER      = 46037,
    SPELL_SUMNON_FURY_MAGE      = 46038,
    SPELL_SUMMON_VOID_SENTINEL  = 45988,
    SPELL_SUMMON_ENTROPIUS      = 46217,

    // Entropius's spells
    SPELL_DARKNESS_E            = 46268,
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

    // Void Spawn's spells
    SPELL_SHADOW_BOLT_VOLLEY    = 46082,

    //Dark Fiend Spells
    SPELL_DARKFIEND_AOE         = 45944,
    SPELL_DARKFIEND_VISUAL      = 45936,
    SPELL_DARKFIEND_SKIN        = 45934,

    //Black Hole Spells
    SPELL_BLACKHOLE_SPAWN       = 46242,
    SPELL_BLACKHOLE_SPAWN2      = 46247,
    SPELL_BLACKHOLE_VISUAL2     = 46235,
    SPELL_BLACKHOLE_GROW        = 46228,
    SPELl_BLACK_HOLE_EFFECT     = 46230,
    SPELL_SINGULARITY           = 46238
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

        void onReset(bool onSpawn)
        {
            BlackHoleSummonTimer = 27000;
            EnrageTimer = 600000;

            Summons.DespawnAll();
            if (!onSpawn)
            {
                if (Creature* muru = pInstance->instance->GetCreature(pInstance->GetData64(DATA_MURU)))
                    muru->AttackStop();

                if (pInstance)
                    pInstance->SetData(DATA_MURU_EVENT, NOT_STARTED);
            }
            me->SetFullTauntImmunity(true);
        }

        void onCombatStart(Unit * /*who*/)
        {
            doCast((Unit*)NULL, SPELL_NEGATIVE_ENERGY_E, true);
            doCast(me, SPELL_ENTROPIUS_SPAWN, false);

            if (pInstance)
                pInstance->SetData(DATA_MURU_EVENT, IN_PROGRESS);
        }

        void onSummon(Creature* summoned)
        {
            switch(summoned->GetEntry())
            {
                case CREATURE_DARK_FIENDS:
                    summoned->CastSpell(summoned,SPELL_DARKFIEND_VISUAL,false);
                    break;
            }
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
                me->CastSpell(px, py, 71.0f, SPELL_DARKNESS_E, false);

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

        void onReset(bool onSpawn)
        {
            DarkFiend = false;
            Gate = false;
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

                if (pInstance->GetData(DATA_EREDAR_TWINS_EVENT) != DONE)
                    me->SetReactState(REACT_PASSIVE);
            }
        }

        void onCombatStart(Unit* /*who*/)
        {
            doCast((Unit*)NULL, SPELL_NEGATIVE_ENERGY, false);

            if (pInstance) {
                pInstance->SendScriptInTestNoLootMessageToAll();
                pInstance->SetData(DATA_MURU_EVENT, IN_PROGRESS);
            }
        }

        void onDamageTaken(Unit * /*done_by*/, uint32 &damage)
        {
            if (damage >= me->GetHealth() && Phase == 0)
            {
                damage = 0;
                Phase = 1;
            }

            if (Phase > 1 && Phase < 4)
                damage = 0;
        }

        void onSummon(Creature* summoned)
        {
            switch(summoned->GetEntry())
            {
                case CREATURE_DARK_FIENDS:
                    summoned->CastSpell(summoned, SPELL_DARKFIEND_VISUAL, false);
                    break;
            }
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
                
            me->SetUInt64Value(UNIT_FIELD_TARGET, 0);

            if (me->hasUnitState(UNIT_STAT_CASTING))
                return;

            if (Phase != 0)
            {
                if (PhaseTimer <= diff)
                {
                    switch (Phase)
                    {
                        case 1:
                            me->RemoveAllAuras();
                            doCast(me, SPELL_OPEN_ALL_PORTALS, false);
                            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            Phase = 2;
                            PhaseTimer = 2000;
                            break;
                        case 2:
                            me->RemoveAllAuras();
                            doCast(me, SPELL_SUMMON_ENTROPIUS, false);
                            me->SetVisibility(VISIBILITY_OFF);
                            Phase = 3;
                            PhaseTimer = 3000;
                            break;
                        case 3:
                            if (!pInstance)
                                return;

                            switch(pInstance->GetData(DATA_MURU_EVENT))
                            {
                                case NOT_STARTED:
                                    onReset(false);
                                    break;
                                case DONE:
                                    Phase = 4;
                                    Summons.DespawnAll(true);
                                    me->DisappearAndDie();
                                    break;
                            }
                            
                            PhaseTimer = 3000;                            
                            break;
                    }
                }
                else
                    PhaseTimer -= diff;
            }

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
                        doCast((Unit*)NULL, SPELL_DARKNESS, false);
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
                        if (Creature* summon = me->SummonCreature(Humanoides[i][0],Humanoides[i][1],Humanoides[i][2],Humanoides[i][3], Humanoides[i][4], TEMPSUMMON_CORPSE_DESPAWN, 0))
                            summon->GetMotionMaster()->MovePoint(0, 1785.72f, 653.95f, 71.21f);

                    for (uint8 i = 3; i < 6; ++i)
                        if (Creature* summon = me->SummonCreature(Humanoides[i][0],Humanoides[i][1],Humanoides[i][2],Humanoides[i][3], Humanoides[i][4], TEMPSUMMON_CORPSE_DESPAWN, 0))
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
	npc_muru_portalAI(Creature* creature) : Creature_NoMovementAINew(creature), Summons(me)
	{
	    pInstance = ((ScriptedInstance*)creature->GetInstanceData());
	}

        ScriptedInstance* pInstance;

        SummonList Summons;

        bool SummonSentinel;
        bool InAction;

        uint32 SummonTimer;

        void onReset(bool onSpawn)
        {
            SummonTimer = 5000;

            InAction = false;
            SummonSentinel = false;

            me->addUnitState(UNIT_STAT_STUNNED);

            Summons.DespawnAll();
        }

        void onSummon(Creature* summoned)
        {
            if (pInstance)
            {
                if (Creature* muru = pInstance->instance->GetCreature(pInstance->GetData64(DATA_MURU)))
                {
                    if (summoned->getAI()) // FIXME: Hack because getAI() may not be initialized and there is no fallback like in old CreatureAI system.
                        summoned->getAI()->attackStart(muru->getAI()->selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true));
                    else
                        summoned->AI()->AttackStart(muru->getAI()->selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true));
                }
            }

            Summons.Summon(summoned);
        }

        void onSummonDespawn(Creature* unit)
        {
            Summons.Despawn(unit);
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
                if (InAction && pInstance && pInstance->GetData(DATA_MURU_EVENT) == NOT_STARTED)
                    onReset(false);
                else if (pInstance && pInstance->GetData(DATA_MURU_EVENT) == DONE)
                    Summons.DespawnAll();
                return;
            }
            if (SummonTimer <= diff)
            {
                doCast((Unit*)NULL, SPELL_SUMMON_VOID_SENTINEL, false);
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
        }

        ScriptedInstance* pInstance;

        uint32 WaitTimer;
        uint32 DarknessTimer;
        bool Spawned;

        void onReset(bool onSpawn)
        {
            WaitTimer = 3000;
            DarknessTimer = 3000;
            bool Spawned = false;
            me->addUnitState(UNIT_STAT_STUNNED);
            
            // Transfert threat list
            if (onSpawn) {
                if (Creature* entropius = pInstance->instance->GetCreature(pInstance->GetData64(DATA_ENTROPIUS))) {
                    std::list<HostilReference*>::iterator itr;
                    for (itr = entropius->getThreatManager().getThreatList().begin(); itr != entropius->getThreatManager().getThreatList().end(); ++itr) {
                        Unit* unit = Unit::GetUnit(*me, (*itr)->getUnitGuid());
                        if (unit)
                            me->AddThreat(unit, 1.0f);
                    }
                }
            }
        }

        void update(const uint32 diff)
        {
            if (DarknessTimer <= diff)
            {
                std::list<Unit*> players;
                players.clear();
                selectUnitList(players, 25, SELECT_TARGET_RANDOM, 5.0f, true);
                for (std::list<Unit*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                {
                    Player* plr = (*itr)->ToPlayer();
                    if (plr && !plr->HasAura(45996)) {
                        SpellEntry const *spellInfo = spellmgr.LookupSpell(45996);
                        if (spellInfo) {
                            for (uint8 i = 0; i < 3 ; ++i) {
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
                DarknessTimer = 500;
            }
            else
                DarknessTimer -= diff;

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

        uint32 WaitTimer;
        bool InAction;

        void onReset(bool /*onSpawn*/)
        {
            WaitTimer = 2000;
            InAction = false;

            me->addUnitState(UNIT_STAT_STUNNED);
        }

        void onHitBySpell(Unit* /*caster*/, const SpellEntry* Spell)
        {
            for (uint8 i = 0; i < 3; ++i)
                if (Spell->Effect[i] == 38) {
                    doCast(me, 45936, true);
                    me->DisappearAndDie();
                }
        }

        void update(const uint32 diff)
        {
            if (WaitTimer <= diff)
            {
                if (!InAction)
                {
                    me->clearUnitState(UNIT_STAT_STUNNED);
                    doCast((Unit*)NULL, SPELL_DARKFIEND_SKIN, false);
                    setZoneInCombat(true);
                    attackStart(selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true));
                    InAction = true;
                    WaitTimer = 500;
                }
                else
                {
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
                    WaitTimer = 500;
                }
            }
            else
                WaitTimer -= diff;
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
	npc_void_sentinelAI(Creature* creature) : CreatureAINew(creature) {}

        uint32 PulseTimer;
        uint32 VoidBlastTimer;
        uint32 StartActiveTimer;

        void onReset(bool /*onSpawn*/)
        {
            PulseTimer = 10000 + rand()%10000;
            VoidBlastTimer = 45000; //is this a correct timer?
            StartActiveTimer = 500;

            float x,y,z,o;
            me->GetHomePosition(x,y,z,o);
            doTeleportTo(x,y,71);

            me->SetFullTauntImmunity(true);
        }

        void onDeath(Unit* killer)
        {
            for (uint8 i = 0; i < 8; ++i)
            {
                if (Creature* spawn = me->SummonCreature(CREATURE_VOID_SPAWN, me->GetPositionX(),me->GetPositionY(),me->GetPositionZ(), rand()%6, TEMPSUMMON_CORPSE_DESPAWN, 0))
                {
                    if (spawn->getAI())
                        spawn->getAI()->attackStart(killer);
                    else
                        spawn->AI()->AttackStart(killer);
                }
            }
        }

        void update(const uint32 diff)
        {
            if (StartActiveTimer >= diff) {
                StartActiveTimer -= diff;
                return;
            }
            
            if (!updateVictim())
                return;

            if (PulseTimer <= diff)
            {
                doCast((Unit*)NULL, SPELL_SHADOW_PULSE, true);
                PulseTimer = 20000;
            }
            else
                PulseTimer -= diff;

            if (VoidBlastTimer <= diff)
            {
                doCast(me->getVictim(), SPELL_VOID_BLAST, false);
                VoidBlastTimer = 45000;
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

        void onReset(bool /*onSpawn*/)
        {
            ShadowBoltVolleyTimer = 5000;
        }

        void update(const uint32 diff)
        {
            if (pInstance && pInstance->GetData(DATA_MURU_EVENT) == NOT_STARTED)
            {
                me->SetHomePosition(me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), 0.0f);
                me->DisappearAndDie();
            }

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
        bool Visual2;

        void onReset(bool onSpawn)
        {
            DespawnTimer = 15000;
            SpellTimer = 5000;
            SingularityTimer = 6000;
            Visual2 = false;

            doCast((Unit*)NULL, SPELL_BLACKHOLE_SPAWN, true);
            doCast((Unit*)NULL, SPELL_BLACKHOLE_SPAWN2, true);
            me->addUnitState(UNIT_STAT_STUNNED);
        }

        void update(const uint32 diff)
        {
            if (DespawnTimer <= diff)
                me->DisappearAndDie();
            else
                DespawnTimer -= diff;

            if (SpellTimer <= diff)
            {
                if (!Visual2)
                {
                    Visual2 = true;
                    me->RemoveAura(SPELL_BLACKHOLE_SPAWN2, 1);
                    doCast((Unit*)NULL, SPELL_BLACKHOLE_VISUAL2, true);
                    me->clearUnitState(UNIT_STAT_STUNNED);
                    setZoneInCombat(true);
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
                }
                std::list<Unit*> players;
                players.clear();
                selectUnitList(players, 25, SELECT_TARGET_RANDOM, 5.0f, true);
                for (std::list<Unit*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    doCast((*itr), SPELl_BLACK_HOLE_EFFECT, true);

                SpellTimer = 300;
            }
            else
                SpellTimer -= diff;

            if (SingularityTimer <= diff)
            {
                std::list<Unit*> players;
                players.clear();
                selectUnitList(players, 25, SELECT_TARGET_RANDOM, 5.0f, true);
                for (std::list<Unit*>::const_iterator itr = players.begin(); itr != players.end(); ++itr)
                    doCast((*itr), SPELL_SINGULARITY, true);

                SingularityTimer = 1000;
            }
            else
                SingularityTimer -= diff;
        }
    };

    CreatureAINew* getAI(Creature* creature)
    {
        return new npc_blackholeAI(creature);
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
        uint32 TempTimer;
        uint32 Phase;
        uint32 HackTimer;

        void onReset(bool /*onSpawn*/)
        {
            FuryTimer = 20000;
            TempTimer = 1000;
            Phase = 0;
            HackTimer = 11000;
            me->RemoveUnitMovementFlag(0x00000100/*MOVEMENTFLAG_WALKING*/);
        }

        void onMovementInform(uint32 type, uint32 id)
        {
            if (type == POINT_MOTION_TYPE)
                if (id == 0)
                    Phase = 1;
        }

        void update(const uint32 diff)
        {
            switch (Phase)
            {
                case 0:
                    // If mob aggro before end of MovePoint
                    if (HackTimer <= diff)
                    {
                        Phase = 1;
                        me->SetReactState(REACT_AGGRESSIVE);
                        if (Creature* muru = pInstance->instance->GetCreature(pInstance->GetData64(DATA_MURU)))
                            attackStart(muru->getAI()->selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true));
                    }
                    else {
                        HackTimer -= diff;
                        me->SetReactState(REACT_PASSIVE);
                    }
                    break;
                case 1:
                    if (TempTimer <= diff)
                    {
                        Phase = 2;
                        me->SetReactState(REACT_AGGRESSIVE);
                        if (Creature* muru = pInstance->instance->GetCreature(pInstance->GetData64(DATA_MURU)))
                            attackStart(muru->getAI()->selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true));
                    }
                    else {
                        TempTimer -= diff;
                        me->SetReactState(REACT_PASSIVE);
                    }
                    break;
                case 2:
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
                    break;
            }
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
        uint32 TempTimer;
        uint32 Phase;
        uint32 HackTimer;

        void onReset(bool /*onSpawn*/)
        {
            FuryTimer = 25000;
            FelFireballTimer = urand(2000, 3000);
            TempTimer = 1000;
            Phase = 0;
            HackTimer = 11000;
            me->RemoveUnitMovementFlag(0x00000100/*MOVEMENTFLAG_WALKING*/);
        }

        void onMovementInform(uint32 type, uint32 id)
        {
            if (type == POINT_MOTION_TYPE)
                if (id == 0)
                    Phase = 1;
        }

        void update(const uint32 diff)
        {
            switch (Phase)
            {
                case 0:
                    // If mob aggro before end of MovePoint
                    if (HackTimer <= diff)
                    {
                        Phase = 1;
                        me->SetReactState(REACT_AGGRESSIVE);
                        if (Creature* muru = pInstance->instance->GetCreature(pInstance->GetData64(DATA_MURU)))
                            attackStart(muru->getAI()->selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true));
                    }
                    else {
                        HackTimer -= diff;
                        me->SetReactState(REACT_PASSIVE);
                    }
                    break;
                case 1:
                    if (TempTimer <= diff)
                    {
                        Phase = 2;
                        me->SetReactState(REACT_AGGRESSIVE);
                        if (Creature* muru = pInstance->instance->GetCreature(pInstance->GetData64(DATA_MURU)))
                            attackStart(muru->getAI()->selectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true));
                    }
                    else {
                        TempTimer -= diff;
                        me->SetReactState(REACT_PASSIVE);
                    }
                    break;
                case 2:
                    if (!updateVictim())
                       return;
                       
                    if (me->GetDistance(me->getVictim()) <= 8.0f)
                        me->GetMotionMaster()->MoveIdle();
                    //else if (me->GetDistance(me->getVictim()) >= 10.0f)
                      //  me->GetMotionMaster()->MoveChase(me->getVictim());

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
                    break;
            }
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
    sScriptMgr.addScript(new npc_dark_fiend());
    sScriptMgr.addScript(new npc_darkness());
    sScriptMgr.addScript(new npc_void_sentinel());
    sScriptMgr.addScript(new npc_void_spawn());
    sScriptMgr.addScript(new npc_blackhole());
    sScriptMgr.addScript(new npc_berserker());
    sScriptMgr.addScript(new npc_mage());
}
