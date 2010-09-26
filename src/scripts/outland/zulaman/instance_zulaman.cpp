 /* Copyright (C) 2006 - 2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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
SDName: instance_zulaman
SD%Complete: 80
SDComment:
SDCategory: Zul'Aman
EndScriptData */

#include "precompiled.h"
#include "def_zulaman.h"

#define ENCOUNTERS     7
#define RAND_VENDOR    2

//187021 //Harkor's Satchel
//186648 //Tanzar's Trunk
//186672 //Ashli's Bag
//186667 //Kraz's Package
// Chests spawn at bear/eagle/dragonhawk/lynx bosses
// The loots depend on how many bosses have been killed, but not the entries of the chests
// But we cannot add loots to gameobject, so we have to use the fixed loot_template
struct SHostageInfo
{
    uint32 npc, go;
    float x, y, z, o;
};

static SHostageInfo HostageInfo[] =
{
    {23790, 186648, -57, 1343, 40.77, 3.2}, // bear
    {23999, 187021, 400, 1414, 74.36, 3.3}, // eagle
    {24001, 186672, -35, 1134, 18.71, 1.9}, // dragonhawk
    {24024, 186667, 413, 1117,  6.32, 3.1}  // lynx

};

struct instance_zulaman : public ScriptedInstance
{
    instance_zulaman(Map *map) : ScriptedInstance(map) {Initialize();};

    uint64 HarkorsSatchelGUID;
    uint64 TanzarsTrunkGUID;
    uint64 AshlisBagGUID;
    uint64 KrazsPackageGUID;
    uint64 AmanishiLookoutGUID;

    uint64 HexLordGateGUID;
    uint64 HexLordDoorGUID;     // Located just in his back
    uint64 ZulJinGateGUID;
    uint64 AkilzonDoorGUID;
    uint64 ZulJinDoorGUID;
    uint64 HalazziDoorGUID;

    uint32 QuestTimer;
    uint32 GauntletWarriorsTimer;
    uint32 GauntletEaglesTimer;
    uint16 BossKilled;
    uint16 QuestMinute;
    uint16 ChestLooted;
    uint16 GongDone;

    uint32 Encounters[ENCOUNTERS];
    uint32 RandVendor[RAND_VENDOR];
    
    std::vector<Creature*> warriorsList;
    std::vector<Creature*> eaglesList;

    void Initialize()
    {
        HarkorsSatchelGUID = 0;
        TanzarsTrunkGUID = 0;
        AshlisBagGUID = 0;
        KrazsPackageGUID = 0;
        AmanishiLookoutGUID = 0;

        HexLordGateGUID = 0;
        HexLordDoorGUID = 0;
        ZulJinGateGUID = 0;
        AkilzonDoorGUID = 0;
        HalazziDoorGUID = 0;
        ZulJinDoorGUID = 0;

        QuestTimer = 0;
        GauntletWarriorsTimer = 0;
        GauntletEaglesTimer = 0;
        QuestMinute = 0;
        BossKilled = 0;
        ChestLooted = 0;
        GongDone = 0;
        
        warriorsList.clear();
        eaglesList.clear();

        for(uint8 i = 0; i < ENCOUNTERS; i++)
            Encounters[i] = NOT_STARTED;
        for(uint8 i = 0; i < RAND_VENDOR; i++)
            RandVendor[i] = NOT_STARTED;
    }

    bool IsEncounterInProgress() const
    {
        for(uint8 i = 0; i < ENCOUNTERS; i++)
            if(Encounters[i] == IN_PROGRESS) return true;

        return false;
    }

    void OnCreatureCreate(Creature *creature, uint32 creature_entry)
    {
        switch(creature_entry)
        {
        case 24175: AmanishiLookoutGUID = creature->GetGUID(); break;
        case 23578://janalai
        case 23863://zuljin
        case 24239://hexlord
        case 23577://halazzi
        case 23576://nalorakk
        default: break;
        }
    }

    void OnObjectCreate(GameObject *go)
    {
        switch(go->GetEntry())
        {
        case 186303: 
            HalazziDoorGUID = go->GetGUID();
            if (GetData(DATA_HALAZZIEVENT) != DONE)
                HandleGameObject(NULL, false, go);
            break;
        case 186304: ZulJinGateGUID  = go->GetGUID(); break;
        case 186305: HexLordGateGUID = go->GetGUID(); break;
        case 186306: HexLordDoorGUID = go->GetGUID(); break;
        case 186858: AkilzonDoorGUID = go->GetGUID(); break;
        case 186859: ZulJinDoorGUID  = go->GetGUID(); HandleGameObject(NULL, true, go); break;

        case 187021: HarkorsSatchelGUID  = go->GetGUID(); break;
        case 186648: TanzarsTrunkGUID = go->GetGUID(); break;
        case 186672: AshlisBagGUID = go->GetGUID(); break;
        case 186667: KrazsPackageGUID  = go->GetGUID(); break;
        case 186728:
            if (GetData(DATA_GONG_EVENT) == DONE)
                go->SetUInt32Value(GAMEOBJECT_STATE, 0);
            break;
        default: break;

        }
        CheckInstanceStatus();
    }

    void OpenDoor(uint64 DoorGUID, bool open)
    {
        if(GameObject *Door = instance->GetGameObjectInMap(DoorGUID))
            Door->SetUInt32Value(GAMEOBJECT_STATE, open ? 0 : 1);
    }

    void SummonHostage(uint8 num)
    {
        if(!QuestMinute)
            return;

        Map::PlayerList const &PlayerList = instance->GetPlayers();
        if (PlayerList.isEmpty())
            return;

        Map::PlayerList::const_iterator i = PlayerList.begin();
        if(Player* i_pl = i->getSource())
        {
            if(Unit* Hostage = i_pl->SummonCreature(HostageInfo[num].npc, HostageInfo[num].x, HostageInfo[num].y, HostageInfo[num].z, HostageInfo[num].o, TEMPSUMMON_DEAD_DESPAWN, 0))
            {
                Hostage->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                Hostage->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            }
        }
    }

    void CheckInstanceStatus()
    {
        if(BossKilled >= 4)
            OpenDoor(HexLordGateGUID, true);

        if(BossKilled >= 5) {
            OpenDoor(ZulJinGateGUID, true);
            OpenDoor(HexLordDoorGUID, true);
        }
    }

    void UpdateWorldState(uint32 field, uint32 value)
    {
        WorldPacket data(SMSG_UPDATE_WORLD_STATE, 8);
        data << field << value;
        instance->SendToPlayers(&data);
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

        debug_log("TSCR: Instance Zul'Aman: GetPlayerInMap, but PlayerList is empty!");
        return NULL;
    }

    const char* Save()
    {
        std::ostringstream ss;
        ss << "S " << BossKilled << " " << ChestLooted << " " << QuestMinute << " " << GongDone;
        char* data = new char[ss.str().length()+1];
        strcpy(data, ss.str().c_str());
        //error_log("TSCR: Zul'aman saved, %s.", data);
        return data;
    }

    void Load(const char* load)
    {
        if(!load) return;
        std::istringstream ss(load);
        //error_log("TSCR: Zul'aman loaded, %s.", ss.str().c_str());
        char dataHead; // S
        uint16 data1, data2, data3, data4;
        ss >> dataHead >> data1 >> data2 >> data3 >> data4;
        //error_log("TSCR: Zul'aman loaded, %d %d %d.", data1, data2, data3);
        if(dataHead == 'S')
        {
            BossKilled = data1;
            ChestLooted = data2;
            QuestMinute = data3;
            GongDone = data4;
        }else error_log("TSCR: Zul'aman: corrupted save data.");
    }

    void SetData(uint32 type, uint32 data)
    {
        switch(type)
        {
        case DATA_NALORAKKEVENT:
            Encounters[0] = data;
            if(data == DONE)
            {
                if(QuestMinute)
                {
                    QuestMinute += 15;
                    UpdateWorldState(3106, QuestMinute);
                }
                SummonHostage(0);
            }
            break;
        case DATA_AKILZONEVENT:
            Encounters[1] = data;
            OpenDoor(AkilzonDoorGUID, data != IN_PROGRESS);
            if(data == DONE)
            {
                if(QuestMinute)
                {
                    QuestMinute += 10;
                    UpdateWorldState(3106, QuestMinute);
                }
                SummonHostage(1);
            }
            break;
        case DATA_JANALAIEVENT:
            Encounters[2] = data;
            if(data == DONE) SummonHostage(2);
            break;
        case DATA_HALAZZIEVENT:
            Encounters[3] = data;
            //OpenDoor(HalazziDoorGUID, data != IN_PROGRESS);
            if(data == DONE) {
                SummonHostage(3);
                OpenDoor(HalazziDoorGUID, true);
            }
            break;
        case DATA_HEXLORDEVENT:
            Encounters[4] = data;
            if(data == IN_PROGRESS)
                OpenDoor(HexLordGateGUID, false);
            else if(data == NOT_STARTED)
                CheckInstanceStatus();
            break;
        case DATA_ZULJINEVENT:
            Encounters[5] = data;
            OpenDoor(ZulJinDoorGUID, data != IN_PROGRESS);
            break;
        case DATA_CHESTLOOTED:
            ChestLooted++;
            SaveToDB();
            break;
        case TYPE_RAND_VENDOR_1:
            RandVendor[0] = data;
            break;
        case TYPE_RAND_VENDOR_2:
            RandVendor[1] = data;
            break;
        case DATA_QUESTMINUTE:  // Started from Harrison Jones script
            QuestMinute = 21;
            break;
        case DATA_GAUNTLET:
            Encounters[6] = data;
            if (data == IN_PROGRESS) {
                GauntletWarriorsTimer = 20000;
                GauntletEaglesTimer = 90000;
            }
            else if (data == FAIL) {
                // Despawn mobs if wiped, not if tempest is pulled
                for (std::vector<Creature*>::iterator itr = warriorsList.begin(); itr != warriorsList.end(); itr++)
                    (*itr)->DisappearAndDie();
                warriorsList.clear();
                for (std::vector<Creature*>::iterator itr = eaglesList.begin(); itr != eaglesList.end(); itr++)
                    (*itr)->DisappearAndDie();
                eaglesList.clear();
                
            }
            else if (data == DONE) {
                GauntletWarriorsTimer = 0;
                GauntletEaglesTimer = 0;
                SaveToDB();
            }
            break;
        case DATA_GONG_EVENT:
            GongDone = data;
            SaveToDB();
            break;
        }

        if(data == DONE && type != DATA_GAUNTLET && type != DATA_GONG_EVENT)   // Don't increase BossKilled for gauntlet or for gong
        {
            BossKilled++;
            if(QuestMinute && BossKilled >= 4)
            {
                QuestMinute = 0;
                UpdateWorldState(3104, 0);
            }
            CheckInstanceStatus();
            SaveToDB();
        }
    }

    uint32 GetData(uint32 type)
    {
        switch(type)
        {
        case DATA_NALORAKKEVENT:    return Encounters[0];
        case DATA_AKILZONEVENT:     return Encounters[1];
        case DATA_JANALAIEVENT:     return Encounters[2];
        case DATA_HALAZZIEVENT:     return Encounters[3];
        case DATA_HEXLORDEVENT:     return Encounters[4];
        case DATA_ZULJINEVENT:      return Encounters[5];
        case DATA_CHESTLOOTED:      return ChestLooted;
        case TYPE_RAND_VENDOR_1:    return RandVendor[0];
        case TYPE_RAND_VENDOR_2:    return RandVendor[1];
        case DATA_GAUNTLET:         return Encounters[6];
        case DATA_GONG_EVENT:       return GongDone;
        default:                    return 0;
        }
    }

    void Update(uint32 diff)
    {
        if(QuestMinute)
        {
            if(QuestTimer < diff)
            {
                QuestMinute--;
                SaveToDB();
                QuestTimer += 60000;
                if(QuestMinute)
                {
                    UpdateWorldState(3104, 1);
                    UpdateWorldState(3106, QuestMinute);
                }else UpdateWorldState(3104, 0);
            }
            QuestTimer -= diff;
        }
        
        if (GetData(DATA_GAUNTLET) == IN_PROGRESS) {
            Player *plr = GetPlayerInMap();
            if (!plr) {
                SetData(DATA_GAUNTLET, FAIL);
                if (Creature *pLookout = instance->GetCreatureInMap(AmanishiLookoutGUID))
                    pLookout->Respawn();
                return;
            }
            
            // Timers probably not blizzlike
            if (GauntletWarriorsTimer <= diff) {
                for (int i = 0; i < 2; i++) {
                    if (Creature *pWarrior = plr->SummonCreature(24225, 227.153259, 1477.462524, 25.917961, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 600000)) {
                        warriorsList.push_back(pWarrior);
                        pWarrior->setActive(true);
                        pWarrior->SetSpeed(MOVE_WALK, 3);
                        pWarrior->GetMotionMaster()->MovePath(24225, false);
                    }
                }
                GauntletWarriorsTimer = 110000;
            }
            else GauntletWarriorsTimer -= diff;
            
            if (GauntletEaglesTimer <= diff) {
                for (int i = 0; i < 5; i++) {
                    if (Creature *pEagle = plr->SummonCreature(24159, 336.942902, 1395.780396, 74.461060, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 600000)) {
                        eaglesList.push_back(pEagle);
                        pEagle->setActive(true);
                        pEagle->SetSpeed(MOVE_WALK, 3);
                        pEagle->GetMotionMaster()->MovePath(24159, false);
                    }
                }
                GauntletEaglesTimer = 90000;
            }
            else GauntletEaglesTimer -= diff;
        }
    }
};

InstanceData* GetInstanceData_instance_zulaman(Map* map)
{
    return new instance_zulaman(map);
}

void AddSC_instance_zulaman()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "instance_zulaman";
    newscript->GetInstanceData = &GetInstanceData_instance_zulaman;
    newscript->RegisterSelf();
}

