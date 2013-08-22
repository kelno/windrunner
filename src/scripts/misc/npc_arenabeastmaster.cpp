#include "precompiled.h"
#include <string>
#include "MapManager.h"
#include "ObjectMgr.h"

enum PetTypes {
    WINDSERPENT,
    SCORPID,
    SPIDER,
    RAVAGER,
    BOAR,
    RAPTOR,
    DRAGONHAWK
};

std::string getPetTypeName(PetTypes type) 
{
    switch(type)
    {
    case WINDSERPENT:
        return "Serpent des vents";
        break;
    case SCORPID:
        return "Scorpide";
        break;
    case SPIDER:
        return "Araignée";
        break;
    case RAVAGER:
        return "Ravageur";
        break;
    case BOAR:
        return "Sanglier";
        break;
    case RAPTOR:
        return "Raptor";
        break;
    case DRAGONHAWK:
       return "Faucon-dragon";
        break;
    default:
        break;
    }
    return "";
}

uint32 getPetTypeEntry(uint32 type) 
{
    switch(type)
    {
    case WINDSERPENT:
        return 20749;
    case SCORPID:
        return 21864;
    case SPIDER:
        return 930;
    case RAVAGER:
        return 16933;
    case BOAR:
        return 1126;
    case RAPTOR:
        return 20634;
    case DRAGONHAWK:
        return 20038;
    default:
        return 0;
    }
}

bool GossipHello_arenabeastmaster(Player *player, Creature *me)
{    
    if(player->getClass() != CLASS_HUNTER)
    {
        me->Whisper("Je n'offre mes services qu'aux chasseurs !",player->GetGUID());
        return true;
    }

    player->ADD_GOSSIP_ITEM( 0, getPetTypeName(WINDSERPENT), GOSSIP_SENDER_MAIN, WINDSERPENT);
    player->ADD_GOSSIP_ITEM( 0, getPetTypeName(SCORPID), GOSSIP_SENDER_MAIN, SCORPID);
    player->ADD_GOSSIP_ITEM( 0, getPetTypeName(SPIDER), GOSSIP_SENDER_MAIN, SPIDER);
    player->ADD_GOSSIP_ITEM( 0, getPetTypeName(RAVAGER), GOSSIP_SENDER_MAIN, RAVAGER);
    player->ADD_GOSSIP_ITEM( 0, getPetTypeName(BOAR), GOSSIP_SENDER_MAIN, BOAR);
    player->ADD_GOSSIP_ITEM( 0, getPetTypeName(RAPTOR), GOSSIP_SENDER_MAIN, RAPTOR);
    player->ADD_GOSSIP_ITEM( 0, getPetTypeName(DRAGONHAWK), GOSSIP_SENDER_MAIN, DRAGONHAWK);
        
	player->PlayerTalkClass->SendGossipMenu(1,me->GetGUID());

    return true;
}

//code from ChatHandler::HandleCreatePetCommand(const char* args)
bool GossipSelect_arenabeastmaster( Player* player, Creature* me, uint32 /* sender */, uint32 type)
{
    
    if(player->GetPet() || player->GetTemporaryUnsummonedPetNumber())
    {
        me->Whisper("Abandonnez d'abord votre familier.",player->GetGUID());
        player->PlayerTalkClass->CloseGossip();
        return true;
    }
    Pet* pet = new Pet(HUNTER_PET);
    if(!pet)
        return true;

    if(!pet->CreateBaseAtCreatureEntry(getPetTypeEntry(type), me))
    {
        delete pet;
        return false;
    }

    pet->SetUInt32Value(UNIT_FIELD_FACTIONTEMPLATE, player->getFaction());
    pet->SetUInt64Value(UNIT_FIELD_SUMMONEDBY, player->GetGUID());
    pet->SetUInt64Value(UNIT_FIELD_CREATEDBY, player->GetGUID());

    if(!pet->InitStatsForLevel(player->getLevel()))
    {
        delete pet;
        return true;
    }

    pet->SetUInt32Value(UNIT_FIELD_LEVEL,player->getLevel()-1);
    pet->GetCharmInfo()->SetPetNumber(objmgr.GeneratePetNumber(), true);
    pet->AIM_Initialize();
    //pet->InitPetCreateSpells();
    pet->SetHealth(pet->GetMaxHealth());

    MapManager::Instance().GetMap(pet->GetMapId(), pet)->Add(pet->ToCreature());

    // visual effect for levelup
    pet->SetUInt32Value(UNIT_FIELD_LEVEL,player->getLevel());

    player->SetPet(pet);
    pet->SavePetToDB(PET_SAVE_AS_CURRENT);
    player->PetSpellInitialize();
     
    pet->SetLoyaltyLevel(BEST_FRIEND);
    pet->SetPower(POWER_HAPPINESS,1050000); //maxed
    pet->SetTP(pet->getLevel()*(pet->GetLoyaltyLevel()-1)); //350 when best friend at lvl 70

    player->PlayerTalkClass->CloseGossip();
		
    return true;
}

void AddSC_arenabeastmaster()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="npc_arenabeastmaster";
    newscript->pGossipHello = &GossipHello_arenabeastmaster;
    newscript->pGossipSelect = &GossipSelect_arenabeastmaster;
    newscript->RegisterSelf();
}
