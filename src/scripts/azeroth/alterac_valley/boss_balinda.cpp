/* Copyright (C) 2006 - 2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* ScriptData
SDName: Boss_Balinda
SD%Complete: 
SDComment: Timers should be adjusted
EndScriptData */

#include "precompiled.h"

#define YELL_AGGRO                  -2100019
#define YELL_EVADE                  -2100020

#define SPELL_ARCANE_EXPLOSION      46608
#define SPELL_CONE_OF_COLD          38384
#define SPELL_FIREBALL              46988
#define SPELL_FROSTBOLT             46987
#define WATER_ELEMENTAL             25040
#define SPELL_WATERBOLT             46983

struct TRINITY_DLL_DECL mob_water_elementalAI : public ScriptedAI
{
    mob_water_elementalAI(Creature *c) : ScriptedAI(c) 
    {
        SetRestoreCombatMovementOnOOM(true);
    }

    uint32 WaterBoltTimer;
    uint64 balindaGUID;
    uint32 ResetTimer;

    void Reset() {
        WaterBoltTimer  = 3000;
        ResetTimer      = 5000;
        SetCombatMovementAllowed(false);
    }
    
    void JustDied(Unit* killer);
    void SummonedCreatureDespawn(Creature*);
    
    void UpdateAI(const uint32 diff) {
        if (!UpdateVictim())
            return;

        if (WaterBoltTimer <= diff) {
            DoCast(m_creature->GetVictim(), SPELL_WATERBOLT);
            WaterBoltTimer = 5000;
        } else WaterBoltTimer -= diff;

        // check if creature is not outside of building
        if (ResetTimer <= diff) {
            float x, y, z;
            m_creature->GetPosition(x, y, z);
            if (x > -6)
                EnterEvadeMode();
            ResetTimer = 5000;
        } else ResetTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

struct TRINITY_DLL_DECL boss_balindaAI : public ScriptedAI
{
    boss_balindaAI(Creature *c) : ScriptedAI(c), summons(m_creature) 
    {
        SetRestoreCombatMovementOnOOM(true);
    }


    uint32 ArcaneExplosionTimer;
    uint32 ConeofcoldTimer;
    uint32 FireboltTimer;
    uint32 FrostboltTimer;
    uint32 ResetTimer;
    uint32 WaterElementalTimer;
       
    SummonList summons;
       
    void Reset() 
    {
        ArcaneExplosionTimer    = (10+rand()%5)*1000;
        ConeofcoldTimer         = 8000;
        FireboltTimer           = 1000;
        FrostboltTimer          = 4000;
        ResetTimer              = 5000;
        WaterElementalTimer     = 0;

        summons.DespawnAll();
        
        me->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_HASTE_SPELLS, true);
        SetCombatMovementAllowed(false);
    }

    void EnterCombat(Unit *who) {
        DoScriptText(YELL_AGGRO, m_creature);
    }

    void KilledUnit(Unit* victim) {}

    void JustDied(Unit* Killer) {
        summons.DespawnAll();
    }
    
    void JustSummoned(Creature* pSummon)
    {
        summons.Summon(pSummon);
    }
    
    void SummonedCreatureDespawn(Creature* pSummon) { summons.Despawn(pSummon); }

    void UpdateAI(const uint32 diff)
    {
        if (!UpdateVictim())
            return;

        if (WaterElementalTimer <= diff) {
            if (Creature* elemental = DoSpawnCreature(WATER_ELEMENTAL, 0, 0, 0, 0, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 45000)) {
                ((mob_water_elementalAI*)elemental->AI())->balindaGUID = m_creature->GetGUID();
                elemental->SetLevel(70);
                elemental->AI()->AttackStart(SelectUnit(SELECT_TARGET_RANDOM, 0));
                elemental->setFaction(m_creature->getFaction());         
            }
            WaterElementalTimer = 50000;
        } else WaterElementalTimer -= diff;

        if (ArcaneExplosionTimer <= diff) {
            if(DoCast(m_creature->GetVictim(), SPELL_ARCANE_EXPLOSION) == SPELL_CAST_OK)
                ArcaneExplosionTimer = (10+rand()%5)*1000;
        } else ArcaneExplosionTimer -= diff;

        if (ConeofcoldTimer <= diff) {
            if(DoCast(m_creature->GetVictim(), SPELL_CONE_OF_COLD) == SPELL_CAST_OK)
                ConeofcoldTimer = (10+rand()%10)*1000;
        } else ConeofcoldTimer -= diff;

        if (FireboltTimer <= diff) {
            if(DoCast(m_creature->GetVictim(), SPELL_FIREBALL) == SPELL_CAST_OK)
                FireboltTimer = (5+rand()%4)*1000;
        } else FireboltTimer -= diff;

        if (FrostboltTimer <= diff) {
            if(DoCast(m_creature->GetVictim(), SPELL_FROSTBOLT) == SPELL_CAST_OK)
                FrostboltTimer = (4+rand()%8)*1000;
        } else FrostboltTimer -= diff;

        // check if creature is not outside of building
        if(ResetTimer <= diff) {
            float x, y, z;
            m_creature->GetPosition(x, y, z);
            if (x > -6) {
                DoScriptText(YELL_EVADE, m_creature);
                EnterEvadeMode();
            }
            ResetTimer = 5000;
        } else ResetTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

void mob_water_elementalAI::JustDied(Unit *killer)
{
       Creature* balinda = (Unit::GetCreature((*m_creature), balindaGUID));
       ((boss_balindaAI*)balinda->AI())->WaterElementalTimer = 8000;
}

void mob_water_elementalAI::SummonedCreatureDespawn(Creature*)
{
       Creature* balinda = (Unit::GetCreature((*m_creature), balindaGUID));
       ((boss_balindaAI*)balinda->AI())->WaterElementalTimer = 8000;
}

CreatureAI* GetAI_boss_balinda(Creature *_Creature)
{
    return new boss_balindaAI (_Creature);
}

CreatureAI* GetAI_mob_water_elemental(Creature *_Creature)
{
    return new mob_water_elementalAI (_Creature);
}

void AddSC_boss_balinda()
{
    Script *newscript;
    
    newscript = new Script;
    newscript->Name = "boss_balinda";
    newscript->GetAI = &GetAI_boss_balinda;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_water_elemental";
    newscript->GetAI = &GetAI_mob_water_elemental;
    newscript->RegisterSelf();
};
