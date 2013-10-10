/* Copyright (C) 2006 - 2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/ >
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
SDName: Boss_Supremus
SD%Complete: 95
SDComment: Need to implement molten punch
SDCategory: Black Temple
EndScriptData */

#include "precompiled.h"
#include "def_black_temple.h"

#define EMOTE_NEW_TARGET             -1564010
#define EMOTE_PUNCH_GROUND           -1564011                //DoScriptText(EMOTE_PUNCH_GROUND, m_creature);
#define EMOTE_GROUND_CRACK           -1564012

//Spells
#define SPELL_MOLTEN_PUNCH           40126 // Summon flamme
#define SPELL_HATEFUL_STRIKE         41926
#define SPELL_MOLTEN_FLAME           40980
#define SPELL_VOLCANIC_ERUPTION      40117 // eruption visual aura + trigger SPELL_VOLCANIC_GEYSER (modified duration index)
#define SPELL_VOLCANIC_GEYSER        42055 //18 secs aura triggered by SPELL_VOLCANIC_ERUPTION (trigger SPELL_VOLCANIC_GEYSER_DAMAGE every sec)
#define SPELL_VOLCANIC_GEYSER_DAMAGE 42052
#define SPELL_VOLCANIC_SUMMON        40276 // (modified duration index)
#define SPELL_BERSERK                45078
#define SPELL_CHARGE                 41581

#define TIMER_VOLCANO                10000
#define TIMER_VOLCANO_FIRST          2500
#define TIMER_HATEFUL_STRIKE         5000
#define TIMER_BERSERK                900000 // 15 minute enrage
#define TIMER_MOLTEN_PUNCH           20000
#define TIMER_PHASE_LENGHT           60000
#define TIMER_SWITCH_TARGET          10000

#define CREATURE_VOLCANO             23085
#define CREATURE_STALKER             23095 //summoned by molten punch, has CREATURE_FLAG_EXTRA_TRIGGER and spell[0]=40980 -> cast it at spawn

// spell SPELL_MOLTEN_FLAME is set in database and is autocasted at spawn
struct molten_flameAI : public ScriptedAI
{
    ScriptedInstance* pInstance;
    Unit* currentTarget;
    
    float x, y, z, groundZ;

    molten_flameAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        currentTarget = nullptr;
    }
    
    void UndermapCheck()
    {

        x = m_creature->GetPositionX();
        y = m_creature->GetPositionY();
        z = m_creature->GetPositionZ();
        groundZ = z;
            
        m_creature->UpdateGroundPositionZ(x, z, groundZ);
            
        if (z < groundZ)
            DoTeleportTo(x, y, groundZ);

    }

    void Reset() 
    {
        UndermapCheck();
        me->SetReactState(REACT_PASSIVE);
    }

    void RandomizeTarget()
    {
        if(!pInstance) 
            return;

        DoZoneInCombat();
        Creature* supremus = me->GetMap()->GetCreatureInMap(pInstance->GetData64(DATA_SUPREMUS));
        if(!supremus) 
            return;
        
        if( !(currentTarget = ((ScriptedAI*)supremus->AI())->SelectUnit(SELECT_TARGET_RANDOM,0,20.0f,100.0f,true)) )
            currentTarget = ((ScriptedAI*)supremus->AI())->SelectUnit(SELECT_TARGET_RANDOM,0,0.0f,100.0f,true);

        if(currentTarget)
            me->GetMotionMaster()->MoveFollowOnPoint(currentTarget);
    }
    
    void UpdateAI(uint32 const diff)
    {
        //change target if we reached it
        if(!currentTarget || me->GetDistance(currentTarget) < 2.0f)
            RandomizeTarget();
    }
};

struct boss_supremusAI : public ScriptedAI
{
    boss_supremusAI(Creature *c) : ScriptedAI(c), summons(m_creature)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
    }

    ScriptedInstance* pInstance;

    uint32 SummonFlameTimer;
    uint32 SwitchTargetTimer;
    uint32 PhaseSwitchTimer;
    uint32 SummonVolcanoTimer;
    uint32 HatefulStrikeTimer;
    uint32 BerserkTimer;

    uint8 currentPhase;

    SummonList summons;

    void Reset()
    {
        if(pInstance && me->isAlive())
            pInstance->SetData(DATA_SUPREMUSEVENT, NOT_STARTED);

        PhaseSwitchTimer = TIMER_PHASE_LENGHT;
        HatefulStrikeTimer = TIMER_HATEFUL_STRIKE;
        SummonFlameTimer = TIMER_MOLTEN_PUNCH;
        SwitchTargetTimer = TIMER_SWITCH_TARGET;
        SummonVolcanoTimer = TIMER_VOLCANO_FIRST;
        BerserkTimer = TIMER_BERSERK;                              // 15 minute enrage

        SetPhase(1);

        summons.DespawnAll();
    }

    void Aggro(Unit *who)
    {
        DoZoneInCombat();

        if(pInstance)
            pInstance->SetData(DATA_SUPREMUSEVENT, IN_PROGRESS);
    }

    void JustDied(Unit *killer)
    {
        if(pInstance)
            pInstance->SetData(DATA_SUPREMUSEVENT, DONE);

        summons.DespawnAll();
    }

    void JustSummoned(Creature *summon) 
    {
        summons.Summon(summon);
    }

    void SummonedCreatureDespawn(Creature *summon) 
    {
        summons.Despawn(summon);
    }

    void StopEruptions()
    {
        for(auto cGUID : summons)
        {
            Creature* volcano = me->GetMap()->GetCreatureInMap(cGUID);
            if(volcano && volcano->HasAura(SPELL_VOLCANIC_ERUPTION, 0))
                volcano->RemoveAura(SPELL_VOLCANIC_ERUPTION, 0);
        }
    }

    Unit* CalculateHatefulStrikeTarget()
    {
        uint32 maxhealthfound = 0;
        Unit* target = NULL;

        auto& m_threatlist = m_creature->getThreatManager().getThreatList();
        for (auto i : m_threatlist)
        {
            Unit* pUnit = me->GetMap()->GetCreatureInMap(i->getUnitGuid());
            if(pUnit && m_creature->IsWithinMeleeRange(pUnit))
            {
                if(pUnit->GetHealth() > maxhealthfound)
                {
                    maxhealthfound = pUnit->GetHealth();
                    target = pUnit;
                }
            }
        }

        return target;
    }

    void ChaseNewRandomTarget()
    {
        if(Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 1, 100, true))
        {
            if(m_creature->GetDistance2d(m_creature->getVictim()) > 40)
                m_creature->CastSpell(m_creature->getVictim(),SPELL_CHARGE,false);
                        
            DoResetThreat();
            m_creature->AddThreat(target, 5000000.0f);
            DoScriptText(EMOTE_NEW_TARGET, m_creature);
            SwitchTargetTimer = TIMER_SWITCH_TARGET;
        }
    }

    void SetPhase(uint8 phase)
    {
        if(phase == 1)
        {
            currentPhase = 1;
            PhaseSwitchTimer = TIMER_PHASE_LENGHT;
            m_creature->SetSpeed(MOVE_RUN, 1.2f);
            m_creature->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, false);
            m_creature->ApplySpellImmune(0, IMMUNITY_EFFECT,SPELL_EFFECT_ATTACK_ME, false);
            if(me->isInCombat())
            {
                StopEruptions();
                DoResetThreat();
                DoZoneInCombat();
                DoScriptText(EMOTE_PUNCH_GROUND, m_creature);
            }
        }
        else if(phase == 2)
        {
            currentPhase = 2;
            PhaseSwitchTimer = TIMER_PHASE_LENGHT;
            DoResetThreat();
            SwitchTargetTimer = TIMER_SWITCH_TARGET;
            SummonVolcanoTimer = TIMER_VOLCANO_FIRST;
            m_creature->SetSpeed(MOVE_RUN, 0.9f);
            DoZoneInCombat();
            m_creature->ApplySpellImmune(0, IMMUNITY_STATE, SPELL_AURA_MOD_TAUNT, true);
            m_creature->ApplySpellImmune(0, IMMUNITY_EFFECT,SPELL_EFFECT_ATTACK_ME, true);
            SwitchTargetTimer = 0;
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (!UpdateVictim())
            return;
        
        if(PhaseSwitchTimer < diff)
        {
            if(currentPhase == 1)
                SetPhase(2);
            else
                SetPhase(1);
        }else PhaseSwitchTimer -= diff;

        if(BerserkTimer < diff)
        {
            if(!m_creature->HasAura(SPELL_BERSERK, 0))
                DoCast(m_creature, SPELL_BERSERK);
        }
        else BerserkTimer -= diff;

        if(SummonFlameTimer < diff)
        {
            DoCast(m_creature, SPELL_MOLTEN_PUNCH);
            SummonFlameTimer = TIMER_MOLTEN_PUNCH;
        }else SummonFlameTimer -= diff;

        switch(currentPhase)
        {
        case 1:

            if(HatefulStrikeTimer < diff)
            {
                if(Unit* target = CalculateHatefulStrikeTarget())
                {
                    DoCast(target, SPELL_HATEFUL_STRIKE);
                    HatefulStrikeTimer = TIMER_HATEFUL_STRIKE;
                }
            }else HatefulStrikeTimer -= diff;

            break;
        case 2:

            if(SwitchTargetTimer < diff)
            {
                ChaseNewRandomTarget();
            }else SwitchTargetTimer -= diff;

            if(SummonVolcanoTimer < diff)
            {
                if(Unit* target = SelectUnit(SELECT_TARGET_RANDOM, 0, 150.0f, true))
                {
                    DoCast(target, SPELL_VOLCANIC_SUMMON);
                    DoScriptText(EMOTE_GROUND_CRACK, m_creature);
                    SummonVolcanoTimer = TIMER_VOLCANO;
                }
            }else SummonVolcanoTimer -= diff;

            break;
        }

        DoMeleeAttackIfReady();
    }
};

struct npc_volcanoAI : public ScriptedAI
{
    npc_volcanoAI(Creature *c) : ScriptedAI(c)
    { }
     
    uint32 UnderMapCheckTimer;
    
    float currentX, currentY, currentZ, groundZ;

    void Reset()
    {
        UnderMapCheckTimer = 0;

        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

        me->SetReactState(REACT_PASSIVE);

        DoCast(m_creature, SPELL_VOLCANIC_ERUPTION, true);
    }
    
    void UndermapCheck()
    {
        currentX = m_creature->GetPositionX();
        currentY = m_creature->GetPositionY();
        currentZ = m_creature->GetPositionZ();
        groundZ = currentZ;
            
        m_creature->UpdateGroundPositionZ(currentX, currentY, groundZ);
            
        if (currentZ < groundZ)
            DoTeleportTo(currentX, currentY, groundZ);
    }

    void UpdateAI(const uint32 diff)
    {        
        if (UnderMapCheckTimer < diff)
        {
            UndermapCheck();                
            UnderMapCheckTimer = 750;
        }else UnderMapCheckTimer -= diff;
    }
};

CreatureAI* GetAI_boss_supremus(Creature* c)
{
    return new boss_supremusAI (c);
}

CreatureAI* GetAI_molten_flame(Creature* c)
{
    return new molten_flameAI (c);
}

CreatureAI* GetAI_npc_volcano(Creature* c)
{
    return new npc_volcanoAI (c);
}

void AddSC_boss_supremus()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name="boss_supremus";
    newscript->GetAI = &GetAI_boss_supremus;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="molten_flame";
    newscript->GetAI = &GetAI_molten_flame;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_volcano";
    newscript->GetAI = &GetAI_npc_volcano;
    newscript->RegisterSelf();
}

