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
npc_sunblade_cabalist
npc_sunblade_dawnpriest
npc_sunblade_duskpriest
npc_sunblade_archmage
npc_shadowsword_vanquisher
npc_shadowsword_manafiend
npc_shadowsword_lifeshaper
npc_shadowsword_soulbinder
npc_shadowsword_deathbringer
npc_volatile_fiend
npc_moorba
npc_kalec_felmyst
EndContentData */

#include "precompiled.h"
#include "def_sunwell_plateau.h"

enum Quotes
{
    YELL_ACTIVATE              =   -1580115,
    YELL_KILL                  =   -1580116,
    YELL_AGGRO                 =   -1580117,
    YELL_AGGRO2                =   -1580118,
};

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
    npc_sunblade_protectorAI(Creature *c) : ScriptedAI(c)
    {
        isActivated = false;
    }
    
    uint32 felLightningTimer;
    
    bool isActivated;
    
    void Reset()
    {
        felLightningTimer = 5000;
        
        m_creature->SetReactState(REACT_DEFENSIVE);

        if (isActivated)
        {
            DoScriptText(YELL_KILL, m_creature);
            isActivated = false;
        }

        DoCast(m_creature, SPELL_SW_RADIANCE);
    }
    
    void JustDied(Unit* killer)
    {
        if (m_creature->GetDefaultMovementType() == WAYPOINT_MOTION_TYPE)
            me->RemoveFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
    }
    
    void Aggro(Unit *pWho)
    {
        DoScriptText(YELL_AGGRO, m_creature);
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        if (felLightningTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_FEL_LIGHTNING);
            felLightningTimer = 7000+rand()%5000;
        }
        else
            felLightningTimer -= diff;
        
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
#define SPELL_DUAL_WIELD        42459

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
        DoCast(m_creature, SPELL_DUAL_WIELD, true);
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
            m_creature->SetSpeed(MOVE_WALK, 4.0f);
            m_creature->GetMotionMaster()->MovePoint(0, protector->GetPositionX(), protector->GetPositionY(), protector->GetPositionZ());
            m_creature->SetUInt64Value(UNIT_FIELD_TARGET, protector->GetGUID());
        }
        else
        {
            m_creature->clearUnitState(UNIT_STAT_ROOT);
            m_creature->SetReactState(REACT_AGGRESSIVE);
            AttackStart(me->SelectNearestTarget(50.0f));
            sinisterStrikeTimer = 2000;
        }

        DoScriptText(YELL_AGGRO2, m_creature);
    }
    
    void OnSpellFinish(Unit *caster, uint32 spellId, Unit *target, bool ok)
    {
        if (spellId == 46475 && ok) {
            if (Unit* puller = Unit::GetUnit(*m_creature, pullerGUID)) {
                //puller = SelectUnit(SELECT_TARGET_RANDOM, 0);
                m_creature->SetUInt64Value(UNIT_FIELD_TARGET, puller->GetGUID());
                m_creature->SetReactState(REACT_AGGRESSIVE);
                m_creature->clearUnitState(UNIT_STAT_ROOT);
                if (target->ToCreature()) {
                    target->ToCreature()->SetReactState(REACT_AGGRESSIVE);
                    ((npc_sunblade_protectorAI*)target->ToCreature()->AI())->felLightningTimer = 5000;
                    ((npc_sunblade_protectorAI*)target->ToCreature()->AI())->isActivated = true;
                    DoScriptText(YELL_ACTIVATE, target->ToCreature());
                }
                target->GetMotionMaster()->MoveChase(puller);
                target->Attack(puller, true);
                m_creature->GetMotionMaster()->MoveChase(puller);
                AttackStart(puller);
            }
            else {
                AttackStart(me->SelectNearestTarget(50.0f));
                if (target->ToCreature()) {
                    target->ToCreature()->SetReactState(REACT_AGGRESSIVE);
                    ((npc_sunblade_protectorAI*)target->ToCreature()->AI())->felLightningTimer = 5000;
                    ((npc_sunblade_protectorAI*)target->ToCreature()->AI())->isActivated = true;
                     DoScriptText(YELL_ACTIVATE, target->ToCreature());
                    target->ToCreature()->AI()->AttackStart(me->SelectNearestTarget(50.0f));
                }
            }
        }

        m_creature->clearUnitState(UNIT_STAT_ROOT);
        m_creature->SetReactState(REACT_AGGRESSIVE);
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

#define SPELL_SHOOT            47001
#define SPELL_SCATTER_SHOT     46681
#define SPELL_SLAYING_SHOT     46557

struct npc_sunblade_slayerAI : public ScriptedAI
{
    npc_sunblade_slayerAI(Creature *c) : ScriptedAI(c) {}
    
    uint32 shootTimer;
    uint32 scatterTimer;
    uint32 slayingTimer;
    
    void Reset()
    {
        DoCast(m_creature, SPELL_SW_RADIANCE);
        
        shootTimer = 1000;
        scatterTimer = 4000;
        slayingTimer = 8000;
    }
    
    void Aggro(Unit *pWho) {}

    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim() || m_creature->IsPolymorphed())
            return;
            
        if (shootTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_SHOOT);
            shootTimer = 4000;
        }
        else
            shootTimer -= diff;

        if (scatterTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_SCATTER_SHOT);
            scatterTimer = 6000+rand()%4000;
        }
        else
            scatterTimer -= diff;

        if (slayingTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_SLAYING_SHOT);
            slayingTimer = 8000+rand()%5000;
        }
        else
            slayingTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_sunblade_slayer(Creature *pCreature)
{
    return new npc_sunblade_slayerAI(pCreature);
}

/*######
## npc_sunblade_cabalist
######*/

#define SPELL_IGNITE_MANA   46543
#define SPELL_SHADOW_BOLT   47248
#define SPELL_SUMMON_IMP    46544

struct npc_sunblade_cabalist : public ScriptedAI
{
    npc_sunblade_cabalist(Creature *c) : ScriptedAI(c), summons(m_creature) {}
    
    uint32 igniteManaTimer;

    SummonList summons;
    
    void Reset()
    {
        igniteManaTimer = 3000;

        summons.DespawnAll();
    }
    
    void Aggro(Unit *pWho)
    {
        DoCast(m_creature, SPELL_SUMMON_IMP);
    }

    void JustSummoned(Creature* pSummon)
    {
        summons.Summon(pSummon);
    }

    void SummonedCreatureDespawn(Creature* unit)
    {
        summons.Despawn(unit);
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim() || m_creature->IsPolymorphed())
            return;

        if (igniteManaTimer <= diff)
        {
            me->InterruptNonMeleeSpells(false);
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0, 30.0f, true), SPELL_IGNITE_MANA);
            igniteManaTimer = 10000+rand()%3000;
            return;
        }
        else
            igniteManaTimer -= diff;
            
        // Continuously cast Shadow bolt when nothing else to do
        DoCast(m_creature->getVictim(), SPELL_SHADOW_BOLT);
    }
};

CreatureAI* GetAI_npc_sunblade_cabalist(Creature *pCreature)
{
    return new npc_sunblade_cabalist(pCreature);
}

/*######
## npc_fire_fiend
######*/

#define SPELL_FIRE_NOVA   46551

struct npc_fire_fiend : public ScriptedAI
{
    npc_fire_fiend(Creature *c) : ScriptedAI(c) {}
    
    uint32 fireNovaTimer;
    
    void Reset()
    {
        fireNovaTimer = urand(5000, 10000);
    }
    
    void Aggro(Unit *pWho) {}
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;

        if (me->hasUnitState(UNIT_STAT_CASTING))
            return;

        if (fireNovaTimer <= diff)
        {
            DoCast(me, SPELL_FIRE_NOVA, false);
            fireNovaTimer = urand(5000, 10000);
        }
        else
            fireNovaTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_fire_fiend(Creature *pCreature)
{
    return new npc_fire_fiend(pCreature);
}

/*######
## npc_sunblade_dawnpriest
######*/

#define SPELL_HOLY_NOVA     46564
#define SPELL_HOLYFORM      46565
#define SPELL_RENEW         46563

struct npc_sunblade_dawnpriest : public ScriptedAI
{
    npc_sunblade_dawnpriest(Creature *c) : ScriptedAI(c) {}
    
    uint32 holyNovaTimer;
    uint32 renewTimer;
    
    void Reset()
    {
        holyNovaTimer = 2000;
        renewTimer = 1500+rand()%1500;
    }
    
    void Aggro(Unit *pWho)
    {
        DoCast(m_creature, SPELL_HOLYFORM);
    }

    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim() || m_creature->IsPolymorphed())
            return;
        
        /*if (m_creature->IsNonMeleeSpellCasted(false))
            return;*/
            
        if (holyNovaTimer <= diff) {
            DoCast(m_creature, SPELL_HOLY_NOVA);
            holyNovaTimer = 2000+rand()%2000;
        }
        else
            holyNovaTimer -= diff;
            
        if (renewTimer <= diff) {
            if (rand()%4)
                DoCast(m_creature, SPELL_RENEW, true);
            else
                DoCast(DoSelectLowestHpFriendly(40.0f, 1), SPELL_RENEW);
            renewTimer = 6000+rand()%2000;
        }
        else
            renewTimer -= diff;
            
        DoMeleeAttackIfReady();
    }
        
};

CreatureAI* GetAI_npc_sunblade_dawnpriest(Creature *pCreature)
{
    return new npc_sunblade_dawnpriest(pCreature);
}

/*######
## npc_sunblade_duskpriest
######*/

#define SPELL_MINDFLAY      46562
#define SPELL_FEAR          46561
#define SPELL_SW_PAIN       46560

struct npc_sunblade_duskpriest : public ScriptedAI
{
    npc_sunblade_duskpriest(Creature *c) : ScriptedAI(c) {}
    
    uint32 mindFlayTimer;
    uint32 fearTimer;
    uint32 swPainTimer;
    
    void Reset()
    {
        fearTimer = 5000;
        swPainTimer = 1000;
    }
    
    void Aggro(Unit *pWho) {}

    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim() || m_creature->IsPolymorphed())
            return;
            
        if (fearTimer <= diff) {
            m_creature->InterruptNonMeleeSpells(true);
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_FEAR);
            fearTimer = 6000+rand()%4000;
            return;
        }
        else
            fearTimer -= diff;
            
        if (swPainTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_SW_PAIN);
            swPainTimer = 3000+rand()%2000;
        }
        else
            swPainTimer -= diff;
        
        //if (m_creature->IsNonMeleeSpellCasted(false))
        if (mindFlayTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_MINDFLAY);
            mindFlayTimer = 6000+rand()%7000;
        }
        else
            mindFlayTimer -= diff;
            
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_sunblade_duskpriest(Creature *pCreature)
{
    return new npc_sunblade_duskpriest(pCreature);
}

/*######
## npc_sunblade_archmage
######*/

#define SPELL_ARCANE_EXPLO  46553
#define SPELL_FROST_NOVA    46555
#define SPELL_BLINK         1953        // Just for visual, blink is bugged for creatures atm

struct npc_sunblade_archmage : public ScriptedAI
{
    npc_sunblade_archmage(Creature *c) : ScriptedAI(c) {}
    
    uint32 arcaneExploTimer;
    uint32 frostNovaTimer;
    uint32 blinkTimer;
    uint32 changeTargetTimer;
    
    void Reset()
    {
        arcaneExploTimer = 1000+rand()%2000;
        frostNovaTimer = 6000+rand()%2000;
        blinkTimer = 8000;
        changeTargetTimer = 8000+rand()%2000;
    }
    
    void Aggro(Unit *pWho) {}

    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim() || m_creature->IsPolymorphed())
            return;
            
        //if (!m_creature->IsWithinLOSInMap(m_creature->getVictim()))
            //DoStartMovement(m_creature->getVictim(), 8, 0);
            
        if (arcaneExploTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_ARCANE_EXPLO);
            arcaneExploTimer = 4000+rand()%2000;
            return;
        }
        else
            arcaneExploTimer -= diff;
            
        if (frostNovaTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_FROST_NOVA);
            frostNovaTimer = 6000+rand()%2000;
            return;
        }
        else
            frostNovaTimer -= diff;
            
        /*if (blinkTimer <= diff) {
            bool InMeleeRange = false;
            std::list<HostilReference*>& t_list = m_creature->getThreatManager().getThreatList();
            for(std::list<HostilReference*>::iterator itr = t_list.begin(); itr!= t_list.end(); ++itr) {
                if(Unit* target = Unit::GetUnit(*m_creature, (*itr)->getUnitGuid())) {
                    //if in melee range
                    if (target->IsWithinDistInMap(m_creature, 5)) {
                        InMeleeRange = true;
                        break;
                    }
                }
            }
            //if anybody is in melee range than escape by blink
            if (InMeleeRange) {
                DoCast(m_creature, SPELL_BLINK);  // For visual only
                float x,y,z;
                m_creature->GetPosition(x,y,z);
                x = rand()%2 ? x+10+rand()%10 : x-10-rand()%10;
                y = rand()%2 ? y+10+rand()%10 : y-10-rand()%10;
                DoTeleportTo(x, y, z);
            }
            blinkTimer = 8000;
            return;
        }
        else 
            blinkTimer -= diff;*/
            
        if (changeTargetTimer <= diff) {
            AttackStart(SelectUnit(SELECT_TARGET_RANDOM, 0));
            changeTargetTimer = 8000+rand()%2000;
        }
        else
            changeTargetTimer -= diff;
            
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_sunblade_archmage(Creature *pCreature)
{
    return new npc_sunblade_archmage(pCreature);
}

/*######
## npc_shadowsword_vanquisher
######*/

#define SPELL_CLEAVE        46468
#define SPELL_MELT_ARMOR    46469

struct npc_shadowsword_vanquisherAI : public ScriptedAI
{
    npc_shadowsword_vanquisherAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }
    
    uint32 cleaveTimer;
    uint32 meltArmorTimer;
    
    ScriptedInstance *pInstance;
    
    void Reset()
    {
        cleaveTimer = 5000;
        meltArmorTimer = 2000;
    }
    
    void Aggro(Unit *pWho)
    {
        if (pInstance && pInstance->GetData(DATA_GAUNTLET_EVENT) == NOT_STARTED)
            pInstance->SetData(DATA_GAUNTLET_EVENT, IN_PROGRESS);
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        if (cleaveTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_CLEAVE);
            cleaveTimer = 5000+rand()%3000;
        }
        else
            cleaveTimer -= diff;
            
        if (meltArmorTimer <= diff) {
            if (!m_creature->getVictim()->HasAura(SPELL_MELT_ARMOR)) {
                DoCast(m_creature->getVictim(), SPELL_MELT_ARMOR);
            }
            meltArmorTimer = 10000+rand()%5000;
        }
        else
            meltArmorTimer -= diff;
            
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_shadowsword_vanquisher(Creature *pCreature)
{
    return new npc_shadowsword_vanquisherAI(pCreature);
}

/*######
## npc_shadowsword_manafiend
######*/

#define SPELL_ARCANE_EXPLOSION_MANAFIEND    46457
#define SPELL_DRAIN_MANA                    46453

struct npc_shadowsword_manafiendAI : public ScriptedAI
{
    npc_shadowsword_manafiendAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }
    
    uint32 arcaneExploTimer;
    uint32 drainManaTimer;
    
    ScriptedInstance *pInstance;
    
    void Reset()
    {
        arcaneExploTimer = 8000+rand()%2000;
        drainManaTimer = 15000+rand()%5000;
    }
    
    void Aggro(Unit *pWho)
    {
        if (pInstance && pInstance->GetData(DATA_GAUNTLET_EVENT) == NOT_STARTED)
            pInstance->SetData(DATA_GAUNTLET_EVENT, IN_PROGRESS);
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        if (arcaneExploTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_ARCANE_EXPLOSION_MANAFIEND);
            arcaneExploTimer = 8000+rand()%2000;
        }
        else
            arcaneExploTimer -= diff;
            
        if (drainManaTimer <= diff) {
            if (m_creature->GetPower(POWER_MANA) <= ((m_creature->GetMaxPower(POWER_MANA) / 100.0f) * 10.0f)) {
                DoCast(SelectUnit(SELECT_TARGET_RANDOM, 1), SPELL_DRAIN_MANA);
                drainManaTimer = 15000+rand()%5000;
            }
        }
        else
            drainManaTimer -= diff;
        
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_shadowsword_manafiend(Creature *pCreature)
{
    return new npc_shadowsword_manafiendAI(pCreature);
}

/*######
## npc_shadowsword_lifeshaper
######*/

#define SPELL_DRAIN_LIFE        46466
#define SPELL_HEALTH_FUNNEL     46467

struct npc_shadowsword_lifeshaperAI : public ScriptedAI
{
    npc_shadowsword_lifeshaperAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }
    
    uint32 drainLifeTimer;
    uint32 healthFunnelTimer;
    
    ScriptedInstance *pInstance;
    
    void Reset()
    {
        drainLifeTimer = 15000+rand()%5000;
        healthFunnelTimer = 18000+rand()%5000;
    }
    
    void Aggro(Unit *pWho)
    {
        if (pInstance && pInstance->GetData(DATA_GAUNTLET_EVENT) == NOT_STARTED)
            pInstance->SetData(DATA_GAUNTLET_EVENT, IN_PROGRESS);
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        if (drainLifeTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_DRAIN_LIFE);
            drainLifeTimer = 15000+rand()%5000;
        }
        else
            drainLifeTimer -= diff;
            
        if (healthFunnelTimer <= diff) {
            DoCast(DoSelectLowestHpFriendly(40.0f, 1), SPELL_HEALTH_FUNNEL);
            healthFunnelTimer = 15000+rand()%5000;
        }
        else
            healthFunnelTimer -= diff;
        
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_shadowsword_lifeshaper(Creature *pCreature)
{
    return new npc_shadowsword_lifeshaperAI(pCreature);
}

/*######
## npc_shadowsword_soulbinder
######*/

#define SPELL_CURSE_EXHAUSTION      46434
#define SPELL_FLASH_DARKNESS        46442
#define SPELL_DOMINATION            46427

struct npc_shadowsword_soulbinderAI : public ScriptedAI
{
    npc_shadowsword_soulbinderAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }
    
    uint32 curseExhaustionTimer;
    uint32 flashDarknessTimer;
    uint32 dominationTimer;
    uint32 despawnTimer;
    
    ScriptedInstance *pInstance;
    
    void Reset()
    {
        curseExhaustionTimer = 5000+rand()%3000;
        flashDarknessTimer = 8000+rand()%2000;
        dominationTimer = 15000;
        despawnTimer = 0;
    }
    
    void Aggro(Unit *pWho) {}
    
    void MovementInform(uint32 type, uint32 i)
    {
        //sLog.outString("Reached waypoint %u (type %u)", i, type);
        if (type == WAYPOINT_MOTION_TYPE && i == 11)
            despawnTimer = 2000;
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (despawnTimer) {
            if (despawnTimer <= diff) {
                m_creature->DisappearAndDie();
                return;
            }
            else
                despawnTimer -= diff;
        }
        
        if (!UpdateVictim())
            return;
            
        if (curseExhaustionTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_CURSE_EXHAUSTION);
            curseExhaustionTimer = 5000+rand()%3000;
        }
        else
            curseExhaustionTimer -= diff;
            
        if (flashDarknessTimer <= diff) {
            DoCast(m_creature->getVictim(), SPELL_FLASH_DARKNESS);
            flashDarknessTimer = 8000+rand()%2000;
        }
        else
            flashDarknessTimer -= diff;
            
        if (dominationTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 1, 40.0f, true), SPELL_DOMINATION);
            dominationTimer = 15000;
        }
        else
            dominationTimer -= diff;
        
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_shadowsword_soulbinder(Creature *pCreature)
{
    return new npc_shadowsword_soulbinderAI(pCreature);
}

/*######
## npc_shadowsword_deathbringer
######*/

#define SPELL_DISEASE_BUFFET        46481
#define SPELL_VOLATILE_DISEASE      46483

struct npc_shadowsword_deathbringerAI : public ScriptedAI
{
    npc_shadowsword_deathbringerAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }
    
    uint32 buffetTimer;
    uint32 volatileTimer;
    uint32 despawnTimer;
    
    ScriptedInstance *pInstance;
    
    void Reset()
    {
        buffetTimer = 5000+rand()%5000;
        volatileTimer = 15000+rand()%5000;
        despawnTimer = 0;
    }
    
    void Aggro(Unit *pWho) {}
    
    void MovementInform(uint32 type, uint32 i)
    {
        if (type == WAYPOINT_MOTION_TYPE && i == 11)
            despawnTimer = 2000;
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (despawnTimer) {
            if (despawnTimer <= diff) {
                m_creature->DisappearAndDie();
                return;
            }
            else
                despawnTimer -= diff;
        }
        
        if (!UpdateVictim())
            return;
        
        if (buffetTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0), SPELL_DISEASE_BUFFET);
            buffetTimer = 5000+rand()%5000;
        }
        else
            buffetTimer -= diff;
            
        if (volatileTimer <= diff) {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0, 40.0f, true), SPELL_VOLATILE_DISEASE);
            volatileTimer = 15000+rand()%5000;
        }
        else
            volatileTimer -= diff;
            
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_shadowsword_deathbringer(Creature *pCreature)
{
    return new npc_shadowsword_deathbringerAI(pCreature);
}

/*######
## npc_volatile_fiend
######*/

#define SPELL_BURNING_DESTRUCTION               47287
#define SPELL_BURNING_DESTRUCTION_TRIGGERED     46218
#define SPELL_BURNING_WINDS                     46308
#define SPELL_FELFIRE_FISSION                   45779

struct npc_volatile_fiendAI : public ScriptedAI
{
    npc_volatile_fiendAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }
    
    uint32 despawnTimer;    // FIXME: Need new hook OnSpellTriggered and despawn when spell 46218 is triggered
    uint32 damageTimer;
    uint32 fissionTimer;
    
    ScriptedInstance *pInstance;
    
    void Reset()
    {
        //DoCast(m_creature, SPELL_BURNING_WINDS);
        despawnTimer = 0;
        damageTimer = 1000;
        fissionTimer = 2000;
        
        m_creature->ApplySpellImmune(0, IMMUNITY_ID, 1499, true);
        m_creature->ApplySpellImmune(0, IMMUNITY_ID, 14310, true);
        m_creature->ApplySpellImmune(0, IMMUNITY_ID, 14311, true);
    }
    
    void Aggro(Unit *pWho) {}
    
    void DamageTaken(Unit *done_by, uint32 &damage)
    {
        DoCast(m_creature, SPELL_BURNING_DESTRUCTION);
    }
    
    void JustDied(Unit *pKilled)
    {
        DoCast(m_creature, SPELL_BURNING_DESTRUCTION_TRIGGERED, true);
    }
    
    void OnSpellFinish(Unit *caster, uint32 spellId, Unit *target, bool ok)
    {
        if (spellId == 47287)
            despawnTimer = 2100;
    }
    
    void MovementInform(uint32 type, uint32 i)
    {
        if (type == WAYPOINT_MOTION_TYPE && i == 11)
            despawnTimer = 2000;
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (despawnTimer) {
            if (despawnTimer <= diff) {
                m_creature->DisappearAndDie();
                return;
            }
            else
                despawnTimer -= diff;
        }

        if (!UpdateVictim())
            return;
            
        if (m_creature->IsWithinMeleeRange(m_creature->getVictim())) {
            if (damageTimer <= diff) {      // Should happen only one time, as creature explodes 2 sec after reaching melee
                DoCast(m_creature, SPELL_FELFIRE_FISSION);
                DoCast(m_creature, SPELL_BURNING_DESTRUCTION_TRIGGERED, true);
                
                damageTimer = 2000;
            }
            else
                damageTimer -= diff;
        
            if (fissionTimer <= diff) {
                DoCast(m_creature, SPELL_FELFIRE_FISSION);
                fissionTimer = 2000;
            }
            else
                fissionTimer -= diff;
        }
        
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_volatile_fiend(Creature *pCreature)
{
    return new npc_volatile_fiendAI(pCreature);
}

/*######
## npc_moorba
######*/

bool GossipHello_npc_moorba(Player* pPlayer, Creature* pCreature)
{
    if (ScriptedInstance *pInstance = ((ScriptedInstance*)pCreature->GetInstanceData())) {
        if (pInstance->GetData(DATA_KALECGOS_EVENT) == DONE)
            pPlayer->ADD_GOSSIP_ITEM(0, "Téléportez-moi dans la salle de Kalecgos.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
        if (pInstance->GetData(DATA_EREDAR_TWINS_EVENT) == DONE)
            pPlayer->ADD_GOSSIP_ITEM(0, "Téléportez-moi dans la salle des Jumelles Erédar.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
        /*if (pInstance->GetData(DATA_BRUTALLUS_EVENT) == DONE)
            pPlayer->ADD_GOSSIP_ITEM(0, "Tééportez-moi dans la salle de Brutallus.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);*/
    }
    
    pPlayer->SEND_GOSSIP_MENU(pCreature->GetNpcTextId(), pCreature->GetGUID());
    
    return true;
}

bool GossipSelect_npc_moorba(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action)
{
    switch (action) {
    case GOSSIP_ACTION_INFO_DEF + 1:
        pPlayer->TeleportTo(580, 1703.977051, 928.625610, 53.077671, 4.748818);
        break;
    case GOSSIP_ACTION_INFO_DEF + 2:
        pPlayer->CastSpell(pPlayer, 46883, true);
        break;
    }
    
    return true;
}

struct npc_kalec_felmystAI : public ScriptedAI
{
    npc_kalec_felmystAI(Creature* c) : ScriptedAI(c)
    {
        me->SetDisableGravity(true);
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }
    
    uint32 waitTimer;
    uint8 phase;
    
    ScriptedInstance* pInstance;
    
    void Reset()
    {
        waitTimer = 0;
        phase = 0;
        
        me->GetMotionMaster()->MovePoint(0, 1483.408203, 717.707275, 93.492821, false);
        me->SetSpeed(MOVE_FLIGHT, 5.0f, true);
    }
    
    void Aggro(Unit* who) {}
    
    void MovementInform(uint32 type, uint32 id)
    {
        if (!pInstance)
            return;

        switch (id) {
        case 0:
        {
            waitTimer = 300;
            break;
        }
        case 1:
        {
            waitTimer = 2000;
            break;
        }
        case 2:
        {
            waitTimer = 500;
            break;
        }
        case 3:
            me->DisappearAndDie();
            break;
        }
    }
    
    void UpdateAI(uint32 const diff)
    {
        me->addUnitState(UNIT_STAT_IGNORE_PATHFINDING);

        if (waitTimer) {
            if (waitTimer <= diff) {
                switch (phase) {
                case 0: // Go to Felyst corpse
                {
                    if (!pInstance)
                        break;

                    if (Creature* felmyst = Unit::GetCreature(*me, pInstance ? pInstance->GetData64(DATA_FELMYST) : 0)) {
                        float x, y, z;
                        felmyst->GetPosition(x, y, z);
                        me->GetMotionMaster()->MovePoint(1, x, y, z+8, false);
                    }
                    waitTimer = 0;
                    phase++;
                    break;
                }
                case 1: // Yell on Felmyst corpse
                    DoScriptText(-1580043, me);
                    waitTimer = 8000;
                    phase++;
                    break;
                case 2: // Takeoff
                    me->GetMotionMaster()->MovePoint(2, 1534.859009, 535.921204, 45.530205, false);
                    waitTimer = 0;
                    phase++;
                    break;
                case 3: // Breathe on fire wall
                    DoCast(me, 46650);
                    if (pInstance)
                        pInstance->HandleGameObject(pInstance->GetData64(DATA_GO_FIRE_BARRIER), true);
                    waitTimer = 4000;
                    phase++;
                    break;
                case 4:
                    me->GetMotionMaster()->MovePoint(3, 1601.979736, 519.187988, 119.142479, false);
                    waitTimer = 0;
                    phase++;
                    break;
                }
            }
            else
                waitTimer -= diff;
        }
    }
};

CreatureAI* GetAI_npc_kalec_felmyst(Creature* creature)
{
    return new npc_kalec_felmystAI(creature);
}

/*######
## npc_doomfire_destroyer
######*/

#define SPELL_CREATE_DOOMFIRE               46306

struct npc_doomfire_destroyerAI : public ScriptedAI
{
    npc_doomfire_destroyerAI(Creature *c) : ScriptedAI(c), Summons(me)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }
    
    ScriptedInstance *pInstance;
    SummonList Summons;
    uint32 summonTimer;
    
    void Reset()
    {
        summonTimer = 10000;
        Summons.DespawnAll();
    }

    void Aggro(Unit* who) {}

    void JustSummoned(Creature* summoned)
    {
        Summons.Summon(summoned);
    }

    void SummonedCreatureDespawn(Creature* unit)
    {
        Summons.Despawn(unit);
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;

        if (summonTimer <= diff)
        {
            Unit* random = SelectUnit(SELECT_TARGET_RANDOM, 0, 100.0f, true);
            if (random)
                DoCast(random, SPELL_CREATE_DOOMFIRE, false);

            summonTimer = 10000;
        }
        else
            summonTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_doomfire_destroyer(Creature *pCreature)
{
    return new npc_doomfire_destroyerAI(pCreature);
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
    
    newscript = new Script;
    newscript->Name = "npc_sunblade_cabalist";
    newscript->GetAI = &GetAI_npc_sunblade_cabalist;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_fire_fiend";
    newscript->GetAI = &GetAI_npc_fire_fiend;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_sunblade_dawnpriest";
    newscript->GetAI = &GetAI_npc_sunblade_dawnpriest;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_sunblade_duskpriest";
    newscript->GetAI = &GetAI_npc_sunblade_duskpriest;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_sunblade_archmage";
    newscript->GetAI = &GetAI_npc_sunblade_archmage;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_shadowsword_vanquisher";
    newscript->GetAI = &GetAI_npc_shadowsword_vanquisher;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_shadowsword_manafiend";
    newscript->GetAI = &GetAI_npc_shadowsword_manafiend;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_shadowsword_lifeshaper";
    newscript->GetAI = &GetAI_npc_shadowsword_lifeshaper;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_shadowsword_soulbinder";
    newscript->GetAI = &GetAI_npc_shadowsword_soulbinder;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_shadowsword_deathbringer";
    newscript->GetAI = &GetAI_npc_shadowsword_deathbringer;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_volatile_fiend";
    newscript->GetAI = &GetAI_npc_volatile_fiend;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_moorba";
    newscript->pGossipHello = &GossipHello_npc_moorba;
    newscript->pGossipSelect = &GossipSelect_npc_moorba;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_kalec_felmyst";
    newscript->GetAI = &GetAI_npc_kalec_felmyst;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_doomfire_destroyer";
    newscript->GetAI = &GetAI_npc_doomfire_destroyer;
    newscript->RegisterSelf();
}
