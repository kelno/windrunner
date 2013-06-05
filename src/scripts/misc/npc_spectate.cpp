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
	NPC_SPECTATOR_ACTION_HIGH_RATING         = 2000,
	NPC_SPECTATOR_ACTION_LOW_RATING          = 3000,
	NPC_SPECTATOR_ACTION_NO_RATED            = 4000,
    NPC_SPECTATOR_ACTION_SELECTED_PLAYER     = 5000
};

enum ArenaRate
{
	NO_RATED       = 0,
	LOW_RATING     = 1,
	HIGH_RATING    = 2
};

const uint16 TopGamesRating = 1800;
const uint8  GamesOnPage    = 15;

std::string GetClassNameById(uint8 id)
{
    std::string sClass = "";
    switch (id)
    {
        case CLASS_WARRIOR:         sClass = "Guerrier ";       break;
        case CLASS_PALADIN:         sClass = "Paladin ";        break;
        case CLASS_HUNTER:          sClass = "Chasseur ";       break;
        case CLASS_ROGUE:           sClass = "Voleur ";         break;
        case CLASS_PRIEST:          sClass = "Prêtre ";         break;
        case CLASS_SHAMAN:          sClass = "Chaman ";         break;
        case CLASS_MAGE:            sClass = "Mage ";           break;
        case CLASS_WARLOCK:         sClass = "Démoniste ";      break;
        case CLASS_DRUID:           sClass = "Druide ";         break;
    }
    return sClass;
}

std::string GetGamesStringData(BattleGround *arena, uint32 rating, bool isRated)
{
    std::string teamsMember[2];
    uint32 firstTeamId = 0;
    for (BattleGround::BattleGroundPlayerMap::const_iterator itr = arena->GetPlayers().begin(); itr != arena->GetPlayers().end(); ++itr)
        if (Player* player = ObjectAccessor::FindPlayer(itr->first))
        {
            if (player->isSpectator())
                continue;

            uint32 team = itr->second.Team;
            if (!firstTeamId)
                firstTeamId = team;

            teamsMember[firstTeamId == team] += GetClassNameById(player->getClass());
        }

    std::string data = "";
    if (isRated)
    {
        data = teamsMember[0] + " - ";
        std::stringstream ss;
        ss << rating;
        data += ss.str();
        data += " - " + teamsMember[1];
    }
    else
    {
    	data = teamsMember[0] + " - " + teamsMember[1];
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

void ShowPage(Player *player, uint32 page, ArenaRate rate)
{
	uint16 highGames  = 0;
	uint16 lowGames   = 0;
	uint16 noRated = 0;
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

            ArenaTeam *first =  objmgr.GetArenaTeamById(arena->GetArenaTeamIdForIndex(0));
            ArenaTeam *second =  objmgr.GetArenaTeamById(arena->GetArenaTeamIdForIndex(1));

            uint32 rating = 0;
            if (arena->isRated())
            {
                uint32 rating = first->GetRating() + second->GetRating();
                rating /= 2;
            }

            if (arena->isRated())
            {
                if (rate == HIGH_RATING && rating > TopGamesRating)
                {
            	    highGames++;
            	    if (highGames > (page + 1) * GamesOnPage)
            	    {
            	        haveNextPage = true;
            	        break;
            	    }

            	    if (highGames > page * GamesOnPage)
            	        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, GetGamesStringData(arena, rating, true), GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_SELECTED_PLAYER + GetFirstPlayerGuid(arena));
                }
                else if (rate == LOW_RATING && rating < TopGamesRating)
                {
            	    lowGames++;
            	    if (lowGames > (page + 1) * GamesOnPage)
            	    {
            	        haveNextPage = true;
            	        break;
            	    }

            	    if (lowGames > page * GamesOnPage)
            	        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, GetGamesStringData(arena, rating, true), GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_SELECTED_PLAYER + GetFirstPlayerGuid(arena));
                }
            }
            else if (rate == NO_RATED)
            {
            	noRated++;
            	if (noRated > (page + 1) * GamesOnPage)
            	{
            	    haveNextPage = true;
            	    break;
            	}

            	if (noRated > page * GamesOnPage)
            	    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_BATTLE, GetGamesStringData(arena, rating, false), GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_SELECTED_PLAYER + GetFirstPlayerGuid(arena));
            }
        }
    }

    switch (rate)
    {
        case NO_RATED:
        	if (page > 0)
        	    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_DOT, "Précédent...", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_NO_RATED + page - 1);

        	if (haveNextPage)
        	    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_DOT, "Suivant...", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_NO_RATED + page + 1);
        	break;
        case LOW_RATING:
        	if (page > 0)
        	    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_DOT, "Précédent...", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_LOW_RATING + page - 1);

        	if (haveNextPage)
        	    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_DOT, "Suivant...", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_LOW_RATING + page + 1);
        	break;
        case HIGH_RATING:
        	if (page > 0)
        	    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_DOT, "Précédent...", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_HIGH_RATING + page - 1);

        	if (haveNextPage)
        	    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_DOT, "Suivant...", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_HIGH_RATING + page + 1);
            break;
    }

    player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Retour", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
}

void spectate(Player* player, uint64 targetGuid)
{
	if (Player* target = ObjectAccessor::FindPlayer(targetGuid))
	{
	    ChatHandler chH = ChatHandler(player);

	    if (target == player || targetGuid == player->GetGUID())
	        return;

	    if (player->isInCombat())
	    {
	        chH.SendSysMessage(LANG_YOU_IN_COMBAT);
	    	return;
	    }

	    if (!target)
	    {
	    	chH.SendSysMessage(LANG_PLAYER_NOT_EXIST_OR_OFFLINE);
	    	return;
	    }

	    if (player->GetPet())
	    {
	    	chH.PSendSysMessage("Vous devez d'abord renvoyer votre familier.");
	    	return;
	    }

	    if (player->GetMap()->IsBattleGroundOrArena() && !player->isSpectator())
	    {
	    	chH.PSendSysMessage("Vous ne pouvez pas faire cela car vous êtes déjà dans un champ de bataille ou une arène.");
	    	return;
	    }

	    Map* cMap = target->GetMap();
	    if (!cMap->IsBattleArena())
	    {
	    	chH.PSendSysMessage("Ce joueur n'est pas dans une arène.");
	    	return;
	    }

	    if (player->GetMap()->IsBattleGround())
	    {
	    	chH.PSendSysMessage("Vous ne pouvez pas faire cela car vous êtes déjà dans un champ de bataille.");
	    	return;
	    }

	    if (BattleGround* bg = target->GetBattleGround())
	    {
	    	if (bg->GetStatus() != STATUS_IN_PROGRESS)
	    	{
	    	    chH.PSendSysMessage("Vous ne pouvez pas faire cela car l'arène n'a pas encore commencé.");
	    	    return;
	    	}

	    	if (bg->isRated())
	    	{
	    		chH.PSendSysMessage("Le mode spectateur est actuellement désactivé pour les arènes cotées.");
	    	    return;
	    	}
	    }

	    if (player->getSpectateCooldown() > 0)
	        return;

	    // all's well, set bg id
	    // when porting out from the bg, it will be reset to 0
	    player->SetBattleGroundId(target->GetBattleGroundId());
	    // remember current position as entry point for return at bg end teleportation
	    if (!player->GetMap()->IsBattleGroundOrArena())
	        player->SetBattleGroundEntryPoint(player->GetMapId(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation());

	    if (target->isSpectator())
	    {
	    	chH.PSendSysMessage("Vous ne pouvez pas faire cela car le joueur ciblé est aussi spectateur.");
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
		//player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Voir les arènes avec une grande cote...", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_HIGH_RATING);
		//player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Voir les arènes avec une petite cote...", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_LOW_RATING);
		player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Voir les arènes non cotées...", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_NO_RATED);
		player->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, "Voir un joueur...", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2, "", 0, true);
		player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
    }
	else
	{
		player->CLOSE_GOSSIP_MENU();
		creature->Say("Arena spectator désactivé", LANG_UNIVERSAL, player->GetGUID());
	}
}

bool GossipHello_npc_spectate(Player* pPlayer, Creature* pCreature)
{
	if(sWorld.getConfig(CONFIG_ARENA_SPECTATOR_ENABLE))
	{
        //pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Voir les arènes avec une grande cote...", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_HIGH_RATING);
        //pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Voir les arènes avec une petite cote...", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_LOW_RATING);
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, "Voir les arènes non cotées...", GOSSIP_SENDER_MAIN, NPC_SPECTATOR_ACTION_NO_RATED);
        pPlayer->ADD_GOSSIP_ITEM_EXTENDED(GOSSIP_ICON_CHAT, "Voir un joueur...", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2, "", 0, true);
	}
	else
	{
		pCreature->Say("Arena spectator désactivé", LANG_UNIVERSAL, pPlayer->GetGUID());
		return true;
	}

    pPlayer->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, pCreature->GetGUID());
    return true;
}

bool GossipSelect_npc_spectate(Player* player, Creature* creature, uint32 /*sender*/, uint32 action)
{
	if (action == GOSSIP_ACTION_INFO_DEF + 1)
		ShowDefaultPage(player, creature);
	else if (action >= NPC_SPECTATOR_ACTION_HIGH_RATING && action < NPC_SPECTATOR_ACTION_LOW_RATING)
	{
		ShowPage(player, action - NPC_SPECTATOR_ACTION_HIGH_RATING, HIGH_RATING);
		player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
	}
	else if (action >= NPC_SPECTATOR_ACTION_LOW_RATING && action < NPC_SPECTATOR_ACTION_NO_RATED)
	{
		ShowPage(player, action - NPC_SPECTATOR_ACTION_LOW_RATING, LOW_RATING);
		player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
	}
	else if (action >= NPC_SPECTATOR_ACTION_NO_RATED && action < NPC_SPECTATOR_ACTION_SELECTED_PLAYER)
	{
		ShowPage(player, action - NPC_SPECTATOR_ACTION_NO_RATED, NO_RATED);
		player->SEND_GOSSIP_MENU(DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
	}
	else if (action >= NPC_SPECTATOR_ACTION_SELECTED_PLAYER)
	{
    	player->CLOSE_GOSSIP_MENU();
    	uint64 targetGuid = action - NPC_SPECTATOR_ACTION_SELECTED_PLAYER;
    	spectate(player, targetGuid);
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
        	    	_Creature->Say("Nom incorrect!", LANG_UNIVERSAL, 0);
        	    	player->CLOSE_GOSSIP_MENU();
        	    	return true;
        	    }

        	    Player* target = objmgr.GetPlayer(sCode);
        	    if (!target)
        	    {
        	    	_Creature->Say("Impossible de trouver le joueur!", LANG_UNIVERSAL, 0);
        	    	player->CLOSE_GOSSIP_MENU();
        	    	return true;
        	    }

        	    spectate(player, target->GetGUID());
        	}
        	else
        		_Creature->Say("Champ vide!", LANG_UNIVERSAL, 0);

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
