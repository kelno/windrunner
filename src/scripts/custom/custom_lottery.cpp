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
        if (pPlayer->GetTotalAccountPlayedTime() > 1728000 || pPlayer->GetSession()->GetSecurity() > 0) {
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
        uint32 winner;
        QueryResult* result = CharacterDatabase.PQuery("SELECT DISTINCT guid FROM lottery ORDER BY RAND() LIMIT 10", HORDE);
        if (!result)
            break;
            
        std::ostringstream oss;
        std::string winner_str;
        
        uint32 num = 1;
            
        do {
            oss.str("");
            Field* fields = result->Fetch();
            
            winner = fields[0].GetUInt32();
            objmgr.GetPlayerNameByGUID(winner, winner_str);
            
            oss << "Le gagnant numéro " << num << " est " << winner_str << " !";
            pCreature->Yell(oss.str().c_str(), LANG_UNIVERSAL, NULL);
            
            num++;
        } while (result->NextRow());
        
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
