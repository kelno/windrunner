#include "precompiled.h"

#define SPELL_ILLUSION_GNOME_MALE 37808
#define SPELL_ILLUSION_GNOME_FEMALE 37809

/*######
## custom_gnominizer
######*/

bool GossipHello_custom_gnominizer(Player *pPlayer, Creature *pCreature)
{
    pPlayer->ADD_GOSSIP_ITEM(0, "Se transformer en gnome mâle.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
    pPlayer->ADD_GOSSIP_ITEM(0, "Se transformer en gnome femelle.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

    pPlayer->PlayerTalkClass->SendGossipMenu(907,pCreature->GetGUID());

    return true;
}

bool GossipSelect_custom_gnominizer(Player *pPlayer, Creature *pCreature, uint32 sender, uint32 action)
{
    switch(action)
    {
    case GOSSIP_ACTION_INFO_DEF:
		pPlayer->AddAura(SPELL_ILLUSION_GNOME_MALE,pPlayer);
        break;
    case GOSSIP_ACTION_INFO_DEF + 1:
        pPlayer->AddAura(SPELL_ILLUSION_GNOME_FEMALE,pPlayer);
        break;
    }
    
    pPlayer->PlayerTalkClass->CloseGossip();
    return true;
}

void AddSC_custom_gnominizer()
{
    Script *newscript;
    
    newscript = new Script;
    newscript->Name = "custom_gnominizer";
    newscript->pGossipHello = &GossipHello_custom_gnominizer;
    newscript->pGossipSelect = &GossipSelect_custom_gnominizer;
    newscript->RegisterSelf();
}