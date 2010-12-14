/*
 * Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
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
SDName: Sunwell_Plateau
SD%Complete: 0
SDComment: Placeholder, Epilogue after Kil'jaeden, Captain Selana Gossips
EndScriptData */

/* ContentData
npc_prophet_velen
npc_captain_selana
npc_sunblade_protector
npc_sunblade_scout
npc_sunblade_slayer
EndContentData */

#include "precompiled.h"
#include "def_sunwell_plateau.h"

/*######
## npc_prophet_velen
######*/

enum ProphetSpeeches
{
    PROPHET_SAY1 = -1580099,
    PROPHET_SAY2 = -1580100,
    PROPHET_SAY3 = -1580101,
    PROPHET_SAY4 = -1580102,
    PROPHET_SAY5 = -1580103,
    PROPHET_SAY6 = -1580104,
    PROPHET_SAY7 = -1580105,
    PROPHET_SAY8 = -1580106
};

enum LiadrinnSpeeches
{
    LIADRIN_SAY1 = -1580107,
    LIADRIN_SAY2 = -1580108,
    LIADRIN_SAY3 = -1580109
};

/*######
## npc_captain_selana
######*/

#define CS_GOSSIP1 "Give me a situation report, Captain."
#define CS_GOSSIP2 "What went wrong?"
#define CS_GOSSIP3 "Why did they stop?"
#define CS_GOSSIP4 "Your insight is appreciated."

/*######
## npc_sunblade_protector
######*/

#define SPELL_SW_RADIANCE       45769
#define SPELL_FEL_LIGHTNING     46480

struct npc_sunblade_protectorAI : public ScriptedAI
{
    npc_sunblade_protectorAI(Creature *c) : ScriptedAI(c) {}
    
    uint32 felLightningTimer;
    
    void Reset()
    {
        felLightningTimer = 5000;
        
        if (m_creature->GetDefaultMovementType() == IDLE_MOTION_TYPE) {
            m_creature->SetReactState(REACT_DEFENSIVE);
            m_creature->SetHasChangedReactState();
            //felLightningTimer = 0;
        }
        DoCast(m_creature, SPELL_SW_RADIANCE);
    }
    
    void Aggro(Unit *pWho) {}
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        if (felLightningTimer) {
            if (felLightningTimer <= diff) {
                DoCast(m_creature->getVictim(), SPELL_FEL_LIGHTNING);
                felLightningTimer = 5000+rand()%10000;
            }
            else
                felLightningTimer -= diff;
        }
        
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_protector(Creature *pCreature)
{
    return new npc_sunblade_protectorAI(pCreature);
}

/*######
## npc_sunblade_scout
######*/

#define SPELL_ACTIVATE_PROTEC   46475
#define SPELL_SINISTER_STRIKE   46558

#define NPC_SUNBLADE_PROTEC     25507

struct npc_sunblade_scoutAI : public ScriptedAI
{    
    npc_sunblade_scoutAI(Creature *c) : ScriptedAI(c) {}
    
    uint32 sinisterStrikeTimer;
    
    uint64 pullerGUID;
    Creature *protector;
    
    bool hasActivated;
    
    void Reset()
    {
        DoCast(m_creature, SPELL_SW_RADIANCE);
        
        sinisterStrikeTimer = 0;
        pullerGUID = 0;
        protector = NULL;
        hasActivated = false;
    }
    
    void Aggro(Unit *pWho)
    {
        pullerGUID = pWho->GetGUID();
        if (protector = m_creature->FindNearestCreature(NPC_SUNBLADE_PROTEC, 60.0f, true)) {
            m_creature->SetReactState(REACT_PASSIVE);
            m_creature->SetSpeed(MOVE_WALK, 3.0f);
            m_creature->GetMotionMaster()->MovePoint(0, protector->GetPositionX(), protector->GetPositionY(), protector->GetPositionZ());
            m_creature->SetUInt64Value(UNIT_FIELD_TARGET, protector->GetGUID());
        }
    }
    
    void OnSpellFinish(Unit *caster, uint32 spellId, Unit *target)
    {
        if (spellId == 46475) {
            if (Unit* puller = Unit::GetUnit(*m_creature, pullerGUID)) {
                //puller = SelectUnit(SELECT_TARGET_RANDOM, 0);
                m_creature->SetUInt64Value(UNIT_FIELD_TARGET, puller->GetGUID());
                m_creature->SetReactState(REACT_AGGRESSIVE);
                m_creature->clearUnitState(UNIT_STAT_ROOT);
                if (target->ToCreature()) {
                    target->ToCreature()->SetReactState(REACT_AGGRESSIVE);
                    ((npc_sunblade_protectorAI*)target->ToCreature()->AI())->felLightningTimer = 5000;
                }
                target->GetMotionMaster()->MoveChase(puller);
                target->Attack(puller, true);
                m_creature->GetMotionMaster()->MoveChase(puller);
                AttackStart(puller);
            }
            else {
                AttackStart(SelectUnit(SELECT_TARGET_RANDOM, 0));
                if (target->ToCreature()) {
                    target->ToCreature()->SetReactState(REACT_AGGRESSIVE);
                    ((npc_sunblade_protectorAI*)target->ToCreature()->AI())->felLightningTimer = 5000;
                    target->ToCreature()->AI()->AttackStart(SelectUnit(SELECT_TARGET_RANDOM, 0));
                }
            }
        }
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!hasActivated) {
            if (protector) {
                if (m_creature->GetDistance(protector) <= 15.0f) {
                    m_creature->GetMotionMaster()->MovementExpired(false);
                    m_creature->StopMoving();
                    m_creature->addUnitState(UNIT_STAT_ROOT);
                    DoCast(protector, SPELL_ACTIVATE_PROTEC);
                    m_creature->SetInFront(protector);
                    hasActivated = true;
                    m_creature->SetSpeed(MOVE_WALK, 1.0f);
                    sinisterStrikeTimer = 2000;
                }
            }
        }
        
        if (!UpdateVictim())
            return;
        
        if (sinisterStrikeTimer) {
            if (sinisterStrikeTimer <= diff) {
                DoCast(m_creature->getVictim(), SPELL_SINISTER_STRIKE);
                sinisterStrikeTimer = 2000+rand()%2000;
            }
            else
                sinisterStrikeTimer -= diff;
        }
        
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_sunblade_scout(Creature *pCreature)
{
    return new npc_sunblade_scoutAI(pCreature);
}

/*######
## npc_sunblade_slayer
######*/

#define SPELL_SHOOT     47001

struct npc_sunblade_slayerAI : public ScriptedAI
{
    npc_sunblade_slayerAI(Creature *c) : ScriptedAI(c) {}
    
    uint32 shootTimer;
    
    void Reset()
    {
        DoCast(m_creature, SPELL_SW_RADIANCE);
        
        shootTimer = 1000;
    }
    
    void Aggro(Unit *pWho) {}
    
    void AttackStart(Unit* who)
    {
        if (m_creature->Attack(who, true))
        {
            m_creature->AddThreat(who, 0.0f);

            if (!InCombat)
            {
                InCombat = true;
                Aggro(who);
            }

            DoStartMovement(who, 13, 0);
        }
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        if (shootTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_SHOOT);
            shootTimer = 4000;
        }
        else
            shootTimer -= diff;
            
        if (m_creature->IsWithinMeleeRange(m_creature->getVictim()))
            DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_sunblade_slayer(Creature *pCreature)
{
    return new npc_sunblade_slayerAI(pCreature);
}

/*######
## AddSC
######*/

void AddSC_sunwell_plateau()
{
    Script *newscript;
    
    newscript = new Script;
    newscript->Name = "npc_sunblade_scout";
    newscript->GetAI = &GetAI_npc_sunblade_scout;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_sunblade_protector";
    newscript->GetAI = &GetAI_npc_protector;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_sunblade_slayer";
    newscript->GetAI = &GetAI_npc_sunblade_slayer;
    newscript->RegisterSelf();
}
