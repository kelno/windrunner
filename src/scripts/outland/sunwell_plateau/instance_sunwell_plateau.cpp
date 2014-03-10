/* Copyright (C) 2006 - 2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

/* ScriptData
SDName: Instance_Sunwell_Plateau
SD%Complete: 20
SDComment: VERIFY SCRIPT, rename Gates
SDCategory: Sunwell_Plateau
EndScriptData */

#include "precompiled.h"
#include "def_sunwell_plateau.h"

#define ENCOUNTERS 6

/*enum GoState{
CLOSE    = 1,
OPEN    = 0
};*/

/* Sunwell Plateau:
0 - Kalecgos and Sathrovarr
1 - Brutallus
2 - Felmyst
3 - Eredar Twins (Alythess and Sacrolash)
4 - M'uru
5 - Kil'Jaeden
*/

/*
* GAUNTLET TODO:
* 
* - les 4 trashs du début ont une position_z < 40, utiliser ce critère pour lancer l'event
* - fin de l'event quand on pull le commandant
* - check si les deux trashs sont random ou si c'est un de chaque sorte à chaque fois
* - fiels : 12 sec
* - autres : 40 sec
*/

struct instance_sunwell_plateau : public ScriptedInstance
{
    instance_sunwell_plateau(Map *map) : ScriptedInstance(map) {Initialize();};

    uint32 Encounters[ENCOUNTERS];

    /** Creatures **/
    uint64 Kalecgos_Dragon;
    uint64 Kalecgos_Human;
    uint64 Sathrovarr;
    uint64 Brutallus;
    uint64 Madrigosa;
    uint64 Felmyst;
    uint64 Alythess;
    uint64 Sacrolash;
    uint64 Muru;
    uint64 BlackHole;
    uint64 Entropius;
    uint64 KilJaeden;
    uint64 KilJaedenController;
    uint64 Anveena;
    uint64 KalecgosKJ;
    uint64 FlightLeft;
    uint64 FlightRight;
    uint64 CommanderGUID;
    uint32 SpectralPlayers;
    uint64 BarrierTriggerGUID;
    std::vector<uint64> northList, centerList, southList;

    /** GameObjects **/
    uint64 ForceField;                                      // Kalecgos Encounter
    uint64 KalecgosWall[2];
    uint64 FireBarrier;                                     // Felmysts Encounter
    uint64 MurusGate[2];                                    // Murus Encounter
    uint64 IceBarrier;
    uint64 SecondGate; // Just after the Twins

    /*** Misc ***/
    uint32 SpectralRealmTimer;
    std::vector<uint64> SpectralRealmList;
    uint32 GauntletStatus;
    uint32 BringersTimer;
    uint32 FiendTimer;
    uint32 IceBarrierTimer;
    bool IceBarrierDone;
    uint32 felmystNorthTimer, felmystCenterTimer, felmystSouthTimer;
    uint32 phaseEntropius;

    void Initialize()
    {
        memset(&Encounters, 0, sizeof(Encounters));

        /*** Creatures ***/
        Kalecgos_Dragon         = 0;
        Kalecgos_Human          = 0;
        Sathrovarr              = 0;
        Brutallus               = 0;
        Madrigosa               = 0;
        Felmyst                 = 0;
        Alythess                = 0;
        Sacrolash               = 0;
        Muru                    = 0;
        BlackHole               = 0;
        Entropius               = 0;
        KilJaeden               = 0;
        KilJaedenController     = 0;
        Anveena                 = 0;
        KalecgosKJ              = 0;
        CommanderGUID           = 0;
        SpectralPlayers         = 0;
        BarrierTriggerGUID      = 0;
        northList.clear();
        centerList.clear();
        southList.clear();

        /*** GameObjects ***/
        ForceField  = 0;
        FireBarrier = 0;
        MurusGate[0] = 0;
        MurusGate[1] = 0;
        KalecgosWall[0] = 0;
        KalecgosWall[1] = 0;
        IceBarrier = 0;

        /*** Misc ***/
        SpectralRealmTimer      = 5000;
        BringersTimer           = 40000;
        FiendTimer              = 0;
        IceBarrierTimer         = 1000;

        IceBarrierDone = false;
        
        felmystNorthTimer = 0;
        felmystCenterTimer = 0;
        felmystSouthTimer = 0;

        /*** Encounters ***/
        for(uint8 i = 0; i < ENCOUNTERS; ++i)
            Encounters[i] = NOT_STARTED;
            
        GauntletStatus = NOT_STARTED;
        phaseEntropius = NOT_STARTED;
    }

    bool IsEncounterInProgress() const
    {
        for(uint8 i = 0; i < ENCOUNTERS; ++i)
            if(Encounters[i] == IN_PROGRESS)
                return true;

        return false;
    }
    
    // Override ScriptedInstance::GetPlayerInMap because of aura 45839 check
    Player* GetPlayerInMap()
    {
        Map::PlayerList const& players = instance->GetPlayers();

        if (!players.isEmpty())
        {
            for(Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
            {
                Player* plr = itr->getSource();
                if (plr)
                    return plr;
            }
        }

        sLog.outError("Instance Sunwell Plateau: GetPlayerInMap called, and not player found!");
        return NULL;
    }
    
    Player* GetAlivePlayerInMap()
    {
        Map::PlayerList const& players = instance->GetPlayers();

        if (!players.isEmpty())
        {
            for(Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
            {
                Player* plr = itr->getSource();
                if (plr && plr->IsAlive() && !plr->isGameMaster())
                    return plr;
            }
        }

        return NULL;
    }

    void OnCreatureCreate(Creature* pCreature, uint32 creature_entry)
    {
        switch(pCreature->GetEntry())
        {
            case 24850: Kalecgos_Dragon     = pCreature->GetGUID(); break;
            case 24891: Kalecgos_Human      = pCreature->GetGUID(); break;
            case 24892: Sathrovarr          = pCreature->GetGUID(); break;
            case 24882: Brutallus           = pCreature->GetGUID(); pCreature->setActive(true); break;
            case 24895: Madrigosa           = pCreature->GetGUID(); pCreature->setActive(true); break;
            case 25038: Felmyst             = pCreature->GetGUID(); break;
            case 25166: Alythess            = pCreature->GetGUID(); break;
            case 25165: Sacrolash           = pCreature->GetGUID(); break;
            case 25741: Muru                = pCreature->GetGUID(); break;
            case 25855: BlackHole           = pCreature->GetGUID(); break;
            case 25840: Entropius           = pCreature->GetGUID(); break;
            case 25315: KilJaeden           = pCreature->GetGUID(); break;
            case 25608: KilJaedenController = pCreature->GetGUID(); break;
            case 26046: Anveena             = pCreature->GetGUID(); break;
            case 25319: KalecgosKJ          = pCreature->GetGUID(); break;
            case 25357: FlightLeft          = pCreature->GetGUID(); pCreature->setActive(true); break;
            case 25358: FlightRight         = pCreature->GetGUID(); pCreature->setActive(true); break;
            case 25837: CommanderGUID       = pCreature->GetGUID(); pCreature->setActive(true); break;
            case 19871: BarrierTriggerGUID  = pCreature->GetGUID(); break;
            case 23472: //fog triggers
                //sLog.outString("PositionX: %f", pCreature->GetPositionX());
                if (pCreature->GetPositionX() > 1480.0f)
                    northList.push_back(pCreature->GetGUID());
                else if (pCreature->GetPositionX() > 1460.0f)
                    centerList.push_back(pCreature->GetGUID());
                else
                    southList.push_back(pCreature->GetGUID());
                //sLog.outString("Lists sizes: %u - %u - %u", northList.size(), centerList.size(), southList.size());
                break;
        }
        m_mNpcEntryGuidStore[pCreature->GetEntry()] = pCreature->GetGUID();
    }

    void OnCreatureDeath(Creature* creature)
    {
    	switch(creature->GetEntry())
    	{
    	    case 25653:
    	    	if (Unit* summoner = creature->GetSummoner())
    	    	{
    	    	    summoner->RemoveAurasDueToSpell(45838);
    	    	    summoner->RemoveAurasDueToSpell(45839);
    	    	}
    		    break;
    	}
    }

    void OnObjectCreate(GameObject* pGo)
    {
        switch(pGo->GetEntry())
        {
            case 188421: ForceField     = pGo->GetGUID(); break;
            case 188523: KalecgosWall[0] = pGo->GetGUID(); break;
            case 188524: KalecgosWall[1] = pGo->GetGUID(); break;
            case 188075:
                if (Encounters[2] == DONE)
                    HandleGameObject(NULL, true, pGo);
                FireBarrier = pGo->GetGUID();
                break;
            case 187990: MurusGate[0]   = pGo->GetGUID(); break;
            case 188118:
                if (Encounters[4] == DONE)
                    HandleGameObject(NULL, true, pGo);
                MurusGate[1]= pGo->GetGUID();
                break;
            /*case 187766:    // The first gate FIXME: Always closed for now, will change later
                HandleGameObject(NULL, false, pGo);
                break;*/
            case 187764:    // The second gate FIXME: Always closed for now, will change later
                if (GetData(DATA_EREDAR_TWINS_EVENT) != DONE)
                    HandleGameObject(NULL, false, pGo);
                SecondGate = pGo->GetGUID();
                break;
            case 187765:    // The third gate FIXME: Always closed for now, will change later
                HandleGameObject(NULL, false, pGo);
                break;
            case 188119:
                IceBarrier = pGo->GetGUID(); break;
                pGo->setActive(true);
                break;
        }
    }
    
    void ShowIceBarrier()
    {
        Map::PlayerList const& players = instance->GetPlayers();

        if (players.isEmpty())
            return;
            
        Player* tmp = GetPlayerInMap();
        GameObject* barrier = GameObject::GetGameObject(*tmp, IceBarrier);
        if (!barrier)
            return;

        for (Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr) {
            if (Player* plr = itr->getSource())
                barrier->SendUpdateToPlayer(plr);
        }
    }

    uint32 GetData(uint32 id)
    {
        switch(id)
        {
            case DATA_KALECGOS_EVENT:     return Encounters[0];
            case DATA_BRUTALLUS_EVENT:    return Encounters[1];
            case DATA_FELMYST_EVENT:      return Encounters[2];
            case DATA_GAUNTLET_EVENT:     return GauntletStatus;
            case DATA_EREDAR_TWINS_EVENT: return Encounters[3];
            case DATA_MURU_EVENT:         return Encounters[4];
            case DATA_KILJAEDEN_EVENT:    return Encounters[5];
            case DATA_MURU_TO_ENTROPIUS:  return phaseEntropius;
        }
        return 0;
    }

    uint64 GetData64(uint32 id)
    {
        switch(id)
        {
            case DATA_KALECGOS_DRAGON:      return Kalecgos_Dragon;
            case DATA_KALECGOS_HUMAN:       return Kalecgos_Human;
            case DATA_SATHROVARR:           return Sathrovarr;
            case DATA_GO_FORCEFIELD:        return ForceField;
            case DATA_GO_KALEC_WALL_1:      return KalecgosWall[0];
            case DATA_GO_KALEC_WALL_2:      return KalecgosWall[1];
            case DATA_BRUTALLUS:            return Brutallus;
            case DATA_MADRIGOSA:            return Madrigosa;
            case DATA_FELMYST:              return Felmyst;
            case DATA_COMMANDER:            return CommanderGUID;
            case DATA_ALYTHESS:             return Alythess;
            case DATA_SACROLASH:            return Sacrolash;
            case DATA_MURU:                 return Muru;
            case DATA_BLACK_HOLE:           return BlackHole;
            case DATA_ENTROPIUS:            return Entropius;
            case DATA_KILJAEDEN:            return KilJaeden;
            case DATA_KILJAEDEN_CONTROLLER: return KilJaedenController;
            case DATA_ANVEENA:              return Anveena;
            case DATA_KALECGOS_KJ:          return KalecgosKJ;
            case MOB_FLIGHT_LEFT:           return FlightLeft;
            case MOB_FLIGHT_RIGHT:          return FlightRight;
            case DATA_PLAYER_GUID:
            {
                Player* Target = GetPlayerInMap();
                if (!Target) sLog.outError("Sunwell: No target found in GetData64()!");
                return Target ? Target->GetGUID() : 0;
            }
            case DATA_GO_ICE_BARRIER:
                return IceBarrier;
            case DATA_GO_FIRE_BARRIER:
                return FireBarrier;
        }
        return 0;
    }

    void SetData(uint32 id, uint32 data)
    {
        switch(id)
        {
            case DATA_KALECGOS_EVENT:
                {
                    Encounters[0] = data;
                    if (data == DONE)
                        ((InstanceMap *)instance)->PermBindAllPlayers(GetPlayerInMap());
                }
                break;
            case DATA_BRUTALLUS_EVENT:     Encounters[1] = data; break;
            case DATA_FELMYST_EVENT:
                /*if (data == DONE)
                    HandleGameObject(FireBarrier, true);*/
                Encounters[2] = data; break;
            case DATA_GAUNTLET_EVENT:
                GauntletStatus = data;
                if (data == IN_PROGRESS) {
                    BringersTimer = 5000;
                    FiendTimer = 2000;
                }
                break;
            case DATA_EREDAR_TWINS_EVENT:
                Encounters[3] = data;
                HandleGameObject(SecondGate, true);
                break;
            case DATA_MURU_EVENT:
                switch(data)
                {
                    case DONE:
                        HandleGameObject(MurusGate[0], true);
                        HandleGameObject(MurusGate[1], true);
                        break;
                    case NOT_STARTED:
                        HandleGameObject(MurusGate[0], true);
                        HandleGameObject(MurusGate[1], false);
                        break;
                }
                Encounters[4] = data; break;
            case DATA_KILJAEDEN_EVENT:     Encounters[5] = data; break;
            case DATA_ACTIVATE_NORTH_TO_LEFT:
                for (std::vector<uint64>::iterator itr = northList.begin(); itr != northList.end(); itr++) {
                    if (Creature *trigger = instance->GetCreatureInMap(*itr)) {
                        if (trigger->GetPositionY() > data)
                            trigger->CastSpell(trigger, 45582, true);
                    }
                }
                break;
            case DATA_ACTIVATE_CENTER_TO_LEFT:
                for (std::vector<uint64>::iterator itr = centerList.begin(); itr != centerList.end(); itr++) {
                    if (Creature *trigger = instance->GetCreatureInMap(*itr)) {
                        if (trigger->GetPositionY() > data)
                            trigger->CastSpell(trigger, 45582, true);
                    }
                }
                break;
            case DATA_ACTIVATE_SOUTH_TO_LEFT:
                for (std::vector<uint64>::iterator itr = southList.begin(); itr != southList.end(); itr++) {
                    if (Creature *trigger = instance->GetCreatureInMap(*itr)) {
                        if (trigger->GetPositionY() > data)
                            trigger->CastSpell(trigger, 45582, true);
                    }
                }
                break;
            case DATA_ACTIVATE_NORTH_TO_RIGHT:
                for (std::vector<uint64>::iterator itr = northList.begin(); itr != northList.end(); itr++) {
                    if (Creature *trigger = instance->GetCreatureInMap(*itr)) {
                        if (trigger->GetPositionY() < data)
                            trigger->CastSpell(trigger, 45582, true);
                    }
                }
                break;
            case DATA_ACTIVATE_CENTER_TO_RIGHT:
                for (std::vector<uint64>::iterator itr = centerList.begin(); itr != centerList.end(); itr++) {
                    if (Creature *trigger = instance->GetCreatureInMap(*itr)) {
                        if (trigger->GetPositionY() < data)
                            trigger->CastSpell(trigger, 45582, true);
                    }
                }
                break;
            case DATA_ACTIVATE_SOUTH_TO_RIGHT:
                for (std::vector<uint64>::iterator itr = southList.begin(); itr != southList.end(); itr++) {
                    if (Creature *trigger = instance->GetCreatureInMap(*itr)) {
                        if (trigger->GetPositionY() < data)
                            trigger->CastSpell(trigger, 45582, true);
                    }
                }
                break;
            case DATA_ICEBARRIER_EVENT:
                IceBarrierDone = true;
                IceBarrierTimer = 3600000;
                if (Creature* trigger = instance->GetCreatureInMap(BarrierTriggerGUID))
                    trigger->CastSpell(trigger, 47030, true);
                ShowIceBarrier();
                break;
            case DATA_MURU_GATE_EVENT:
                HandleGameObject(MurusGate[0], false);
                HandleGameObject(MurusGate[1], false);
                break;
            case DATA_MURU_TO_ENTROPIUS:
                phaseEntropius = data;
                break;
        }

        if (data == DONE)
            SaveToDB();
    }

    void Update(uint32 diff)
    {
        if (!IceBarrierDone) {
            if (IceBarrierTimer <= diff) {
                ShowIceBarrier();
                IceBarrierTimer = 1000;
            }
            else
                IceBarrierTimer -= diff;
        }
        
        /*if (felmystNorthTimer) {
            if (felmystNorthTimer <= diff) {
                for (std::vector<uint64>::iterator itr = northList.begin(); itr != northList.end(); itr++) {
                    if (Creature *trigger = instance->GetCreatureInMap(*itr))
                        trigger->CastSpell(trigger, 45582, true);
                }
                felmystNorthTimer = 0;
            }
            else
                felmystNorthTimer -= diff;
        }
        
        if (felmystCenterTimer) {
            if (felmystCenterTimer <= diff) {
                for (std::vector<uint64>::iterator itr = centerList.begin(); itr != centerList.end(); itr++) {
                    if (Creature *trigger = instance->GetCreatureInMap(*itr))
                        trigger->CastSpell(trigger, 45582, true);
                }
                felmystCenterTimer = 0;
            }
            else
                felmystCenterTimer -= diff;
        }
        
        if (felmystSouthTimer) {
            if (felmystSouthTimer <= diff) {
                for (std::vector<uint64>::iterator itr = southList.begin(); itr != southList.end(); itr++) {
                    if (Creature *trigger = instance->GetCreatureInMap(*itr))
                        trigger->CastSpell(trigger, 45582, true);
                }
                felmystSouthTimer = 0;
            }
            else
                felmystSouthTimer -= diff;
        }*/

        Unit* Commander = instance->GetCreatureInMap(CommanderGUID);
        if (!Commander || Commander->IsInCombat() || Commander->isDead() || !GetAlivePlayerInMap())
            GauntletStatus = NOT_STARTED;

        if (GauntletStatus != IN_PROGRESS || GetData(DATA_EREDAR_TWINS_EVENT) == DONE) {
            BringersTimer = 40000;
            FiendTimer = 0;
            
            return;
        }

        if (BringersTimer <= diff) {
            float x, y, z;
            
            Commander->GetRandomPoint(Commander->GetPositionX(), Commander->GetPositionY(), Commander->GetPositionZ(), 1.0f, x, y, z);
            if (Creature *Bringer1 = Commander->SummonCreature(MOB_SOULBINDER, x, y, z, Commander->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0)) {
                //Bringer1->MonsterSay("[DEBUG] First bringer!", LANG_UNIVERSAL, NULL);
                Bringer1->setActive(true);
                Bringer1->SetSpeed(MOVE_WALK, 3.5);
                Bringer1->GetMotionMaster()->MovePath(25851, false);
            }
            
            Commander->GetRandomPoint(Commander->GetPositionX(), Commander->GetPositionY(), Commander->GetPositionZ(), 1.0f, x, y, z);
            if (Creature *Bringer2 = Commander->SummonCreature(MOB_DEATHBRINGER, x, y, z, Commander->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0)) {
                //Bringer2->MonsterSay("[DEBUG] Second bringer!", LANG_UNIVERSAL, NULL);
                Bringer2->setActive(true);
                Bringer2->SetSpeed(MOVE_WALK, 3.5);
                Bringer2->GetMotionMaster()->MovePath(25851, false);
            }
                
            BringersTimer = 80000;
        }
        else
            BringersTimer -= diff;
    
        if (FiendTimer <= diff) {
            float x, y, z;
            
            Commander->GetRandomPoint(Commander->GetPositionX(), Commander->GetPositionY(), Commander->GetPositionZ(), 1.0f, x, y, z);
            if (Creature *Fiend = Commander->SummonCreature(CREATURE_VOLATILE_FELFIRE_FIEND, x, y, z, Commander->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0)) {
                //Fiend->MonsterSay("[DEBUG] Fiend!", LANG_UNIVERSAL, NULL);
                Fiend->SetSummoner(Commander);
                Fiend->setActive(true);
                Fiend->SetSpeed(MOVE_WALK, 5);
            }
                
            FiendTimer = 45000;
        }
        else
            FiendTimer -= diff;
    }

    const char* Save()
    {
        OUT_SAVE_INST_DATA;
        std::ostringstream stream;
        stream << Encounters[0] << " "  << Encounters[1] << " "  << Encounters[2] << " "  << Encounters[3] << " "
            << Encounters[4] << " "  << Encounters[5];
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
            >> Encounters[4] >> Encounters[5];
        for(uint8 i = 0; i < ENCOUNTERS; ++i)
            if(Encounters[i] == IN_PROGRESS)                // Do not load an encounter as "In Progress" - reset it instead.
                Encounters[i] = NOT_STARTED;
        OUT_LOAD_INST_DATA_COMPLETE;
    }
};

InstanceData* GetInstanceData_instance_sunwell_plateau(Map* map)
{
    return new instance_sunwell_plateau(map);
}

void AddSC_instance_sunwell_plateau()
{
    Script *newscript;
    newscript = new Script;
    newscript->Name = "instance_sunwell_plateau";
    newscript->GetInstanceData = &GetInstanceData_instance_sunwell_plateau;
    newscript->RegisterSelf();
}

