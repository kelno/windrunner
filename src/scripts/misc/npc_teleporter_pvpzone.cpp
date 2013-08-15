#include "precompiled.h"

#define GOSSIP_MENU 907 //change me later?
#define PVPZONE_ARRIVAL_ALLIANCE 1, 4809.387207, -2012.331177, 1068.865967, 1.220856 
#define PVPZONE_ARRIVAL_HORDE 1, 4803.802734, -1924.144043, 1069.050171, 5.099141

bool GossipHello_npc_teleporter_pvpzone(Player *pPlayer, Creature *pCreature)
{
    pPlayer->ADD_GOSSIP_ITEM(0, "Téléportez-moi dans la zone PvP.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
        
    pPlayer->PlayerTalkClass->SendGossipMenu(GOSSIP_MENU,pCreature->GetGUID());

    return true;
}

bool GossipSelect_npc_teleporter_pvpzone(Player *pPlayer, Creature *pCreature, uint32 sender, uint32 action)
{
    if (action == GOSSIP_ACTION_INFO_DEF)
    {
	
	    if(pPlayer->GetTeam() == HORDE)
			pPlayer->TeleportTo(PVPZONE_ARRIVAL_HORDE);
		else
			pPlayer->TeleportTo(PVPZONE_ARRIVAL_ALLIANCE);
			
		pPlayer->PlayerTalkClass->CloseGossip();
		return true;
	}
	return false;
}

void AddSC_npc_teleporter_pvpzone()
{
    Script *newscript;
    
    newscript = new Script;
    newscript->Name = "npc_teleporter_pvpzone";
    newscript->pGossipHello = &GossipHello_npc_teleporter_pvpzone;
    newscript->pGossipSelect = &GossipSelect_npc_teleporter_pvpzone;
    newscript->RegisterSelf();
}