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
SDName: Loch_Modan
SD%Complete: 100
SDComment: Quest support: 309, 3181
SDCategory: Loch Modan
EndScriptData */

/* ContentData
npc_mountaineer_pebblebitty
npc_miran
EndContentData */

#include "precompiled.h"
#include "EscortAI.h"

/*######
## npc_mountaineer_pebblebitty
######*/

#define GOSSIP_MP "Open the gate please, i need to get to Searing Gorge"

#define GOSSIP_MP1 "But i need to get there, now open the gate!"
#define GOSSIP_MP2 "Ok, so what is this other way?"
#define GOSSIP_MP3 "Doesn't matter, i'm invulnerable."
#define GOSSIP_MP4 "Yes..."
#define GOSSIP_MP5 "Ok, i'll try to remember that."
#define GOSSIP_MP6 "A key? Ok!"

bool GossipHello_npc_mountaineer_pebblebitty(Player* pPlayer, Creature* pCreature)
{
    if (pCreature->isQuestGiver())
        pPlayer->PrepareQuestMenu( pCreature->GetGUID() );

    if (!pPlayer->GetQuestRewardStatus(3181) == 1)
        pPlayer->ADD_GOSSIP_ITEM( 0, GOSSIP_MP, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    pPlayer->SEND_GOSSIP_MENU(pCreature->GetNpcTextId(), pCreature->GetGUID());

    return true;
}

bool GossipSelect_npc_mountaineer_pebblebitty(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action)
{
    switch (action)
    {
        case GOSSIP_ACTION_INFO_DEF+1:
            pPlayer->ADD_GOSSIP_ITEM( 0, GOSSIP_MP1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            pPlayer->SEND_GOSSIP_MENU(1833, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+2:
            pPlayer->ADD_GOSSIP_ITEM( 0, GOSSIP_MP2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            pPlayer->SEND_GOSSIP_MENU(1834, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+3:
            pPlayer->ADD_GOSSIP_ITEM( 0, GOSSIP_MP3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
            pPlayer->SEND_GOSSIP_MENU(1835, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+4:
            pPlayer->ADD_GOSSIP_ITEM( 0, GOSSIP_MP4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 5);
            pPlayer->SEND_GOSSIP_MENU(1836, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+5:
            pPlayer->ADD_GOSSIP_ITEM( 0, GOSSIP_MP5, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 6);
            pPlayer->SEND_GOSSIP_MENU(1837, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+6:
            pPlayer->ADD_GOSSIP_ITEM( 0, GOSSIP_MP6, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 7);
            pPlayer->SEND_GOSSIP_MENU(1838, pCreature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+7:
            pPlayer->CLOSE_GOSSIP_MENU();
            break;
    }
    return true;
}

/*#########
##npc_miran
#########*/

enum eMiran
{
    MIRAN_SAY_AMBUSH_ONE        = -1780127,
    MIRAN_SAY_AMBUSH_TWO        = -1780128, 
    DARK_IRON_RAIDER_SAY_AMBUSH = -1780129, 
    MIRAN_SAY_QUEST_END         = -1780130,

    QUEST_PROTECTING_THE_SHIPMENT  = 309,
    DARK_IRON_RAIDER               = 2149

};

struct npc_miranAI : public npc_escortAI
{
    npc_miranAI(Creature* pCreature) : npc_escortAI(pCreature) { }    

    void Reset() {}
    
    void Aggro(Unit *pWho) {}

    void WaypointReached(uint32 uiPointId)
    {
        Player* pPlayer = GetPlayerForEscort();
        if (!pPlayer)
            return;

        switch(uiPointId)
        {
        case 8:
            DoScriptText(MIRAN_SAY_AMBUSH_ONE, me);
            me->SummonCreature(DARK_IRON_RAIDER, -5697.27,-3736.36,318.54, 2.02, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 30000);
            me->SummonCreature(DARK_IRON_RAIDER, -5697.27,-3736.36,318.54, 2.07, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 30000);
            if (Unit* scoff = FindCreature(DARK_IRON_RAIDER, 30, me))
                DoScriptText(DARK_IRON_RAIDER_SAY_AMBUSH, scoff);
            DoScriptText(MIRAN_SAY_AMBUSH_TWO, me);
        break;
        case 11:
            DoScriptText(MIRAN_SAY_QUEST_END, me);
            if (Player* pPlayer = GetPlayerForEscort())
                pPlayer->GroupEventHappens(QUEST_PROTECTING_THE_SHIPMENT, me);
        break;
        }
    }

    void JustSummoned(Creature* pSummoned)
    {
        pSummoned->AI()->AttackStart(me);
    }
};

bool QuestAccept_npc_miran(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_PROTECTING_THE_SHIPMENT)
    {        
        pCreature->setFaction(231);

        ((npc_escortAI*)(pCreature->AI()))->Start(true, true, false, pPlayer->GetGUID(), pCreature->GetEntry());
    }
    return true;
}
CreatureAI* GetAI_npc_miran(Creature *pCreature)
{
    return new npc_miranAI(pCreature);
}

/*######
## AddSC
######*/

void AddSC_loch_modan()
{
    Script* newscript;

    newscript = new Script;
    newscript->Name="npc_mountaineer_pebblebitty";
    newscript->pGossipHello =  &GossipHello_npc_mountaineer_pebblebitty;
    newscript->pGossipSelect = &GossipSelect_npc_mountaineer_pebblebitty;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_miran";
    newscript->GetAI = &GetAI_npc_miran;
    newscript->pQuestAccept = &QuestAccept_npc_miran;
    newscript->RegisterSelf();
}

