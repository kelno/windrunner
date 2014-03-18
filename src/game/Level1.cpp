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

#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Opcodes.h"
#include "Chat.h"
#include "Log.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "Language.h"
#include "CellImpl.h"
#include "InstanceSaveMgr.h"
#include "Util.h"

#ifdef _DEBUG_VMAPS
#include "VMapFactory.h"
#endif

bool ChatHandler::HandleNpcSayCommand(const char* args)
{
    if(!*args)
        return false;

    Creature* pCreature = getSelectedCreature();
    if(!pCreature)
    {
        SendSysMessage(LANG_SELECT_CREATURE);
        SetSentErrorMessage(true);
        return false;
    }

    pCreature->Say(args, LANG_UNIVERSAL, 0);

    // make some emotes
    char lastchar = args[strlen(args) - 1];
    switch(lastchar)
    {
        case '?':   pCreature->HandleEmoteCommand(EMOTE_ONESHOT_QUESTION);      break;
        case '!':   pCreature->HandleEmoteCommand(EMOTE_ONESHOT_EXCLAMATION);   break;
        default:    pCreature->HandleEmoteCommand(EMOTE_ONESHOT_TALK);          break;
    }

    return true;
}

bool ChatHandler::HandleNpcYellCommand(const char* args)
{
    if(!*args)
        return false;

    Creature* pCreature = getSelectedCreature();
    if(!pCreature)
    {
        SendSysMessage(LANG_SELECT_CREATURE);
        SetSentErrorMessage(true);
        return false;
    }

    pCreature->Yell(args, LANG_UNIVERSAL, 0);

    // make an emote
    pCreature->HandleEmoteCommand(EMOTE_ONESHOT_SHOUT);

    return true;
}

//show text emote by creature in chat
bool ChatHandler::HandleNpcTextEmoteCommand(const char* args)
{
    if(!*args)
        return false;

    Creature* pCreature = getSelectedCreature();

    if(!pCreature)
    {
        SendSysMessage(LANG_SELECT_CREATURE);
        SetSentErrorMessage(true);
        return false;
    }

    pCreature->TextEmote(args, 0);

    return true;
}

// make npc whisper to player
bool ChatHandler::HandleNpcWhisperCommand(const char* args)
{
    if(!*args)
        return false;

    char* receiver_str = strtok((char*)args, " ");
    char* text = strtok(NULL, "");

    uint64 guid = m_session->GetPlayer()->GetSelection();
    Creature* pCreature = ObjectAccessor::GetCreature(*m_session->GetPlayer(), guid);

    if(!pCreature || !receiver_str || !text)
    {
        return false;
    }

    uint64 receiver_guid= atol(receiver_str);

    pCreature->Whisper(text,receiver_guid);

    return true;
}

bool ChatHandler::HandleNameAnnounceCommand(const char* args)
{
    WorldPacket data;
    if(!*args)
        return false;
    //char str[1024];
    //sprintf(str, GetTrinityString(LANG_ANNOUNCE_COLOR), m_session->GetPlayer()->GetName(), args);
    //sWorld.SendWorldText(LANG_ANNOUNCE_COLOR, m_session->GetPlayer()->GetName(), args);
    return true;
}

bool ChatHandler::HandleGMNameAnnounceCommand(const char* args)
{
    WorldPacket data;
    if(!*args)
        return false;

    sWorld.SendGMText(LANG_GM_ANNOUNCE_COLOR, m_session->GetPlayer()->GetName(), args);
    return true;
}

// global announce
bool ChatHandler::HandleAnnounceCommand(const char* args)
{
    if(!*args)
        return false;

    sWorld.SendWorldText(LANG_SYSTEMMESSAGE,args);
    return true;
}

// announce to logged in GMs
bool ChatHandler::HandleGMAnnounceCommand(const char* args)
{
    if(!*args)
        return false;

    sWorld.SendGMText(LANG_GM_BROADCAST,args);
    return true;
}

//notification player at the screen
bool ChatHandler::HandleNotifyCommand(const char* args)
{
    if(!*args)
        return false;

    std::string str = GetTrinityString(LANG_GLOBAL_NOTIFY);
    str += args;

    WorldPacket data(SMSG_NOTIFICATION, (str.size()+1));
    data << str;
    sWorld.SendGlobalMessage(&data);

    return true;
}

//notification GM at the screen
bool ChatHandler::HandleGMNotifyCommand(const char* args)
{
    if(!*args)
        return false;

    std::string str = GetTrinityString(LANG_GM_NOTIFY);
    str += args;

    WorldPacket data(SMSG_NOTIFICATION, (str.size()+1));
    data << str;
    sWorld.SendGlobalGMMessage(&data);

    return true;
}

//Enable\Dissable GM Mode
bool ChatHandler::HandleGMmodeCommand(const char* args)
{
    if(!*args)
    {
        if(m_session->GetPlayer()->isGameMaster())
            m_session->SendNotification(LANG_GM_ON);
        else
            m_session->SendNotification(LANG_GM_OFF);
        return true;
    }

    std::string argstr = (char*)args;

    if (argstr == "on")
    {
        m_session->GetPlayer()->SetGameMaster(true);
        m_session->SendNotification(LANG_GM_ON);
        #ifdef _DEBUG_VMAPS
        VMAP::IVMapManager *vMapManager = VMAP::VMapFactory::createOrGetVMapManager();
        vMapManager->processCommand("stoplog");
        #endif
        return true;
    }

    if (argstr == "off")
    {
        m_session->GetPlayer()->SetGameMaster(false);
        m_session->SendNotification(LANG_GM_OFF);
        #ifdef _DEBUG_VMAPS
        VMAP::IVMapManager *vMapManager = VMAP::VMapFactory::createOrGetVMapManager();
        vMapManager->processCommand("startlog");
        #endif
        return true;
    }

    SendSysMessage(LANG_USE_BOL);
    SetSentErrorMessage(true);
    return false;
}

// Enables or disables hiding of the staff badge
bool ChatHandler::HandleGMChatCommand(const char* args)
{
    if(!*args)
    {
        if(m_session->GetPlayer()->isGMChat())
            m_session->SendNotification(LANG_GM_CHAT_ON);
        else
            m_session->SendNotification(LANG_GM_CHAT_OFF);
        return true;
    }

    std::string argstr = (char*)args;

    if (argstr == "on")
    {
        m_session->GetPlayer()->SetGMChat(true);
        m_session->SendNotification(LANG_GM_CHAT_ON);
        return true;
    }

    if (argstr == "off")
    {
        m_session->GetPlayer()->SetGMChat(false);
        m_session->SendNotification(LANG_GM_CHAT_OFF);
        return true;
    }

    SendSysMessage(LANG_USE_BOL);
    SetSentErrorMessage(true);
    return false;
}

std::string ChatHandler::PGetParseString(int32 entry, ...)
{
        const char *format = GetTrinityString(entry);
        va_list ap;
        char str [1024];
        va_start(ap, entry);
        vsnprintf(str,1024,format, ap );
        va_end(ap);
        return (std::string)str;
}

bool ChatHandler::HandleGMTicketListCommand(const char* args)
{
  SendSysMessage(LANG_COMMAND_TICKETSHOWLIST);
  for(GmTicketList::iterator itr = objmgr.m_GMTicketList.begin(); itr != objmgr.m_GMTicketList.end(); ++itr)
  {
    if((*itr)->closed != 0)
      continue;
    std::string gmname;
    std::stringstream ss;
    ss << PGetParseString(LANG_COMMAND_TICKETLISTGUID, (*itr)->guid);
    std::string currentName; // Refresh data if the character was renamed
    if (!objmgr.GetPlayerNameByGUID((*itr)->playerGuid, currentName))
        continue;
    ss << PGetParseString(LANG_COMMAND_TICKETLISTNAME, currentName.c_str(), currentName.c_str());
    ss << PGetParseString(LANG_COMMAND_TICKETLISTAGECREATE, (secsToTimeString(time(NULL) - (*itr)->createtime, true, false)).c_str());
    ss << PGetParseString(LANG_COMMAND_TICKETLISTAGE, (secsToTimeString(time(NULL) - (*itr)->timestamp, true, false)).c_str());
    if(objmgr.GetPlayerNameByGUID((*itr)->assignedToGM, gmname))
    {
      ss << PGetParseString(LANG_COMMAND_TICKETLISTASSIGNEDTO, gmname.c_str());
    }
    SendSysMessage(ss.str().c_str());
  }
  return true;
}


bool ChatHandler::HandleGMTicketListOnlineCommand(const char* args)
{
  SendSysMessage(LANG_COMMAND_TICKETSHOWONLINELIST);
  for(GmTicketList::iterator itr = objmgr.m_GMTicketList.begin(); itr != objmgr.m_GMTicketList.end(); ++itr)
  {
    if((*itr)->closed != 0 || !objmgr.GetPlayer((*itr)->playerGuid))
      continue;

    std::string gmname;
    std::stringstream ss;
    ss << PGetParseString(LANG_COMMAND_TICKETLISTGUID, (*itr)->guid);
    std::string currentName; // Refresh data if the character was renamed
    if (!objmgr.GetPlayerNameByGUID((*itr)->playerGuid, currentName))
        continue;
    ss << PGetParseString(LANG_COMMAND_TICKETLISTNAME, currentName.c_str(), currentName.c_str());
    ss << PGetParseString(LANG_COMMAND_TICKETLISTAGECREATE, (secsToTimeString(time(NULL) - (*itr)->createtime, true, false)).c_str());
    ss << PGetParseString(LANG_COMMAND_TICKETLISTAGE, (secsToTimeString(time(NULL) - (*itr)->timestamp, true, false)).c_str());
    if(objmgr.GetPlayerNameByGUID((*itr)->assignedToGM, gmname))
    {
      ss << PGetParseString(LANG_COMMAND_TICKETLISTASSIGNEDTO, gmname.c_str());
    }
    SendSysMessage(ss.str().c_str());
  }
  return true;
}

bool ChatHandler::HandleGMTicketListClosedCommand(const char* args)
{
  SendSysMessage(LANG_COMMAND_TICKETSHOWCLOSEDLIST);
  for(GmTicketList::iterator itr = objmgr.m_GMTicketList.begin(); itr != objmgr.m_GMTicketList.end(); ++itr)
  {
    if((*itr)->closed == 0)
      continue;

    std::string gmname;
    std::stringstream ss;
    ss << PGetParseString(LANG_COMMAND_TICKETLISTGUID, (*itr)->guid);
    std::string currentName; // Refresh data if the character was renamed
    if (!objmgr.GetPlayerNameByGUID((*itr)->playerGuid, currentName))
        continue;
    ss << PGetParseString(LANG_COMMAND_TICKETLISTNAME, currentName.c_str(), currentName.c_str());
    ss << PGetParseString(LANG_COMMAND_TICKETLISTAGECREATE, (secsToTimeString(time(NULL) - (*itr)->createtime, true, false)).c_str());
    ss << PGetParseString(LANG_COMMAND_TICKETLISTAGE, (secsToTimeString(time(NULL) - (*itr)->timestamp, true, false)).c_str());
    if(objmgr.GetPlayerNameByGUID((*itr)->assignedToGM, gmname))
    {
      ss << PGetParseString(LANG_COMMAND_TICKETLISTASSIGNEDTO, gmname.c_str());
    }
    SendSysMessage(ss.str().c_str());
  }
  return true;
}

bool ChatHandler::HandleGMTicketGetByIdCommand(const char* args)
{
  if(!*args)
    return false;

  uint64 tguid = atoi(args);
  GM_Ticket *ticket = objmgr.GetGMTicket(tguid);
  if(!ticket || ticket->closed != 0) 
  {
    SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
    return true;
  }

  std::string gmname;
  std::stringstream ss;
  ss << PGetParseString(LANG_COMMAND_TICKETLISTGUID, ticket->guid);
  std::string currentName; // Refresh data if the character was renamed
    if (!objmgr.GetPlayerNameByGUID(ticket->playerGuid, currentName))
        return false;
    ss << PGetParseString(LANG_COMMAND_TICKETLISTNAME, currentName.c_str(), currentName.c_str());
  ss << PGetParseString(LANG_COMMAND_TICKETLISTAGECREATE, (secsToTimeString(time(NULL) - ticket->createtime, true, false)).c_str());
  ss << PGetParseString(LANG_COMMAND_TICKETLISTAGE, (secsToTimeString(time(NULL) - ticket->timestamp, true, false)).c_str());
  if(objmgr.GetPlayerNameByGUID(ticket->assignedToGM, gmname))
  {
    ss << PGetParseString(LANG_COMMAND_TICKETLISTASSIGNEDTO, gmname.c_str());
  }
  ss << PGetParseString(LANG_COMMAND_TICKETLISTMESSAGE, ticket->message.c_str());
  if(ticket->comment != "")
  {
    ss <<  PGetParseString(LANG_COMMAND_TICKETLISTCOMMENT, ticket->comment.c_str());
  }
  SendSysMessage(ss.str().c_str());
  return true;
}

bool ChatHandler::HandleGMTicketGetByNameCommand(const char* args)
{
  if(!*args)
    return false;

  Player *plr = objmgr.GetPlayer(args);  
  if(!plr)
  {
    SendSysMessage(LANG_NO_PLAYERS_FOUND);
    return true;
  }
  
  GM_Ticket *ticket = objmgr.GetGMTicketByPlayer(plr->GetGUID()); 
  if(!ticket)
  {
    SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
    return true;
  }

  std::string gmname;
  std::stringstream ss;
  ss << PGetParseString(LANG_COMMAND_TICKETLISTGUID, ticket->guid);
  std::string currentName; // Refresh data if the character was renamed
    if (!objmgr.GetPlayerNameByGUID(ticket->playerGuid, currentName))
        return false;
    ss << PGetParseString(LANG_COMMAND_TICKETLISTNAME, currentName.c_str(), currentName.c_str());
  ss << PGetParseString(LANG_COMMAND_TICKETLISTAGECREATE, (secsToTimeString(time(NULL) - ticket->createtime, true, false)).c_str());
  ss << PGetParseString(LANG_COMMAND_TICKETLISTAGE, (secsToTimeString(time(NULL) - ticket->timestamp, true, false)).c_str());
  if(objmgr.GetPlayerNameByGUID(ticket->assignedToGM, gmname))
  {
    ss << PGetParseString(LANG_COMMAND_TICKETLISTASSIGNEDTO, gmname.c_str());
  }
  ss <<  PGetParseString(LANG_COMMAND_TICKETLISTMESSAGE, ticket->message.c_str());
  if(ticket->comment != "")
  {
    ss <<  PGetParseString(LANG_COMMAND_TICKETLISTCOMMENT, ticket->comment.c_str());
  }
  SendSysMessage(ss.str().c_str());
  return true;
}

bool ChatHandler::HandleGMTicketCloseByIdCommand(const char* args)
{
  if(!*args)
    return false;

  uint64 tguid = atoi(args);
  GM_Ticket *ticket = objmgr.GetGMTicket(tguid);
  if(!ticket || ticket->closed != 0)
  {
    SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
    return true;
  }
  if(ticket && ticket->assignedToGM != 0 && ticket->assignedToGM != m_session->GetPlayer()->GetGUID())
  {
    PSendSysMessage(LANG_COMMAND_TICKETCANNOTCLOSE, ticket->guid);
    return true;
  }
  std::stringstream ss;
  ss << PGetParseString(LANG_COMMAND_TICKETLISTGUID, ticket->guid);
  std::string currentName; // Refresh data if the character was renamed
    if (!objmgr.GetPlayerNameByGUID(ticket->playerGuid, currentName))
        return false;
    ss << PGetParseString(LANG_COMMAND_TICKETLISTNAME, currentName.c_str(), currentName.c_str());
  ss << PGetParseString(LANG_COMMAND_TICKETCLOSED, m_session->GetPlayer()->GetName());
  SendGlobalGMSysMessage(ss.str().c_str());
  Player *plr = objmgr.GetPlayer(ticket->playerGuid);
  ticket->timestamp = time(NULL);
  objmgr.RemoveGMTicket(ticket, m_session->GetAccountId()); 

  if(!plr || !plr->IsInWorld())
    return true;

  // send abandon ticket
  WorldPacket data(SMSG_GMTICKET_DELETETICKET, 4);
  data << uint32(9);
  plr->GetSession()->SendPacket( &data );
  return true;
}

bool ChatHandler::HandleGMTicketAssignToCommand(const char* args)
{
  if(!*args)
    return false;

  char* tguid = strtok((char*)args, " ");
  uint64 ticketGuid = atoi(tguid);
  char* targetgm = strtok( NULL, " ");

  if(!targetgm)
    return false;

  std::string targm = targetgm;

  if(!normalizePlayerName(targm))
    return true;

  Player *cplr = m_session->GetPlayer();
  std::string gmname;
  GM_Ticket *ticket = objmgr.GetGMTicket(ticketGuid);

  if(!ticket || ticket->closed != 0)
  {
    SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
    return true;
  }
  uint64 tarGUID = objmgr.GetPlayerGUIDByName(targm.c_str());
  uint64 accid = objmgr.GetPlayerAccountIdByGUID(tarGUID);
  QueryResult *result = LoginDatabase.PQuery("SELECT `gmlevel` FROM `account` WHERE `id` = '%u'", accid);
  if(!tarGUID|| !result || result->Fetch()->GetUInt32() < SEC_GAMEMASTER1)
  {
    SendSysMessage(LANG_COMMAND_TICKETASSIGNERROR_A);
    return true;
  }

  if (result)
    delete result;

  if(ticket->assignedToGM == tarGUID)
  {
    PSendSysMessage(LANG_COMMAND_TICKETASSIGNERROR_B, ticket->guid);
    return true;
  }
  objmgr.GetPlayerNameByGUID(tarGUID, gmname);
  if(ticket->assignedToGM != 0 && ticket->assignedToGM != cplr->GetGUID())
  {
    PSendSysMessage(LANG_COMMAND_TICKETALREADYASSIGNED, ticket->guid, gmname.c_str());
    return true;
  }

  ticket->assignedToGM = tarGUID;
  objmgr.AddOrUpdateGMTicket(*ticket);
  std::stringstream ss;
  ss << PGetParseString(LANG_COMMAND_TICKETLISTGUID, ticket->guid);
  std::string currentName; // Refresh data if the character was renamed
    if (!objmgr.GetPlayerNameByGUID(ticket->playerGuid, currentName))
        return false;
    ss << PGetParseString(LANG_COMMAND_TICKETLISTNAME, currentName.c_str(), currentName.c_str());
  ss << PGetParseString(LANG_COMMAND_TICKETLISTASSIGNEDTO, gmname.c_str());
  SendGlobalGMSysMessage(ss.str().c_str());
  return true;
}

bool ChatHandler::HandleGMTicketUnAssignCommand(const char* args)
{
  if(!*args)
    return false;

  uint64 ticketGuid = atoi(args);
  Player *cplr = m_session->GetPlayer();
  GM_Ticket *ticket = objmgr.GetGMTicket(ticketGuid);

  if(!ticket|| ticket->closed != 0)
  {
    SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
    return true;
  }
  if(ticket->assignedToGM == 0)
  {
    PSendSysMessage(LANG_COMMAND_TICKETNOTASSIGNED, ticket->guid);
    return true;
  }

  std::string gmname;
  objmgr.GetPlayerNameByGUID(ticket->assignedToGM, gmname);
  Player *plr = objmgr.GetPlayer(ticket->assignedToGM);
  if(plr && plr->IsInWorld() && plr->GetSession()->GetSecurity() > cplr->GetSession()->GetSecurity())
  {
    SendSysMessage(LANG_COMMAND_TICKETUNASSIGNSECURITY);
    return true;
  }

  std::stringstream ss;
  ss << PGetParseString(LANG_COMMAND_TICKETLISTGUID, ticket->guid);
  std::string currentName; // Refresh data if the character was renamed
    if (!objmgr.GetPlayerNameByGUID(ticket->playerGuid, currentName))
        return false;
    ss << PGetParseString(LANG_COMMAND_TICKETLISTNAME, currentName.c_str(), currentName.c_str());
  ss << PGetParseString(LANG_COMMAND_TICKETLISTASSIGNEDTO, gmname.c_str());
  ss << PGetParseString(LANG_COMMAND_TICKETLISTUNASSIGNED, cplr->GetName());
  SendGlobalGMSysMessage(ss.str().c_str());
  ticket->assignedToGM = 0;
  objmgr.AddOrUpdateGMTicket(*ticket);
  return true;
}

bool ChatHandler::HandleGMTicketCommentCommand(const char* args)
{
  if(!*args)
    return false;

  char* tguid = strtok((char*)args, " ");
  uint64 ticketGuid = atoi(tguid);
  char* comment = strtok( NULL, "\n");

  if(!comment)
    return false;

  Player *cplr = m_session->GetPlayer();
  GM_Ticket *ticket = objmgr.GetGMTicket(ticketGuid);

  if(!ticket || ticket->closed != 0)
  {
    PSendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
    return true;
  }
  if(ticket->assignedToGM != 0 && ticket->assignedToGM != cplr->GetGUID())
  {
    PSendSysMessage(LANG_COMMAND_TICKETALREADYASSIGNED, ticket->guid);
    return true;
  }

  std::string gmname;
  objmgr.GetPlayerNameByGUID(ticket->assignedToGM, gmname);
  ticket->comment = comment;
  objmgr.AddOrUpdateGMTicket(*ticket);
  std::stringstream ss;
  ss << PGetParseString(LANG_COMMAND_TICKETLISTGUID, ticket->guid);
  std::string currentName; // Refresh data if the character was renamed
    if (!objmgr.GetPlayerNameByGUID(ticket->playerGuid, currentName))
        return false;
    ss << PGetParseString(LANG_COMMAND_TICKETLISTNAME, currentName.c_str(), currentName.c_str());
  if(objmgr.GetPlayerNameByGUID(ticket->assignedToGM, gmname))
  {
    ss << PGetParseString(LANG_COMMAND_TICKETLISTASSIGNEDTO, gmname.c_str());
  }
  ss << PGetParseString(LANG_COMMAND_TICKETLISTADDCOMMENT, cplr->GetName(), ticket->comment.c_str());
  SendGlobalGMSysMessage(ss.str().c_str());
  return true;
}

bool ChatHandler::HandleGMTicketDeleteByIdCommand(const char* args)
{
  if(!*args)
    return false;
  uint64 ticketGuid = atoi(args);
  GM_Ticket *ticket = objmgr.GetGMTicket(ticketGuid);

  if(!ticket)
  {
    SendSysMessage(LANG_COMMAND_TICKETNOTEXIST);
    return true;
  }
  if(ticket->closed == 0)
  {
    SendSysMessage(LANG_COMMAND_TICKETCLOSEFIRST);
    return true;
  }

  std::stringstream ss;
  ss << PGetParseString(LANG_COMMAND_TICKETLISTGUID, ticket->guid);
  std::string currentName; // Refresh data if the character was renamed
    if (!objmgr.GetPlayerNameByGUID(ticket->playerGuid, currentName))
        return false;
    ss << PGetParseString(LANG_COMMAND_TICKETLISTNAME, currentName.c_str(), currentName.c_str());
  ss << PGetParseString(LANG_COMMAND_TICKETDELETED, m_session->GetPlayer()->GetName());
  SendGlobalGMSysMessage(ss.str().c_str());
  Player *plr = objmgr.GetPlayer(ticket->playerGuid);
  objmgr.RemoveGMTicket(ticket, -1, true);
  if(plr && plr->IsInWorld())
  {
    // Force abandon ticket
    WorldPacket data(SMSG_GMTICKET_DELETETICKET, 4);
    data << uint32(9);
    plr->GetSession()->SendPacket( &data );
  }

  ticket = NULL;
  return true;
}

bool ChatHandler::HandleGMTicketReloadCommand(const char*)
{
  objmgr.LoadGMTickets();
    return true;
}

//Enable\Dissable Invisible mode
bool ChatHandler::HandleVisibleCommand(const char* args)
{
    if (!*args)
    {
        PSendSysMessage(LANG_YOU_ARE, m_session->GetPlayer()->isGMVisible() ?  GetTrinityString(LANG_VISIBLE) : GetTrinityString(LANG_INVISIBLE));
        return true;
    }

    std::string argstr = (char*)args;

    if (argstr == "on")
    {
        m_session->GetPlayer()->SetGMVisible(true);
        m_session->SendNotification(LANG_INVISIBLE_VISIBLE);
        return true;
    }

    if (argstr == "off")
    {
        m_session->SendNotification(LANG_INVISIBLE_INVISIBLE);
        m_session->GetPlayer()->SetGMVisible(false);
        return true;
    }

    SendSysMessage(LANG_USE_BOL);
    SetSentErrorMessage(true);
    return false;
}

bool ChatHandler::HandleGPSCommand(const char* args)
{
    WorldObject *obj = NULL;
    if (*args)
    {
        std::string name = args;
        if(normalizePlayerName(name))
            obj = objmgr.GetPlayer(name.c_str());

        if(!obj)
        {
            SendSysMessage(LANG_PLAYER_NOT_FOUND);
            SetSentErrorMessage(true);
            return false;
        }
    }
    else
    {
        obj = getSelectedUnit();

        if(!obj)
        {
            SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
            SetSentErrorMessage(true);
            return false;
        }
    }
    CellPair cell_val = Trinity::ComputeCellPair(obj->GetPositionX(), obj->GetPositionY());
    Cell cell(cell_val);

    uint32 zone_id = obj->GetZoneId();
    uint32 area_id = obj->GetAreaId();

    MapEntry const* mapEntry = sMapStore.LookupEntry(obj->GetMapId());
    AreaTableEntry const* zoneEntry = GetAreaEntryByAreaID(zone_id);
    AreaTableEntry const* areaEntry = GetAreaEntryByAreaID(area_id);

    float zone_x = obj->GetPositionX();
    float zone_y = obj->GetPositionY();

    Map2ZoneCoordinates(zone_x,zone_y,zone_id);

    Map const *map = obj->GetMap();
    float ground_z = map->GetHeight(obj->GetPositionX(), obj->GetPositionY(), MAX_HEIGHT);
    float floor_z = map->GetHeight(obj->GetPositionX(), obj->GetPositionY(), obj->GetPositionZ());

    GridPair p = Trinity::ComputeGridPair(obj->GetPositionX(), obj->GetPositionY());

    int gx=63-p.x_coord;
    int gy=63-p.y_coord;

    uint32 have_map = Map::ExistMap(obj->GetMapId(),gx,gy) ? 1 : 0;
    uint32 have_vmap = Map::ExistVMap(obj->GetMapId(),gx,gy) ? 1 : 0;

    if(have_vmap)
    {
        if(map->IsOutdoors(obj->GetPositionX(), obj->GetPositionY(), obj->GetPositionZ()))
            PSendSysMessage("You are outdoors");
        else
            PSendSysMessage("You are indoor");
        
        uint32 mogpFlags;
        int32 adtId, rootId, groupId;
        WMOAreaTableEntry const* wmoEntry = 0;

        if (map->GetAreaInfo(obj->GetPositionX(), obj->GetPositionY(), obj->GetPositionZ(), mogpFlags, adtId, rootId, groupId))
        {
            if (wmoEntry = GetWMOAreaTableEntryByTripple(rootId, adtId, groupId))
                PSendSysMessage(LANG_GPS_WMO_DATA, wmoEntry->Id, wmoEntry->Flags, mogpFlags);
        }
    }
    else PSendSysMessage("no VMAP available for area info");

    PSendSysMessage(LANG_MAP_POSITION,
        obj->GetMapId(), (mapEntry ? mapEntry->name[m_session->GetSessionDbcLocale()] : "<unknown>" ),
        zone_id, (zoneEntry ? zoneEntry->area_name[m_session->GetSessionDbcLocale()] : "<unknown>" ),
        area_id, (areaEntry ? areaEntry->area_name[m_session->GetSessionDbcLocale()] : "<unknown>" ),
        obj->GetPositionX(), obj->GetPositionY(), obj->GetPositionZ(), obj->GetOrientation(),
        cell.GridX(), cell.GridY(), cell.CellX(), cell.CellY(), obj->GetInstanceId(),
        zone_x, zone_y, ground_z, floor_z, have_map, have_vmap );
        
    //more correct format for script, you just have to copy/paste !
    PSendSysMessage(LANG_GPS_FOR_SCRIPT, obj->GetPositionX(), obj->GetPositionY(), obj->GetPositionZ(), obj->GetOrientation());
    
    sLog.outDebug("%f, %f, %f, %f", obj->GetPositionX(), obj->GetPositionY(), obj->GetPositionZ(), obj->GetOrientation());

    return true;
}

//Summon Player
bool ChatHandler::HandleNamegoCommand(const char* args)
{
    if(!*args)
        return false;

    std::string name = args;

    if(!normalizePlayerName(name))
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    Player *target = objmgr.GetPlayer(name.c_str());
    if (target)
    {
        if(target->IsBeingTeleported()==true)
        {
            PSendSysMessage(LANG_IS_TELEPORTED, target->GetName());
            SetSentErrorMessage(true);
            return false;
        }

        Map* pMap = m_session->GetPlayer()->GetMap();

        if(pMap->IsBattleGroundOrArena())
        {
            // only allow if gm mode is on
            if (!target->isGameMaster())
            {
                PSendSysMessage(LANG_CANNOT_GO_TO_BG_GM,target->GetName());
                SetSentErrorMessage(true);
                return false;
            }
            // if both players are in different bgs
            else if (target->GetBattleGroundId() && m_session->GetPlayer()->GetBattleGroundId() != target->GetBattleGroundId())
            {
                PSendSysMessage(LANG_CANNOT_GO_TO_BG_FROM_BG,target->GetName());
                SetSentErrorMessage(true);
                return false;
            }
            // all's well, set bg id
            // when porting out from the bg, it will be reset to 0
            target->SetBattleGroundId(m_session->GetPlayer()->GetBattleGroundId());
            // remember current position as entry point for return at bg end teleportation
            target->SetBattleGroundEntryPoint(target->GetMapId(),target->GetPositionX(),target->GetPositionY(),target->GetPositionZ(),target->GetOrientation());
        }
        else if(pMap->IsDungeon())
        {
            Map* cMap = target->GetMap();
            if( cMap->Instanceable() && cMap->GetInstanceId() != pMap->GetInstanceId())
            {
                // cannot summon from instance to instance
                PSendSysMessage(LANG_CANNOT_SUMMON_TO_INST,target->GetName());
                SetSentErrorMessage(true);
                return false;
            }

            // we are in instance, and can summon only player in our group with us as lead
            if ( !m_session->GetPlayer()->GetGroup() || !target->GetGroup() ||
                (target->GetGroup()->GetLeaderGUID() != m_session->GetPlayer()->GetGUID()) ||
                (m_session->GetPlayer()->GetGroup()->GetLeaderGUID() != m_session->GetPlayer()->GetGUID()))
                // the last check is a bit excessive, but let it be, just in case
            {
                PSendSysMessage(LANG_CANNOT_SUMMON_TO_INST,target->GetName());
                SetSentErrorMessage(true);
                return false;
            }
        }

        PSendSysMessage(LANG_SUMMONING, target->GetName(),"");
        if (needReportToTarget(target))
            ChatHandler(target).PSendSysMessage(LANG_SUMMONED_BY, GetName());

        // stop flight if need
        if(target->isInFlight())
        {
            target->GetMotionMaster()->MovementExpired();
            target->CleanupAfterTaxiFlight();
        }
        // save only in non-flight case
        else
            target->SaveRecallPosition();

        // before GM
        float x,y,z;
        m_session->GetPlayer()->GetClosePoint(x,y,z,target->GetObjectSize());
        target->TeleportTo(m_session->GetPlayer()->GetMapId(),x,y,z,target->GetOrientation());
    }
    else if (uint64 guid = objmgr.GetPlayerGUIDByName(name))
    {
        PSendSysMessage(LANG_SUMMONING, name.c_str(),GetTrinityString(LANG_OFFLINE));

        // in point where GM stay
        Player::SavePositionInDB(m_session->GetPlayer()->GetMapId(),
            m_session->GetPlayer()->GetPositionX(),
            m_session->GetPlayer()->GetPositionY(),
            m_session->GetPlayer()->GetPositionZ(),
            m_session->GetPlayer()->GetOrientation(),
            m_session->GetPlayer()->GetZoneId(),
            guid);
    }
    else
    {
        PSendSysMessage(LANG_NO_PLAYER, args);
        SetSentErrorMessage(true);
    }

    return true;
}

//Teleport to Player
bool ChatHandler::HandleGonameCommand(const char* args)
{
    if(!*args)
        return false;

    Player* _player = m_session->GetPlayer();

    std::string name = args;

    if(!normalizePlayerName(name))
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    Player *target = objmgr.GetPlayer(name.c_str());
    if (target)
    {
        Map* cMap = target->GetMap();
        if (!cMap)
            return false;
        if(cMap->IsBattleGroundOrArena())
        {
            // only allow if gm mode is on
            if (!_player->isGameMaster())
            {
                PSendSysMessage(LANG_CANNOT_GO_TO_BG_GM,target->GetName());
                SetSentErrorMessage(true);
                return false;
            }
            // if both players are in different bgs
            else if (_player->GetBattleGroundId() && _player->GetBattleGroundId() != target->GetBattleGroundId())
            {
                PSendSysMessage(LANG_CANNOT_GO_TO_BG_FROM_BG,target->GetName());
                SetSentErrorMessage(true);
                return false;
            }
            // all's well, set bg id
            // when porting out from the bg, it will be reset to 0
            _player->SetBattleGroundId(target->GetBattleGroundId());
            // remember current position as entry point for return at bg end teleportation
            _player->SetBattleGroundEntryPoint(_player->GetMapId(),_player->GetPositionX(),_player->GetPositionY(),_player->GetPositionZ(),_player->GetOrientation());

        }
        else if(cMap->IsDungeon())
        {
            Map* pMap = MapManager::Instance().GetMap(_player->GetMapId(),_player);

            // we have to go to instance, and can go to player only if:
            //   1) we are in his group (either as leader or as member)
            //   2) we are not bound to any group and have GM mode on
            if (_player->GetGroup())
            {
                // we are in group, we can go only if we are in the player group
                if (_player->GetGroup() != target->GetGroup())
                {
                    PSendSysMessage(LANG_CANNOT_GO_TO_INST_PARTY,target->GetName());
                    SetSentErrorMessage(true);
                    return false;
                }
            }
            else
            {
                // we are not in group, let's verify our GM mode
                if (!_player->isGameMaster())
                {
                    PSendSysMessage(LANG_CANNOT_GO_TO_INST_GM,target->GetName());
                    SetSentErrorMessage(true);
                    return false;
                }
            }

            // if the player or the player's group is bound to another instance
            // the player will not be bound to another one
            InstancePlayerBind *pBind = _player->GetBoundInstance(target->GetMapId(), target->GetDifficulty());
            if(!pBind)
            {
                Group *group = _player->GetGroup();
                // if no bind exists, create a solo bind
                InstanceGroupBind *gBind = group ? group->GetBoundInstance(target->GetMapId(), target->GetDifficulty()) : NULL;
                if (!gBind)
                    if (InstanceSave *save = sInstanceSaveManager.GetInstanceSave(target->GetInstanceId()))
                        _player->BindToInstance(save, !save->CanReset());
            }

            _player->SetDifficulty(target->GetDifficulty());
        }

        PSendSysMessage(LANG_APPEARING_AT, target->GetName());

        if (_player->IsVisibleGloballyFor(target))
            ChatHandler(target).PSendSysMessage(LANG_APPEARING_TO, _player->GetName());

        // stop flight if need
        if(_player->isInFlight())
        {
            _player->GetMotionMaster()->MovementExpired();
            _player->CleanupAfterTaxiFlight();
        }
        // save only in non-flight case
        else
            _player->SaveRecallPosition();

        // to point to see at target with same orientation
        float x,y,z;
        target->GetContactPoint(m_session->GetPlayer(),x,y,z);

        _player->TeleportTo(target->GetMapId(), x, y, z, _player->GetAngle(target), TELE_TO_GM_MODE);

        return true;
    }

    if (uint64 guid = objmgr.GetPlayerGUIDByName(name))
    {
        PSendSysMessage(LANG_APPEARING_AT, name.c_str());

        // to point where player stay (if loaded)
        float x,y,z,o;
        uint32 map;
        bool in_flight;
        if(Player::LoadPositionFromDB(map,x,y,z,o,in_flight,guid))
        {
            // stop flight if need
            if(_player->isInFlight())
            {
                _player->GetMotionMaster()->MovementExpired();
                _player->CleanupAfterTaxiFlight();
            }
            // save only in non-flight case
            else
                _player->SaveRecallPosition();

            _player->TeleportTo(map, x, y, z,_player->GetOrientation());
            return true;
        }
    }

    PSendSysMessage(LANG_NO_PLAYER, args);

    SetSentErrorMessage(true);
    return false;
}

// Teleport player to last position
bool ChatHandler::HandleRecallCommand(const char* args)
{
    Player* chr = NULL;

    if(!*args)
    {
        chr = getSelectedPlayer();
        if(!chr)
            chr = m_session->GetPlayer();
    }
    else
    {
        std::string name = args;

        if(!normalizePlayerName(name))
        {
            SendSysMessage(LANG_PLAYER_NOT_FOUND);
            SetSentErrorMessage(true);
            return false;
        }

        chr = objmgr.GetPlayer(name.c_str());

        if(!chr)
        {
            PSendSysMessage(LANG_NO_PLAYER, args);
            SetSentErrorMessage(true);
            return false;
        }
    }

    if(chr->IsBeingTeleported())
    {
        PSendSysMessage(LANG_IS_TELEPORTED, chr->GetName());
        SetSentErrorMessage(true);
        return false;
    }

    // stop flight if need
    if(chr->isInFlight())
    {
        chr->GetMotionMaster()->MovementExpired();
        chr->CleanupAfterTaxiFlight();
    }

    chr->TeleportTo(chr->m_recallMap, chr->m_recallX, chr->m_recallY, chr->m_recallZ, chr->m_recallO);
    return true;
}

//Edit Player KnownTitles
bool ChatHandler::HandleModifyKnownTitlesCommand(const char* args)
{
    if(!*args)
        return false;

    uint64 titles = 0;

    sscanf((char*)args, I64FMTD, &titles);

    Player *chr = getSelectedPlayer();
    if (!chr)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    uint64 titles2 = titles;

    for(int i=1; i < sCharTitlesStore.GetNumRows(); ++i)
        if(CharTitlesEntry const* tEntry = sCharTitlesStore.LookupEntry(i))
            titles2 &= ~(uint64(1) << tEntry->bit_index);

    titles &= ~titles2;                                     // remove not existed titles

    chr->SetUInt64Value(PLAYER_FIELD_KNOWN_TITLES, titles);
    SendSysMessage(LANG_DONE);

    return true;
}

//Edit Player HP
bool ChatHandler::HandleModifyHPCommand(const char* args)
{
    if(!*args)
        return false;

    //    char* pHp = strtok((char*)args, " ");
    //    if (!pHp)
    //        return false;

    //    char* pHpMax = strtok(NULL, " ");
    //    if (!pHpMax)
    //        return false;

    //    int32 hpm = atoi(pHpMax);
    //    int32 hp = atoi(pHp);

    int32 hp = atoi((char*)args);
    int32 hpm = atoi((char*)args);

    if (hp <= 0 || hpm <= 0 || hpm < hp)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    // maxing values since being at the max of an int32 can cause problem when healing
    if(hp > 500000000)
        hp = 500000000;
    if(hpm > 500000000)
        hpm = 500000000;            

    Player *chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_HP, chr->GetName(), hp, hpm);
    if (needReportToTarget(chr))
        ChatHandler(chr).PSendSysMessage(LANG_YOURS_HP_CHANGED, GetName(), hp, hpm);

    chr->SetMaxHealth( hpm );
    chr->SetHealth( hp );

    return true;
}

//Edit Player Mana
bool ChatHandler::HandleModifyManaCommand(const char* args)
{
    if(!*args)
        return false;

    // char* pmana = strtok((char*)args, " ");
    // if (!pmana)
    //     return false;

    // char* pmanaMax = strtok(NULL, " ");
    // if (!pmanaMax)
    //     return false;

    // int32 manam = atoi(pmanaMax);
    // int32 mana = atoi(pmana);
    int32 mana = atoi((char*)args);
    int32 manam = atoi((char*)args);

    if (mana <= 0 || manam <= 0 || manam < mana)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Player *chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_MANA, chr->GetName(), mana, manam);
    if (needReportToTarget(chr))
        ChatHandler(chr).PSendSysMessage(LANG_YOURS_MANA_CHANGED, GetName(), mana, manam);

    chr->SetMaxPower(POWER_MANA,manam );
    chr->SetPower(POWER_MANA, mana );

    return true;
}

//Edit Player Energy
bool ChatHandler::HandleModifyEnergyCommand(const char* args)
{
    if(!*args)
        return false;

    // char* pmana = strtok((char*)args, " ");
    // if (!pmana)
    //     return false;

    // char* pmanaMax = strtok(NULL, " ");
    // if (!pmanaMax)
    //     return false;

    // int32 manam = atoi(pmanaMax);
    // int32 mana = atoi(pmana);

    int32 energy = atoi((char*)args)*10;
    int32 energym = atoi((char*)args)*10;

    if (energy <= 0 || energym <= 0 || energym < energy)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Player *chr = getSelectedPlayer();
    if (!chr)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_ENERGY, chr->GetName(), energy/10, energym/10);
    if (needReportToTarget(chr))
        ChatHandler(chr).PSendSysMessage(LANG_YOURS_ENERGY_CHANGED, GetName(), energy/10, energym/10);

    chr->SetMaxPower(POWER_ENERGY,energym );
    chr->SetPower(POWER_ENERGY, energy );

    sLog.outDetail(GetTrinityString(LANG_CURRENT_ENERGY),chr->GetMaxPower(POWER_ENERGY));

    return true;
}

//Edit Player Rage
bool ChatHandler::HandleModifyRageCommand(const char* args)
{
    if(!*args)
        return false;

    // char* pmana = strtok((char*)args, " ");
    // if (!pmana)
    //     return false;

    // char* pmanaMax = strtok(NULL, " ");
    // if (!pmanaMax)
    //     return false;

    // int32 manam = atoi(pmanaMax);
    // int32 mana = atoi(pmana);

    int32 rage = atoi((char*)args)*10;
    int32 ragem = atoi((char*)args)*10;

    if (rage <= 0 || ragem <= 0 || ragem < rage)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Player *chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_RAGE, chr->GetName(), rage/10, ragem/10);
    if (needReportToTarget(chr))
        ChatHandler(chr).PSendSysMessage(LANG_YOURS_RAGE_CHANGED, GetName(), rage/10, ragem/10);

    chr->SetMaxPower(POWER_RAGE,ragem );
    chr->SetPower(POWER_RAGE, rage );

    return true;
}

/* Edit unit Faction 
.modify faction #factionid [#UNIT_FIELD_FLAGS] [#UNIT_NPC_FLAGS] [#UNIT_DYNAMIC_FLAGS]
*/
bool ChatHandler::HandleModifyFactionCommand(const char* args)
{
    if(!*args)
        return false;

    Unit* u = getSelectedUnit();
    if(!u)
    {
        SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
        SetSentErrorMessage(true);
        return false;
    }

    char* pfactionid = extractKeyFromLink((char*)args,"Hfaction");

    if(!pfactionid) // just show info
    {
        uint32 factionid = u->getFaction();
        uint32 flag      = u->GetUInt32Value(UNIT_FIELD_FLAGS);
        uint32 npcflag   = u->GetUInt32Value(UNIT_NPC_FLAGS);
        uint32 dyflag    = u->GetUInt32Value(UNIT_DYNAMIC_FLAGS);
        PSendSysMessage(LANG_CURRENT_FACTION,u->GetGUIDLow(),factionid,flag,npcflag,dyflag);
        return true;
    }

    uint32 factionid = atoi(pfactionid);
    if(!factionid)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return true;
    }

    if(!sFactionTemplateStore.LookupEntry(factionid))
    {
        PSendSysMessage(LANG_WRONG_FACTION, factionid);
        SetSentErrorMessage(true);
        return false;
    }

    //player case only
    if (getSelectedPlayer()) 
    {
        u->setFaction(factionid);
        PSendSysMessage("You changed %s    's faction to %i", u->GetName(),factionid);
        return true;
    } 

    // else, creature case :
    uint32 flag;
    char *pflag = strtok(NULL, " ");
    if (!pflag)
        flag = u->GetUInt32Value(UNIT_FIELD_FLAGS);
    else
        flag = atoi(pflag);

    char* pnpcflag = strtok(NULL, " ");

    uint32 npcflag;
    if(!pnpcflag)
        npcflag   = u->GetUInt32Value(UNIT_NPC_FLAGS);
    else
        npcflag = atoi(pnpcflag);

    char* pdyflag = strtok(NULL, " ");

    uint32  dyflag;
    if(!pdyflag)
        dyflag   = u->GetUInt32Value(UNIT_DYNAMIC_FLAGS);
    else
        dyflag = atoi(pdyflag);

    PSendSysMessage(LANG_YOU_CHANGE_FACTION, u->GetGUIDLow(),factionid,flag,npcflag,dyflag);

    u->setFaction(factionid);
    u->SetUInt32Value(UNIT_FIELD_FLAGS,flag);
    u->SetUInt32Value(UNIT_NPC_FLAGS,npcflag);
    u->SetUInt32Value(UNIT_DYNAMIC_FLAGS,dyflag);

    return true;
}

//Edit Player Spell
bool ChatHandler::HandleModifySpellCommand(const char* args)
{
    if(!*args) return false;
    char* pspellflatid = strtok((char*)args, " ");
    if (!pspellflatid)
        return false;

    char* pop = strtok(NULL, " ");
    if (!pop)
        return false;

    char* pval = strtok(NULL, " ");
    if (!pval)
        return false;

    uint16 mark;

    char* pmark = strtok(NULL, " ");

    uint8 spellflatid = atoi(pspellflatid);
    uint8 op   = atoi(pop);
    uint16 val = atoi(pval);
    if(!pmark)
        mark = 65535;
    else
        mark = atoi(pmark);

    Player *chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_SPELLFLATID, spellflatid, val, mark, chr->GetName());
    if (needReportToTarget(chr))
        ChatHandler(chr).PSendSysMessage(LANG_YOURS_SPELLFLATID_CHANGED, GetName(), spellflatid, val, mark);

    WorldPacket data(SMSG_SET_FLAT_SPELL_MODIFIER, (1+1+2+2));
    data << uint8(spellflatid);
    data << uint8(op);
    data << uint16(val);
    data << uint16(mark);
    chr->GetSession()->SendPacket(&data);

    return true;
}

//Edit Player TP
bool ChatHandler::HandleModifyTalentCommand (const char* args)
{
    if (!*args)
        return false;

    int tp = atoi((char*)args);
    if (tp>0)
    {
        Player* player = getSelectedPlayer();
        if(!player)
        {
            SendSysMessage(LANG_NO_CHAR_SELECTED);
            SetSentErrorMessage(true);
            return false;
        }
        player->SetFreeTalentPoints(tp);
        return true;
    }
    return false;
}

//Enable On\OFF all taxi paths
bool ChatHandler::HandleTaxiCheatCommand(const char* args)
{
    if (!*args)
    {
        SendSysMessage(LANG_USE_BOL);
        SetSentErrorMessage(true);
        return false;
    }

    std::string argstr = (char*)args;

    Player *chr = getSelectedPlayer();
    if (!chr)
    {
        chr=m_session->GetPlayer();
    }

    if (argstr == "on")
    {
        chr->SetTaxiCheater(true);
        PSendSysMessage(LANG_YOU_GIVE_TAXIS, chr->GetName());
        if (needReportToTarget(chr))
            ChatHandler(chr).PSendSysMessage(LANG_YOURS_TAXIS_ADDED, GetName());
        return true;
    }

    if (argstr == "off")
    {
        chr->SetTaxiCheater(false);
        PSendSysMessage(LANG_YOU_REMOVE_TAXIS, chr->GetName());
        if (needReportToTarget(chr))
            ChatHandler(chr).PSendSysMessage(LANG_YOURS_TAXIS_REMOVED, GetName());

        return true;
    }

    SendSysMessage(LANG_USE_BOL);
    SetSentErrorMessage(true);
    return false;
}

//Edit Player Aspeed
bool ChatHandler::HandleModifyASpeedCommand(const char* args)
{
    if (!*args)
        return false;

    float ASpeed = (float)atof((char*)args);

    if (ASpeed > 30.f || ASpeed < 0.1f)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Player *chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    if(chr->isInFlight())
    {
        PSendSysMessage(LANG_CHAR_IN_FLIGHT,chr->GetName());
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_ASPEED, ASpeed, chr->GetName());
    if (needReportToTarget(chr))
        ChatHandler(chr).PSendSysMessage(LANG_YOURS_ASPEED_CHANGED, GetName(), ASpeed);

    chr->SetSpeed(MOVE_WALK,    ASpeed,true);
    chr->SetSpeed(MOVE_RUN,     ASpeed,true);
    chr->SetSpeed(MOVE_SWIM,    ASpeed,true);
    //chr->SetSpeed(MOVE_TURN,    ASpeed,true);
    chr->SetSpeed(MOVE_FLIGHT,     ASpeed,true);
    return true;
}

//Edit Player Speed
bool ChatHandler::HandleModifySpeedCommand(const char* args)
{
    if (!*args)
        return false;

    float Speed = (float)atof((char*)args);

    if (Speed > 30.0f || Speed < 0.1f)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Player *chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    if(chr->isInFlight())
    {
        PSendSysMessage(LANG_CHAR_IN_FLIGHT,chr->GetName());
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_SPEED, Speed, chr->GetName());
    if (needReportToTarget(chr))
        ChatHandler(chr).PSendSysMessage(LANG_YOURS_SPEED_CHANGED, GetName(), Speed);

    chr->SetSpeed(MOVE_RUN,Speed,true);

    return true;
}

//Edit Player Swim Speed
bool ChatHandler::HandleModifySwimCommand(const char* args)
{
    if (!*args)
        return false;

    float Swim = (float)atof((char*)args);

    if (Swim > 30.0f || Swim < 0.1f)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Player *chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    if(chr->isInFlight())
    {
        PSendSysMessage(LANG_CHAR_IN_FLIGHT,chr->GetName());
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_SWIM_SPEED, Swim, chr->GetName());
    if (needReportToTarget(chr))
        ChatHandler(chr).PSendSysMessage(LANG_YOURS_SWIM_SPEED_CHANGED, GetName(), Swim);

    chr->SetSpeed(MOVE_SWIM,Swim,true);

    return true;
}

//Edit Player Walk Speed
bool ChatHandler::HandleModifyBWalkCommand(const char* args)
{
    if (!*args)
        return false;

    float BSpeed = (float)atof((char*)args);

    if (BSpeed > 30.0f || BSpeed < 0.1f)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Player *chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    if(chr->isInFlight())
    {
        PSendSysMessage(LANG_CHAR_IN_FLIGHT,chr->GetName());
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_BACK_SPEED, BSpeed, chr->GetName());
    if (needReportToTarget(chr))
        ChatHandler(chr).PSendSysMessage(LANG_YOURS_BACK_SPEED_CHANGED, GetName(), BSpeed);

    chr->SetSpeed(MOVE_RUN_BACK,BSpeed,true);

    return true;
}

//Edit Player Fly
bool ChatHandler::HandleModifyFlyCommand(const char* args)
{
    if (!*args)
        return false;

    float FSpeed = (float)atof((char*)args);

    if (FSpeed > 30.0f || FSpeed < 0.1f)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Player *chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_FLY_SPEED, FSpeed, chr->GetName());
    if (needReportToTarget(chr))
        ChatHandler(chr).PSendSysMessage(LANG_YOURS_FLY_SPEED_CHANGED, GetName(), FSpeed);

    chr->SetSpeed(MOVE_FLIGHT,FSpeed,true);

    return true;
}

//Edit Player Scale
bool ChatHandler::HandleModifyScaleCommand(const char* args)
{
    if (!*args)
        return false;

    float Scale = (float)atof((char*)args);
    if (Scale > 30.0f || Scale <= 0.0f)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    Unit* u = getSelectedUnit();
    if (u == NULL)
    {
        SendSysMessage(LANG_SELECT_CHAR_OR_CREATURE);
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_YOU_CHANGE_SIZE, Scale, u->GetName());

    Player* p = getSelectedPlayer();
    if (p && needReportToTarget(p))
        ChatHandler(p).PSendSysMessage(LANG_YOURS_SIZE_CHANGED, GetName(), Scale);

    u->SetFloatValue(OBJECT_FIELD_SCALE_X, Scale);

    return true;
}

//Enable Player mount
bool ChatHandler::HandleModifyMountCommand(const char* args)
{
    if(!*args)
        return false;

    uint16 mId = 1147;
    float speed = (float)15;
    uint32 num = 0;

    num = atoi((char*)args);
    switch(num)
    {
        case 1:
            mId=14340;
            break;
        case 2:
            mId=4806;
            break;
        case 3:
            mId=6471;
            break;
        case 4:
            mId=12345;
            break;
        case 5:
            mId=6472;
            break;
        case 6:
            mId=6473;
            break;
        case 7:
            mId=10670;
            break;
        case 8:
            mId=10719;
            break;
        case 9:
            mId=10671;
            break;
        case 10:
            mId=10672;
            break;
        case 11:
            mId=10720;
            break;
        case 12:
            mId=14349;
            break;
        case 13:
            mId=11641;
            break;
        case 14:
            mId=12244;
            break;
        case 15:
            mId=12242;
            break;
        case 16:
            mId=14578;
            break;
        case 17:
            mId=14579;
            break;
        case 18:
            mId=14349;
            break;
        case 19:
            mId=12245;
            break;
        case 20:
            mId=14335;
            break;
        case 21:
            mId=207;
            break;
        case 22:
            mId=2328;
            break;
        case 23:
            mId=2327;
            break;
        case 24:
            mId=2326;
            break;
        case 25:
            mId=14573;
            break;
        case 26:
            mId=14574;
            break;
        case 27:
            mId=14575;
            break;
        case 28:
            mId=604;
            break;
        case 29:
            mId=1166;
            break;
        case 30:
            mId=2402;
            break;
        case 31:
            mId=2410;
            break;
        case 32:
            mId=2409;
            break;
        case 33:
            mId=2408;
            break;
        case 34:
            mId=2405;
            break;
        case 35:
            mId=14337;
            break;
        case 36:
            mId=6569;
            break;
        case 37:
            mId=10661;
            break;
        case 38:
            mId=10666;
            break;
        case 39:
            mId=9473;
            break;
        case 40:
            mId=9476;
            break;
        case 41:
            mId=9474;
            break;
        case 42:
            mId=14374;
            break;
        case 43:
            mId=14376;
            break;
        case 44:
            mId=14377;
            break;
        case 45:
            mId=2404;
            break;
        case 46:
            mId=2784;
            break;
        case 47:
            mId=2787;
            break;
        case 48:
            mId=2785;
            break;
        case 49:
            mId=2736;
            break;
        case 50:
            mId=2786;
            break;
        case 51:
            mId=14347;
            break;
        case 52:
            mId=14346;
            break;
        case 53:
            mId=14576;
            break;
        case 54:
            mId=9695;
            break;
        case 55:
            mId=9991;
            break;
        case 56:
            mId=6448;
            break;
        case 57:
            mId=6444;
            break;
        case 58:
            mId=6080;
            break;
        case 59:
            mId=6447;
            break;
        case 60:
            mId=4805;
            break;
        case 61:
            mId=9714;
            break;
        case 62:
            mId=6448;
            break;
        case 63:
            mId=6442;
            break;
        case 64:
            mId=14632;
            break;
        case 65:
            mId=14332;
            break;
        case 66:
            mId=14331;
            break;
        case 67:
            mId=8469;
            break;
        case 68:
            mId=2830;
            break;
        case 69:
            mId=2346;
            break;
        default:
            SendSysMessage(LANG_NO_MOUNT);
            SetSentErrorMessage(true);
            return false;
    }

    Player *chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    PSendSysMessage(LANG_YOU_GIVE_MOUNT, chr->GetName());
    if (needReportToTarget(chr))
        ChatHandler(chr).PSendSysMessage(LANG_MOUNT_GIVED, GetName());

    chr->SetUInt32Value( UNIT_FIELD_FLAGS , 0x001000 );
    chr->Mount(mId);

    WorldPacket data( SMSG_FORCE_RUN_SPEED_CHANGE, (8+4+1+4) );
    data.append(chr->GetPackGUID());
    data << (uint32)0;
    data << (uint8)0;                                       //new 2.1.0
    data << float(speed);
    chr->SendMessageToSet( &data, true );

    data.Initialize( SMSG_FORCE_SWIM_SPEED_CHANGE, (8+4+4) );
    data.append(chr->GetPackGUID());
    data << (uint32)0;
    data << float(speed);
    chr->SendMessageToSet( &data, true );

    return true;
}

//Edit Player money
bool ChatHandler::HandleModifyMoneyCommand(const char* args)
{
    if (!*args)
        return false;

    Player *chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    int32 addmoney = atoi((char*)args);

    uint32 moneyuser = chr->GetMoney();

    if(addmoney < 0)
    {
        int32 newmoney = moneyuser + addmoney;

        sLog.outDetail(GetTrinityString(LANG_CURRENT_MONEY), moneyuser, addmoney, newmoney);
        if(newmoney <= 0 )
        {
            PSendSysMessage(LANG_YOU_TAKE_ALL_MONEY, chr->GetName());
            if (needReportToTarget(chr))
                ChatHandler(chr).PSendSysMessage(LANG_YOURS_ALL_MONEY_GONE, GetName());

            chr->SetMoney(0);
        }
        else
        {
            PSendSysMessage(LANG_YOU_TAKE_MONEY, abs(addmoney), chr->GetName());
            if (needReportToTarget(chr))
                ChatHandler(chr).PSendSysMessage(LANG_YOURS_MONEY_TAKEN, GetName(), abs(addmoney));
            chr->SetMoney( newmoney );
        }
    }
    else
    {
        PSendSysMessage(LANG_YOU_GIVE_MONEY, addmoney, chr->GetName());
        if (needReportToTarget(chr))
            ChatHandler(chr).PSendSysMessage(LANG_YOURS_MONEY_GIVEN, GetName(), addmoney);
        chr->ModifyMoney( addmoney );
    }

    sLog.outDetail(GetTrinityString(LANG_NEW_MONEY), moneyuser, addmoney, chr->GetMoney() );

    return true;
}

//Edit Player field
bool ChatHandler::HandleModifyBitCommand(const char* args)
{
    if( !*args )
        return false;

    Player *chr = getSelectedPlayer();
    if (chr == NULL)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    char* pField = strtok((char*)args, " ");
    if (!pField)
        return false;

    char* pBit = strtok(NULL, " ");
    if (!pBit)
        return false;

    uint16 field = atoi(pField);
    uint32 bit   = atoi(pBit);

    if (field < 1 || field >= PLAYER_END)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    if (bit < 1 || bit > 32)
    {
        SendSysMessage(LANG_BAD_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    if ( chr->HasFlag( field, (1<<(bit-1)) ) )
    {
        chr->RemoveFlag( field, (1<<(bit-1)) );
        PSendSysMessage(LANG_REMOVE_BIT, bit, field);
    }
    else
    {
        chr->SetFlag( field, (1<<(bit-1)) );
        PSendSysMessage(LANG_SET_BIT, bit, field);
    }

    return true;
}

bool ChatHandler::HandleModifyHonorCommand (const char* args)
{
    if (!*args)
        return false;

    Player *target = getSelectedPlayer();
    if(!target)
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    int32 amount = (uint32)atoi(args);

    target->ModifyHonorPoints(amount);

    PSendSysMessage(LANG_COMMAND_MODIFY_HONOR, target->GetName(), target->GetHonorPoints());

    return true;
}

bool ChatHandler::HandleTeleCommand(const char * args)
{
    if(!*args)
        return false;

    Player* _player = m_session->GetPlayer();

    // id, or string, or [name] Shift-click form |color|Htele:id|h[name]|h|r
    GameTele const* tele = extractGameTeleFromLink((char*)args);

    if (!tele)
    {
        SendSysMessage(LANG_COMMAND_TELE_NOTFOUND);
        SetSentErrorMessage(true);
        return false;
    }

    MapEntry const * me = sMapStore.LookupEntry(tele->mapId);
    if(!me || me->IsBattleGroundOrArena())
    {
        SendSysMessage(LANG_CANNOT_TELE_TO_BG);
        SetSentErrorMessage(true);
        return false;
    }

    // stop flight if need
    if(_player->isInFlight())
    {
        _player->GetMotionMaster()->MovementExpired();
        _player->CleanupAfterTaxiFlight();
    }
    // save only in non-flight case
    else
        _player->SaveRecallPosition();

    _player->TeleportTo(tele->mapId, tele->position_x, tele->position_y, tele->position_z, tele->orientation);
    return true;
}

bool ChatHandler::HandleLookupAreaCommand(const char* args)
{
    if (!*args)
        return false;

    std::string namepart = args;
    std::wstring wnamepart;

    if (!Utf8toWStr (namepart,wnamepart))
        return false;

    uint32 counter = 0;                                     // Counter for figure out that we found smth.

    // converting string that we try to find to lower case
    wstrToLower (wnamepart);

    // Search in AreaTable.dbc
    for (uint32 areaflag = 0; areaflag < sAreaStore.GetNumRows (); ++areaflag)
    {
        AreaTableEntry const *areaEntry = sAreaStore.LookupEntry (areaflag);
        if (areaEntry)
        {
            int loc = m_session ? m_session->GetSessionDbcLocale () : sWorld.GetDefaultDbcLocale();
            std::string name = areaEntry->area_name[loc];
            if (name.empty())
                continue;

            if (!Utf8FitTo (name, wnamepart))
            {
                loc = 0;
                for(; loc < MAX_LOCALE; ++loc)
                {
                    if (m_session && loc==m_session->GetSessionDbcLocale ())
                        continue;

                    name = areaEntry->area_name[loc];
                    if (name.empty ())
                        continue;

                    if (Utf8FitTo (name, wnamepart))
                        break;
                }
            }

            if (loc < MAX_LOCALE)
            {
                // send area in "id - [name]" format
                std::ostringstream ss;
                if (m_session)
                    ss << areaEntry->ID << " - |cffffffff|Harea:" << areaEntry->ID << "|h[" << name << " " << localeNames[loc]<< "]|h|r";
                else
                    ss << areaEntry->ID << " - " << name << " " << localeNames[loc];

                SendSysMessage (ss.str ().c_str());

                ++counter;
            }
        }
    }
    if (counter == 0)                                      // if counter == 0 then we found nth
        SendSysMessage (LANG_COMMAND_NOAREAFOUND);
    return true;
}

//Find tele in game_tele order by name
bool ChatHandler::HandleLookupTeleCommand(const char * args)
{
    if(!*args)
    {
        SendSysMessage(LANG_COMMAND_TELE_PARAMETER);
        SetSentErrorMessage(true);
        return false;
    }

    char const* str = strtok((char*)args, " ");
    if(!str)
        return false;

    std::string namepart = str;
    std::wstring wnamepart;

    if(!Utf8toWStr(namepart,wnamepart))
        return false;

    // converting string that we try to find to lower case
    wstrToLower( wnamepart );

    std::ostringstream reply;

    GameTeleMap const & teleMap = objmgr.GetGameTeleMap();
    for(GameTeleMap::const_iterator itr = teleMap.begin(); itr != teleMap.end(); ++itr)
    {
        GameTele const* tele = &itr->second;

        if(tele->wnameLow.find(wnamepart) == std::wstring::npos)
            continue;

        if (m_session)
            reply << "  |cffffffff|Htele:" << itr->first << "|h[" << tele->name << "]|h|r\n";
        else
            reply << "  " << itr->first << " " << tele->name << "\n";
    }

    if(reply.str().empty())
        SendSysMessage(LANG_COMMAND_TELE_NOLOCATION);
    else
        PSendSysMessage(LANG_COMMAND_TELE_LOCATION,reply.str().c_str());

    return true;
}

//Enable\Dissable accept whispers (for GM)
bool ChatHandler::HandleWhispersCommand(const char* args)
{
    if(!*args)
    {
        PSendSysMessage(LANG_COMMAND_WHISPERACCEPTING, m_session->GetPlayer()->isAcceptWhispers() ?  GetTrinityString(LANG_ON) : GetTrinityString(LANG_OFF));
        return true;
    }

    std::string argstr = (char*)args;
    // whisper on
    if (argstr == "on")
    {
        m_session->GetPlayer()->SetAcceptWhispers(true);
        SendSysMessage(LANG_COMMAND_WHISPERON);
        return true;
    }

    // whisper off
    if (argstr == "off")
    {
        m_session->GetPlayer()->SetAcceptWhispers(false);
        SendSysMessage(LANG_COMMAND_WHISPEROFF);
        return true;
    }

    SendSysMessage(LANG_USE_BOL);
    SetSentErrorMessage(true);
    return false;
}

//Play sound
bool ChatHandler::HandlePlaySoundCommand(const char* args)
{
    // USAGE: .debug playsound #soundid
    // #soundid - ID decimal number from SoundEntries.dbc (1st column)
    // this file have about 5000 sounds.
    // In this realization only caller can hear this sound.
    if( *args )
    {
        uint32 dwSoundId = atoi((char*)args);

        if( !sSoundEntriesStore.LookupEntry(dwSoundId) )
        {
            PSendSysMessage(LANG_SOUND_NOT_EXIST, dwSoundId);
            SetSentErrorMessage(true);
            return false;
        }

        WorldPacket data(SMSG_PLAY_OBJECT_SOUND,4+8);
        data << uint32(dwSoundId) << m_session->GetPlayer()->GetGUID();
        m_session->SendPacket(&data);

        PSendSysMessage(LANG_YOU_HEAR_SOUND, dwSoundId);
        return true;
    }

    return false;
}

//Save all players in the world
bool ChatHandler::HandleSaveAllCommand(const char* /*args*/)
{
    ObjectAccessor::Instance().SaveAllPlayers();
    SendSysMessage(LANG_PLAYERS_SAVED);
    return true;
}

//Send mail by command
bool ChatHandler::HandleSendMailCommand(const char* args)
{
    if(!*args)
        return false;

    // format: name "subject text" "mail text"

    char* pName = strtok((char*)args, " ");
    if(!pName)
        return false;

    char* tail1 = strtok(NULL, "");
    if(!tail1)
        return false;

    char* msgSubject;
    if(*tail1=='"')
        msgSubject = strtok(tail1+1, "\"");
    else
    {
        char* space = strtok(tail1, "\"");
        if(!space)
            return false;
        msgSubject = strtok(NULL, "\"");
    }

    if (!msgSubject)
        return false;

    char* tail2 = strtok(NULL, "");
    if(!tail2)
        return false;

    char* msgText;
    if(*tail2=='"')
        msgText = strtok(tail2+1, "\"");
    else
    {
        char* space = strtok(tail2, "\"");
        if(!space)
            return false;
        msgText = strtok(NULL, "\"");
    }

    if (!msgText)
        return false;

    // pName, msgSubject, msgText isn't NUL after prev. check
    std::string name    = pName;
    std::string subject = msgSubject;
    std::string text    = msgText;

    if(!normalizePlayerName(name))
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    uint64 receiver_guid = objmgr.GetPlayerGUIDByName(name);
    if(!receiver_guid)
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    uint32 mailId = objmgr.GenerateMailID();
    // from console show not existed sender
    uint32 sender_guidlo = m_session ? m_session->GetPlayer()->GetGUIDLow() : 0;

    uint32 messagetype = MAIL_NORMAL;
    uint32 stationery = MAIL_STATIONERY_GM;
    uint32 itemTextId = !text.empty() ? objmgr.CreateItemText( text ) : 0;

    Player *receiver = objmgr.GetPlayer(receiver_guid);

    WorldSession::SendMailTo(receiver,messagetype, stationery, sender_guidlo, GUID_LOPART(receiver_guid), subject, itemTextId, NULL, 0, 0, MAIL_CHECK_MASK_NONE);

    PSendSysMessage(LANG_MAIL_SENT, name.c_str());
    return true;
}

// teleport player to given game_tele.entry
bool ChatHandler::HandleNameTeleCommand(const char * args)
{
    if(!*args)
        return false;

    char* pName = strtok((char*)args, " ");

    if(!pName)
        return false;

    std::string name = pName;

    if(!normalizePlayerName(name))
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    char* tail = strtok(NULL, "");
    if(!tail)
        return false;

    // id, or string, or [name] Shift-click form |color|Htele:id|h[name]|h|r
    GameTele const* tele = extractGameTeleFromLink(tail);
    if(!tele)
    {
        SendSysMessage(LANG_COMMAND_TELE_NOTFOUND);
        SetSentErrorMessage(true);
        return false;
    }

    MapEntry const * me = sMapStore.LookupEntry(tele->mapId);
    if(!me || me->IsBattleGroundOrArena())
    {
        SendSysMessage(LANG_CANNOT_TELE_TO_BG);
        SetSentErrorMessage(true);
        return false;
    }

    Player *chr = objmgr.GetPlayer(name.c_str());
    if (chr)
    {

        if(chr->IsBeingTeleported()==true)
        {
            PSendSysMessage(LANG_IS_TELEPORTED, chr->GetName());
            SetSentErrorMessage(true);
            return false;
        }

        PSendSysMessage(LANG_TELEPORTING_TO, chr->GetName(),"", tele->name.c_str());
        if (needReportToTarget(chr))
            ChatHandler(chr).PSendSysMessage(LANG_TELEPORTED_TO_BY, GetName());

        // stop flight if need
        if(chr->isInFlight())
        {
            chr->GetMotionMaster()->MovementExpired();
            chr->CleanupAfterTaxiFlight();
        }
        // save only in non-flight case
        else
            chr->SaveRecallPosition();

        chr->TeleportTo(tele->mapId,tele->position_x,tele->position_y,tele->position_z,tele->orientation);
    }
    else if (uint64 guid = objmgr.GetPlayerGUIDByName(name.c_str()))
    {
        PSendSysMessage(LANG_TELEPORTING_TO, name.c_str(), GetTrinityString(LANG_OFFLINE), tele->name.c_str());
        Player::SavePositionInDB(tele->mapId,tele->position_x,tele->position_y,tele->position_z,tele->orientation,MapManager::Instance().GetZoneId(tele->mapId,tele->position_x,tele->position_y,tele->position_z),guid);
    }
    else
        PSendSysMessage(LANG_NO_PLAYER, name.c_str());

    return true;
}

//Teleport group to given game_tele.entry
bool ChatHandler::HandleGroupTeleCommand(const char * args)
{
    if(!*args)
        return false;

    Player *player = getSelectedPlayer();
    if (!player)
    {
        SendSysMessage(LANG_NO_CHAR_SELECTED);
        SetSentErrorMessage(true);
        return false;
    }

    // id, or string, or [name] Shift-click form |color|Htele:id|h[name]|h|r
    GameTele const* tele = extractGameTeleFromLink((char*)args);
    if(!tele)
    {
        SendSysMessage(LANG_COMMAND_TELE_NOTFOUND);
        SetSentErrorMessage(true);
        return false;
    }

    MapEntry const * me = sMapStore.LookupEntry(tele->mapId);
    if(!me || me->IsBattleGroundOrArena())
    {
        SendSysMessage(LANG_CANNOT_TELE_TO_BG);
        SetSentErrorMessage(true);
        return false;
    }
    Group *grp = player->GetGroup();
    if(!grp)
    {
        PSendSysMessage(LANG_NOT_IN_GROUP,player->GetName());
        SetSentErrorMessage(true);
        return false;
    }

    for(GroupReference *itr = grp->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player *pl = itr->getSource();

        if(!pl || !pl->GetSession() )
            continue;

        if(pl->IsBeingTeleported())
        {
            PSendSysMessage(LANG_IS_TELEPORTED, pl->GetName());
            continue;
        }

        PSendSysMessage(LANG_TELEPORTING_TO, pl->GetName(),"", tele->name.c_str());
        if (needReportToTarget(pl))
            ChatHandler(pl).PSendSysMessage(LANG_TELEPORTED_TO_BY, GetName());

        // stop flight if need
        if(pl->isInFlight())
        {
            pl->GetMotionMaster()->MovementExpired();
            pl->CleanupAfterTaxiFlight();
        }
        // save only in non-flight case
        else
            pl->SaveRecallPosition();

        pl->TeleportTo(tele->mapId, tele->position_x, tele->position_y, tele->position_z, tele->orientation);
    }

    return true;
}

//Summon group of player
bool ChatHandler::HandleGroupgoCommand(const char* args)
{
    if(!*args)
        return false;

    std::string name = args;

    if(!normalizePlayerName(name))
    {
        SendSysMessage(LANG_PLAYER_NOT_FOUND);
        SetSentErrorMessage(true);
        return false;
    }

    Player *player = objmgr.GetPlayer(name.c_str());
    if (!player)
    {
        PSendSysMessage(LANG_NO_PLAYER, args);
        SetSentErrorMessage(true);
        return false;
    }

    Group *grp = player->GetGroup();

    if(!grp)
    {
        PSendSysMessage(LANG_NOT_IN_GROUP,player->GetName());
        SetSentErrorMessage(true);
        return false;
    }

    Map* gmMap = MapManager::Instance().GetMap(m_session->GetPlayer()->GetMapId(),m_session->GetPlayer());
    bool to_instance =  gmMap->Instanceable();

    // we are in instance, and can summon only player in our group with us as lead
    if ( to_instance && (
        !m_session->GetPlayer()->GetGroup() || (grp->GetLeaderGUID() != m_session->GetPlayer()->GetGUID()) ||
        (m_session->GetPlayer()->GetGroup()->GetLeaderGUID() != m_session->GetPlayer()->GetGUID()) ) )
        // the last check is a bit excessive, but let it be, just in case
    {
        SendSysMessage(LANG_CANNOT_SUMMON_TO_INST);
        SetSentErrorMessage(true);
        return false;
    }

    for(GroupReference *itr = grp->GetFirstMember(); itr != NULL; itr = itr->next())
    {
        Player *pl = itr->getSource();

        if(!pl || pl==m_session->GetPlayer() || !pl->GetSession() )
            continue;

        if(pl->IsBeingTeleported()==true)
        {
            PSendSysMessage(LANG_IS_TELEPORTED, pl->GetName());
            SetSentErrorMessage(true);
            return false;
        }

        if (to_instance)
        {
            Map* plMap = MapManager::Instance().GetMap(pl->GetMapId(),pl);

            if ( plMap->Instanceable() && plMap->GetInstanceId() != gmMap->GetInstanceId() )
            {
                // cannot summon from instance to instance
                PSendSysMessage(LANG_CANNOT_SUMMON_TO_INST,pl->GetName());
                SetSentErrorMessage(true);
                return false;
            }
        }

        PSendSysMessage(LANG_SUMMONING, pl->GetName(),"");
        if (needReportToTarget(pl))
            ChatHandler(pl).PSendSysMessage(LANG_SUMMONED_BY, GetName());

        // stop flight if need
        if(pl->isInFlight())
        {
            pl->GetMotionMaster()->MovementExpired();
            pl->CleanupAfterTaxiFlight();
        }
        // save only in non-flight case
        else
            pl->SaveRecallPosition();

        // before GM
        float x,y,z;
        m_session->GetPlayer()->GetClosePoint(x,y,z,pl->GetObjectSize());
        pl->TeleportTo(m_session->GetPlayer()->GetMapId(),x,y,z,pl->GetOrientation());
    }

    return true;
}

//teleport at coordinates
bool ChatHandler::HandleGoXYCommand(const char* args)
{
    if(!*args)
        return false;

    Player* _player = m_session->GetPlayer();

    char* px = strtok((char*)args, " ");
    char* py = strtok(NULL, " ");
    char* pmapid = strtok(NULL, " ");

    if (!px || !py)
        return false;

    float x = (float)atof(px);
    float y = (float)atof(py);
    uint32 mapid;
    if (pmapid)
        mapid = (uint32)atoi(pmapid);
    else mapid = _player->GetMapId();

    if(!MapManager::IsValidMapCoord(mapid,x,y))
    {
        PSendSysMessage(LANG_INVALID_TARGET_COORD,x,y,mapid);
        SetSentErrorMessage(true);
        return false;
    }

    // stop flight if need
    if(_player->isInFlight())
    {
        _player->GetMotionMaster()->MovementExpired();
        _player->CleanupAfterTaxiFlight();
    }
    // save only in non-flight case
    else
        _player->SaveRecallPosition();

    Map const *map = MapManager::Instance().GetBaseMap(mapid);
    float z = std::max(map->GetHeight(x, y, MAX_HEIGHT), map->GetWaterLevel(x, y));

    _player->TeleportTo(mapid, x, y, z, _player->GetOrientation());

    return true;
}

//teleport at coordinates, including Z
bool ChatHandler::HandleGoXYZCommand(const char* args)
{
    if(!*args)
        return false;

    Player* _player = m_session->GetPlayer();

    char* px = strtok((char*)args, " ");
    char* py = strtok(NULL, " ");
    char* pz = strtok(NULL, " ");
    char* pmapid = strtok(NULL, " ");

    if (!px || !py || !pz)
        return false;

    float x = (float)atof(px);
    float y = (float)atof(py);
    float z = (float)atof(pz);
    uint32 mapid;
    if (pmapid)
        mapid = (uint32)atoi(pmapid);
    else
        mapid = _player->GetMapId();

    if(!MapManager::IsValidMapCoord(mapid,x,y,z))
    {
        PSendSysMessage(LANG_INVALID_TARGET_COORD,x,y,mapid);
        SetSentErrorMessage(true);
        return false;
    }

    // stop flight if need
    if(_player->isInFlight())
    {
        _player->GetMotionMaster()->MovementExpired();
        _player->CleanupAfterTaxiFlight();
    }
    // save only in non-flight case
    else
        _player->SaveRecallPosition();

    _player->TeleportTo(mapid, x, y, z, _player->GetOrientation());

    return true;
}

//teleport at coordinates
bool ChatHandler::HandleGoZoneXYCommand(const char* args)
{
    if(!*args)
        return false;

    Player* _player = m_session->GetPlayer();

    char* px = strtok((char*)args, " ");
    char* py = strtok(NULL, " ");
    char* tail = strtok(NULL,"");

    char* cAreaId = extractKeyFromLink(tail,"Harea");       // string or [name] Shift-click form |color|Harea:area_id|h[name]|h|r

    if (!px || !py)
        return false;

    float x = (float)atof(px);
    float y = (float)atof(py);
    uint32 areaid = cAreaId ? (uint32)atoi(cAreaId) : _player->GetZoneId();

    AreaTableEntry const* areaEntry = GetAreaEntryByAreaID(areaid);

    if( x<0 || x>100 || y<0 || y>100 || !areaEntry )
    {
        PSendSysMessage(LANG_INVALID_ZONE_COORD,x,y,areaid);
        SetSentErrorMessage(true);
        return false;
    }

    // update to parent zone if exist (client map show only zones without parents)
    AreaTableEntry const* zoneEntry = areaEntry->zone ? GetAreaEntryByAreaID(areaEntry->zone) : areaEntry;

    Map const *map = MapManager::Instance().GetBaseMap(zoneEntry->mapid);

    if(map->Instanceable())
    {
        PSendSysMessage(LANG_INVALID_ZONE_MAP,areaEntry->ID,areaEntry->area_name[m_session->GetSessionDbcLocale()],map->GetId(),map->GetMapName());
        SetSentErrorMessage(true);
        return false;
    }

    Zone2MapCoordinates(x,y,zoneEntry->ID);

    if(!MapManager::IsValidMapCoord(zoneEntry->mapid,x,y))
    {
        PSendSysMessage(LANG_INVALID_TARGET_COORD,x,y,zoneEntry->mapid);
        SetSentErrorMessage(true);
        return false;
    }

    // stop flight if need
    if(_player->isInFlight())
    {
        _player->GetMotionMaster()->MovementExpired();
        _player->CleanupAfterTaxiFlight();
    }
    // save only in non-flight case
    else
        _player->SaveRecallPosition();

    float z = std::max(map->GetHeight(x, y, MAX_HEIGHT), map->GetWaterLevel(x, y));
    _player->TeleportTo(zoneEntry->mapid, x, y, z, _player->GetOrientation());

    return true;
}

//teleport to grid
bool ChatHandler::HandleGoGridCommand(const char* args)
{
    if(!*args)    return false;
    Player* _player = m_session->GetPlayer();

    char* px = strtok((char*)args, " ");
    char* py = strtok(NULL, " ");
    char* pmapid = strtok(NULL, " ");

    if (!px || !py)
        return false;

    float grid_x = (float)atof(px);
    float grid_y = (float)atof(py);
    uint32 mapid;
    if (pmapid)
        mapid = (uint32)atoi(pmapid);
    else mapid = _player->GetMapId();

    // center of grid
    float x = (grid_x-CENTER_GRID_ID+0.5f)*SIZE_OF_GRIDS;
    float y = (grid_y-CENTER_GRID_ID+0.5f)*SIZE_OF_GRIDS;

    if(!MapManager::IsValidMapCoord(mapid,x,y))
    {
        PSendSysMessage(LANG_INVALID_TARGET_COORD,x,y,mapid);
        SetSentErrorMessage(true);
        return false;
    }

    // stop flight if need
    if(_player->isInFlight())
    {
        _player->GetMotionMaster()->MovementExpired();
        _player->CleanupAfterTaxiFlight();
    }
    // save only in non-flight case
    else
        _player->SaveRecallPosition();

    Map const *map = MapManager::Instance().GetBaseMap(mapid);
    float z = std::max(map->GetHeight(x, y, MAX_HEIGHT), map->GetWaterLevel(x, y));
    _player->TeleportTo(mapid, x, y, z, _player->GetOrientation());

    return true;
}

bool ChatHandler::HandleDrunkCommand(const char* args)
{
    if(!*args)    return false;

    uint32 drunklevel = (uint32)atoi(args);
    if(drunklevel > 100)
        drunklevel = 100;

    uint16 drunkMod = drunklevel * 0xFFFF / 100;

    //m_session->GetPlayer()->SetDrunkValue(drunkMod);
    Unit *pUnit = getSelectedUnit();
    Player *plr;
    if (pUnit && pUnit->GetTypeId() == TYPEID_PLAYER)
        plr = reinterpret_cast<Player*>(pUnit);
    else
        plr = m_session->GetPlayer();
        
    plr->SetDrunkValue(drunkMod);

    return true;
}

bool ChatHandler::HandleNpcGuidCommand(const char* args)
{
    Creature* target = getSelectedCreature();
    if (!target)
        return false;
        
    PSendSysMessage("GUID: %u", target->GetDBTableGUIDLow());
    
    return true;
}

bool ChatHandler::HandleBlinkCommand(const char* args)
{
    uint32 distance = 0;

    if(args)
        distance = (uint32)atoi(args);

    if(!distance)
        distance = 15;

    Player* player = m_session->GetPlayer();
    if(!player)
        return true;

    float currentX, currentY, currentZ, currentO;
    player->GetPosition(currentX, currentY, currentZ);
    currentO = player->GetOrientation();

    float newX = currentX + cos(currentO) * distance;
    float newY = currentY + sin(currentO) * distance;
    float newZ = currentZ;

    player->TeleportTo(m_session->GetPlayer()->GetMapId(),newX,newY,newZ,currentO);

    return true;
}