/* Custom Script - WoWManiaCore
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
SDName: npc_lottery
SD%Complete: 100
SDComment: NPC lottery
SDCategory: Custom Scripts
EndScriptData */

#include "precompiled.h"
#include "ObjectMgr.h"
#include <cstring>

/*######
## npc_lottery
######*/

bool GossipHello_npc_lottery(Player *pPlayer, Creature *pCreature)
{
    pPlayer->ADD_GOSSIP_ITEM(0, "Je m'inscris à la loterie.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
    
    if (pPlayer->isGameMaster())
        pPlayer->ADD_GOSSIP_ITEM(0, "Lancer le tirage au sort", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
        
    pPlayer->PlayerTalkClass->SendGossipMenu(43, pCreature->GetGUID());

    return true;
}

bool GossipSelect_npc_lottery(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action)
{
    switch (action) {
    case GOSSIP_ACTION_INFO_DEF:
        // Check not already registered and check 30d played
        if (pPlayer->GetTotalAccountPlayedTime() > 2592000 || pPlayer->GetSession()->GetSecurity() > 0) {
            uint32 playerAccountId = pPlayer->GetSession()->GetAccountId();
            QueryResult* result = CharacterDatabase.PQuery("SELECT * FROM lottery WHERE accountid = %u OR ip = '%s'", playerAccountId, pPlayer->GetSession()->GetRemoteAddress().c_str());
            if (!result) {
                CharacterDatabase.PExecute("INSERT INTO lottery VALUES (%u, %u, "I64FMTD", %u, '%s')", pPlayer->GetGUIDLow(), playerAccountId, time(NULL), pPlayer->GetTeam(), pPlayer->GetSession()->GetRemoteAddress().c_str());
                pPlayer->PlayerTalkClass->SendGossipMenu(44, pCreature->GetGUID());
            }
            else {
                pPlayer->PlayerTalkClass->SendGossipMenu(45, pCreature->GetGUID());
            }
        }
        else {
            pPlayer->PlayerTalkClass->SendGossipMenu(46, pCreature->GetGUID());
        }
        break;
    case GOSSIP_ACTION_INFO_DEF+1:
    {
        uint32 hordeWin = 0, allianceWin = 0;
        QueryResult* result = CharacterDatabase.PQuery("SELECT guid FROM lottery WHERE faction = %u ORDER BY RAND() LIMIT 3", HORDE);
        if (!result)
            break;
            
        std::ostringstream hsst, asst;
        std::string hWinName, aWinName;
        //objmgr.GetPlayerNameByGUID(hordeWin, hWinName);
        hsst << "Pour la Horde, le gagnant de la peluche est ";
        //objmgr.GetPlayerNameByGUID(allianceWin, aWinName);
        asst << "Pour l'Alliance, le gagnant de la peluche est " << aWinName;
        
        uint8 i = 0;
            
        do {
            Field* fields = result->Fetch();
            
            hordeWin = fields[0].GetUInt32();
            objmgr.GetPlayerNameByGUID(hordeWin, hWinName);
            
            if (i == 0)
                hsst << hWinName << " et les gagnants de la monture sont ";
            else if (i == 1)
                hsst << hWinName << " et ";
            else
                hsst << hWinName << " !";
            
            i++;
        } while (result->NextRow());
        
        i = 0;
        
        result = CharacterDatabase.PQuery("SELECT guid FROM lottery WHERE faction = %u ORDER BY RAND() LIMIT 3", ALLIANCE);
        if (!result)
            break;
            
        do {
            Field* fields = result->Fetch();
            
            allianceWin = fields[0].GetUInt32();
            objmgr.GetPlayerNameByGUID(allianceWin, aWinName);
            
            if (i == 0)
                asst << aWinName << " et les gagnants de la monture sont ";
            else if (i == 1)
                asst << aWinName << " et ";
            else
                asst << aWinName << " !";
            
            i++;
        } while (result->NextRow());
        
        pCreature->Yell(hsst.str().c_str(), LANG_UNIVERSAL, NULL);
        pCreature->Yell(asst.str().c_str(), LANG_UNIVERSAL, NULL);
        
        break;
    }
    default:
        break;
    }
    
    return true;
}

void AddSC_npc_lottery()
{
    Script* newscript;
    
    newscript = new Script;
    newscript->Name = "npc_lottery";
    newscript->pGossipHello = &GossipHello_npc_lottery;
    newscript->pGossipSelect = &GossipSelect_npc_lottery;
    newscript->RegisterSelf();
}
