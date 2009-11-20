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
SDName: Feralas
SD%Complete: 100
SDComment: Quest support: 3520. Special vendor Gregan Brewspewer
SDCategory: Feralas
EndScriptData */

#include "precompiled.h"

/*######
## npc_gregan_brewspewer
######*/

#define GOSSIP_HELLO "Buy somethin', will ya?"

bool GossipHello_npc_gregan_brewspewer(Player* pPlayer, Creature* pCreature)
{
    if (pCreature->isQuestGiver())
        pPlayer->PrepareQuestMenu(pCreature->GetGUID());

    if (pCreature->isVendor() && pPlayer->GetQuestStatus(3909) == QUEST_STATUS_INCOMPLETE)
        pPlayer->ADD_GOSSIP_ITEM(0, GOSSIP_HELLO, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    pPlayer->SEND_GOSSIP_MENU(2433, pCreature->GetGUID());
    
    return true;
}

bool GossipSelect_npc_gregan_brewspewer(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action )
{
    if (action == GOSSIP_ACTION_INFO_DEF+1)
    {
        pPlayer->ADD_GOSSIP_ITEM(1, GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);
        pPlayer->SEND_GOSSIP_MENU(2434, pCreature->GetGUID());
    }
    if (action == GOSSIP_ACTION_TRADE)
        pPlayer->SEND_VENDORLIST(pCreature->GetGUID());
        
    return true;
}

/*######
## npc_screecher_spirit
######*/

bool GossipHello_npc_screecher_spirit(Player* pPlayer, Creature* pCreature)
{
    pPlayer->SEND_GOSSIP_MENU(2039, pCreature->GetGUID() );
    pPlayer->TalkedToCreature(pCreature->GetEntry(), pCreature->GetGUID());
    pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

    return true;
}

/*######
## AddSC
######*/

void AddSC_feralas()
{
    Script* newscript;

    newscript = new Script;
    newscript->Name="npc_gregan_brewspewer";
    newscript->pGossipHello = &GossipHello_npc_gregan_brewspewer;
    newscript->pGossipSelect = &GossipSelect_npc_gregan_brewspewer;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_screecher_spirit";
    newscript->pGossipHello = &GossipHello_npc_screecher_spirit;
    newscript->RegisterSelf();
}

