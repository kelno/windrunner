/* ScriptData
SDName: Instance_Blackrock_Spire
SD%Complete: n/a
SDComment:
SDCategory: Blackrock Depths
EndScriptData */

#include "precompiled.h"
#include "def_blackrock_spire.h"
#include "GameEvent.h"

enum BlackrockSpireData
{
    MAX_ENCOUNTER           = 1,
};

struct instance_blackrock_spire : public ScriptedInstance
{
    instance_blackrock_spire(Map* map) : ScriptedInstance(map) {}
    
    uint32 Encounters[MAX_ENCOUNTER];
    
    uint64 firstDoorGUID;
    
    uint64 runesGUID[7];
    
    bool isEventActive()
    {
        const GameEvent::ActiveEvents& activeEvents = gameeventmgr.GetActiveEventList();
        bool active = activeEvents.find(57) != activeEvents.end();

        return active;
    }
    
    void Initialize()
    {        
        firstDoorGUID = 0;
        
        for (uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            Encounters[i] = NOT_STARTED;
    }
    
    void RefreshData() {
        if (Encounters[0] == DONE)
            HandleGameObject(firstDoorGUID, true);
    }
    
    bool IsEncounterInProgress() const
    {
        for (uint8 i = 0; i < MAX_ENCOUNTER; ++i) {
            if (Encounters[i] == IN_PROGRESS)
                return true;
        }

        return false;
    }
    
    void OnCreatureCreate(Creature *creature, uint32 entry)
    {
        switch (entry) {
        case 10899:
            if (isEventActive())
                creature->SetDisplayId(15760);

            break;
        }
    }
    
    void OnObjectCreate(GameObject* go)
    {
        switch(go->GetEntry())
        {
        case 175194:    runesGUID[0] = go->GetGUID(); go->UseDoorOrButton(); break;
        case 175195:    runesGUID[1] = go->GetGUID(); go->UseDoorOrButton(); break;
        case 175196:    runesGUID[2] = go->GetGUID(); go->UseDoorOrButton(); break;
        case 175197:    runesGUID[3] = go->GetGUID(); go->UseDoorOrButton(); break;
        case 175198:    runesGUID[4] = go->GetGUID(); go->UseDoorOrButton(); break;
        case 175199:    runesGUID[5] = go->GetGUID(); go->UseDoorOrButton(); break;
        case 175200:    runesGUID[6] = go->GetGUID(); go->UseDoorOrButton(); break;
        case 184247:    firstDoorGUID = go->GetGUID(); if (GetData(DATA_FIRSTROOM_RUNES) == DONE) HandleGameObject(firstDoorGUID, true); break;
        default: break;
        }
    }
    
    uint32 GetData(uint32 identifier)
    {
        switch (identifier)
        {
        case DATA_FIRSTROOM_RUNES:  return Encounters[0];
        default: break;
        }

        return 0;
    }
    
    void SetData(uint32 type, uint32 data)
    {
        switch (type) {
        case DATA_FIRSTROOM_RUNES:
            Encounters[DATA_FIRSTROOM_RUNES] = data;
            if (data == DONE)
                HandleGameObject(firstDoorGUID, true);
            break;
        default: break;
        }
        
        if (data == DONE)
            SaveToDB();
    }
    
    const char* Save()
    {
        OUT_SAVE_INST_DATA;
        std::ostringstream stream;
        stream << Encounters[0]/* << " "  << Encounters[1] << " "  << Encounters[2] << " "  << Encounters[3] << " "
            << Encounters[4] << " "  << Encounters[5] << " "  << Encounters[6] << " "  << Encounters[7] << " "
            << Encounters[8] << " "  << Encounters[9] << " "  << Encounters[10] << " "  << Encounters[11]*/;
            
        char* out = new char[stream.str().length() + 1];
        strcpy(out, stream.str().c_str());
        
        if (out) {
            OUT_SAVE_INST_DATA_COMPLETE;
            return out;
        }

        return NULL;
    }

    void Load(const char* in)
    {
        if (!in) {
            OUT_LOAD_INST_DATA_FAIL;
            return;
        }

        OUT_LOAD_INST_DATA(in);
        std::istringstream stream(in);
        stream >> Encounters[0]/* >> Encounters[1] >> Encounters[2] >> Encounters[3]
            >> Encounters[4] >> Encounters[5] >> Encounters[6] >> Encounters[7]
            >> Encounters[8] >> Encounters[9] >> Encounters[10] >> Encounters[11]*/;
            
        for(uint8 i = 0; i < MAX_ENCOUNTER; ++i) {
            if(Encounters[i] == IN_PROGRESS)                // Do not load an encounter as "In Progress" - reset it instead.
                Encounters[i] = NOT_STARTED;
        }
        
        //RefreshData();
        SetData(DATA_FIRSTROOM_RUNES, DONE);
        
        OUT_LOAD_INST_DATA_COMPLETE;
    }
    
    Player* GetPlayerInMap()
    {
        Map::PlayerList const& players = instance->GetPlayers();

        if (!players.isEmpty())
        {
            for(Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
            {
                if (Player* plr = itr->getSource())
                    return plr;
            }
        }

        sLog.outError("Instance Blackrock Depths: GetPlayerInMap, but PlayerList is empty!");
        return NULL;
    }
    
    bool CheckRunes()
    {
        Player* player = GetPlayerInMap();
        
        if (!player)
            return false;
        
        for (uint8 i = 0; i < 7; i++) {
            if (GameObject* rune = GameObject::GetGameObject(*player, runesGUID[i])) {
                float dist = (i == 5) ? 3.0f : 5.0f;
                if (Creature* cre = rune->FindCreatureInGrid(9819, dist, true))
                    return false;
                if (Creature* cre = rune->FindCreatureInGrid(9818, dist, true))
                    return false;
            }
        }
        
        return true;
    }
    
    void Update(uint32 diff)
    {
        if (!GetPlayerInMap())
            return;
            
        if (CheckRunes())
            SetData(DATA_FIRSTROOM_RUNES, DONE);
    }
};

InstanceData* GetInstanceData_instance_blackrock_spire(Map* pMap)
{
    return new instance_blackrock_spire(pMap);
}

void AddSC_instance_blackrock_spire()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "instance_blackrock_spire";
    newscript->GetInstanceData = &GetInstanceData_instance_blackrock_spire;
    newscript->RegisterSelf();
}
