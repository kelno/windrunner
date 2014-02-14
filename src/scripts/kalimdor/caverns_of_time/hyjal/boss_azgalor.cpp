#include "precompiled.h"
#include "def_hyjal.h"
#include "hyjal_trash.h"

enum Spells
{
    SPELL_RAIN_OF_FIRE      = 31340,
    SPELL_DOOM              = 31347,
    SPELL_HOWL_OF_AZGALOR   = 31344,
    SPELL_CLEAVE            = 31345,
    SPELL_BERSERK           = 26662,
};

enum Timers
{
    TIMER_BERSERK            = 600000,
    TIMER_DOOM               = 45000,
    TIMER_CLEAVE             = 5000,
    TIMER_HOWL_OF_AZGALOR    = 15000,
    TIMER_RAIN_OF_FIRE       = 13000,
    TIMER_RAIN_OF_FIRE_FIRST = 30000,
};

enum Texts
{
	SAY_ONDEATH = -1801022,
	SAY_ONSLAY1 = -1801023,
	SAY_ONSLAY2 = -1801024,
	SAY_ONSLAY3 = -1801025,
	SAY_DOOM1 = -1801026,
	SAY_DOOM2 = -1801027,
	SAY_ONAGGRO = -1801028
};

struct boss_azgalorAI : public hyjal_trashAI
{
    boss_azgalorAI(Creature *c) : hyjal_trashAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        go = false;
        pos = 0;
        SpellEntry *TempSpell = (SpellEntry*)spellmgr.LookupSpell(SPELL_HOWL_OF_AZGALOR);
        if(TempSpell)
            TempSpell->EffectRadiusIndex[0] = 12;//100yards instead of 50000?!
    }

    uint32 RainTimer;
    uint32 DoomTimer;
    uint32 HowlTimer;
    uint32 CleaveTimer;
    uint32 EnrageTimer;

    bool go;
    uint32 pos;

    void Reset()
    {
        damageTaken = 0;
        RainTimer = TIMER_RAIN_OF_FIRE_FIRST;
        DoomTimer = TIMER_DOOM;
        HowlTimer = TIMER_HOWL_OF_AZGALOR;
        CleaveTimer = TIMER_CLEAVE;
        EnrageTimer = TIMER_BERSERK;

        if(pInstance && IsEvent)
            pInstance->SetData(DATA_AZGALOREVENT, NOT_STARTED);
    }

    void EnterCombat(Unit *who)
    {
        if(pInstance && IsEvent)
            pInstance->SetData(DATA_AZGALOREVENT, IN_PROGRESS);

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
            Unit* target = Unit::GetUnit((*m_creature), pInstance->GetData64(DATA_THRALL));
            if (target && target->IsAlive())
                m_creature->AddThreat(target,0.0);
        }
    }

    void JustDied(Unit *victim)
    {
        hyjal_trashAI::JustDied(victim);
        if(pInstance && IsEvent)
            pInstance->SetData(DATA_AZGALOREVENT, DONE);

        DoScriptText(SAY_ONDEATH,me);
    }

    bool HasTwoPlayersInFront()
    {
        uint8 playersInArc = 0;
        std::list<HostilReference*> const& threatlist = me->getThreatManager().getThreatList();

        for (auto itr : threatlist)
        {
            Unit* unit = itr->getTarget();
            if  (   unit->ToPlayer()
                && me->GetDistance2d(unit) < 8.0f
                && me->HasInArc(M_PI,unit)
                )
                playersInArc++;

            if(playersInArc > 1)
                return true;
        }

        return false;
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
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(0, 5492.91,    -2404.61,    1462.63);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(1, 5531.76,    -2460.87,    1469.55);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(2, 5554.58,    -2514.66,    1476.12);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(3, 5554.16,    -2567.23,    1479.90);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(4, 5540.67,    -2625.99,    1480.89);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(5, 5508.16,    -2659.2,    1480.15);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(6, 5489.62,    -2704.05,    1482.18);
                    ((npc_escortAI*)(m_creature->AI()))->AddWaypoint(7, 5457.04,    -2726.26,    1485.10);
                    ((npc_escortAI*)(m_creature->AI()))->Start(false, true, true);
                    ((npc_escortAI*)(m_creature->AI()))->SetDespawnAtEnd(false);
                }
            }
        }

        //Return since we have no target
        if (!UpdateVictim() )
            return;

        if(RainTimer < diff)
        {
            if(DoCast(SelectUnit(SELECT_TARGET_RANDOM,0,30,true), SPELL_RAIN_OF_FIRE) == SPELL_CAST_OK)
                RainTimer = TIMER_RAIN_OF_FIRE;
        }else RainTimer -= diff;

        if(DoomTimer < diff)
        {
            if(DoCast(SelectUnit(SELECT_TARGET_RANDOM, 0, 150.0, true, true), SPELL_DOOM) == SPELL_CAST_OK)
            {
                DoomTimer = TIMER_DOOM;
                DoScriptText(SAY_DOOM1 - rand()%2,me);
            }
        }else DoomTimer -= diff;

        if(HowlTimer < diff)
        {
            if(DoCast(m_creature, SPELL_HOWL_OF_AZGALOR) == SPELL_CAST_OK)
                HowlTimer = TIMER_HOWL_OF_AZGALOR;
        }else HowlTimer -= diff;

        if(CleaveTimer < diff)
        {
            if(HasTwoPlayersInFront())
                if(DoCast(m_creature->GetVictim(), SPELL_CLEAVE) == SPELL_CAST_OK)
                    CleaveTimer = TIMER_CLEAVE;
        }else CleaveTimer -= diff;

        if(EnrageTimer < diff)
        {
            if(DoCast(m_creature, SPELL_BERSERK, true) == SPELL_CAST_OK)
                EnrageTimer = TIMER_BERSERK;
        }else EnrageTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_azgalor(Creature *_Creature)
{
    return new boss_azgalorAI (_Creature);
}

enum DoomguardSpells
{
    SPELL_THRASH       = 12787,
    SPELL_CRIPPLE      = 31406,
    SPELL_WARSTOMP     = 31408
};

struct mob_lesser_doomguardAI : public hyjal_trashAI
{
    mob_lesser_doomguardAI(Creature *c) : hyjal_trashAI(c)
    {
        pInstance = ((ScriptedInstance*)c->GetInstanceData());
        if(pInstance)
            AzgalorGUID = pInstance->GetData64(DATA_AZGALOR);
    }

    uint32 CrippleTimer;
    uint32 WarstompTimer;
    uint32 CheckTimer;
    uint64 AzgalorGUID;
    ScriptedInstance* pInstance;

    void Reset()
    {
        CrippleTimer = 50000;
        WarstompTimer = 10000;
        DoCast(m_creature, SPELL_THRASH);
        CheckTimer = 5000;
    }

    void EnterCombat(Unit *who)
    {
    }

    void KilledUnit(Unit *victim)
    {

    }

    void WaypointReached(uint32 i)
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

    void JustDied(Unit *victim)
    {

    }

    void UpdateAI(const uint32 diff)
    {
        if(CheckTimer < diff)
        {
            if(AzgalorGUID)
            {
                Creature* boss = Unit::GetCreature((*m_creature),AzgalorGUID);
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
        if (!UpdateVictim() )
            return;

        if(WarstompTimer < diff)
        {
            DoCast(m_creature, SPELL_WARSTOMP);
            WarstompTimer = 10000+rand()%5000;
        }else WarstompTimer -= diff;

        if(CrippleTimer < diff)
        {
            DoCast(SelectUnit(SELECT_TARGET_RANDOM,0,100,true), SPELL_CRIPPLE);
            CrippleTimer = 25000+rand()%5000;
        }else CrippleTimer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mob_lesser_doomguard(Creature *_Creature)
{
    return new mob_lesser_doomguardAI (_Creature);
}

void AddSC_boss_azgalor()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name="boss_azgalor";
    newscript->GetAI = &GetAI_boss_azgalor;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="mob_lesser_doomguard";
    newscript->GetAI = &GetAI_mob_lesser_doomguard;
    newscript->RegisterSelf();
}
