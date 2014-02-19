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
#include "Log.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "World.h"
#include "Opcodes.h"
#include "ObjectMgr.h"
#include "Chat.h"
#include "Database/DatabaseEnv.h"
#include "ChannelMgr.h"
#include "Group.h"
#include "Guild.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "ScriptCalls.h"
#include "Player.h"
#include "SpellAuras.h"
#include "Language.h"
#include "Util.h"
#include "../scripts/ScriptMgr.h"
#include "IRCMgr.h"
#include "Config/ConfigEnv.h"

void WorldSession::HandleMessagechatOpcode( WorldPacket & recv_data )
{
    PROFILE;
    
    CHECK_PACKET_SIZE(recv_data,4+4+1);

    uint32 type;
    uint32 lang;

    recv_data >> type;
    recv_data >> lang;

    if(type >= MAX_CHAT_MSG_TYPE)
    {
        sLog.outError("CHAT: Wrong message type received: %u", type);
        return;
    }

    //sLog.outDebug("CHAT: packet received. type %u, lang %u", type, lang );

    // prevent talking at unknown language (cheating)
    LanguageDesc const* langDesc = GetLanguageDescByID(lang);
    if(!langDesc)
    {
        SendNotification(LANG_UNKNOWN_LANGUAGE);
        return;
    }
    if(langDesc->skill_id != 0 && !_player->HasSkill(langDesc->skill_id))
    {
        // also check SPELL_AURA_COMPREHEND_LANGUAGE (client offers option to speak in that language)
        Unit::AuraList const& langAuras = _player->GetAurasByType(SPELL_AURA_COMPREHEND_LANGUAGE);
        bool foundAura = false;
        for(Unit::AuraList::const_iterator i = langAuras.begin();i != langAuras.end(); ++i)
        {
            if((*i)->GetModifier()->m_miscvalue == lang)
            {
                foundAura = true;
                break;
            }
        }
        if(!foundAura)
        {
            SendNotification(LANG_NOT_LEARNED_LANGUAGE);
            return;
        }
    }

    if(lang == LANG_ADDON)
    {
        // Disabled addon channel?
        if(!sWorld.getConfig(CONFIG_ADDON_CHANNEL))
            return;
    }
    // LANG_ADDON should not be changed nor be affected by flood control
    else
    {
        // send in universal language if player in .gmon mode (ignore spell effects)
        if (_player->isGameMaster())
            lang = LANG_UNIVERSAL;
        else
        {
            // send in universal language in two side iteration allowed mode
            if (sWorld.getConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_CHAT))
                lang = LANG_UNIVERSAL;
            else
            {
                switch(type)
                {
                    case CHAT_MSG_PARTY:
                    case CHAT_MSG_RAID:
                    case CHAT_MSG_RAID_LEADER:
                    case CHAT_MSG_RAID_WARNING:
                        // allow two side chat at group channel if two side group allowed
                        if(sWorld.getConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GROUP))
                            lang = LANG_UNIVERSAL;
                        break;
                    case CHAT_MSG_GUILD:
                    case CHAT_MSG_OFFICER:
                        // allow two side chat at guild channel if two side guild allowed
                        if(sWorld.getConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD))
                            lang = LANG_UNIVERSAL;
                        break;
                }
            }

            // but overwrite it by SPELL_AURA_MOD_LANGUAGE auras (only single case used)
            Unit::AuraList const& ModLangAuras = _player->GetAurasByType(SPELL_AURA_MOD_LANGUAGE);
            if(!ModLangAuras.empty())
                lang = ModLangAuras.front()->GetModifier()->m_miscvalue;
        }

        //Flood control for these channels only
        if (type == CHAT_MSG_SAY
        || type == CHAT_MSG_YELL
        || type == CHAT_MSG_EMOTE
        || type == CHAT_MSG_TEXT_EMOTE
        || type == CHAT_MSG_CHANNEL
        || type == CHAT_MSG_BG_SYSTEM_NEUTRAL
        || type == CHAT_MSG_BG_SYSTEM_ALLIANCE
        || type == CHAT_MSG_BG_SYSTEM_HORDE
        || type == CHAT_MSG_BATTLEGROUND
        || type == CHAT_MSG_BATTLEGROUND_LEADER)
        {
            GetPlayer()->UpdateSpeakTime();
            if ( !_player->CanSpeak() )
            {
                std::string timeStr = secsToTimeString(m_muteTime - time(NULL));
                SendNotification(GetTrinityString(LANG_WAIT_BEFORE_SPEAKING),timeStr.c_str());
                return;
            }
        }
    }

   if (GetPlayer()->HasAura(1852,0) && type != CHAT_MSG_WHISPER)
    {
        std::string msg="";
        recv_data >> msg;
        if (ChatHandler(this).ParseCommands(msg.c_str()) == 0)
        {
            SendNotification(GetTrinityString(LANG_GM_SILENCE), GetPlayer()->GetName());
            return;
        }
    }

    switch(type)
    {
        case CHAT_MSG_SAY:
        case CHAT_MSG_EMOTE:
        case CHAT_MSG_YELL:
        {
            std::string msg = "";
            recv_data >> msg;

            if(msg.empty())
                break;

            if (ChatHandler(this).ParseCommands(msg.c_str()) > 0)
                break;
            
            if (GetPlayer()->isSpectator())
            {
                SendNotification("Vous ne pouvez pas effectuer cette action lorsque vous êtes spectateur");
                return;
            }

            // strip invisible characters for non-addon messages
            if (lang != LANG_ADDON && sWorld.getConfig(CONFIG_CHAT_FAKE_MESSAGE_PREVENTING))
                stripLineInvisibleChars(msg);

            if(msg.empty())
                break;
                
            if (strncmp(msg.c_str(), "|cff", 4) == 0) {
                char* cEntry = ChatHandler(GetPlayer()).extractKeyFromLink(((char*)msg.c_str()), "Hitem");
                if (cEntry) {
                    if (uint32 entry = atoi(cEntry)) {
                        ItemPrototype const* proto = objmgr.GetItemPrototype(entry);
                        if (!proto)
                            break;
                    }
                    else
                        break;
                }
            }

            if (sWorld.IsPhishing(msg)) {
                sWorld.LogPhishing(GetPlayer()->GetGUIDLow(), 0, msg);
                break;
            }

            if(type == CHAT_MSG_SAY)
                GetPlayer()->Say(msg, lang);
            else if(type == CHAT_MSG_EMOTE)
                GetPlayer()->TextEmote(msg);
            else if(type == CHAT_MSG_YELL)
                GetPlayer()->Yell(msg, lang);
        } break;

        case CHAT_MSG_WHISPER:
        {
            std::string to, msg;
            recv_data >> to;
            CHECK_PACKET_SIZE(recv_data,4+4+(to.size()+1)+1);
            recv_data >> msg;

            // strip invisible characters for non-addon messages
            if (lang != LANG_ADDON && sWorld.getConfig(CONFIG_CHAT_FAKE_MESSAGE_PREVENTING))
                stripLineInvisibleChars(msg);

            if(msg.empty())
                break;
                
            if (ChatHandler(this).ParseCommands(msg.c_str()) > 0)
                break;

            if (strncmp(msg.c_str(), "|cff", 4) == 0) {
                char* cEntry = ChatHandler(GetPlayer()).extractKeyFromLink(((char*)msg.c_str()), "Hitem");
                if (cEntry) {
                    if (uint32 entry = atoi(cEntry)) {
                        ItemPrototype const* proto = objmgr.GetItemPrototype(entry);
                        if (!proto)
                            break;
                    }
                    else
                        break;
                }
            }

            if(!normalizePlayerName(to))
            {
                WorldPacket data(SMSG_CHAT_PLAYER_NOT_FOUND, (to.size()+1));
                data<<to;
                SendPacket(&data);
                break;
            }

            Player *player = objmgr.GetPlayer(to.c_str());
            uint32 tSecurity = GetSecurity();
            uint32 pSecurity = player ? player->GetSession()->GetSecurity() : 0;
            if(!player || tSecurity == SEC_PLAYER && pSecurity > SEC_PLAYER && !player->isAcceptWhispers())
            {
                WorldPacket data(SMSG_CHAT_PLAYER_NOT_FOUND, (to.size()+1));
                data<<to;
                SendPacket(&data);
                return;
            }

            //can still whisper GM's
            if (pSecurity <= SEC_PLAYER && 
                GetPlayer()->GetSession()->GetSecurity() <= SEC_PLAYER && 
                GetPlayer()->getLevel() < sWorld.getConfig(CONFIG_WHISPER_MINLEVEL) && 
                lang != LANG_ADDON &&
                GetPlayer()->GetTotalPlayedTime() < DAY)
            {
                ChatHandler(this).PSendSysMessage("Vous devez atteindre le niveau %u ou avoir un temps de jeu total de 24h pour pouvoir chuchoter aux autres joueurs.", sWorld.getConfig(CONFIG_WHISPER_MINLEVEL));
                break;
            }

            if (sWorld.IsPhishing(msg)) {
                sWorld.LogPhishing(GetPlayer()->GetGUIDLow(), player->GetGUIDLow(), msg);
                break;
            }

            if (!sWorld.getConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_CHAT) && tSecurity == SEC_PLAYER && pSecurity == SEC_PLAYER )
            {
                uint32 sidea = GetPlayer()->GetTeam();
                uint32 sideb = player->GetTeam();
                if( sidea != sideb )
                {
                    WorldPacket data(SMSG_CHAT_PLAYER_NOT_FOUND, (to.size()+1));
                    data<<to;
                    SendPacket(&data);
                    return;
                }
            }

            if (GetPlayer()->HasAura(1852,0) && !player->isGameMaster())
            {
                SendNotification(GetTrinityString(LANG_GM_SILENCE), GetPlayer()->GetName());
                return;
            }

            GetPlayer()->Whisper(msg, lang,player->GetGUID());
        } break;

        case CHAT_MSG_PARTY:
        {
            std::string msg = "";
            recv_data >> msg;

            if(msg.empty())
                break;

            if (ChatHandler(this).ParseCommands(msg.c_str()) > 0)
                break;

            // strip invisible characters for non-addon messages
            if (lang != LANG_ADDON && sWorld.getConfig(CONFIG_CHAT_FAKE_MESSAGE_PREVENTING))
                stripLineInvisibleChars(msg);

            if(msg.empty())
                break;

            if (sWorld.IsPhishing(msg)) {
                sWorld.LogPhishing(GetPlayer()->GetGUIDLow(), 0, msg);
                break;
            }

            // if player is in battleground, he cannot say to battleground members by /p
            Group *group = GetPlayer()->GetOriginalGroup();
            // so if player hasn't OriginalGroup and his player->GetGroup() is BG raid, then return
            if (!group && (!(group = GetPlayer()->GetGroup()) || group->isBGGroup()))
                return;

            WorldPacket data;
            ChatHandler::FillMessageData(&data, this, CHAT_MSG_PARTY, lang, NULL, 0, msg.c_str(),NULL);
            group->BroadcastPacket(&data, false, group->GetMemberGroup(GetPlayer()->GetGUID()));
        }
        break;
        case CHAT_MSG_GUILD:
        {
            std::string msg = "";
            recv_data >> msg;

            if(msg.empty())
                break;

            if (ChatHandler(this).ParseCommands(msg.c_str()) > 0)
                break;

            // strip invisible characters for non-addon messages
            if (lang != LANG_ADDON && sWorld.getConfig(CONFIG_CHAT_FAKE_MESSAGE_PREVENTING))
                stripLineInvisibleChars(msg);

            if(msg.empty())
                break;

            if (GetPlayer()->GetGuildId())
            {
                Guild *guild = objmgr.GetGuildById(GetPlayer()->GetGuildId());
                if (guild) {
                    guild->BroadcastToGuild(this, msg, lang == LANG_ADDON ? LANG_ADDON : LANG_UNIVERSAL);
                    if (sWorld.getConfig(CONFIG_IRC_ENABLED) && lang != LANG_ADDON)
                        sIRCMgr.onIngameGuildMessage(guild->GetId(), _player->GetName(), msg.c_str());
                }
            }

            break;
        }
        case CHAT_MSG_OFFICER:
        {
            std::string msg = "";
            recv_data >> msg;

            if(msg.empty())
                break;

            if (ChatHandler(this).ParseCommands(msg.c_str()) > 0)
                break;

            // strip invisible characters for non-addon messages
            if (lang != LANG_ADDON && sWorld.getConfig(CONFIG_CHAT_FAKE_MESSAGE_PREVENTING))
                stripLineInvisibleChars(msg);

            if(msg.empty())
                break;

            if (GetPlayer()->GetGuildId())
            {
                Guild *guild = objmgr.GetGuildById(GetPlayer()->GetGuildId());
                if (guild)
                    guild->BroadcastToOfficers(this, msg, lang == LANG_ADDON ? LANG_ADDON : LANG_UNIVERSAL);
            }
            break;
        }
        case CHAT_MSG_RAID:
        {
            std::string msg="";
            recv_data >> msg;

            if(msg.empty())
                break;

            if (ChatHandler(this).ParseCommands(msg.c_str()) > 0)
                break;

            // strip invisible characters for non-addon messages
            if (lang != LANG_ADDON && sWorld.getConfig(CONFIG_CHAT_FAKE_MESSAGE_PREVENTING))
                stripLineInvisibleChars(msg);

            if(msg.empty())
                break;

            // if player is in battleground, he cannot say to battleground members by /raid
            Group *group = GetPlayer()->GetOriginalGroup();
            // so if player hasn't OriginalGroup and his player->GetGroup() is BG raid or his group isn't raid, then return
            if (!group && !(group = GetPlayer()->GetGroup()) || group->isBGGroup() || !group->isRaidGroup())
                return;

            WorldPacket data;
            ChatHandler::FillMessageData(&data, this, CHAT_MSG_RAID, lang, "", 0, msg.c_str(),NULL);
            group->BroadcastPacket(&data, false);
        } break;
        case CHAT_MSG_RAID_LEADER:
        {
            std::string msg="";
            recv_data >> msg;

            if(msg.empty())
                break;

            if (ChatHandler(this).ParseCommands(msg.c_str()) > 0)
                break;

            // strip invisible characters for non-addon messages
            if (lang != LANG_ADDON && sWorld.getConfig(CONFIG_CHAT_FAKE_MESSAGE_PREVENTING))
                stripLineInvisibleChars(msg);

            if(msg.empty())
                break;

            // if player is in battleground, he cannot say to battleground members by /raid
            Group *group = GetPlayer()->GetOriginalGroup();
            if (!group && !(group = GetPlayer()->GetGroup()) || group->isBGGroup() || !group->isRaidGroup() || !group->IsLeader(GetPlayer()->GetGUID()))
                return;

            WorldPacket data;
            ChatHandler::FillMessageData(&data, this, CHAT_MSG_RAID_LEADER, lang, "", 0, msg.c_str(),NULL);
            group->BroadcastPacket(&data, false);
        } break;
        case CHAT_MSG_RAID_WARNING:
        {
            std::string msg="";
            recv_data >> msg;

            // strip invisible characters for non-addon messages
            if (lang != LANG_ADDON && sWorld.getConfig(CONFIG_CHAT_FAKE_MESSAGE_PREVENTING))
                stripLineInvisibleChars(msg);

            if(msg.empty())
                break;

            if (ChatHandler(this).ParseCommands(msg.c_str()) > 0)
                break;

            Group *group = GetPlayer()->GetGroup();
            if(!group || !group->isRaidGroup() || !(group->IsLeader(GetPlayer()->GetGUID()) || group->IsAssistant(GetPlayer()->GetGUID())) || group->isBGGroup())
                return;

            WorldPacket data;
            // in battleground, raid warning is sent only to players in battleground - code is ok
            ChatHandler::FillMessageData(&data, this, CHAT_MSG_RAID_WARNING, lang, "", 0, msg.c_str(),NULL);
            group->BroadcastPacket(&data, false);
        } break;

        case CHAT_MSG_BATTLEGROUND:
        {
            std::string msg="";
            recv_data >> msg;

            // strip invisible characters for non-addon messages
            if (lang != LANG_ADDON && sWorld.getConfig(CONFIG_CHAT_FAKE_MESSAGE_PREVENTING))
                stripLineInvisibleChars(msg);

            if(msg.empty())
                break;

            if (ChatHandler(this).ParseCommands(msg.c_str()) > 0)
                break;

            //battleground raid is always in Player->GetGroup(), never in GetOriginalGroup()
            Group *group = GetPlayer()->GetGroup();
            if(!group || !group->isBGGroup())
                return;

            WorldPacket data;
            ChatHandler::FillMessageData(&data, this, CHAT_MSG_BATTLEGROUND, lang, "", 0, msg.c_str(),NULL);
            group->BroadcastPacket(&data, false);
        } break;

        case CHAT_MSG_BATTLEGROUND_LEADER:
        {
            std::string msg="";
            recv_data >> msg;

            // strip invisible characters for non-addon messages
            if (lang != LANG_ADDON && sWorld.getConfig(CONFIG_CHAT_FAKE_MESSAGE_PREVENTING))
                stripLineInvisibleChars(msg);

            if(msg.empty())
                break;

            if (ChatHandler(this).ParseCommands(msg.c_str()) > 0)
                break;

            //battleground raid is always in Player->GetGroup(), never in GetOriginalGroup()
            Group *group = GetPlayer()->GetGroup();
            if (!group || !group->isBGGroup() || !group->IsLeader(GetPlayer()->GetGUID()))
                return;

            WorldPacket data;
            ChatHandler::FillMessageData(&data, this, CHAT_MSG_BATTLEGROUND_LEADER, lang, "", 0, msg.c_str(),NULL);
            group->BroadcastPacket(&data, false);
        } break;

        case CHAT_MSG_CHANNEL:
        {
            std::string channel = "", msg = "";
            recv_data >> channel;

            // recheck
            CHECK_PACKET_SIZE(recv_data,4+4+(channel.size()+1)+1);

            recv_data >> msg;

            // strip invisible characters for non-addon messages
            if (lang != LANG_ADDON && sWorld.getConfig(CONFIG_CHAT_FAKE_MESSAGE_PREVENTING))
                stripLineInvisibleChars(msg);

            if(msg.empty())
                break;

            if (ChatHandler(this).ParseCommands(msg.c_str()) > 0)
                break;
                
            if (strncmp(msg.c_str(), "|cff", 4) == 0) {
                char* cEntry = ChatHandler(GetPlayer()).extractKeyFromLink(((char*)msg.c_str()), "Hitem");
                if (cEntry) {
                    if (uint32 entry = atoi(cEntry)) {
                        ItemPrototype const* proto = objmgr.GetItemPrototype(entry);
                        if (!proto)
                            break;
                    }
                    else
                        break;
                }
            }

            if (sWorld.IsPhishing(msg)) {
                sWorld.LogPhishing(GetPlayer()->GetGUIDLow(), 0, msg);
                break;
            }

            if(ChannelMgr* cMgr = channelMgr(_player->GetTeam()))
            {
                if(Channel *chn = cMgr->GetChannel(channel,_player))
                {
                    chn->Say(_player->GetGUID(),msg.c_str(),lang);
                    if (sWorld.getConfig(CONFIG_IRC_ENABLED) && lang != LANG_ADDON)
                    {
                        ChannelFaction faction = _player->GetTeam() == ALLIANCE ? CHAN_FACTION_ALLIANCE : CHAN_FACTION_HORDE;
                        sIRCMgr.onIngameChannelMessage(faction,channel.c_str(),_player->GetName(), msg.c_str());
                    }
                }
            }
        } break;

        case CHAT_MSG_AFK:
        {
            std::string msg;
            recv_data >> msg;

            if((msg.empty() || !_player->isAFK()) && !_player->IsInCombat() )
            {
                if(!_player->isAFK())
                {
                    if(msg.empty())
                        msg  = GetTrinityString(LANG_PLAYER_AFK_DEFAULT);
                    _player->afkMsg = msg;
                }
                _player->ToggleAFK();
                if(_player->isAFK() && _player->isDND())
                    _player->ToggleDND();
            }
        } break;

        case CHAT_MSG_DND:
        {
            std::string msg;
            recv_data >> msg;

            if(msg.empty() || !_player->isDND())
            {
                if(!_player->isDND())
                {
                    if(msg.empty())
                        msg  = GetTrinityString(LANG_PLAYER_DND_DEFAULT);
                    _player->dndMsg = msg;
                }
                _player->ToggleDND();
                if(_player->isDND() && _player->isAFK())
                    _player->ToggleAFK();
            }
        } break;

        default:
            sLog.outError("CHAT: unknown message type %u, lang: %u", type, lang);
            break;
    }
}

void WorldSession::HandleEmoteOpcode( WorldPacket & recv_data )
{
    PROFILE;
    
    if(!GetPlayer()->IsAlive())
        return;
    CHECK_PACKET_SIZE(recv_data,4);

    uint32 emote;
    recv_data >> emote;
    GetPlayer()->HandleEmoteCommand(emote);
}

void WorldSession::HandleTextEmoteOpcode( WorldPacket & recv_data )
{
    PROFILE;
    
    if(!GetPlayer()->IsAlive())
        return;

    if (!GetPlayer()->CanSpeak())
    {
        std::string timeStr = secsToTimeString(m_muteTime - time(NULL));
        SendNotification(GetTrinityString(LANG_WAIT_BEFORE_SPEAKING),timeStr.c_str());
        return;
    }

    CHECK_PACKET_SIZE(recv_data,4+4+8);

    uint32 text_emote, emoteNum;
    uint64 guid;

    recv_data >> text_emote;
    recv_data >> emoteNum;
    recv_data >> guid;

    const char *nam = 0;
    uint32 namlen = 1;

    Unit* unit = ObjectAccessor::GetUnit(*_player, guid);
    Creature *pCreature = dynamic_cast<Creature *>(unit);
    if(unit)
    {
        nam = unit->GetName();
        namlen = (nam ? strlen(nam) : 0) + 1;
    }

    EmotesTextEntry const *em = sEmotesTextStore.LookupEntry(text_emote);
    if (em)
    {
        uint32 emote_anim = em->textid;

        WorldPacket data;

        switch(emote_anim)
        {
            case EMOTE_STATE_SLEEP:
            case EMOTE_STATE_SIT:
            case EMOTE_STATE_KNEEL:
            case EMOTE_ONESHOT_NONE:
                break;
            default:
                GetPlayer()->HandleEmoteCommand(emote_anim);
                break;
        }

        data.Initialize(SMSG_TEXT_EMOTE, (20+namlen));
        data << GetPlayer()->GetGUID();
        data << (uint32)text_emote;
        data << emoteNum;
        data << (uint32)namlen;
        if( namlen > 1 )
        {
            data.append(nam, namlen);
        }
        else
        {
            data << (uint8)0x00;
        }

        GetPlayer()->SendMessageToSetInRange(&data,sWorld.getConfig(CONFIG_LISTEN_RANGE_TEXTEMOTE),true);

        //Send scripted event call
        if (pCreature) {
            sScriptMgr.ReceiveEmote(GetPlayer(),pCreature,text_emote);
            pCreature->AI()->ReceiveEmote(GetPlayer(), text_emote);
        }
    }
}

void WorldSession::HandleChatIgnoredOpcode(WorldPacket& recv_data )
{
    PROFILE;
    
    CHECK_PACKET_SIZE(recv_data, 8+1);

    uint64 iguid;
    uint8 unk;
    //sLog.outDebug("WORLD: Received CMSG_CHAT_IGNORED");

    recv_data >> iguid;
    recv_data >> unk;                                       // probably related to spam reporting

    Player *player = objmgr.GetPlayer(iguid);
    if(!player || !player->GetSession())
        return;

    WorldPacket data;
    ChatHandler::FillMessageData(&data, this, CHAT_MSG_IGNORED, LANG_UNIVERSAL, NULL, GetPlayer()->GetGUID(), GetPlayer()->GetName(),NULL);
    player->GetSession()->SendPacket(&data);
}

void WorldSession::HandleChannelDeclineInvite(WorldPacket &recvPacket)
{
    // TODO
}

