/* Copyright (C) 2006 - 2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
SDName: Instance_Black_Temple
SD%Complete: 100
SDComment: Instance Data Scripts and functions to acquire mobs and set encounter status for use in various Black Temple Scripts
SDCategory: Black Temple
EndScriptData */

#include "precompiled.h"
#include "def_black_temple.h"

#define MAX_ENCOUNTER     9

/* Black Temple encounters:
0 - High Warlord Naj'entus event
1 - Supremus Event
2 - Shade of Akama Event
3 - Teron Gorefiend Event
4 - Gurtogg Bloodboil Event
5 - Reliquary Of Souls Event
6 - Mother Shahraz Event
7 - Illidari Council Event
8 - Illidan Stormrage Event
*/

struct instance_black_temple : public ScriptedInstance
{
    instance_black_temple(Map *map) : ScriptedInstance(map) {Initialize();};

    uint32 m_auiEncounter[MAX_ENCOUNTER];
    std::string str_data;

    uint64 Najentus;
    uint64 Akama;                                           // This is the Akama that starts the Illidan encounter.
    uint64 Akama_Shade;                                     // This is the Akama that starts the Shade of Akama encounter.
    uint64 ShadeOfAkama;
    uint64 Supremus;
    uint64 LadyMalande;
    uint64 GathiosTheShatterer;
    uint64 HighNethermancerZerevor;
    uint64 VerasDarkshadow;
    uint64 IllidariCouncil;
    uint64 BloodElfCouncilVoice;
    uint64 IllidanStormrage;
    uint64 TeronGorefiend;

    uint64 NajentusGate;
    uint64 MainTempleDoors;
    uint64 ShadeOfAkamaDoor;
    uint64 CommonDoor;//Teron
    uint64 TeronDoor;
    uint64 GuurtogDoor;
    uint64 MotherDoor;
    uint64 TempleDoor;//Befor mother
    uint64 CouncilDoor;
    uint64 SimpleDoor;//council
    uint64 IllidanGate;
    uint64 IllidanDoor[2];
    
    std::list<uint64> ashtongues;

    void Initialize()
    {
        memset(&m_auiEncounter, 0, sizeof(m_auiEncounter));

        Najentus = 0;
        Akama = 0;
        Akama_Shade = 0;
        ShadeOfAkama = 0;
        Supremus = 0;
        LadyMalande = 0;
        GathiosTheShatterer = 0;
        HighNethermancerZerevor = 0;
        VerasDarkshadow = 0;
        IllidariCouncil = 0;
        BloodElfCouncilVoice = 0;
        IllidanStormrage = 0;
        TeronGorefiend = 0;

        NajentusGate    = 0;
        MainTempleDoors = 0;
        ShadeOfAkamaDoor= 0;
        CommonDoor              = 0;//teron
        TeronDoor               = 0;
        GuurtogDoor             = 0;
        MotherDoor              = 0;
        TempleDoor              = 0;
        SimpleDoor              = 0;//Bycouncil
        CouncilDoor             = 0;
        IllidanGate     = 0;
        IllidanDoor[0]  = 0;
        IllidanDoor[1]  = 0;

        for(uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            m_auiEncounter[i] = NOT_STARTED;
            
        ashtongues.clear();
    }

    bool IsEncounterInProgress() const
    {
        for(uint8 i = 0; i < MAX_ENCOUNTER; ++i)
            if(m_auiEncounter[i] == IN_PROGRESS) return true;

        return false;
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

        debug_log("TSCR: Instance Black Temple: GetPlayerInMap, but PlayerList is empty!");
        return NULL;
    }

    void OnCreatureCreate(Creature *creature, uint32 creature_entry)
    {
        switch(creature_entry)
        {
        case 22887:    Najentus = creature->GetGUID();                  break;
        case 23089:    Akama = creature->GetGUID();                     break;
        case 22990:    Akama_Shade = creature->GetGUID();               break;
        case 22841:    ShadeOfAkama = creature->GetGUID();              break;
        case 22898:    Supremus = creature->GetGUID();                  break;
        case 22917:    IllidanStormrage = creature->GetGUID();          break;
        case 22949:    GathiosTheShatterer = creature->GetGUID();       break;
        case 22950:    HighNethermancerZerevor = creature->GetGUID();   break;
        case 22951:    LadyMalande = creature->GetGUID();               break;
        case 22952:    VerasDarkshadow = creature->GetGUID();           break;
        case 23426:    IllidariCouncil = creature->GetGUID();           break;
        case 23499:    BloodElfCouncilVoice = creature->GetGUID();      break;
        case 22871:    TeronGorefiend = creature->GetGUID();            break;
        case 22844:
        case 22845:
        case 22846:
        case 22847:
        case 22848:
        case 22849:
        case 23374:
            if (m_auiEncounter[2] == DONE)
                creature->setFaction(1820);
            else
                ashtongues.push_back(creature->GetGUID());
            break;
        }
    }

    void OnObjectCreate(GameObject* go)
    {
        switch(go->GetEntry())
        {
        case 185483: NajentusGate = go->GetGUID();// Gate past Naj'entus (at the entrance to Supermoose's courtyards)
            if(m_auiEncounter[0] == DONE)HandleGameObject(NULL,true,go);break;
        case 185882: MainTempleDoors = go->GetGUID();// Main Temple Doors - right past Supermoose (Supremus)
            if(m_auiEncounter[1] == DONE)HandleGameObject(NULL,true,go);break;
        case 185478: ShadeOfAkamaDoor = go->GetGUID();break;
        case 185480: CommonDoor = go->GetGUID();
            if(m_auiEncounter[3] == DONE)HandleGameObject(NULL,true,go);;break;
        case 186153: TeronDoor = go->GetGUID();
            if(m_auiEncounter[3] == DONE)HandleGameObject(NULL,true,go);;break;
        case 185892: GuurtogDoor = go->GetGUID();
            if(m_auiEncounter[4] == DONE)HandleGameObject(NULL,true,go);break;
        case 185479: TempleDoor = go->GetGUID();
            if(m_auiEncounter[5] == DONE)HandleGameObject(NULL,true,go);break;
        case 185482: MotherDoor = go->GetGUID();
            if(m_auiEncounter[6] == DONE)HandleGameObject(NULL,true,go);break;
        case 185481: CouncilDoor = go->GetGUID();
            if(m_auiEncounter[7] == DONE)HandleGameObject(NULL,true,go);break;
        case 186152: SimpleDoor = go->GetGUID();
            if(m_auiEncounter[7] == DONE)HandleGameObject(NULL,true,go);break;
        case 185905: IllidanGate = go->GetGUID(); break; // Gate leading to Temple Summit
        case 186261: IllidanDoor[0] = go->GetGUID(); break; // Right door at Temple Summit
        case 186262: IllidanDoor[1] = go->GetGUID(); break; // Left door at Temple Summit
        }
    }

    uint64 GetData64(uint32 identifier)
    {
        switch(identifier)
        {
        case DATA_HIGHWARLORDNAJENTUS:         return Najentus;
        case DATA_AKAMA:                       return Akama;
        case DATA_AKAMA_SHADE:                 return Akama_Shade;
        case DATA_SHADEOFAKAMA:                return ShadeOfAkama;
        case DATA_SUPREMUS:                    return Supremus;
        case DATA_ILLIDANSTORMRAGE:            return IllidanStormrage;
        case DATA_GATHIOSTHESHATTERER:         return GathiosTheShatterer;
        case DATA_HIGHNETHERMANCERZEREVOR:     return HighNethermancerZerevor;
        case DATA_LADYMALANDE:                 return LadyMalande;
        case DATA_VERASDARKSHADOW:             return VerasDarkshadow;
        case DATA_ILLIDARICOUNCIL:             return IllidariCouncil;
        case DATA_GAMEOBJECT_NAJENTUS_GATE:    return NajentusGate;
        case DATA_GAMEOBJECT_ILLIDAN_GATE:     return IllidanGate;
        case DATA_GAMEOBJECT_ILLIDAN_DOOR_R:   return IllidanDoor[0];
        case DATA_GAMEOBJECT_ILLIDAN_DOOR_L:   return IllidanDoor[1];
        case DATA_GAMEOBJECT_SUPREMUS_DOORS:   return MainTempleDoors;
        case DATA_BLOOD_ELF_COUNCIL_VOICE:     return BloodElfCouncilVoice;
        case DATA_TERON:                       return TeronGorefiend;
        }

        return 0;
    }

    void SetData(uint32 type, uint32 data)
    {
        switch(type)
        {
        case DATA_HIGHWARLORDNAJENTUSEVENT:
            if(data == DONE)
            {
                HandleGameObject(NajentusGate, true);
            }
            m_auiEncounter[0] = data;break;
        case DATA_SUPREMUSEVENT:
            if(data == DONE)
            {
                HandleGameObject(NajentusGate, true);
            }
            m_auiEncounter[1] = data; break;
        case DATA_SHADEOFAKAMAEVENT:
            if(data == IN_PROGRESS)
            {
                HandleGameObject(ShadeOfAkamaDoor, false);
            }else HandleGameObject(ShadeOfAkamaDoor, true);
            m_auiEncounter[2] = data;
            
            if (data == DONE) {
                for (std::list<uint64>::const_iterator itr = ashtongues.begin(); itr != ashtongues.end(); itr++) {
                    if (Creature* tmp = instance->GetCreatureInMap(*itr))
                        tmp->setFaction(1820);
                }
            }
        case DATA_TERONGOREFIENDEVENT:
            if(data == IN_PROGRESS)
            {
                HandleGameObject(TeronDoor, false);
                HandleGameObject(CommonDoor, false);
            }else
            {
                HandleGameObject(TeronDoor, true);
                HandleGameObject(CommonDoor, true);
            }
            m_auiEncounter[3] = data; break;
        case DATA_GURTOGGBLOODBOILEVENT:
            if(data == DONE)
            {
                HandleGameObject(GuurtogDoor, true);
            }
            m_auiEncounter[4] = data; break;
        case DATA_RELIQUARYOFSOULSEVENT:
            if(data == DONE)
            {
                HandleGameObject(TempleDoor, true);
            }
            m_auiEncounter[5] = data;         break;
        case DATA_MOTHERSHAHRAZEVENT:
            if(data == DONE)
            {
                HandleGameObject(MotherDoor, true);
            }
            m_auiEncounter[6] = data; break;
        case DATA_ILLIDARICOUNCILEVENT:
            if(data == IN_PROGRESS)
            {
                HandleGameObject(CouncilDoor, false);
                HandleGameObject(SimpleDoor, false);
            }else
            {
                HandleGameObject(CouncilDoor, true);
                HandleGameObject(SimpleDoor, true);
            }
            m_auiEncounter[7] = data; break;
        case DATA_ILLIDANSTORMRAGEEVENT:      m_auiEncounter[8] = data;         break;
        }
        
        if(data == DONE)
            SaveToDB();
    }

    uint32 GetData(uint32 type)
    {
        switch(type)
        {
        case DATA_HIGHWARLORDNAJENTUSEVENT:         return m_auiEncounter[0];
        case DATA_SUPREMUSEVENT:                    return m_auiEncounter[1];
        case DATA_SHADEOFAKAMAEVENT:                return m_auiEncounter[2];
        case DATA_TERONGOREFIENDEVENT:              return m_auiEncounter[3];
        case DATA_GURTOGGBLOODBOILEVENT:            return m_auiEncounter[4];
        case DATA_RELIQUARYOFSOULSEVENT:            return m_auiEncounter[5];
        case DATA_MOTHERSHAHRAZEVENT:               return m_auiEncounter[6];
        case DATA_ILLIDARICOUNCILEVENT:             return m_auiEncounter[7];
        case DATA_ILLIDANSTORMRAGEEVENT:            return m_auiEncounter[8];
        }

        return 0;
    }

   std::string GetSaveData()
    {
        return str_data;
    }

    const char* Save()
    {
        OUT_SAVE_INST_DATA;
        std::ostringstream stream;
        stream << m_auiEncounter[0] << " " << m_auiEncounter[1] << " "
                << m_auiEncounter[2] << " " << m_auiEncounter[3] << " " << m_auiEncounter[4]
            << " " << m_auiEncounter[5] << " " << m_auiEncounter[6] << " " << m_auiEncounter[7]
            << " " << m_auiEncounter[8];
        char* out = new char[stream.str().length() + 1];
        strcpy(out, stream.str().c_str());
        if(out)
        {
            OUT_SAVE_INST_DATA_COMPLETE;
            return out;
        }

        return NULL;
    }

    void Load(const char* in)
    {
        if (!in)
        {
            OUT_LOAD_INST_DATA_FAIL;
            return;
        }

        OUT_LOAD_INST_DATA(in);

        std::istringstream loadStream(in);
        loadStream >> m_auiEncounter[0] >> m_auiEncounter[1] >> m_auiEncounter[2]
        >> m_auiEncounter[3] >> m_auiEncounter[4] >> m_auiEncounter[5] >> m_auiEncounter[6]
        >> m_auiEncounter[7] >> m_auiEncounter[8];

        for(uint8 i = 0; i < MAX_ENCOUNTER; ++i)
        {
            if (m_auiEncounter[i] == IN_PROGRESS)
                m_auiEncounter[i] = NOT_STARTED;
        }

        OUT_LOAD_INST_DATA_COMPLETE;
    }
};

InstanceData* GetInstanceData_instance_black_temple(Map* map)
{
    return new instance_black_temple(map);
}

void AddSC_instance_black_temple()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "instance_black_temple";
    newscript->GetInstanceData = &GetInstanceData_instance_black_temple;
    newscript->RegisterSelf();
}

