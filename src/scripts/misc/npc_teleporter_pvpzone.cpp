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
    bool set = false;
    float x,y,z,o;
    uint32 map = 0;
    QueryResult* query = WorldDatabase.PQuery("SELECT position_x, position_y, position_z, orientation, map FROM game_tele WHERE name = 'duelzone'");
    if (query) {
        Field* fields = query->Fetch();
        if(fields)
        {
            x = fields[0].GetFloat();
            y = fields[1].GetFloat();
            z = fields[2].GetFloat();
            o = fields[3].GetFloat();
            map = fields[4].GetUInt32();
        }
    }
    
    if (action == GOSSIP_ACTION_INFO_DEF)
    {
        if(!map) //return to default values
        {
            if(pPlayer->GetTeam() == HORDE)
                pPlayer->TeleportTo(PVPZONE_ARRIVAL_HORDE);
            else
                pPlayer->TeleportTo(PVPZONE_ARRIVAL_ALLIANCE);
        } else {
            pPlayer->TeleportTo(map,x,y,z,o);
        }
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