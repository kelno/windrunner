/* ContentData
npc_wastewalker_worker
EndContentData */

#include "precompiled.h"
#include "CreatureTextMgr.h"

#define SPELL_SLAP 6754
#define WAYPOINT_EVASION 12949440
#define CREATURE_SLAVE  17963
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
        message(MESSAGE_RESUME_WORK,0);
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
            EnterEvadeMode();
            me->setFaction(35);
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
            me->AddMessageEvent(2000,MESSAGE_RESUME_WORK);
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
                me->AddMessageEvent(1000,MESSAGE_KNEEL_DOWN);
                me->AddMessageEvent(5000,MESSAGE_KNEEL_UP);
                return;
            }
        }
        //else, if not hit by slaver
        me->SetReactState(REACT_AGGRESSIVE);
        if(caster->GetTypeId() == TYPEID_PLAYER)
            AttackStart(caster); //attack start only players, just to be sure
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

#define SPELL_HARMSTRING 9080
#define SPELL_HEAD_CRACK 16172

struct npc_coilfang_slavehandlerAI : public ScriptedAI
{
    npc_coilfang_slavehandlerAI(Creature *c) : ScriptedAI(c)
    {}

    enum Says
    {
        SAY_WORK_HARDER = 0,
        SAY_HELP = 1,
    };

    enum Messages
    {
        MESSAGE_SEND_SLAVES,
    };

    uint32 slapTimer;
    uint32 harmStringTimer;
    uint32 headCrackTimer;

    void Reset()
    {
        slapTimer = 10000 + rand()%30000;
        harmStringTimer = 10000 + rand()%5000;
        headCrackTimer = 15000 + rand()%5000;
    }

    uint64 message(uint32 id, uint64 data) 
    { 
        if(id == MESSAGE_SEND_SLAVES)
        {
            Unit* target = ObjectAccessor::Instance().GetObjectInWorld(data,(Unit*)NULL);
            if(!target)
                return 0;
            std::list<Creature*> slavesList;
            GetSlaves(slavesList,20.0f);
            for(auto itr : slavesList)
                itr->AI()->AttackStart(target);
        }
        return 0;
    }

    void EnterCombat(Unit* who)
    {
        sCreatureTextMgr.SendChat(me, SAY_HELP, 0);
        me->AddMessageEvent(1000,MESSAGE_SEND_SLAVES,who->GetGUID());
    }

    void GetSlaves(std::list<Creature*>& list, float distance)
    {
        FindCreatures(list,CREATURE_WORKER,distance,m_creature);
        FindCreatures(list,CREATURE_SLAVE,distance,m_creature);
    }

    void JustDied(Unit* killer)
    {
        std::list<Creature*> slavesList;
        GetSlaves(slavesList,20.0f);
        for(auto itr : slavesList)
            itr->AddMessageEvent(1000,MESSAGE_START_EVADE);
    }

    void UpdateAI(const uint32 diff)
    {
        if(!me->GetVictim())
        {
            if(slapTimer < diff)
            {
                std::list<Creature*> slavesList;
                GetSlaves(slavesList,5.0f);
                if(slavesList.empty())
                {
                    slapTimer = 5000; //recheck in 5 sec
                    return;
                }
                DoCast(*slavesList.begin(),SPELL_SLAP);
                sCreatureTextMgr.SendChat(me, SAY_WORK_HARDER, 0);
                slapTimer = 20000 + rand()%40000;
            } else slapTimer -= diff;
        }

        if(!UpdateVictim())
            return; 

        if(harmStringTimer < diff)
        {
            if(DoCast(me->GetVictim(),SPELL_HARMSTRING) == SPELL_CAST_OK)
                harmStringTimer = 20000;
        } else harmStringTimer -= diff;

        if(headCrackTimer < diff)
        {
            if(DoCast(me->GetVictim(),SPELL_HEAD_CRACK) == SPELL_CAST_OK)
                headCrackTimer = 15000 + rand()%5000;
        } else headCrackTimer -= diff;

        DoMeleeAttackIfReady();
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