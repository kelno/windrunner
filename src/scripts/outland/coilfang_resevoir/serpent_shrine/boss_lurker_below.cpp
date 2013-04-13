/* Copyright (C) 2006 - 2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
SDName: boss_the_lurker_below
SD%Complete: 80
SDComment: Coilfang Frenzy, find out how could we fishing in the strangepool
SDCategory: The Lurker Below
EndScriptData */

#include "precompiled.h"
#include "def_serpent_shrine.h"
#include "SimpleAI.h"
#include "Spell.h"

#define SPELL_SPOUT         37433
#define SPELL_SPOUT_ANIM    42835
#define SPELL_SPOUT_BREATH  37431
#define SPELL_KNOCKBACK     19813
#define SPELL_GEYSER        37478
#define SPELL_WHIRL         37660
#define SPELL_WATERBOLT     37138
#define SPELL_SUBMERGE      37550
#define SPELL_EMERGE        20568

#define EMOTE_SPOUT "prend une profonde respiration."

#define SPOUT_DIST  100

#define MOB_COILFANG_GUARDIAN 21873
#define MOB_COILFANG_AMBUSHER 21865

//Ambusher spells
#define SPELL_SPREAD_SHOT   37790
#define SPELL_SHOOT         37770

//Guardian spells
#define SPELL_ARCINGSMASH   38761 // Wrong SpellId. Can't find the right one.
#define SPELL_HAMSTRING     26211

float AddPos[9][3] =
{
    {-6.21f , -491.38f, -22.16f},   //MOVE_AMBUSHER_1 X, Y, Z
    {0.90f  , -495.27f, -22.20f},   //MOVE_AMBUSHER_2 X, Y, Z
    {60.38f , -494.72f, -22.16f},   //MOVE_AMBUSHER_3 X, Y, Z
    {71.14f , -493.37f, -22.54f},   //MOVE_AMBUSHER_4 X, Y, Z
    {82.11f , -349.43f, -22.21f},   //MOVE_AMBUSHER_5 X, Y, Z
    {93.45f , -355.93f, -21.59f},   //MOVE_AMBUSHER_6 X, Y, Z
    {47.60f , -364.82f, -21.68f},   //MOVE_GUARDIAN_1 X, Y, Z
    {-14.71f, -446.60f, -21.85f},   //MOVE_GUARDIAN_2 X, Y, Z
    {94.24f , -400.72f, -21.65f}    //MOVE_GUARDIAN_3 X, Y, Z
};

struct boss_the_lurker_belowAI : public Scripted_NoMovementAI
{
    boss_the_lurker_belowAI(Creature *c) : Scripted_NoMovementAI(c), summons(m_creature)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        SpellEntry *TempSpell = GET_SPELL(SPELL_SPOUT_ANIM);
        if (TempSpell)
        {
            TempSpell->Effect[0] = 0;//remove all spell effect, only anim is needed
            TempSpell->Effect[1] = 0;
            TempSpell->Effect[2] = 0;
        }
    }

    ScriptedInstance* pInstance;
    SummonList summons;

    bool Spawned;
    bool Submerged;
    bool InRange;
    bool CanStartEvent;
    uint32 RotTimer;
    uint32 SpoutAnimTimer;
    uint32 WaterboltTimer;
    uint32 SpoutTimer;
    uint32 WhirlTimer;
    uint32 PhaseTimer;
    uint32 GeyserTimer;
    uint32 CheckTimer;
    uint32 WaitTimer;
    uint32 WaitTimer2;
    uint32 phaseRotate;
    uint32 phaseRotateTimer;
    uint32 nbPops;

    bool CheckCanStart()//check if players fished
    {
        if(pInstance && pInstance->GetData(DATA_STRANGE_POOL) != IN_PROGRESS)
            return false;
        return true;
    }

    void Reset()
    {
        nbPops = 0;
        phaseRotate = 0;
        phaseRotateTimer = 0;
        m_creature->AddUnitMovementFlag(MOVEMENTFLAG_SWIMMING + MOVEMENTFLAG_LEVITATING);
        SpoutAnimTimer = 1000;
        RotTimer = 0;
        WaterboltTimer = 15000;//give time to get in range when fight starts
        SpoutTimer = 45000;
        WhirlTimer = 18000;//after avery spout
        PhaseTimer = 90000;
        GeyserTimer = rand()%5000 + 15000;
        CheckTimer = 15000;//give time to get in range when fight starts
        WaitTimer = 60000;//never reached
        WaitTimer2 = 60000;//never reached

        Submerged = true;//will be false at combat start
        Spawned = false;
        InRange = false;
        CanStartEvent = false;

        summons.DespawnAll();

        if (pInstance)
        {
            pInstance->SetData(DATA_THELURKERBELOWEVENT, NOT_STARTED);
            pInstance->SetData(DATA_STRANGE_POOL, NOT_STARTED);
        }
        DoCast(m_creature,SPELL_SUBMERGE);//submerge anim
        m_creature->SetVisibility(VISIBILITY_OFF);//we start invis under water, submerged
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
        m_creature->setFaction(35);
        m_creature->addUnitState(UNIT_STAT_FORCEROOT);
    }

    void JustDied(Unit* Killer)
    {
        if (pInstance)
            pInstance->SetData(DATA_THELURKERBELOWEVENT, DONE);

        summons.DespawnAll();
    }

    void MoveInLineOfSight(Unit *who)
    {
    }

    void AttackStart(Unit *victim)
    {
        if (RotTimer)
            return;

        Scripted_NoMovementAI::AttackStart(victim);
    }

    void AttackStart(Unit *victim, bool melee)
    {
       if (RotTimer)
            return;

       ScriptedAI::AttackStart(victim, melee);
    }

    void AttackStartNoMove(Unit *pTarget)
    {
        if (RotTimer)
            return;

       ScriptedAI::AttackStartNoMove(pTarget);
    }

    void Aggro(Unit* who)
    {
        if (pInstance)
        {
            pInstance->SendScriptInTestNoLootMessageToAll();
            pInstance->SetData(DATA_THELURKERBELOWEVENT, IN_PROGRESS);
        }
            
        //Scripted_NoMovementAI::Aggro(who);
    }

    void JustSummoned(Creature* pSummon)
    {
        summons.Summon(pSummon);

        nbPops++;
        switch (nbPops)
        {
            case 1:
                pSummon->GetMotionMaster()->MovePoint(0, 0.17f, -468.30f, -19.79f);
                break;
            case 2:
                pSummon->GetMotionMaster()->MovePoint(0, 8.43f, -471.48f, -19.79f);
                break;
            case 3:
                pSummon->GetMotionMaster()->MovePoint(0, 56.75f, -466.73f, -19.79f);
                break;
            case 4:
                pSummon->GetMotionMaster()->MovePoint(0, 63.88f, -464.75f, -19.79f);
                break;
            case 5:
                pSummon->GetMotionMaster()->MovePoint(0, 65.88f, -374.74f, -19.79f);
                break;
            case 6:
                pSummon->GetMotionMaster()->MovePoint(0, 78.52f, -381.99f, -19.72f);
                break;
            case 7:
                pSummon->GetMotionMaster()->MovePoint(0, 42.88f, -391.15f, -18.97f);
                break;
            case 8:
                pSummon->GetMotionMaster()->MovePoint(0, 13.63f, -430.81f, -19.46f);
                break;
            case 9:
                pSummon->GetMotionMaster()->MovePoint(0, 62.66f, -413.97f, -19.27f);
                break;
        }
    }

    void SummonedCreatureDespawn(Creature* unit)
    {
        summons.Despawn(unit);
    }

    void UpdateAI(const uint32 diff)
    {
        if(!CanStartEvent)//boss is invisible, don't attack
        {
            if(CheckCanStart())
            {
                if(Submerged)
                {
                    m_creature->SetVisibility(VISIBILITY_ON);
                    Submerged = false;
                    WaitTimer2 = 500;
                }
                if(!Submerged && WaitTimer2 < diff)//wait 500ms before emerge anim
                {
                    m_creature->RemoveAllAuras();
                    m_creature->RemoveFlag(UNIT_NPC_EMOTESTATE,EMOTE_STATE_SUBMERGED);
                    DoCast(m_creature, SPELL_EMERGE);
                    WaitTimer2 = 60000;//never reached
                    WaitTimer = 3000;
                }else WaitTimer2 -= diff;
                
                if(WaitTimer < diff)//wait 3secs for emerge anim, then attack
                {
                    WaitTimer = 3000;
                    CanStartEvent=true;//fresh fished from pool
                    m_creature->SetReactState(REACT_AGGRESSIVE);
                    m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
                    m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

                    DoZoneInCombat(NULL, true);
                    if (Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0))
                        AttackStart(target);

                    if (pInstance)
                        pInstance->SetData(DATA_STRANGE_POOL, DONE);
                }else WaitTimer -= diff;
            }
            return;
        }

        if(m_creature->getThreatManager().getThreatList().empty())//check if should evade
        {
            if(m_creature->isInCombat())
                EnterEvadeMode();

            return;
        }

        if (!Submerged)
        {
            if (PhaseTimer < diff)
            {
                m_creature->InterruptNonMeleeSpells(false);
                DoCast(m_creature,SPELL_SUBMERGE);
                PhaseTimer = 60000;//60secs submerged
                Submerged = true;
            }else PhaseTimer-=diff;

            if (SpoutTimer < diff)
            {
                SpoutTimer = 45000;
                WhirlTimer = 23000;//whirl directly after spout
                RotTimer = 23000;
                phaseRotate = 1;
            }else SpoutTimer -= diff;

            if (phaseRotateTimer <= diff)
            {
                switch (phaseRotate)
                {
                    case 1:
                        m_creature->MonsterTextEmote(EMOTE_SPOUT,0,true);
                        m_creature->SetReactState(REACT_PASSIVE);
                        DoCast(m_creature, SPELL_SPOUT_BREATH);
                        phaseRotate = 2;
                        phaseRotateTimer = 3000;
                        break;
                    case 2:
                        if(rand()%2)
                            m_creature->StartAutoRotate(CREATURE_ROTATE_LEFT,20000);
                        else
                            m_creature->StartAutoRotate(CREATURE_ROTATE_RIGHT,20000);
                        phaseRotate = 3;
                        break;
                }
            }
            else
                phaseRotateTimer -= diff;

            //Whirl directly after a Spout and at random times
            if (WhirlTimer < diff)
            {
                WhirlTimer = 18000;
                DoCast(m_creature, SPELL_WHIRL);
            }else WhirlTimer -= diff;

            if(CheckTimer < diff)//check if there are players in melee range
            {
                InRange = false;
                Map* pMap = m_creature->GetMap();
                Map::PlayerList const &PlayerList = pMap->GetPlayers();                
                if (!PlayerList.isEmpty())
                {
                    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                    {
                        if(m_creature->IsWithinMeleeRange(i->getSource()))
                            InRange = true;
                    }
                }
                CheckTimer = 2000;
            }else CheckTimer -= diff;

            if(RotTimer)
            {
                m_creature->SetUInt64Value(UNIT_FIELD_TARGET, 0);

                if (phaseRotate != 3)
                    return;

                Map* pMap = m_creature->GetMap();
                if (pMap->IsDungeon())
                {
                    Map::PlayerList const &PlayerList = pMap->GetPlayers();
                    for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                    {
                        if (i->getSource() && i->getSource()->isAlive() && !i->getSource()->isPet())
                        {
                            if (m_creature->HasInArc((double)diff/20000*(double)M_PI*2,i->getSource()))
                            {
                                if (m_creature->IsWithinDistInMap(i->getSource(), SPOUT_DIST))
                                {
                                    if (!i->getSource()->IsInWater())
                                    {
                                        DoCast(i->getSource(),SPELL_SPOUT,true);
                                    }
                                }
                            }
                        }
                    }
                }

                if(SpoutAnimTimer < diff)
                {
                    DoCast(m_creature,SPELL_SPOUT_ANIM,true);
                    SpoutAnimTimer = 1000;
                }
                else
                    SpoutAnimTimer -= diff;

                if(RotTimer < diff)
                {
                    RotTimer = 0;
                    phaseRotate = 0;
                    m_creature->SetReactState(REACT_AGGRESSIVE);
                }
                else
                    RotTimer -= diff;

                return;
            }

            if (GeyserTimer < diff)
            {
                Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 1);
                if (!target && m_creature->getVictim())
                    target = m_creature->getVictim();
                if (target)
                    DoCast(target, SPELL_GEYSER);
                GeyserTimer = rand()%5000 + 15000;
            }else GeyserTimer -= diff;

            if(!InRange)//if no players in melee range cast Waterbolt
            {
                if (WaterboltTimer < diff)
                {
                    if (Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0))
                        DoCast(target,SPELL_WATERBOLT);

                    WaterboltTimer = 3000;
                }else WaterboltTimer -= diff;
            }

            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();

        }else//submerged
        {
            if (PhaseTimer < diff)
            {
                Submerged = false;
                m_creature->InterruptNonMeleeSpells(false);//shouldn't be any
                m_creature->RemoveAllAuras();
                m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
                m_creature->RemoveFlag(UNIT_NPC_EMOTESTATE,EMOTE_STATE_SUBMERGED);
                DoCast(m_creature, SPELL_EMERGE, true);
                Spawned = false;
                SpoutTimer = 3000; // directly cast Spout after emerging!
                PhaseTimer = 90000;
                return;
            }else PhaseTimer-=diff;

            if(m_creature->getThreatManager().getThreatList().empty())//check if should evade
            {
                EnterEvadeMode();
                return;
            }

            if (!m_creature->isInCombat())
                DoZoneInCombat();

            if (!Spawned)
            {
                nbPops = 0;
                m_creature->SetUInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
                //spawn adds
                for (uint8 i = 0; i < 9; ++i)
                {
                    if (i < 6)
                        m_creature->SummonCreature(MOB_COILFANG_AMBUSHER,AddPos[i][0],AddPos[i][1],AddPos[i][2], 0, TEMPSUMMON_CORPSE_DESPAWN, 0);
                    else
                        m_creature->SummonCreature(MOB_COILFANG_GUARDIAN,AddPos[i][0],AddPos[i][1],AddPos[i][2], 0, TEMPSUMMON_CORPSE_DESPAWN, 0);
                }
                Spawned = true;
            }
        }
    }
};

struct mob_coilfang_guardianAI : public ScriptedAI
{
    mob_coilfang_guardianAI(Creature *c) : ScriptedAI(c) {}

    uint32 ArcingsmashTimer;
    uint32 HamstringTimer;
    uint32 tempTimer;

    void Reset()
    {
        ArcingsmashTimer = 5000;
        HamstringTimer = 2000;
        tempTimer = 11000;

        m_creature->AddUnitMovementFlag(MOVEMENTFLAG_SWIMMING + MOVEMENTFLAG_LEVITATING);
    }

    void Aggro(Unit *who)
    {
    }

    void MovementInform(uint32 type, uint32 id) 
    {
        if(type == POINT_MOTION_TYPE)
        {
            switch (id)
            {
                case 0:
                    DoZoneInCombat(NULL, true);
                    AttackStart(SelectUnit(SELECT_TARGET_RANDOM, 0));
                    break;
            }
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (tempTimer)
        {
            if (tempTimer <= diff)
            {
                DoZoneInCombat(NULL, true);
                if (Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0))
                {
                    AttackStart(target);
                    tempTimer = 0;
                }
                else
                    tempTimer = 500;
            }
            else
                tempTimer -= diff;
        }

        //Return since we have no target
        if (!UpdateVictim())
            return;

        if (ArcingsmashTimer < diff)
        {
            DoCast(m_creature->getVictim(), SPELL_ARCINGSMASH);
            ArcingsmashTimer = urand(10000, 15000);
        }else ArcingsmashTimer -= diff;

        if(HamstringTimer < diff)
        {
            DoCast(m_creature->getVictim(), SPELL_HAMSTRING);
            HamstringTimer = urand(10000, 15000);
        }else HamstringTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

struct mob_coilfang_ambusherAI : public Scripted_NoMovementAI
{
    mob_coilfang_ambusherAI(Creature *c) : Scripted_NoMovementAI(c)
    {
        SpellEntry *TempSpell = GET_SPELL(SPELL_SPREAD_SHOT);
        if (TempSpell)
        {
            TempSpell->Effect[0] = 0;
            TempSpell->Effect[1] = 0;
            TempSpell->Effect[2] = 0;
        }
        TempSpell = GET_SPELL(SPELL_SHOOT);
        if (TempSpell)
        {
            TempSpell->Effect[0] = 0;
            TempSpell->Effect[1] = 0;
            TempSpell->Effect[2] = 0;
        }
    }

    uint32 MultiShotTimer;
    uint32 ShootBowTimer;
    uint32 tempTimer;

    void Reset()
    {
        MultiShotTimer = 10000;
        ShootBowTimer = 4000;
        tempTimer = 11000;

        m_creature->AddUnitMovementFlag(MOVEMENTFLAG_SWIMMING + MOVEMENTFLAG_LEVITATING);
    }

    void Aggro(Unit *who)
    {
    }

    void MovementInform(uint32 type, uint32 id) 
    {
        if(type == POINT_MOTION_TYPE)
        {
            switch (id)
            {
                case 0:
                    DoZoneInCombat(NULL, true);
                    break;
            }
        }
    }

    void OnSpellFinish(Unit *caster, uint32 spellId, Unit *target, bool ok)
    {
        if (spellId == 37790)
        {
            if (caster->ToCreature())
            {
                if (m_creature == caster->ToCreature())
                {
                    uint32 damage = 0;
                    uint32 absorb, resist;
                    damage = target->CalcArmorReducedDamage(target, 3500);
                    target->CalcAbsorbResist(target, SPELL_SCHOOL_MASK_NORMAL, SPELL_DIRECT_DAMAGE, damage, &absorb, &resist, SPELL_SPREAD_SHOT);
                    uint32 damageTaken = m_creature->DealDamage(target, damage - absorb - resist);
                    m_creature->SendSpellNonMeleeDamageLog(target, SPELL_SPREAD_SHOT, damageTaken, SPELL_SCHOOL_MASK_NORMAL, absorb, resist, true, 0, false);
                }
            }
        }
        else if (spellId = 37770)
        {
            if (caster->ToCreature())
            {
                if (m_creature == caster->ToCreature())
                {
                    uint32 damage = 0;
                    uint32 absorb, resist;
                    damage = target->CalcArmorReducedDamage(target, 3500);
                    target->CalcAbsorbResist(target, SPELL_SCHOOL_MASK_NORMAL, SPELL_DIRECT_DAMAGE, damage, &absorb, &resist, SPELL_SHOOT);
                    uint32 damageTaken = m_creature->DealDamage(target, damage - absorb - resist);
                    m_creature->SendSpellNonMeleeDamageLog(target, SPELL_SHOOT, damageTaken, SPELL_SCHOOL_MASK_NORMAL, absorb, resist, true, 0, false);
                }
            }
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (tempTimer)
        {
            if (tempTimer <= diff)
            {
                DoZoneInCombat(NULL, true);
            }
            else
                tempTimer -= diff;
        }

        if (!UpdateVictim())
            return;

        if (MultiShotTimer < diff)
        {
            if (Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0, 45.0f, false))
                DoCast(target, SPELL_SPREAD_SHOT);

            MultiShotTimer = urand(5000, 15000);
        }
        else
            MultiShotTimer -= diff;

        if (ShootBowTimer < diff)
        {
            if (!m_creature->hasUnitState(UNIT_STAT_CASTING))
            {
                if (Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0, 45.0f, false))
                    DoCast(target, SPELL_SHOOT);
            }
            ShootBowTimer = urand(2000, 5000);
        }
        else
            ShootBowTimer -= diff;
    }
};

CreatureAI* GetAI_mob_coilfang_ambusher(Creature* pCreature)
{
    return new mob_coilfang_ambusherAI (pCreature);
}

CreatureAI* GetAI_mob_coilfang_guardian(Creature* pCreature)
{
    return new mob_coilfang_guardianAI (pCreature);
}

CreatureAI* GetAI_boss_the_lurker_below(Creature* pCreature)
{
    return new boss_the_lurker_belowAI (pCreature);
}

void AddSC_boss_the_lurker_below()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name="boss_the_lurker_below";
    newscript->GetAI = &GetAI_boss_the_lurker_below;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="mob_coilfang_guardian";
    newscript->GetAI = &GetAI_mob_coilfang_guardian;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="mob_coilfang_ambusher";
    newscript->GetAI = &GetAI_mob_coilfang_ambusher;
    newscript->RegisterSelf();
}
