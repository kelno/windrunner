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
	uint64 KilJaeden;
	uint64 KilJaedenController;
	uint64 Anveena;
	uint64 KalecgosKJ;
    uint64 FlightLeft;
    uint64 FlightRight;
    uint64 CommanderGUID;
	uint32 SpectralPlayers;

	/** GameObjects **/
	uint64 ForceField;                                      // Kalecgos Encounter
	uint64 KalecgosWall[2];
	uint64 FireBarrier;                                     // Felmysts Encounter
	uint64 MurusGate[2];                                    // Murus Encounter

	/*** Misc ***/
	uint32 SpectralRealmTimer;
	std::vector<uint64> SpectralRealmList;
    uint32 GauntletStatus;
    uint32 BringersTimer;
    uint32 FiendTimer;

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
        KilJaeden               = 0;
        KilJaedenController     = 0;
        Anveena                 = 0;
        KalecgosKJ              = 0;
        CommanderGUID           = 0;
        SpectralPlayers         = 0;

        /*** GameObjects ***/
        ForceField  = 0;
        FireBarrier = 0;
        MurusGate[0] = 0;
        MurusGate[1] = 0;
        KalecgosWall[0] = 0;
        KalecgosWall[1] = 0;

        /*** Misc ***/
        SpectralRealmTimer      = 5000;
        BringersTimer           = 0;
        FiendTimer              = 0;

        /*** Encounters ***/
        for(uint8 i = 0; i < ENCOUNTERS; ++i)
            Encounters[i] = NOT_STARTED;
            
        GauntletStatus = NOT_STARTED;
    }

    bool IsEncounterInProgress() const
    {
        for(uint8 i = 0; i < ENCOUNTERS; ++i)
            if(Encounters[i] == IN_PROGRESS)
                return true;

        return false;
    }

    Player* GetPlayerInMap()
    {
        Map::PlayerList const& players = instance->GetPlayers();

        if (!players.isEmpty())
        {
            for(Map::PlayerList::const_iterator itr = players.begin(); itr != players.end(); ++itr)
            {
                Player* plr = itr->getSource();
                if (plr && !plr->HasAura(45839))
                        return plr;
            }
        }

        debug_log("TSCR: Instance Sunwell Plateau: GetPlayerInMap, but PlayerList is empty!");
        return NULL;
    }

    /*void HandleGameObject(uint64 guid, uint32 state)
    {
        Player *player = GetPlayerInMap();

        if (!player || !guid)
        {
            debug_log("TSCR: Sunwell Plateau: HandleGameObject fail");
            return;
        }

        if (GameObject *go = GameObject::GetGameObject(*player,guid))
            go->SetGoState(state);
    }*/

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
            case 25315: KilJaeden           = pCreature->GetGUID(); break;
            case 25608: KilJaedenController = pCreature->GetGUID(); break;
            case 26046: Anveena             = pCreature->GetGUID(); break;
            case 25319: KalecgosKJ          = pCreature->GetGUID(); break;
            case 25357: FlightLeft          = pCreature->GetGUID(); pCreature->setActive(true); break;
            case 25358: FlightRight         = pCreature->GetGUID(); pCreature->setActive(true); break;
            case 25837: CommanderGUID       = pCreature->GetGUID(); pCreature->setActive(true); break;
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
            case 187766:    // The first gate FIXME: Always closed for now, will change later
                HandleGameObject(NULL, false, pGo);
                break;
            case 187764:    // The second gate FIXME: Always closed for now, will change later
                HandleGameObject(NULL, false, pGo);
                break;
            case 187765:    // The third gate FIXME: Always closed for now, will change later
                HandleGameObject(NULL, false, pGo);
                break;
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
            case DATA_GO_KALEC_WALL_1:		return KalecgosWall[0];
            case DATA_GO_KALEC_WALL_2:		return KalecgosWall[1];
            case DATA_BRUTALLUS:            return Brutallus;
            case DATA_MADRIGOSA:            return Madrigosa;
            case DATA_FELMYST:              return Felmyst;
            case DATA_COMMANDER:            return CommanderGUID;
            case DATA_ALYTHESS:             return Alythess;
            case DATA_SACROLASH:            return Sacrolash;
            case DATA_MURU:                 return Muru;
            case DATA_KILJAEDEN:            return KilJaeden;
            case DATA_KILJAEDEN_CONTROLLER: return KilJaedenController;
            case DATA_ANVEENA:              return Anveena;
            case DATA_KALECGOS_KJ:          return KalecgosKJ;
            case MOB_FLIGHT_LEFT:           return FlightLeft;
            case MOB_FLIGHT_RIGHT:          return FlightRight;
            case DATA_PLAYER_GUID:
                Player* Target = GetPlayerInMap();
                return Target->GetGUID();
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
                    HandleGameObject(FireBarrier, true);*/ // FIXME: Re-add this when opening sunwell part 2
                Encounters[2] = data; break;
            case DATA_GAUNTLET_EVENT:
                GauntletStatus = data;
                break;
            case DATA_EREDAR_TWINS_EVENT:  Encounters[3] = data; break;
            case DATA_MURU_EVENT:
                switch(data)
                {
                    case DONE:
                        HandleGameObject(MurusGate[0], true);
                        HandleGameObject(MurusGate[1], true);
                        break;
                    case IN_PROGRESS:
                        HandleGameObject(MurusGate[0], false);
                        HandleGameObject(MurusGate[1], false);
                        break;
                    case NOT_STARTED:
                        HandleGameObject(MurusGate[0], true);
                        HandleGameObject(MurusGate[1], false);
                        break;
                }
                Encounters[4] = data; break;
            case DATA_KILJAEDEN_EVENT:     Encounters[5] = data; break;
        }

        if (data == DONE)
            SaveToDB();
    }

    void Update(uint32 const diff)
    {

        Unit* Commander = instance->GetCreatureInMap(CommanderGUID);
        if (!Commander || Commander->isInCombat() || Commander->isDead())
            GauntletStatus = NOT_STARTED;

        if (GauntletStatus != IN_PROGRESS) {
            BringersTimer = 0;
            FiendTimer = 0;
            
            return;
        }

        if (BringersTimer <= diff) {
            float x, y, z;
            
            Commander->GetNearPoint(Commander, x, y, z, 1.0f, 1.0f, 0);
            if (Creature *Bringer1 = Commander->SummonCreature(MOB_SOULBINDER, x, y, z, Commander->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0))
                Bringer1->MonsterSay("[DEBUG] First bringer!", LANG_UNIVERSAL, NULL);
            
            Commander->GetNearPoint(Commander, x, y, z, 1.0f, 1.0f, 0);
            if (Creature *Bringer2 = Commander->SummonCreature(MOB_DEATHBRINGER, x, y, z, Commander->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0))
                Bringer2->MonsterSay("[DEBUG] Second bringer!", LANG_UNIVERSAL, NULL);
                
            BringersTimer = 40000;
        }
        else
            BringersTimer -= diff;
    
        if (FiendTimer <= diff) {
            float x, y, z;
            
            Commander->GetNearPoint(Commander, x, y, z, 1.0f, 1.0f, 0);
            if (Creature *Fiend = Commander->SummonCreature(MOB_VOLATILE_FIEND, x, y, z, Commander->GetOrientation(), TEMPSUMMON_MANUAL_DESPAWN, 0))
                Fiend->MonsterSay("[DEBUG] Fiend!", LANG_UNIVERSAL, NULL);
                
            FiendTimer = 12000;
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

