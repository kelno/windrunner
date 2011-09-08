/*
 * Copyright (C) 2005-2008 MaNGOS <http://www.mangosproject.org/>
 *
 * Copyright (C) 2008 Trinity <http://www.trinitycore.org/>
 *
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
SDName: Razorfen Downs
SD%Complete: 100
SDComment: /
SDCategory: Razorfen Downs
EndScriptData */

#include "precompiled.h"

bool GossipHello_npc_henry_stern(Player *pPlayer, Creature *pCreature)
{
    if (pPlayer->HasSkill(SKILL_COOKING) && pPlayer->GetBaseSkillValue(SKILL_COOKING) >= 175)
        pPlayer->ADD_GOSSIP_ITEM(0, "Je voudrais apprendre à confectionner du Thé de dorépine.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
    if (pPlayer->HasSkill(SKILL_ALCHEMY) && pPlayer->GetBaseSkillValue(SKILL_ALCHEMY) >= 175)
        pPlayer->ADD_GOSSIP_ITEM(0, "Je voudrais apprendre à faire de la potion de sang de troll hargneux.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
    
    pPlayer->SEND_GOSSIP_MENU(3377, pCreature->GetGUID());
    
    return true;
}

bool GossipSelect_npc_henry_stern(Player *pPlayer, Creature *pCreature, uint32 sender, uint32 action)
{
    if (action == GOSSIP_ACTION_INFO_DEF+1)
        pPlayer->learnSpell(13028);
    else if (action == GOSSIP_ACTION_INFO_DEF+2)
        pPlayer->learnSpell(3451);
        
    pPlayer->CLOSE_GOSSIP_MENU();
    
    return true;
}

void AddSC_razorfen_downs()
{
    Script *newscript;
    
    newscript = new Script;
    newscript->Name = "npc_henry_stern";
    newscript->pGossipHello = &GossipHello_npc_henry_stern;
    newscript->pGossipSelect = &GossipSelect_npc_henry_stern;
    newscript->RegisterSelf();
}
