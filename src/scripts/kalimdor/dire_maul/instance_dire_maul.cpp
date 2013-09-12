/* Copyright (C) 2009 - 2010 WoWMania Core
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
SDName: Instance_Dire_Maul
SD%Complete: 0
SDComment: 
SDCategory: Dire_Maul
EndScriptData */

#include "precompiled.h"
#include "def_dire_maul.h"
#include "GameEvent.h"

#define ENCOUNTERS 1

/* Dire Maul
    0 - Zevrin Thornhoof
*/

struct instance_dire_maul : public ScriptedInstance
{
    instance_dire_maul(Map *pMap) : ScriptedInstance(pMap) { Initialize(); };
    
    uint32 Encounters[ENCOUNTERS];
    
    uint64 pusillinGUID;
    uint64 ironbarkGUID;
    uint64 ironbarkDoorGUID;
    
    void Initialize()
    {
        pusillinGUID = 0;
        ironbarkGUID = 0;
        ironbarkDoorGUID = 0;
    }
    
    bool isEventActive()
    {
        const GameEvent::ActiveEvents& activeEvents = gameeventmgr.GetActiveEventList();
        bool active = activeEvents.find(57) != activeEvents.end();

        return active;
    }
    
    bool IsEncounterInProgress() const
    {
        for (uint8 i = 0; i < ENCOUNTERS; ++i)
            if (Encounters[i] == IN_PROGRESS)
                return true;

        return false;
    }
    
    void OnCreatureCreate(Creature* creature, uint32 entry)
    {
        switch (entry) {
        case 14354: pusillinGUID = creature->GetGUID(); break;
        case 11491: ironbarkGUID = creature->GetGUID(); break;
        case 11486:
            sLog.outString("Pom");
            if (isEventActive())
                creature->SetDisplayId(15734);
                
            break;
        }
    }
    
    void OnObjectCreate(GameObject *pGo)
    {
        switch (pGo->GetEntry()) {
        case 179549: ironbarkDoorGUID = pGo->GetGUID(); break;
        }
    }
    
    uint64 GetData64(uint32 id)
    {
        switch (id) {
        case DATA_GUID_PUSILLIN: 
            return pusillinGUID;
        case DATA_GUID_IRONBARKDOOR: 
            return ironbarkDoorGUID;
        default:
            return 0;
        }
    }
    
    uint32 GetData(uint32 type)
    {
        switch (type) {
        case DATA_ZEVRIM_THORNHOOF: 
            return Encounters[0];
        default: 
            return 0;
        }
    }
    
    void SetData(uint32 type, uint32 data)
    {
        switch (type) {
        case DATA_ZEVRIM_THORNHOOF:
            Encounters[0] = data;
            if (data == DONE) {
                if (Creature *pIronbark = instance->GetCreatureInMap(ironbarkGUID))
                    pIronbark->UpdateEntry(14241);
            }
            break;
        }
    }
    
    const char* Save()
    {
        OUT_SAVE_INST_DATA;
        std::ostringstream stream;
        stream << Encounters[0] /*<< " "  << Encounters[1] << " "  << Encounters[2] << " "  << Encounters[3] << " "
            << Encounters[4] << " "  << Encounters[5]*/;
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
        stream >> Encounters[0] /*>> Encounters[1] >> Encounters[2] >> Encounters[3]
            >> Encounters[4] >> Encounters[5]*/;
        for(uint8 i = 0; i < ENCOUNTERS; ++i)
            if(Encounters[i] == IN_PROGRESS)                // Do not load an encounter as "In Progress" - reset it instead.
                Encounters[i] = NOT_STARTED;
        OUT_LOAD_INST_DATA_COMPLETE;
    }
};

InstanceData* GetInstanceData_instance_dire_maul(Map* pMap)
{
    return new instance_dire_maul(pMap);
}

void AddSC_instance_dire_maul()
{
    Script *newscript;
    
    newscript = new Script;
    newscript->Name = "instance_dire_maul";
    newscript->GetInstanceData = &GetInstanceData_instance_dire_maul;
    newscript->RegisterSelf();
}
