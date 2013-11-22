#include "precompiled.h"
#include "MapManager.h"

#define ORI_MAX 0.5

enum Firework
{
    GOSSIP_START,
    GOSSIP_SELECT_EVENT,
    GOSSIP_RELOAD,

    GOBJECT_RAMP = 34,
    CREATURE_LAUNCHER = 1412,

    SPELL_FIREWORK_BLUE = 11540,
    SPELL_FIREWORK_RED = 6668,
    SPELL_FIREWORK_GREEN = 11541,
    SPELL_FIREWORK_BLUE_WHITE_RED = 11543, //multixplosion
    SPELL_FIREWORK_YELLOW_PINK = 11544, //spirate
    SPELL_FIREWORK_PURPLE = 30161, //spirale/Multixplosion
    SPELL_FIREWORK_LOVE = 44940,
    SPELL_ROCKET_RED = 47004, //boum au sol avant de partir
    //poulet fusées au milieu hihi
    GOB_ROCKET_RED = 180851,
    GOB_ROCKET_BLUE = 180854,
    GOB_ROCKET_GREEN = 180855,
    GOB_ROCKET_WHITE = 180857,
    GOB_ROCKET_YELLOW = 180858,

    GOB_ROCKET_RED_BIG = 180860,
    GOB_ROCKET_BLUE_BIG = 180861,
    GOB_ROCKET_GREEN_BIG = 180862,
    GOB_ROCKET_PURPLE_BIG = 180863,
    GOB_ROCKET_WHITE_BIG = 180864,
    GOB_ROCKET_YELLOW_BIG = 180865,
};

struct FireworkEvent {

    FireworkEvent(uint32 spellorGobId, float size, uint8 posX, uint8 posY, float oriX = 0.0f, float oriY = 0.0f) :
        spellorGobId(spellorGobId),size(size),posX(posX),posY(posY),oriX(oriX),oriY(oriY) 
    { }

    uint32 spellorGobId;
    float size;
	uint8 posX;
	uint8 posY;
	float oriX;
	float oriY;
};

struct firework_controllerAI : public ScriptedAI
{
    firework_controllerAI(Creature* creature) : 
        ScriptedAI(creature),
        cellSize(3.0f),
        gridSize(25),
        eventId(0)
	{
        SetupEvent();
    } 

    float gridStartX, gridStartY, gridZ;
    float cellSize;
    uint8 gridSize;
    uint32 endTime;
    uint8 eventId;

    uint32 lastEventTime; //time of the last event
    uint32 currentTime; //time counter (increment with each updateAI)
    bool eventStarted;

    std::multimap<uint32,FireworkEvent*> eventMap;
    
	void Reset() {
        me->GetPosition(gridStartX,gridStartY,gridZ);
        me->SetReactState(REACT_PASSIVE);
        me->SetVisibility(VISIBILITY_OFF);

        eventStarted = false;
        currentTime = 0;
        lastEventTime = 0;
	}

    bool AddEvent(uint32 time, uint32 spellorGobId, float size, uint8 posX, uint8 posY, float oriX = 0.0f, float oriY = 0.0f)
    {
        if(!spellorGobId)
		    return false;
	    if(posX >= gridSize || posY >= gridSize)
		    return false;
	    if(oriX < -ORI_MAX || oriY < -ORI_MAX || oriX > ORI_MAX || oriY > ORI_MAX)
		    return false;
		
        FireworkEvent* event = new FireworkEvent(spellorGobId,size,posX,posY,oriX,oriY);
	    //...
        eventMap.insert(std::make_pair(time,event));

        if(time > endTime)
            endTime = time;

	    return true;
    }

    void SetupEvent()
    {
        eventMap.clear();
        endTime = 0;
        QueryResult* result = WorldDatabase.PQuery("SELECT time, spell, posX, posY, oriX, oriY FROM game_event_fireworks WHERE groupid = %u",eventId); //order by is here to handle non race specific spells first
        if (result) {
            do {
                Field* fields = result->Fetch();
                if(!AddEvent(fields[0].GetUInt32(),fields[1].GetUInt32(), fields[2].GetUInt8(),fields[3].GetUInt8(),fields[4].GetFloat(), fields[5].GetFloat()))
                    sLog.outError("firework_controller : invalid db entry");
            } while (result->NextRow());
        }
    }

    //get real coords
    void GetTargetCoords(uint8 posX, uint8 posY, float& realPositionX, float& realPositionY)
    {
        realPositionX = gridStartX + posX * cellSize;
        realPositionY = gridStartY + posY * cellSize;
    }

    float GetAdaptedGobSize(uint32 spellId)
    {
        switch(spellId)
        {
            //...
        default:
            return 1.0f;
            break;
        }
    }

    void ExecEvent(FireworkEvent* event)
    {
        float x,y,z;
        GetTargetCoords(event->posX,event->posY,x,y);
        z = gridZ;
        if(event->spellorGobId < 100000) //if it's a spell
        {    
            if(Creature* c = me->SummonCreature(CREATURE_LAUNCHER,x,y,z,0,TEMPSUMMON_TIMED_OR_DEAD_DESPAWN,3000))
            {
                c->SetFloatValue(OBJECT_FIELD_SCALE_X, event->size);
                c->CastSpell(c,event->spellorGobId,true);
            }
            //si ça marche pas invoquer la créature etout
            if(GameObject* gob = me->SummonGameObject(GOBJECT_RAMP,x,y,z,0.0f,event->oriX,event->oriY,0,0,0))
            {
                gob->SetFloatValue(OBJECT_FIELD_SCALE_X, GetAdaptedGobSize(event->spellorGobId) * event->size);
                //Update model for client
                Map* map = MapManager::Instance().GetMap(me->GetMapId(),me); 
                map->Remove(gob,false); 
                map->Add(gob);
                gob->CastSpell(nullptr,event->spellorGobId);
                gob->Delete();
            }
        } else {
            if(GameObject* gob = me->SummonGameObject(event->spellorGobId,x,y,z,0.0f,event->oriX,event->oriY,0,0,0))
            {
                gob->SetFloatValue(OBJECT_FIELD_SCALE_X, GetAdaptedGobSize(event->spellorGobId) * event->size);
                Map* map = MapManager::Instance().GetMap(me->GetMapId(),me); 
                map->Remove(gob,false); 
                map->Add(gob);
                gob->Delete(); //this trigger explosion
            }
        }
    }

	void UpdateAI(uint32 const diff)
	{
        if(!eventStarted)
            return;

        currentTime += diff;

        for (auto it1 = eventMap.cbegin(), it2 = it1, end = eventMap.cend(); it1 != end; it1 = it2)
        {
            //not yet time
            if(currentTime < it1->first)
                break;

            // skip if already done
            if(lastEventTime == it1->first) 
            {
                it2++;
                continue;
            }

            lastEventTime = it1->first;
            do
            {
                ExecEvent(it2->second);
                ++it2;
            } while (it2->first == it1->first);
        }

        if(currentTime > endTime)
            eventStarted = false;
	}
};

bool GossipHello_firework_controller(Player *player, Creature *_Creature)
{    
    player->ADD_GOSSIP_ITEM( 0, "(Re)start event", GOSSIP_SENDER_MAIN, GOSSIP_START);
    player->ADD_GOSSIP_ITEM_EXTENDED( 0, "Change Event", GOSSIP_SENDER_MAIN, GOSSIP_SELECT_EVENT, "", 0, true);
    player->ADD_GOSSIP_ITEM( 0, "Reload event", GOSSIP_SENDER_MAIN, GOSSIP_RELOAD);
        
	player->PlayerTalkClass->SendGossipMenu(3,_Creature->GetGUID());

    return true;
}

bool GossipSelect_firework_controller(Player *pPlayer, Creature* c, uint32 sender, uint32 action)
{
    switch (action)
    {
    case GOSSIP_START:
        ((firework_controllerAI*)c->AI())->Reset();
        ((firework_controllerAI*)c->AI())->eventStarted = true;
        break;
    case GOSSIP_RELOAD:
        ((firework_controllerAI*)c->AI())->Reset();
        ((firework_controllerAI*)c->AI())->SetupEvent();
        break;
    }
    pPlayer->CLOSE_GOSSIP_MENU();
    return true;
}

bool GossipSelectWithCode_firework_controller( Player *player, Creature *c, uint32 sender, uint32 action, const char* Code )
{
    if(action == GOSSIP_SELECT_EVENT)
    {
        uint8 event = (uint8)atoi(Code);
        ((firework_controllerAI*)c->AI())->eventId = event;
    }
	
    player->CLOSE_GOSSIP_MENU();

    return true;
}

CreatureAI* GetAI_firework_controller(Creature *_Creature)
{
    return new firework_controllerAI (_Creature);
}

void AddSC_firework_controller()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="npc_firework_controller";
    newscript->GetAI = &GetAI_firework_controller;
    newscript->pGossipHello = &GossipHello_firework_controller;
    newscript->pGossipSelect = &GossipSelect_firework_controller;
	newscript->pGossipSelectWithCode = &GossipSelectWithCode_firework_controller;
    newscript->RegisterSelf();
}
