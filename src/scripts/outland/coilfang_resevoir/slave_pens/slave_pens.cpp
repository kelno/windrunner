/* ContentData
npc_wastewalker_worker
EndContentData */

#include "precompiled.h"
#include "CreatureTextMgr.h"

#define SPELL_SLAP 6754
#define WAYPOINT_EVASION 12949440
#define CREATURE_WORKER 17964
#define CREATURE_SLAVER 17959

enum WorkersMessages
{
    MESSAGE_START_EVADE,
    MESSAGE_KNEEL_DOWN,
    MESSAGE_KNEEL_UP,
    MESSAGE_RESUME_WORK,
};

struct npc_wastewalker_workerAI : public ScriptedAI
{
    npc_wastewalker_workerAI(Creature *c) : ScriptedAI(c)
    {
        message(MESSAGE_RESUME_WORK, 0);
    }

    enum Says 
    {
        SAY_GROUP_ON_SLAP = 0,
        SAY_GROUP_EVASION = 1,
    };

    //start evading to instance entrance
    uint64 message(uint32 id, uint64 data) 
    { 
        switch(id)
        {
        case MESSAGE_START_EVADE:
            sCreatureTextMgr.SendChat(me, SAY_GROUP_EVASION, 0);
            me->LoadPath(WAYPOINT_EVASION);
            me->SetDefaultMovementType(WAYPOINT_MOTION_TYPE);
            me->GetMotionMaster()->Initialize();
            break;
        case MESSAGE_KNEEL_DOWN:
            me->SetStandState(UNIT_STAND_STATE_KNEEL);
            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, 0);
            break;
        case MESSAGE_KNEEL_UP:
            me->SetStandState(UNIT_STAND_STATE_STAND);
            sCreatureTextMgr.SendChat(me, SAY_GROUP_ON_SLAP, 0);
            AddMessageEvent(MESSAGE_RESUME_WORK, 2000);
            break;
        case MESSAGE_RESUME_WORK:
            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_WORK_NOSHEATHE);
            break;
        }
        return 0;     
    }
    
    void Reset()
    {
        me->SetReactState(REACT_PASSIVE);
    }

    //waypoint path has only one point, despawn uppon reaching it
    void MovementInform(uint32 movementType, uint32 /*data*/) 
    {
        if(movementType == WAYPOINT_MOTION_TYPE)
            me->ForcedDespawn();
    }

    //show some respect when hit by a slaver
    void SpellHit(Unit* caster, const SpellEntry* spellInfo)
    {
        if(caster->GetTypeId() == TYPEID_UNIT)
        {
            if(caster->ToCreature()->GetEntry() == CREATURE_SLAVER)
            {
                AddMessageEvent(MESSAGE_KNEEL_DOWN, 1000);
                AddMessageEvent(MESSAGE_KNEEL_UP, 5000);
                return;
            }
        }
        //else, if not hit by slaver
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void DamageTaken(Unit *done_by, uint32 &damage) 
    {
        me->SetReactState(REACT_AGGRESSIVE);
    }

    void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim())
            return;

        DoMeleeAttackIfReady();
    }
};

struct npc_coilfang_slavehandlerAI : public ScriptedAI
{
    npc_coilfang_slavehandlerAI(Creature *c) : ScriptedAI(c)
    {}

    void JustDied(Unit* killer)
    {
        CellPair pair(Trinity::ComputeCellPair(m_creature->GetPositionX(), m_creature->GetPositionY()));
        Cell cell(pair);
        cell.data.Part.reserved = ALL_DISTRICT;
        cell.SetNoCreate();
        std::list<Creature*> workersList;

        Trinity::AllCreaturesOfEntryInRange check(m_creature, CREATURE_WORKER, 25.0f);
        Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange> searcher(workersList, check);
        TypeContainerVisitor<Trinity::CreatureListSearcher<Trinity::AllCreaturesOfEntryInRange>, GridTypeMapContainer> visitor(searcher);

        cell.Visit(pair, visitor, *m_creature->GetMap());

        for(auto itr : workersList)
            itr->AI()->message(MESSAGE_START_EVADE,0);
    }

    void UpdateAI(const uint32 diff)
    {
        //handled in smartAI
    }
};

CreatureAI* GetAI_npc_wastewalker_worker(Creature *pCreature)
{
    return new npc_wastewalker_workerAI(pCreature);
}

CreatureAI* GetAI_npc_coilfang_slavehandler(Creature *pCreature)
{
    return new npc_coilfang_slavehandlerAI(pCreature);
}

void AddSC_slave_pens()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "npc_wastewalker_worker";
    newscript->GetAI = &GetAI_npc_wastewalker_worker;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_coilfang_slavehandler";
    newscript->GetAI = &GetAI_npc_coilfang_slavehandler;
    newscript->RegisterSelf();
}