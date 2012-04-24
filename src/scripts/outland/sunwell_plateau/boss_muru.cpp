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
    SPELL_NEGATIVE_ENERGY        = 46009, //(this trigger 46008)
    SPELL_DARKNESS                = 45999,
    SPELL_OPEN_ALL_PORTALS        = 46177,
    SPELL_OPEN_PORTAL            = 45977,
    SPELL_OPEN_PORTAL_2            = 45976,
    SPELL_SUMMON_BERSERKER        = 46037,
    SPELL_SUMNON_FURY_MAGE        = 46038,
    SPELL_SUMMON_VOID_SENTINEL    = 45988,
    SPELL_SUMMON_ENTROPIUS        = 46217,

    // Entropius's spells
    SPELL_DARKNESS_E            = 46269,
    SPELL_BLACKHOLE             = 46282,
    SPELL_NEGATIVE_ENERGY_E     = 46284,
    SPELL_ENTROPIUS_SPAWN        = 46223,

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
    SPELL_DARKFIEND_AOE            = 45944,
    SPELL_DARKFIEND_VISUAL        = 45936,
    SPELL_DARKFIEND_SKIN        = 45934,

    //Black Hole Spells
    SPELL_BLACKHOLE_SPAWN        = 46242,
    SPELL_BLACKHOLE_GROW        = 46228
};

enum BossTimers{
    TIMER_DARKNESS                = 0,
    TIMER_HUMANOIDES            = 1,
    TIMER_PHASE                    = 2,
    TIMER_SENTINEL                = 3
};

float DarkFiends[8][4] =
{
    {1819.9f,    609.80f,    69.74f,    1.94f},
    {1829.39f,   617.89f,    69.73f,    2.61f},
    {1801.98f,   633.62f,    69.74f,    5.71f},
    {1830.88f,   629.99f,    69.73f,    3.52f},
    {1800.38f,   621.41f,    69.74f,    0.22f},
    {1808.3f ,   612.45f,    69.73f,    1.02f},
    {1823.9f ,   639.69f,    69.74f,    4.12f},
    {1811.85f,   640.46f,    69.73f,    4.97f}
};

float Humanoides[6][5] =
{
    {CREATURE_FURY_MAGE, 1780.16f,    666.83f,    71.19f,    5.21f},
    {CREATURE_FURY_MAGE, 1847.93f,    600.30f,    71.30f,    2.57f},
    {CREATURE_BERSERKER, 1779.97f,    660.64f,    71.19f,    5.28f},
    {CREATURE_BERSERKER, 1786.2f ,    661.01f,    71.19f,    4.51f},
    {CREATURE_BERSERKER, 1845.17f,    602.63f,    71.28f,    2.43f},
    {CREATURE_BERSERKER, 1842.91f,    599.93f,    71.23f,    2.44f}
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
            BlackHoleSummonTimer = 15000;
		    EnrageTimer = 600000;
            doCast((Unit*)NULL, SPELL_NEGATIVE_ENERGY_E, false);

            Summons.DespawnAll();

		    if (!onSpawn)
		    {
			    if (Creature* muru = pInstance->instance->GetCreature(pInstance->GetData64(DATA_MURU)))
				    muru->AttackStop();

                if (pInstance)
                    pInstance->SetData(DATA_MURU_EVENT, NOT_STARTED);
		    }
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
                case CREATURE_DARKNESS:
                    summoned->addUnitState(UNIT_STAT_STUNNED);
                    float x,y,z,o;
                    summoned->GetHomePosition(x,y,z,o);
                    me->SummonCreature(CREATURE_DARK_FIENDS, x,y,z,o, TEMPSUMMON_CORPSE_DESPAWN, 0);
                    break;
            }
            if (summoned->getAI())
                summoned->getAI()->attackStart(selectUnit(TARGET_RANDOM, 0, 50.0f, true));
            else
                summoned->AI()->AttackStart(selectUnit(TARGET_RANDOM, 0, 50.0f, true));
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
                Unit* random = selectUnit(TARGET_RANDOM, 0, 100.0f, true);
                if (!random)
                    return;

                doCast(random, SPELL_DARKNESS_E, false);

                random = selectUnit(TARGET_RANDOM, 0, 100.0f, true);
                if (!random)
                    return;

                random->CastSpell(random, SPELL_BLACKHOLE, false);
                BlackHoleSummonTimer = 15000;
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
	
	class boss_muruAI : public CreatureAINew
    {
    public:
	    boss_muruAI(Creature* creature) : CreatureAINew(creature), Summons(me)
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

        bool DarkFiend;

        void onReset(bool /*onSpawn*/)
        {
            DarkFiend = false;
            Phase = 0;

            EnrageTimer = 600000;
            DarknessTimer = 45000;
            HumanoidesTimer = 10000;
            PhaseTimer = 0;
            SentinelTimer = 31500;

            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
			me->GetMotionMaster()->Clear();
			me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetVisibility(VISIBILITY_ON);

            Summons.DespawnAll();

            if (pInstance)
                pInstance->SetData(DATA_MURU_EVENT, NOT_STARTED);
        }

        void onCombatStart(Unit * /*who*/)
        {
            doCast((Unit*)NULL, SPELL_NEGATIVE_ENERGY,false);

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

		    if (Phase > 1 && Phase < 4)
			    damage = 0;
        }

        void onSummon(Creature* summoned)
        {
            switch(summoned->GetEntry())
            {
                case CREATURE_DARK_FIENDS:
                    summoned->CastSpell(summoned,SPELL_DARKFIEND_VISUAL,false);
                    break;
            }
            if (summoned->getAI())
                summoned->getAI()->attackStart(selectUnit(TARGET_RANDOM,0, 50.0f, true));
            else
                summoned->AI()->AttackStart(selectUnit(TARGET_RANDOM,0, 50.0f, true));
            Summons.Summon(summoned);
        }
	
	    void onSummonDespawn(Creature* unit)
		{
		    Summons.Despawn(unit);
        }

        void update(const uint32 diff)
        {
            if (!updateVictim())
                return;

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
                            //doCast(me, SPELL_SUMMON_ENTROPIUS, false);
                            me->SummonCreature(25840, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0);
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
            else EnrageTimer -= diff;

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
				    DarkFiend = false;
                    for (uint8 i = 0; i < 8; ++i)
                        me->SummonCreature(CREATURE_DARK_FIENDS,DarkFiends[i][0],DarkFiends[i][1],DarkFiends[i][2], DarkFiends[i][3], TEMPSUMMON_CORPSE_DESPAWN, 0);
                    DarknessTimer = 42000;
			    }
		    }
		    else
			    DarknessTimer -= diff;

		    if (HumanoidesTimer <= diff)
		    {
			    for (uint8 i = 0; i < 6; ++i)
                    me->SummonCreature(Humanoides[i][0],Humanoides[i][1],Humanoides[i][2],Humanoides[i][3], Humanoides[i][4], TEMPSUMMON_CORPSE_DESPAWN, 0);
                HumanoidesTimer = 60000;
		    }
		    else
			    HumanoidesTimer -= diff;

		    if (SentinelTimer <= diff)
		    {
                doCast((Unit*)NULL, SPELL_OPEN_PORTAL_2, false);
                SentinelTimer = 30000;
		    }
		    else
			    SentinelTimer -= diff;
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
	
	class npc_muru_portalAI : public CreatureAINew
    {
    public:
	    npc_muru_portalAI(Creature* creature) : CreatureAINew(creature), Summons(me)
		{
		    pInstance = ((ScriptedInstance*)creature->GetInstanceData());
		}

        ScriptedInstance* pInstance;

        SummonList Summons;

        bool SummonSentinel;
        bool InAction;

        uint32 SummonTimer;

        void onReset(bool /*onSpawn*/)
        {
            SummonTimer = 5000;

            InAction = false;
            SummonSentinel = false;

            me->addUnitState(UNIT_STAT_STUNNED);
		    me->GetMotionMaster()->Clear();
		    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);

            Summons.DespawnAll();
        }

        void onSummon(Creature* summoned)
        {
            if (pInstance) {
                if (Player* Target = ObjectAccessor::GetPlayer(*me, pInstance->GetData64(DATA_PLAYER_GUID))) {
                    if (summoned->getAI()) // FIXME: Hack because getAI() may not be initialized and there is no fallback like in old CreatureAI system.
                        summoned->getAI()->attackStart(Target);
                    else
                        summoned->AI()->AttackStart(Target);
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
            } else SummonTimer -= diff;
        }
	};
	
	CreatureAINew* getAI(Creature* creature)
    {
        return new npc_muru_portalAI(creature);
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
                if (Spell->Effect[i] == 38)
                    me->DisappearAndDie();
        }

        void update(const uint32 diff)
        {
            if (!updateVictim())
                return;

            if (WaitTimer <= diff)
            {
                if (!InAction)
                {
                    me->clearUnitState(UNIT_STAT_STUNNED);
                    doCast((Unit*)NULL, SPELL_DARKFIEND_SKIN, false);
                    attackStart(selectUnit(TARGET_RANDOM, 0, 100.0f, true));
                    InAction = true;
                    WaitTimer = 500;
                }
                else
                {
                    if (me->GetDistance(me->getVictim()) < 5)
                    {
                        doCast((Unit*)NULL, SPELL_DARKFIEND_AOE, false);
                        me->DisappearAndDie();
                    }
                    WaitTimer = 500;
                }
            } else WaitTimer -= diff;
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

        void onReset(bool /*onSpawn*/)
        {
            PulseTimer = 3000;
            VoidBlastTimer = 45000; //is this a correct timer?

            float x,y,z,o;
            me->GetHomePosition(x,y,z,o);
            doTeleportTo(x,y,71);
        }

        void onDeath(Unit* /*killer*/)
        {
            for (uint8 i = 0; i < 8; ++i)
                me->SummonCreature(CREATURE_VOID_SPAWN, me->GetPositionX(),me->GetPositionY(),me->GetPositionZ(), rand()%6, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 180000);
        }

        void update(const uint32 diff)
        {
            if (!updateVictim())
                return;

            if (PulseTimer <= diff)
            {
                doCast((Unit*)NULL, SPELL_SHADOW_PULSE, true);
                PulseTimer = 3000;
            } else PulseTimer -= diff;

            if (VoidBlastTimer <= diff)
            {
                doCast(me->getVictim(), SPELL_VOID_BLAST, false);
                VoidBlastTimer = 45000;
            } else VoidBlastTimer -= diff;

            doMeleeAttackIfReady();
        }
	};
	
	CreatureAINew* getAI(Creature* creature)
    {
        return new npc_void_sentinelAI(creature);
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
        uint8 Phase;
        uint8 NeedForAHack;

        void onReset(bool /*onSpawn*/)
        {
            DespawnTimer = 15000;
            SpellTimer = 5000;
            Phase = 0;

            me->addUnitState(UNIT_STAT_STUNNED);
            doCast((Unit*)NULL, SPELL_BLACKHOLE_SPAWN, true);
        }

        void update(const uint32 diff)
        {
            if (SpellTimer <= diff)
            {
                Unit* Victim = Unit::GetUnit(*me, pInstance ? pInstance->GetData64(DATA_PLAYER_GUID) : 0);
                switch (NeedForAHack)
                {
                    case 0:
                        me->clearUnitState(UNIT_STAT_STUNNED);
                        doCast((Unit*)NULL, SPELL_BLACKHOLE_GROW, false);
                        if (Victim)
                            attackStart(Victim);
                        SpellTimer = 700;
                        NeedForAHack = 2;
                        break;
                    case 1:
                        me->AddAura(SPELL_BLACKHOLE_GROW, me);
                        NeedForAHack = 2;
                        SpellTimer = 600;
                        break;
                    case 2:
                        SpellTimer = 400;
                        NeedForAHack = 3;
                        me->RemoveAura(SPELL_BLACKHOLE_GROW, 1);
                        break;
                    case 3:
                        SpellTimer = urand(400,900);
                        NeedForAHack = 1;
                        if (Unit* Temp = me->getVictim())
                        {
                            if (Temp->GetPositionZ() > 73 && Victim)
                                attackStart(Victim);
                        } else
                            return;
                }
            } else SpellTimer -= diff;

            if (DespawnTimer <= diff)
                me->DisappearAndDie();
            else DespawnTimer -= diff;
        }
	};
	
	CreatureAINew* getAI(Creature* creature)
    {
        return new npc_blackholeAI(creature);
    }
};

void AddSC_boss_muru()
{
    sScriptMgr.addScript(new boss_entropius());
	sScriptMgr.addScript(new boss_muru());
	sScriptMgr.addScript(new npc_muru_portal());
	sScriptMgr.addScript(new npc_dark_fiend());
	sScriptMgr.addScript(new npc_void_sentinel());
	sScriptMgr.addScript(new npc_blackhole());
}
