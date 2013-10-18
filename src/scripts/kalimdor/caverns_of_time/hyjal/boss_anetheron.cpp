
#include "precompiled.h"
#include "def_hyjal.h"
#include "hyjal_trash.h"

enum Spells
{
    SPELL_CARRION_SWARM  = 31306,
    SPELL_SLEEP          = 31298,
    SPELL_VAMPIRIC_AURA  = 38196,
    SPELL_INFERNO        = 31299
};

enum Timers
{
    TIMER_INFERNO             = 50000,
    TIMER_INFERNO_START       = 20000,
    TIMER_SLEEP               = 20000,
    TIMER_CARRION_SWARM       = 15000,
    TIMER_CARRION_SWARM_FIRST = 25000
};

enum Texts {
	SAY_ONDEATH = -1801010,
	SAY_ONSLAY1 = -1801011,
	SAY_ONSLAY2 = -1801012,
	SAY_ONSLAY3 = -1801013,
	SAY_SWARM1 = -1801014,
	SAY_SWARM2 = -1801015,
	SAY_SLEEP1 = -1801016,
	SAY_SLEEP2 = -1801017,
	SAY_INFERNO1 = -1801018,
	SAY_INFERNO2 = -1801019,
	SAY_ONAGGRO = -1801020
};

struct boss_anetheronAI : public hyjal_trashAI
{
    boss_anetheronAI(Creature *c) : hyjal_trashAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        go = false;
        pos = 0;
        SpellEntry *TempSpell = (SpellEntry*)spellmgr.spellmgr.LookupSpell(SPELL_SLEEP);
        if(TempSpell && TempSpell->EffectImplicitTargetA[0] != 1)
        {
            TempSpell->EffectImplicitTargetA[0] = 1;
            TempSpell->EffectImplicitTargetB[0] = 0;
        }
    }

    uint32 SwarmTimer;
    uint32 SleepTimer;
    uint32 InfernoTimer;
    bool go;
    uint32 pos;

    void Reset()
    {
        damageTaken = 0;
        SwarmTimer = TIMER_CARRION_SWARM_FIRST;
        SleepTimer = TIMER_SLEEP;
        InfernoTimer = TIMER_INFERNO_START;

        DoCast(m_creature, SPELL_VAMPIRIC_AURA,true);

        if(pInstance && IsEvent)
            pInstance->SetData(DATA_ANETHERONEVENT, NOT_STARTED);
    }

    void Aggro(Unit *who)
    {
        if(pInstance && IsEvent)
            pInstance->SetData(DATA_ANETHERONEVENT, IN_PROGRESS);

        DoScriptText(SAY_ONAGGRO,me);
    }

    void KilledUnit(Unit *victim)
    {
        DoScriptText(SAY_ONSLAY1 - rand()%3,me);
    }

    void WaypointReached(uint32 i)
    {
        pos = i;
        if (i == 7 && pInstance)
        {
            Unit* target = Unit::GetUnit((*m_creature), pInstance->GetData64(DATA_JAINAPROUDMOORE));
            if (target && target->isAlive())
                m_creature->AddThreat(target,0.0);
        }
    }

    void JustDied(Unit *victim)
    {
        hyjal_trashAI::JustDied(victim);
        if(pInstance && IsEvent)
            pInstance->SetData(DATA_ANETHERONEVENT, DONE);

        DoScriptText(SAY_ONDEATH,me);
    }

    void UpdateAI(const uint32 diff)
    {
        if (IsEvent)
        {
            //Must update npc_escortAI
            npc_escortAI::UpdateAI(diff);
            if(!go)
            {
                go = true;
                if(pInstance)
                {
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(0, 4896.08,    -1576.35,    1333.65);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(1, 4898.68,    -1615.02,    1329.48);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(2, 4907.12,    -1667.08,    1321.00);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(3, 4963.18,    -1699.35,    1340.51);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(4, 4989.16,    -1716.67,    1335.74);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(5, 5026.27,    -1736.89,    1323.02);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(6, 5037.77,    -1770.56,    1324.36);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(7, 5067.23,    -1789.95,    1321.17);
                    ((npc_escortAI*)(m_creature->AI()))->Start(false, true, true);
                    ((npc_escortAI*)(m_creature->AI()))->SetDespawnAtEnd(false);
                }
            }
        }

        //Return since we have no target
        if (!UpdateVictim() )
            return;

        if(SwarmTimer < diff)
        {
            Unit* target = SelectUnit(SELECT_TARGET_RANDOM,0,100,true);
            if(target)
                DoCast(target,SPELL_CARRION_SWARM);

            SwarmTimer = TIMER_CARRION_SWARM;
            DoScriptText(SAY_SWARM1 - rand()%2,me);
        }else SwarmTimer -= diff;

        if(SleepTimer < diff)
        {
            for(uint8 i=0;i<3;++i)
            {
                Unit* target = SelectUnit(SELECT_TARGET_RANDOM,0,100,true);
                if(target)
                    target->CastSpell(target,SPELL_SLEEP,true);
            }
            SleepTimer = TIMER_SLEEP;
            DoScriptText(SAY_SLEEP1 - rand()%2,me);
        }else SleepTimer -= diff;

        if(InfernoTimer < diff)
        {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM,0,100,true), SPELL_INFERNO);
            InfernoTimer = TIMER_INFERNO;
            DoScriptText(SAY_INFERNO1 - rand()%2,me);
        }else InfernoTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_anetheron(Creature *_Creature)
{
    return new boss_anetheronAI (_Creature);
}

enum InfernoSpells
{
    SPELL_IMMOLATION     = 31303,
    SPELL_INFERNO_EFFECT = 31302
};

enum InfernoTimers
{
    TIMER_IMMOLATION     = 2000
};

struct mob_towering_infernalAI : public ScriptedAI
{
    mob_towering_infernalAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        if(pInstance)
            AnetheronGUID = pInstance->GetData64(DATA_ANETHERON);
    }

    uint32 ImmolationTimer;
    uint32 CheckTimer;
    uint64 AnetheronGUID;
    ScriptedInstance* pInstance;

    void Reset()
    {
        DoCast(m_creature, SPELL_INFERNO_EFFECT);
        ImmolationTimer = TIMER_IMMOLATION;
        CheckTimer = 5000;
    }

    void Aggro(Unit *who)
    {

    }

    void KilledUnit(Unit *victim)
    {

    }

    void JustDied(Unit *victim)
    {

    }

    void MoveInLineOfSight(Unit *who)
    {
        if (m_creature->GetDistance(who) <= 50 && !InCombat && m_creature->IsHostileTo(who))
        {
            m_creature->AddThreat(who,0.0);
            m_creature->Attack(who,false);
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if(CheckTimer < diff)
        {
            if(AnetheronGUID)
            {
                Creature* boss = Unit::GetCreature((*m_creature),AnetheronGUID);
                if(!boss || (boss && boss->isDead()))
                {
                    m_creature->setDeathState(JUST_DIED);
                    m_creature->RemoveCorpse();
                    return;
                }
            }
            CheckTimer = 5000;
        }else CheckTimer -= diff;

        //Return since we have no target
        if (!UpdateVictim())
            return;

        if(ImmolationTimer < diff)
        {
            DoCast(m_creature, SPELL_IMMOLATION);
            ImmolationTimer = TIMER_IMMOLATION;
        }else ImmolationTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mob_towering_infernal(Creature *_Creature)
{
    return new mob_towering_infernalAI (_Creature);
}

void AddSC_boss_anetheron()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name="boss_anetheron";
    newscript->GetAI = &GetAI_boss_anetheron;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="mob_towering_infernal";
    newscript->GetAI = &GetAI_mob_towering_infernal;
    newscript->RegisterSelf();
}
