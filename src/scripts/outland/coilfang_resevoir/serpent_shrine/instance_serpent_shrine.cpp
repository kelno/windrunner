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
SDName: Instance_Serpent_Shrine
SD%Complete: 100
SDComment: Instance Data Scripts and functions to acquire mobs and set encounter status for use in various Serpent Shrine Scripts
SDCategory: Coilfang Resevoir, Serpent Shrine Cavern
EndScriptData */

#include "precompiled.h"
#include "def_serpent_shrine.h"

#define ENCOUNTERS 6
#define SPELL_SCALDINGWATER 37284
#define MOB_COILFANG_FRENZY 21508
#define TRASHMOB_COILFANG_PRIESTESS 21220  //6*2
#define TRASHMOB_COILFANG_SHATTERER 21301  //6*3

#define MIN_KILLS 30

//NOTE: there are 6 platforms
//there should be 3 shatterers and 2 priestess on all platforms, total of 30 elites, else it won't work!
//delete all other elites not on platforms! these mobs should only be on those platforms nowhere else.

/* Serpentshrine cavern encounters:
0 - Hydross The Unstable event
1 - Leotheras The Blind Event
2 - The Lurker Below Event
3 - Fathom-Lord Karathress Event
4 - Morogrim Tidewalker Event
5 - Lady Vashj Event
*/

bool GOHello_go_bridge_console(Player *player, GameObject* go)
{
    ScriptedInstance* pInstance = (ScriptedInstance*)go->GetInstanceData();

    if(!pInstance)
        return false;

    if (pInstance)
        pInstance->SetData(DATA_CONTROL_CONSOLE, DONE);

    return true;
}

struct instance_serpentshrine_cavern : public ScriptedInstance
{
    instance_serpentshrine_cavern(Map *map) : ScriptedInstance(map) {Initialize();};

    uint64 LurkerBelow;
    uint64 Sharkkis;
    uint64 Tidalvess;
    uint64 Caribdis;
    uint64 LadyVashj;
    uint64 Karathress;
    uint64 KarathressEvent_Starter;
    uint64 LeotherasTheBlind;
    uint64 LeotherasEventStarter;
    uint64 SerpentshrineConsole;

    uint64 ControlConsole;
    uint64 BridgePart[3];
    uint32 StrangePool;
    uint32 FishingTimer;
    uint32 LurkerSubEvent;
    uint32 WaterCheckTimer;
    uint32 FrenzySpawnTimer;
    uint32 Water;
    uint32 TrashKills;

    bool ShieldGeneratorDeactivated[4];
    uint32 Encounters[ENCOUNTERS];

    void Initialize()
    {
        LurkerBelow = 0;
        Sharkkis = 0;
        Tidalvess = 0;
        Caribdis = 0;
        LadyVashj = 0;
        Karathress = 0;
        KarathressEvent_Starter = 0;
        LeotherasTheBlind = 0;
        LeotherasEventStarter = 0;
        SerpentshrineConsole = 0;

        ControlConsole = 0;
        BridgePart[0] = 0;
        BridgePart[1] = 0;
        BridgePart[2] = 0;
        StrangePool = 0;
        Water = WATERSTATE_NONE;

        ShieldGeneratorDeactivated[0] = false;
        ShieldGeneratorDeactivated[1] = false;
        ShieldGeneratorDeactivated[2] = false;
        ShieldGeneratorDeactivated[3] = false;
        FishingTimer = 1000;
        LurkerSubEvent = 0;
        WaterCheckTimer = 500;
        FrenzySpawnTimer = 2000;
        TrashKills = 0;

        for(uint8 i = 0; i < ENCOUNTERS; i++)
            Encounters[i] = NOT_STARTED;
    }

    bool IsEncounterInProgress() const
    {
        for(uint8 i = 0; i < ENCOUNTERS; i++)
            if(Encounters[i] == IN_PROGRESS) return true;

        return false;
    }
    
    void Update (uint32 diff)
    {
        //Lurker Fishing event
        if(LurkerSubEvent == LURKER_FISHING)
        {
            if(FishingTimer < diff)
            {
                LurkerSubEvent = LURKER_HOOKED;
                SetData(DATA_STRANGE_POOL, IN_PROGRESS);//just fished, signal Lurker script to emerge and start fight, we use IN_PROGRESS so it won't get saved and lurker will be alway invis at start if server restarted
            }else FishingTimer -= diff;
        }
        //Water checks
        if(WaterCheckTimer < diff)
        {
            Water = WATERSTATE_NONE;

            if (GetData(DATA_THELURKERBELOWEVENT) != DONE)
            {
                Water = WATERSTATE_FRENZY;

                if(TrashKills >= MIN_KILLS)
                {
                    if(GameObject *Console = instance->GetGameObjectInMap(SerpentshrineConsole))
                        if (Console->HasFlag(GAMEOBJECT_FLAGS, GO_FLAG_IN_USE))
                            Water = WATERSTATE_SCALDING;
                }
            }
                
            Map::PlayerList const &PlayerList = instance->GetPlayers();

            if (PlayerList.isEmpty())
                return;

            for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
            {
                if (Player* pPlayer = i->getSource())
                {
                    if (pPlayer->IsAlive() && /*i->getSource()->GetPositionZ() <= -21.434931f*/pPlayer->IsInWater() && player->isAttackableByAOE())
                    {
                        if(Water == WATERSTATE_SCALDING)
                        {

                            if(!pPlayer->HasAura(SPELL_SCALDINGWATER))
                            {
                                pPlayer->CastSpell(pPlayer, SPELL_SCALDINGWATER,true);
                            }
                        }
                    }
                    if(!pPlayer->IsInWater())
                        pPlayer->RemoveAurasDueToSpell(SPELL_SCALDINGWATER);
                }
                                    
            }
            WaterCheckTimer = 500;//remove stress from core
        }
        else
            WaterCheckTimer -= diff;

        if(FrenzySpawnTimer < diff)
        {
            if(Water == WATERSTATE_FRENZY)
            {
                Map::PlayerList const &PlayerList = instance->GetPlayers();

                if (PlayerList.isEmpty())
                    return;

                std::list<Player*> targetList;

                for (Map::PlayerList::const_iterator i = PlayerList.begin(); i != PlayerList.end(); ++i)
                {
                    if (Player* pPlayer = i->getSource())
                    {
                        if (pPlayer->IsAlive() && pPlayer->IsInWater() && pPlayer->isAttackableByAOE())
                            targetList.push_back(pPlayer);
                    }
                }

                if (targetList.empty())
                    return;

                Trinity::Containers::RandomResizeList(targetList, 25);
                std::list<Player*>::const_iterator i = targetList.begin();

                if (i != targetList.end())
                {
                    if(Creature* frenzy = (*i)->SummonCreature(MOB_COILFANG_FRENZY, (*i)->GetPositionX(), (*i)->GetPositionY(), (*i)->GetPositionZ(), (*i)->GetOrientation(), TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 2000))
                    {
                        frenzy->Attack((*i), false);
                        frenzy->AddUnitMovementFlag(MOVEMENTFLAG_SWIMMING + MOVEMENTFLAG_LEVITATING);
                    }
                }
            }
            FrenzySpawnTimer = 2000;
        }
        else
            FrenzySpawnTimer -= diff;
    }

    void OnObjectCreate(GameObject *go)
    {
        switch(go->GetEntry())
        {
            case 184568:
                ControlConsole = go->GetGUID();
                go->setActive(true);
            break;

            case 184203:
                BridgePart[0] = go->GetGUID();
                go->setActive(true);
            break;

            case 184204:
                BridgePart[1] = go->GetGUID();
                go->setActive(true);
            break;

            case 184205:
                BridgePart[2] = go->GetGUID();
                go->setActive(true);
            break;
            case 185114:
                SerpentshrineConsole = go->GetGUID();
                break;
            case GAMEOBJECT_FISHINGNODE_ENTRY://no way checking if fish is hooked, so we create a timed event
            {
                uint32 rnd = urand(0, 100);
                if (30 <= rnd)
                {
                    if(LurkerSubEvent == LURKER_NOT_STARTED)
                    {
                        FishingTimer = 10000+rand()%15000;//random time before lurker emerges
                        LurkerSubEvent = LURKER_FISHING;
                    }
                }
                break;
            }
        }
    }

    void OpenDoor(uint64 DoorGUID, bool open)
    {
        if(GameObject *Door = instance->GetGameObjectInMap(DoorGUID))
            Door->SetUInt32Value(GAMEOBJECT_STATE, open ? 0 : 1);
    }

    void OnCreatureCreate(Creature *creature, uint32 creature_entry)
    {
        switch(creature_entry)
        {
            case 21212: LadyVashj = creature->GetGUID();            break;
            case 21214: Karathress = creature->GetGUID();           break;
            case 21966: Sharkkis = creature->GetGUID();             break;
            case 21217: LurkerBelow = creature->GetGUID();          break;
            case 21965: Tidalvess = creature->GetGUID();            break;
            case 21964: Caribdis = creature->GetGUID();             break;
            case 21215: LeotherasTheBlind = creature->GetGUID();    break;
        }
    }

    void OnCreatureDeath(Creature* creature)
    {
        switch (creature->GetEntry())
        {
            case TRASHMOB_COILFANG_PRIESTESS:
            case TRASHMOB_COILFANG_SHATTERER:
                TrashKills++;
                SaveToDB();
                break;
        }
    }

    void SetData64(uint32 type, uint64 data)
    {
        if(type == DATA_KARATHRESSEVENT_STARTER)
            KarathressEvent_Starter = data;
        if(type == DATA_LEOTHERAS_EVENT_STARTER)
            LeotherasEventStarter = data;
    }

    uint64 GetData64(uint32 identifier)
    {
        switch(identifier)
        {
            case DATA_THELURKERBELOW:           return LurkerBelow;
            case DATA_SHARKKIS:                 return Sharkkis;
            case DATA_TIDALVESS:                return Tidalvess;
            case DATA_CARIBDIS:                 return Caribdis;
            case DATA_LADYVASHJ:                return LadyVashj;
            case DATA_KARATHRESS:               return Karathress;
            case DATA_KARATHRESSEVENT_STARTER:  return KarathressEvent_Starter;
            case DATA_LEOTHERAS:                return LeotherasTheBlind;
            case DATA_LEOTHERAS_EVENT_STARTER:  return LeotherasEventStarter;
        }
        return 0;
    }

    void SetData(uint32 type, uint32 data)
    {
        switch(type)
        {
        case DATA_STRANGE_POOL:
            {
                StrangePool = data;
                if(data == NOT_STARTED)
                    LurkerSubEvent = LURKER_NOT_STARTED;
            }
            break;
        case DATA_CONTROL_CONSOLE:
            if(data = DONE)
            {
                OpenDoor(BridgePart[0], true);
                OpenDoor(BridgePart[1], true);
                OpenDoor(BridgePart[2], true);
            }
            ControlConsole = data;
            break;
        case DATA_HYDROSSTHEUNSTABLEEVENT:  Encounters[0] = data;   break;
        case DATA_LEOTHERASTHEBLINDEVENT:   Encounters[1] = data;   break;
        case DATA_THELURKERBELOWEVENT:      Encounters[2] = data;   break;
        case DATA_KARATHRESSEVENT:          Encounters[3] = data;   break;
        case DATA_MOROGRIMTIDEWALKEREVENT:  Encounters[4] = data;   break;
            //Lady Vashj
        case DATA_LADYVASHJEVENT:
            if(data == NOT_STARTED)
            {
                ShieldGeneratorDeactivated[0] = false;
                ShieldGeneratorDeactivated[1] = false;
                ShieldGeneratorDeactivated[2] = false;
                ShieldGeneratorDeactivated[3] = false;
            }
            Encounters[5] = data;   break;
        case DATA_SHIELDGENERATOR1:ShieldGeneratorDeactivated[0] = (data) ? true : false;   break;
        case DATA_SHIELDGENERATOR2:ShieldGeneratorDeactivated[1] = (data) ? true : false;   break;
        case DATA_SHIELDGENERATOR3:ShieldGeneratorDeactivated[2] = (data) ? true : false;   break;
        case DATA_SHIELDGENERATOR4:ShieldGeneratorDeactivated[3] = (data) ? true : false;   break;
        }

        if(data = DONE)
            SaveToDB();
    }

    uint32 GetData(uint32 type)
    {
        switch(type)
        {
            case DATA_HYDROSSTHEUNSTABLEEVENT:  return Encounters[0];
            case DATA_LEOTHERASTHEBLINDEVENT:   return Encounters[1];
            case DATA_THELURKERBELOWEVENT:      return Encounters[2];
            case DATA_KARATHRESSEVENT:          return Encounters[3];
            case DATA_MOROGRIMTIDEWALKEREVENT:  return Encounters[4];
                //Lady Vashj
            case DATA_LADYVASHJEVENT:           return Encounters[5];
            case DATA_SHIELDGENERATOR1:         return ShieldGeneratorDeactivated[0];
            case DATA_SHIELDGENERATOR2:         return ShieldGeneratorDeactivated[1];
            case DATA_SHIELDGENERATOR3:         return ShieldGeneratorDeactivated[2];
            case DATA_SHIELDGENERATOR4:         return ShieldGeneratorDeactivated[3];
            case DATA_CANSTARTPHASE3:
                if(ShieldGeneratorDeactivated[0] && ShieldGeneratorDeactivated[1] && ShieldGeneratorDeactivated[2] && ShieldGeneratorDeactivated[3])return 1;break;
            case DATA_STRANGE_POOL:             return StrangePool;
        }
        return 0;
    }
    const char* Save()
    {
        OUT_SAVE_INST_DATA;
        std::ostringstream stream;
        stream << Encounters[0] << " " << Encounters[1] << " " << Encounters[2] << " "
            << Encounters[3] << " " << Encounters[4] << " " << Encounters[5] << " " << TrashKills;
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
        if(!in)
        {
            OUT_LOAD_INST_DATA_FAIL;
            return;
        }
        OUT_LOAD_INST_DATA(in);
        std::istringstream stream(in);
        stream >> Encounters[0] >> Encounters[1] >> Encounters[2] >> Encounters[3]
        >> Encounters[4] >> Encounters[5] >> TrashKills;
        for(uint8 i = 0; i < ENCOUNTERS; ++i)
            if(Encounters[i] == IN_PROGRESS)                // Do not load an encounter as "In Progress" - reset it instead.
                Encounters[i] = NOT_STARTED;
        OUT_LOAD_INST_DATA_COMPLETE;
    }
};

InstanceData* GetInstanceData_instance_serpentshrine_cavern(Map* map)
{
    return new instance_serpentshrine_cavern(map);
}

void AddSC_instance_serpentshrine_cavern()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "instance_serpent_shrine";
    newscript->GetInstanceData = &GetInstanceData_instance_serpentshrine_cavern;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="go_bridge_console";
    newscript->pGOHello = &GOHello_go_bridge_console;
    newscript->RegisterSelf();
}

