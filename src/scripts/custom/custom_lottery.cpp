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
    pPlayer->ADD_GOSSIP_ITEM(0, "Je m'inscris à la lotterie.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
    
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
        if (pPlayer->GetTotalPlayedTime() > 2592000 || pPlayer->GetSession()->GetSecurity() > 0) {
            uint32 playerAccountId = pPlayer->GetSession()->GetAccountId();
            QueryResult* result = CharacterDatabase.PQuery("SELECT * FROM lottery WHERE accountid = %u", playerAccountId);
            if (!result) {
                CharacterDatabase.PExecute("INSERT INTO lottery VALUES (%u, %u, "I64FMTD", %u)", pPlayer->GetGUIDLow(), playerAccountId, time(NULL), pPlayer->GetTeam());
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
        QueryResult* result = CharacterDatabase.PQuery("SELECT guid FROM lottery WHERE faction = %u ORDER BY RAND() LIMIT 1", HORDE);
        if (!result)
            break;
            
        Field* fields = result->Fetch();
        hordeWin = fields[0].GetUInt32();
        
        result = CharacterDatabase.PQuery("SELECT guid FROM lottery WHERE faction = %u ORDER BY RAND() LIMIT 1", ALLIANCE);
        if (!result)
            break;
            
        fields = result->Fetch();
        allianceWin = fields[0].GetUInt32();
        
        std::ostringstream hsst, asst;
        std::string hWinName, aWinName;
        objmgr.GetPlayerNameByGUID(hordeWin, hWinName);
        hsst << "Le gagnant de la Horde est " << hWinName << " !";
        objmgr.GetPlayerNameByGUID(allianceWin, aWinName);
        asst << "Le gagnant de l'Alliance est " << aWinName << " !";
        
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
