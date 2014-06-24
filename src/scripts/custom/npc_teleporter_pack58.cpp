#include "precompiled.h"

//alliance TP
#define NPC_KELNOR 9165
//horde TP
#define NPC_GARGASH 91655

bool QuestComplete_npc_teleporter_pack58(Player* pPlayer, Creature* pCreature, Quest const* pQuest)
{
    if(pCreature->GetEntry() == NPC_KELNOR)
    {
        //Northshire
        pPlayer->TeleportTo(0, -8921.09, -119.16, 82.0, 6.0);
    } else //if(pCreature->GetEntry() == NPC_GARGASH)
    { 
       //Valley of Trials
       pPlayer->TeleportTo(1, -618.52, -4251.67, 38.72, 0);
    }
    
    return true;
}

void AddSC_npc_teleporter_pack58()
{
    Script *newscript; 
 
    newscript = new Script;
    newscript->Name="npc_teleporter_pack58";
    newscript->pQuestComplete = &QuestComplete_npc_teleporter_pack58;
    newscript->RegisterSelf();
}