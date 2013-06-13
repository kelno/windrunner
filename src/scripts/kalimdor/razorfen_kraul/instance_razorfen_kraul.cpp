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
SDName: Instance_Razorfen_Kraul
SD%Complete:
SDComment:
SDCategory: Razorfen Kraul
EndScriptData */

#include "precompiled.h"
#include "def_razorfen_kraul.h"


#define WARD_KEEPERS_NR 2

struct instance_razorfen_kraul : public ScriptedInstance
{
    instance_razorfen_kraul(Map *map) : ScriptedInstance(map) {Initialize();};

    uint64 DoorWardGUID;
    int WardKeeperAlive;

    void Initialize()
    {
        WardKeeperAlive = 0;
        DoorWardGUID = 0;
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
        sLog.outError("TSCR: Instance Razorfen Kraul: GetPlayerInMap, but PlayerList is empty!");
        return NULL;
    }

    void OnObjectCreate(GameObject *go)
    {
        switch(go->GetEntry())
        {
            case 21099:
                DoorWardGUID = go->GetGUID();
                break;
        }
    }

    void OnCreatureCreate(Creature* pCreature, uint32 creature_entry)
    {
        switch(creature_entry)
        {
            case 4625:
                WardKeeperAlive++;
                break;
        }
    }

    void SetData(uint32 type, uint32 data)
    {
        switch(type)
        {
            case TYPE_WARD_KEEPERS:
                if (data == NOT_STARTED)
                {
                    if (WardKeeperAlive > 0)
                    {
                        WardKeeperAlive--;
                        if (WardKeeperAlive == 0)
                        {
                            if (GameObject *door = instance->GetGameObject(DoorWardGUID))
                                door->SwitchDoorOrButton(true);
                        }
                    }
                }
                break;
        }
    }
};

InstanceData* GetInstanceData_instance_razorfen_kraul(Map* map)
{
    return new instance_razorfen_kraul(map);
}

void AddSC_instance_razorfen_kraul()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "instance_razorfen_kraul";
    newscript->GetInstanceData = &GetInstanceData_instance_razorfen_kraul;
    newscript->RegisterSelf();
}
