#include "precompiled.h"

#define SPELL_COLD_SLAP             46145
#define SPELL_AHUNE_SHIELD          45954
#define SPELL_AHUNE_SLIP_FLOOR      45945   // Nice snow visual on floor
#define SPELL_AHUNE_SELF_STUN       46416
#define SPELL_AHUNE_EMERGE          46402
#define SPELL_AHUNE_GHOST_DISGUISE  46786
#define SPELL_SUBMERGE              37550

#define SPELL_ICE_SPEAR             46360

#define SPELL_HAILSTONE_CHILL       46458

#define NPC_AHUNITE_HAILSTONE       /*(heroicMode ? 26342 : */25755//)
#define NPC_AHUNITE_FROSTWIND       /*(heroicMode ? 26341 : */25757//)
#define NPC_AHUNITE_COLDWAVE        /*(heroicMode ? 26340 : */25756//)
#define NPC_AHUNE                   25740
#define NPC_AHUNE_FROZEN_CORE       25865
#define NPC_AHUNE_GHOST             26239
#define NPC_ICE_SPEAR_BUNNY         25985

#define GO_AHUNE_ICE_CHEST          187892

struct boss_ahuneAI : public Scripted_NoMovementAI
{
    boss_ahuneAI(Creature* c) : Scripted_NoMovementAI(c), summons(m_creature)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        heroicMode = m_creature->GetMap()->IsHeroic();
    }
    
    ScriptedInstance* pInstance;
    
    bool heroicMode;
    
    uint8 phase;
    
    uint32 phaseChangeTimer;
    uint32 summonTimer;
    uint32 slipperyFloorTimer;
    uint32 iceSpearTimer;
    
    SummonList summons;
    
    void Reset()
    {
        phase = 1;
        phaseChangeTimer = 90000;
        summonTimer = 8000;
        slipperyFloorTimer = 55000;
        iceSpearTimer = 8000;
        
        if (heroicMode) {
            me->SetMaxHealth(230000);
            me->SetHealth(me->GetMaxHealth());
        }
        
        DoCast(me, SPELL_AHUNE_SHIELD, true);
        me->RemoveAurasDueToSpell(SPELL_SUBMERGE);
        m_creature->RemoveFlag(UNIT_NPC_EMOTESTATE, EMOTE_STATE_SUBMERGED);
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        
        me->ApplySpellImmune(0, IMMUNITY_SCHOOL, SPELL_SCHOOL_MASK_FROST, true);
        
        summons.DespawnAll();
        
        if (Creature* frozenCore = me->FindCreatureInGrid(NPC_AHUNE_FROZEN_CORE, 10.0f, true))
            frozenCore->DisappearAndDie();
            
        if (Creature* frozenCore = me->SummonCreature(NPC_AHUNE_FROZEN_CORE, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0))
            frozenCore->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            
        if (Creature* ghost = me->FindCreatureInGrid(NPC_AHUNE_GHOST, 10.0f, true))
            ghost->DisappearAndDie();    
        
        if (Creature* ghost = me->SummonCreature(NPC_AHUNE_GHOST, -89.566528, -253.344315, -1.089609, 1.683741, TEMPSUMMON_MANUAL_DESPAWN, 0)) {
            ghost->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            ghost->SetVisibility(VISIBILITY_OFF);
        }
    }

    void Aggro(Unit* pWho) 
    {
        me->SummonCreature(NPC_AHUNITE_HAILSTONE, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 0);
        
        //DoCast(me, SPELL_AHUNE_SLIP_FLOOR, true);
    }

    void JustDied(Unit* pKiller)
    {
        if (Creature* frozenCore = me->FindCreatureInGrid(NPC_AHUNE_FROZEN_CORE, 10.0f, true))
            frozenCore->AI()->JustDied(pKiller);

        summons.DespawnAll();
        
        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
    }

    void JustSummoned(Creature* pSummon)
    {
        summons.Summon(pSummon);
        
        if (pSummon->GetEntry() == NPC_AHUNE_FROZEN_CORE)
            return;
        
        if (Unit *target = SelectUnit(SELECT_TARGET_RANDOM, 0))
            pSummon->AI()->AttackStart(target);
    }

    void SummonedCreatureDespawn(Creature* pSummon)
    {
        summons.Despawn(pSummon);
        
        if (pSummon->GetEntry() == NPC_AHUNITE_HAILSTONE)
            pInstance->RemoveAuraOnAllPlayers(SPELL_HAILSTONE_CHILL);
    }
    
    void HandleColdSlap()
    {
        Map::PlayerList const& players = pInstance->instance->GetPlayers();

        if (!players.isEmpty()) {
            for(Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr) {
                if (Player* plr = itr->getSource()) {
                    if (plr->IsWithinMeleeRange(me)) {
                        DoCast(plr, SPELL_COLD_SLAP);
                        plr->CastSpell(plr, SPELL_ICE_SPEAR, true);
                    }
                }
            }
        }
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;

        if (phase == 1) {
            HandleColdSlap();
            
            if (summonTimer <= diff) {
                me->SummonCreature(RAND(NPC_AHUNITE_FROSTWIND, NPC_AHUNITE_COLDWAVE), me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 0);
                
                summonTimer = 8000;
            }
            else
                summonTimer -= diff;
            
            DoMeleeAttackIfReady();
        }
        
        if (iceSpearTimer <= diff) {
            if (Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0, 150.0f, true))
                me->SummonCreature(NPC_ICE_SPEAR_BUNNY, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), target->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0);
            
            iceSpearTimer = 5000;
        }
        else
            iceSpearTimer -= diff;
        
        if (phaseChangeTimer <= diff) {
            phase = (phase == 1) ? 2 : 1;
            
            if (phase == 1) {
                me->RemoveAurasDueToSpell(SPELL_AHUNE_SELF_STUN);
                me->RemoveAurasDueToSpell(SPELL_SUBMERGE);
                m_creature->RemoveFlag(UNIT_NPC_EMOTESTATE, EMOTE_STATE_SUBMERGED);
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                DoCast(me, SPELL_AHUNE_EMERGE, true);
                DoCast(me, SPELL_AHUNE_SHIELD, true);
                me->SetReactState(REACT_AGGRESSIVE);
                
                if (Creature* ghost = me->FindCreatureInGrid(NPC_AHUNE_GHOST, 10.0f, true)) {
                    ghost->RemoveAurasDueToSpell(SPELL_AHUNE_GHOST_DISGUISE);
                    ghost->SetVisibility(VISIBILITY_OFF);
                }
                
                me->SummonCreature(NPC_AHUNITE_HAILSTONE, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 0);
                summonTimer = 4000;
                
                if (Creature* frozenCore = me->FindCreatureInGrid(NPC_AHUNE_FROZEN_CORE, 10.0f, true)) {
                    frozenCore->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    me->SetHealth(frozenCore->GetHealth());
                }
                    
                phaseChangeTimer = 90000;
            }
            else {
                me->RemoveAurasDueToSpell(SPELL_AHUNE_SHIELD);
                DoCast(me, SPELL_SUBMERGE);
                me->SetFlag(UNIT_NPC_EMOTESTATE, EMOTE_STATE_SUBMERGED);
                me->SetReactState(REACT_PASSIVE);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                
                if (Creature* frozenCore = me->FindCreatureInGrid(NPC_AHUNE_FROZEN_CORE, 10.0f, true)) {
                    frozenCore->SetHealth(me->GetHealth());
                    frozenCore->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                }
                    
                if (Creature* ghost = me->FindCreatureInGrid(NPC_AHUNE_GHOST, 10.0f, true)) {
                    ghost->AddAura(SPELL_AHUNE_GHOST_DISGUISE, ghost);
                    ghost->SetVisibility(VISIBILITY_ON);
                }
                    
                phaseChangeTimer = 30000;
            }
        }
        else
            phaseChangeTimer -= diff;
            
        if (slipperyFloorTimer <= diff) {
            //DoCast(me, SPELL_AHUNE_SLIP_FLOOR, true);
            
            slipperyFloorTimer = 55000;
        }
        else
            slipperyFloorTimer -= diff;
    }
};

CreatureAI* GetAI_boss_ahune(Creature* pCreature)
{
    return new boss_ahuneAI(pCreature);
}

struct boss_frozen_coreAI : public Scripted_NoMovementAI
{
    boss_frozen_coreAI(Creature* c) : Scripted_NoMovementAI(c) {}
    
    void Reset()
    {
        me->SetReactState(REACT_PASSIVE);
    }
    
    void Aggro(Unit* pWho) {}
    
    void JustDied(Unit* pKiller)
    {
        if (Creature* ahune = me->FindCreatureInGrid(NPC_AHUNE, 10.0f, true))
            me->Kill(ahune);

        pKiller->SummonGameObject(GO_AHUNE_ICE_CHEST, -96.525841, -200.255798, -1.262261, 4.748316, 0, 0, 0, 0, 86400);
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
    }
};

CreatureAI* GetAI_boss_frozen_core(Creature* pCreature)
{
    return new boss_frozen_coreAI(pCreature);
}

#define SPELL_PULVERIZE         2676
#define SPELL_CHILLING_AURA     46885

struct mob_ahunite_hailstoneAI : public ScriptedAI
{
    mob_ahunite_hailstoneAI(Creature* c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }
    
    uint32 pulverizeTimer;
    
    ScriptedInstance* pInstance;
    
    void Reset()
    {
        me->AddAura(SPELL_CHILLING_AURA, me);
        
        pulverizeTimer = 3000;
    }
    
    void Aggro(Unit* pWho) {}
    
    void JustDied(Unit* pKiller)
    {
        if (pInstance)
            pInstance->RemoveAuraOnAllPlayers(SPELL_HAILSTONE_CHILL);
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;

        if (pulverizeTimer <= diff) {
            DoCast(me->getVictim(), SPELL_PULVERIZE);
            
            pulverizeTimer = 6000;
        }
        else
            pulverizeTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mob_ahunite_hailstoneAI(Creature* pCreature)
{
    return new mob_ahunite_hailstoneAI(pCreature);
}

#define SPELL_KNOCKBACK_DELAYER     46878

#define GO_ICE_SPEAR                188077

struct npc_ice_spear_bunnyAI : public Scripted_NoMovementAI
{
    npc_ice_spear_bunnyAI(Creature* c) : Scripted_NoMovementAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }
    
    uint32 spawnTimer;
    uint32 activateTimer;
    uint32 deathTimer;
    
    ScriptedInstance* pInstance;
    
    void Reset()
    {
        me->SetReactState(REACT_PASSIVE);
        DoCast(me, SPELL_KNOCKBACK_DELAYER);
        
        spawnTimer = 500;
        activateTimer = 2500;
        deathTimer = 99999;
    }
    
    void Aggro(Unit* pWho) {}
    
    void HandleIceSpear()
    {
        if (pInstance) {
            Map::PlayerList const& players = pInstance->instance->GetPlayers();

            if (!players.isEmpty()) {
                for(Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr) {
                    if (Player* plr = itr->getSource()) {
                        if (plr->IsWithinMeleeRange(me))
                            plr->CastSpell(plr, SPELL_ICE_SPEAR, true);
                    }
                }
            }
        }
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (spawnTimer <= diff) {
            if (GameObject* go = me->SummonGameObject(GO_ICE_SPEAR, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ(), me->GetOrientation(), 0, 0, 0, 0, 86400))
                go->SetFlag(GAMEOBJECT_FLAGS, GO_FLAG_INTERACT_COND);
                
            spawnTimer = 99999;
        }
        else
            spawnTimer -= diff;
            
        if (activateTimer <= diff) {
            if (GameObject* go = me->FindGOInGrid(GO_ICE_SPEAR, 3.0f)) {
                go->UseDoorOrButton();
                HandleIceSpear();
                activateTimer = 99999;
                deathTimer = 1500;
            }
        }
        else
            activateTimer -= diff;
            
        if (deathTimer <= diff) {
            if (GameObject* go = me->FindGOInGrid(GO_ICE_SPEAR, 3.0f)) {
                go->SetLootState(GO_JUST_DEACTIVATED);
                go->SetRespawnTime(86400);     // One day
                me->DisappearAndDie();
            }
            
            deathTimer = 99999;
        }
        else
            deathTimer -= diff;
    }
};

CreatureAI* GetAI_npc_ice_spear_bunny(Creature* pCreature)
{
    return new npc_ice_spear_bunnyAI(pCreature);
}

void AddSC_boss_ahune()
{
    Script* newscript;
    
    newscript = new Script;
    newscript->Name = "boss_ahune";
    newscript->GetAI = &GetAI_boss_ahune;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "boss_frozen_core";
    newscript->GetAI = &GetAI_boss_frozen_core;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "mob_ahunite_hailstone";
    newscript->GetAI = &GetAI_mob_ahunite_hailstoneAI;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_ice_spear_bunny";
    newscript->GetAI = &GetAI_npc_ice_spear_bunny;
    newscript->RegisterSelf();
}
