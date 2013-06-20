/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/* ScriptData
Name: Arena Spectator
%Complete: 100
Comment: Script allow spectate arena games
Category: Custom Script
EndScriptData */

#include "precompiled.h"
#include "Language.h"
#include "Chat.h"
#include "ObjectMgr.h"
#include "BattleGroundMgr.h"
#include "ArenaTeam.h"
#include "World.h"

enum NpcSpectatorActions
{
	NPC_SPECTATOR_ACTION_2                   = 2000,
	NPC_SPECTATOR_ACTION_3                   = 3000,
	NPC_SPECTATOR_ACTION_5                   = 4000,
    NPC_SPECTATOR_ACTION_SELECTED_PLAYER     = 5000
};

const uint8  GamesOnPage    = 15;

std::string GetClassNameById(uint8 id)
{
    std::string sClass = "";
    switch (id)
    {
        case CLASS_WARRIOR:         sClass = "War";       break;
        case CLASS_PALADIN:         sClass = "Pal";       break;
        case CLASS_HUNTER:          sClass = "Hunt";      break;
        case CLASS_ROGUE:           sClass = "Rogue";     break;
        case CLASS_PRIEST:          sClass = "Priest";    break;
        case CLASS_SHAMAN:          sClass = "Cham";      break;
        case CLASS_MAGE:            sClass = "Mage";      break;
        case CLASS_WARLOCK:         sClass = "Demo";      break;
        case CLASS_DRUID:           sClass = "Druide";    break;
    }
    return sClass;
}

std::string GetGamesStringData(BattleGround *arena)
{
	std::string data = "";

    for (uint8 i = 0; i < arena->GetArenaType(); i++)
    {
	    ArenaTeam *team =  objmgr.GetArenaTeamById(arena->GetArenaTeamIdForIndex(i));

	    std::list<ArenaTeamMember>::iterator begin = team->membersBegin();
	    std::list<ArenaTeamMember>::iterator end = team->membersEnd();

	    for (std::list<ArenaTeamMember>::iterator itr = begin; itr != end; itr++)
	    {
	        data += GetClassNameById(itr->Class);
	        if (itr != end)
	            data += "/";
	    }

	    if (arena->isRated())
	    {
	    	std::stringstream ss;
	    	ss << team->GetRating();
	    	data += "(" + ss.str() + ")";
	    }
	    else
	        data += "(0)";

	    data += " - ";
	}

    return data;
}

uint64 GetFirstPlayerGuid(BattleGround *arena)
{
    for (BattleGround::BattleGroundPlayerMap::const_iterator itr = arena->GetPlayers().begin(); itr != arena->GetPlayers().end(); ++itr)
        if (Player* player = ObjectAccessor::FindPlayer(itr->first))
            return itr->first;
    return 0;
}

void ShowPage(Player *player, uint32 page, ArenaType type)
{
	uint16 nbArenas  = 0;
	bool haveNextPage = false;

    for (uint8 i = BATTLEGROUND_AV; i <= BATTLEGROUND_RL; ++i)
    {
        if (!sBattleGroundMgr.IsArenaType((BattleGroundTypeId)i))
            continue;

        BattleGroundSet bgs = sBattleGroundMgr.GetBattleGroundByType((BattleGroundTypeId)i);
        for (BattleGroundSet::iterator itr = bgs.begin(); itr != bgs.end(); ++itr)
        {
            BattleGround* arena = itr->second;

            if (!arena->GetPlayersSize())
               continue;

            if (arena->GetStatus() != STATUS_IN_PROGRESS)
                continue;

            if (type == arena->GetArenaType())
            {
            	nbArenas++;
            	if (nbArenas > (page + 1) * GamesOnPage)
            	{
            	    haveNextPage = true;
            	    break;
            	}

            	if (nbArenas > page * GamesOnPage)
            	    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, GetGamesStringData(arena), GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_SELECTED_PLAYER + GetFirstPlayerGuid(arena));
            }
        }
    }

    switch (type)
    {
        case ARENA_TYPE_2v2:
        	if (page > 0)
        	    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_DOT, "Précédent...", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_2 + page - 1);

        	if (haveNextPage)
        	    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_DOT, "Suivant...", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_2 + page + 1);
        	break;
        case ARENA_TYPE_3v3:
        	if (page > 0)
        	    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_DOT, "Précédent...", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_3 + page - 1);

        	if (haveNextPage)
        	    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_DOT, "Suivant...", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_3 + page + 1);
        	break;
        case ARENA_TYPE_5v5:
        	if (page > 0)
        	    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_DOT, "Précédent...", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_5 + page - 1);

        	if (haveNextPage)
        	    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_DOT, "Suivant...", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_5 + page + 1);
            break;
    }

    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Retour", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
}

void spectate(Player* player, uint64 targetGuid, Creature *mobArena)
{
	if (Player* target = ObjectAccessor::FindPlayer(targetGuid))
	{
	    if (target == player || targetGuid == player->GetGUID())
	        return;

	    if (player->isInCombat())
	    {
	    	mobArena->Whisper("Vous êtes en combat!", player->GetGUID());
	    	return;
	    }

	    if (!target)
	    {
	    	mobArena->Whisper("Le joueur n'existe pas ou n'est pas en ligne", player->GetGUID());
	    	return;
	    }

	    if (player->GetPet())
	    {
	    	mobArena->Whisper("Vous devez d'abord renvoyer votre familier.", player->GetGUID());
	    	return;
	    }

	    if (player->GetMap()->IsBattleGroundOrArena() && !player->isSpectator())
	    {
	    	mobArena->Whisper("Vous ne pouvez pas faire cela car vous êtes déjà dans un champ de bataille ou une arène.", player->GetGUID());
	    	return;
	    }

	    Map* cMap = target->GetMap();
	    if (!cMap->IsBattleArena())
	    {
	    	mobArena->Whisper("Ce joueur n'est pas dans une arène.", player->GetGUID());
	    	return;
	    }

	    if (player->GetMap()->IsBattleGround())
	    {
	    	mobArena->Whisper("Vous ne pouvez pas faire cela car vous êtes déjà dans un champ de bataille.", player->GetGUID());
	    	return;
	    }

	    if (BattleGround* bg = target->GetBattleGround())
	    {
	    	if (bg->GetStatus() != STATUS_IN_PROGRESS)
	    	{
	    		mobArena->Whisper("Vous ne pouvez pas faire cela car l'arène n'a pas encore commencé.", player->GetGUID());
	    	    return;
	    	}

	    	if (!bg->canEnterSpectator(player))
	    	{
	    		mobArena->Whisper("Il n'y a plus de places pour ce match.", player->GetGUID());
	    		return;
	    	}
	    }

	    if (player->getSpectateCooldown() > 0)
	    {
	    	mobArena->Whisper("Un cooldown de 10 secondes est appliqué entre chaque mode spectateur.", player->GetGUID());
	    	return;
	    }

	    // all's well, set bg id
	    // when porting out from the bg, it will be reset to 0
	    player->SetBattleGroundId(target->GetBattleGroundId());
	    // remember current position as entry point for return at bg end teleportation
	    if (!player->GetMap()->IsBattleGroundOrArena())
	        player->SetBattleGroundEntryPoint(player->GetMapId(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation());

	    if (target->isSpectator())
	    {
	    	mobArena->Whisper("Vous ne pouvez pas faire cela car le joueur ciblé est aussi spectateur.", player->GetGUID());
	    	return;
	    }

	    // stop flight if need
	    if (player->isInFlight())
	    {
	    	player->GetMotionMaster()->MovementExpired();
	    	player->CleanupAfterTaxiFlight();
	    }
	    // save only in non-flight case
	    else
	    	player->SaveRecallPosition();

	    // search for two teams
	    BattleGround *bGround = target->GetBattleGround();
        if (!bGround)
            return;
        
	    if (bGround->isRated())
	    {
	        uint32 slot = bGround->GetArenaType() - 2;
	    	if (bGround->GetArenaType() > 3)
	    	    slot = 2;
	    	uint32 firstTeamID = target->GetArenaTeamId(slot);
	    	uint32 secondTeamID = 0;
	    	Player *firstTeamMember  = target;
	    	Player *secondTeamMember = NULL;
	    	for (BattleGround::BattleGroundPlayerMap::const_iterator itr = bGround->GetPlayers().begin(); itr != bGround->GetPlayers().end(); ++itr)
	    	    if (Player* tmpPlayer = ObjectAccessor::FindPlayer(itr->first))
	    	    {
	    	        if (tmpPlayer->isSpectator())
	    	            continue;

	    	        uint32 tmpID = tmpPlayer->GetArenaTeamId(slot);
	    	        if (tmpID != firstTeamID && tmpID > 0)
	    	        {
	    	            secondTeamID = tmpID;
	    	            secondTeamMember = tmpPlayer;
	    	            break;
	    	        }
	    	    }

	    	if (firstTeamID > 0 && secondTeamID > 0 && secondTeamMember)
	    	{
	    	    ArenaTeam *firstTeam  = objmgr.GetArenaTeamById(firstTeamID);
	    	    ArenaTeam *secondTeam = objmgr.GetArenaTeamById(secondTeamID);
	    	    if (firstTeam && secondTeam)
	    	    {
                    ChatHandler chH = ChatHandler(player);
	    	        chH.PSendSysMessage("Vous entrez dans une arène cotée.");
	    	        chH.PSendSysMessage("Equipes :");
	    	        chH.PSendSysMessage("%s - %s", firstTeam->GetName().c_str(), secondTeam->GetName().c_str());
	    	        chH.PSendSysMessage("%u - %u", firstTeam->GetRating(), secondTeam->GetRating());
	    	    }
	    	}
	    }

	    // to point to see at target with same orientation
	    float x, y, z;
	    target->GetContactPoint(player, x, y, z);

	    target->GetBattleGround()->AddSpectator(player->GetGUID());
	    player->TeleportTo(target->GetMapId(), x, y, z, player->GetAngle(target), TELE_TO_GM_MODE);
	    player->SetSpectate(true);
    }
}

void ShowDefaultPage(Player* player, Creature* creature)
{
	if(sWorld.getConfig(CONFIG_ARENA_SPECTATOR_ENABLE))
    {
		player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Arènes 2V2", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_2);
		player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Arènes 3V3", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_3);
		player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Arènes 5V5", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_5);
		player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, "Voir un joueur...", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2, "", 0, true);
		player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }
	else
	{
		player->CLOSE_GOSSIP_MENU();
		creature->Whisper("Arena spectator désactivé", player->GetGUID());
	}
}

bool GossipHello_npc_spectate(Player* pPlayer, Creature* pCreature)
{
	if(sWorld.getConfig(CONFIG_ARENA_SPECTATOR_ENABLE))
	{
		pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Arènes 2V2", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_2);
		pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Arènes 3V3", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_3);
		pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Arènes 5V5", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_5);
        pPlayer->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, "Voir un joueur...", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2, "", 0, true);
	}
	else
	{
		pCreature->Whisper("Arena spectator désactivé", pPlayer->GetGUID());
		return true;
	}

    pPlayer->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, pCreature->GetGUID());
    return true;
}

bool GossipSelect_npc_spectate(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
{
	if (action == GOSSIP_ACTION_INFO_DEF + 1)
		ShowDefaultPage(player, creature);
	else if (action >= NPC_SPECTATOR_ACTION_2 && action < NPC_SPECTATOR_ACTION_3)
	{
		ShowPage(player, action - NPC_SPECTATOR_ACTION_2, ARENA_TYPE_2v2);
		player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
	}
	else if (action >= NPC_SPECTATOR_ACTION_3 && action < NPC_SPECTATOR_ACTION_5)
	{
		ShowPage(player, action - NPC_SPECTATOR_ACTION_3, ARENA_TYPE_3v3);
		player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
	}
	else if (action >= NPC_SPECTATOR_ACTION_5 && action < NPC_SPECTATOR_ACTION_SELECTED_PLAYER)
	{
		ShowPage(player, action - NPC_SPECTATOR_ACTION_5, ARENA_TYPE_5v5);
		player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
	}
	else if (action >= NPC_SPECTATOR_ACTION_SELECTED_PLAYER)
	{
    	player->CLOSE_GOSSIP_MENU();
    	uint64 targetGuid = action - NPC_SPECTATOR_ACTION_SELECTED_PLAYER;
    	spectate(player, targetGuid, creature);
    }

    return true;
}

bool GossipSelectWithCode_npc_spectate( Player *player, Creature *_Creature, uint32 sender, uint32 action, const char* sCode )
{
    if(sender == GOSSIP_SENDER_MAIN)
    {
        if(action == GOSSIP_ACTION_INFO_DEF + 2)
        {
        	std::string name = sCode;
        	if(!name.empty())
        	{
        	    if(!normalizePlayerName(name))
        	    {
        	    	_Creature->Whisper("Nom incorrect!", player->GetGUID());
        	    	player->CLOSE_GOSSIP_MENU();
        	    	return true;
        	    }

        	    Player* target = objmgr.GetPlayer(name.c_str());
        	    if (!target)
        	    {
        	    	_Creature->Whisper("Impossible de trouver le joueur!", player->GetGUID());
        	    	player->CLOSE_GOSSIP_MENU();
        	    	return true;
        	    }

        	    spectate(player, target->GetGUID(), _Creature);
        	}
        	else
        		_Creature->Whisper("Champ vide!", player->GetGUID());

        	player->CLOSE_GOSSIP_MENU();
        	return true;
        }
    }
    return false;
}

void AddSC_arena_spectator_script()
{
	Script* newscript;

	newscript = new Script;
	newscript->Name = "npc_spectate";
	newscript->pGossipHello = &GossipHello_npc_spectate;
	newscript->pGossipSelect = &GossipSelect_npc_spectate;
	newscript->pGossipSelectWithCode = &GossipSelectWithCode_npc_spectate;
	newscript->RegisterSelf();
}
