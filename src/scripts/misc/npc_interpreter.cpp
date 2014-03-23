#include "precompiled.h"
#include <sstream>

struct npc_interpreterAI : public ScriptedAI
{
    npc_interpreterAI(Creature *c) : ScriptedAI(c) 
    {}

    void EnterCombat(Unit* /* who */) {}
};

bool GossipHello_npc_interpreter(Player *player, Creature *_Creature)
{    
    player->ADD_GOSSIP_ITEM_EXTENDED( 0, "Repeter", GOSSIP_SENDER_MAIN, 1, "", 0, true);
        
    player->PlayerTalkClass->SendGossipMenu(3,_Creature->GetGUID());

    return true;
}

bool GossipSelectWithCode_npc_interpreter( Player *player, Creature *_Creature, uint32 sender, uint32 action, const char* Code )
{
    std::stringstream text;
    text << player->GetName() << " dit : " << Code;
    ((npc_interpreterAI*)_Creature->AI())->DoSay(text.str().c_str(),LANG_UNIVERSAL,NULL);

    player->PlayerTalkClass->CloseGossip();
    return true;
}

CreatureAI* GetAI_npc_interpreter(Creature *_Creature)
{
    return new npc_interpreterAI (_Creature);
}
 
void AddSC_npc_interpreter()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="npc_interpreter";
    newscript->GetAI = &GetAI_npc_interpreter;
    newscript->pGossipHello = &GossipHello_npc_interpreter;
    newscript->pGossipSelectWithCode = &GossipSelectWithCode_npc_interpreter;
    newscript->RegisterSelf();
}
