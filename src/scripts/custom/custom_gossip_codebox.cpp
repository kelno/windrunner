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
SDName: Custom_Gossip_Codebox
SD%Complete: 100
SDComment: Show a codebox in gossip option
SDCategory: Script Examples
EndScriptData */

#include "precompiled.h"
#include <cstring>

//This function is called when the player opens the gossip menubool
bool GossipHello_custom_gossip_codebox(Player *player, Creature *_Creature)
{
    player->ADD_GOSSIP_ITEM_EXTENDED(0, "A quiz: what's your name?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1, "", 0, true);
    player->ADD_GOSSIP_ITEM(0, "I'm not interested", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);

    player->PlayerTalkClass->SendGossipMenu(907,_Creature->GetGUID());
    return true;
}

//This function is called when the player clicks an option on the gossip menubool
bool GossipSelect_custom_gossip_codebox(Player *player, Creature *_Creature, uint32 sender, uint32 action )
{
    if(action == GOSSIP_ACTION_INFO_DEF+2)
    {
        _Creature->Say("Normal select, guess you're not interested.", LANG_UNIVERSAL, 0);
        player->CLOSE_GOSSIP_MENU();
    }
    return true;
}

bool GossipSelectWithCode_custom_gossip_codebox( Player *player, Creature *_Creature, uint32 sender, uint32 action, const char* sCode )
{
    if(sender == GOSSIP_SENDER_MAIN)
    {
        if(action == GOSSIP_ACTION_INFO_DEF+1)
        {
            if(std::strcmp(sCode, player->GetName())!=0)
            {
                _Creature->Say("Wrong!", LANG_UNIVERSAL, 0);
                _Creature->CastSpell(player, 12826, true);
            }
            else
            {
                _Creature->Say("You're right, you are allowed to see my inner secrets.", LANG_UNIVERSAL, 0);
                _Creature->CastSpell(player, 26990, true);
            }
            player->CLOSE_GOSSIP_MENU();
            return true;
        }
    }
    return false;
}

void AddSC_custom_gossip_codebox()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="custom_gossip_codebox";
    newscript->pGossipHello =           &GossipHello_custom_gossip_codebox;
    newscript->pGossipSelect =          &GossipSelect_custom_gossip_codebox;
    newscript->pGossipSelectWithCode =  &GossipSelectWithCode_custom_gossip_codebox;
    newscript->RegisterSelf();
}

