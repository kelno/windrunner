#include "precompiled.h"
#include "WorldPacket.h"

#define X_DIRECTION 0.89102085f
#define Y_DIRECTION 0.45396239f

enum Actions {
    ACTION_SPEED,
    ACTION_ANGLE,
    ACTION_GO
};

struct TRINITY_DLL_DECL catapultmasterAI : public ScriptedAI
{
    float horizontal_speed;
    float vertical_speed;
    catapultmasterAI(Creature* creature) : 
        ScriptedAI(creature),
        horizontal_speed(-200),
        vertical_speed(-10)
    {}

    void EnterCombat(Unit* who)
    {}
};

bool GossipHello_catapultmaster(Player *player, Creature *_Creature)
{    
    player->ADD_GOSSIP_ITEM_EXTENDED( 0, "Vitesse", GOSSIP_SENDER_MAIN, ACTION_SPEED, "", 0, true);
    player->ADD_GOSSIP_ITEM_EXTENDED( 0, "Angle", GOSSIP_SENDER_MAIN, ACTION_ANGLE, "", 0, true);
    player->ADD_GOSSIP_ITEM( 0, "GO !", GOSSIP_SENDER_MAIN, ACTION_GO);
        
    player->PlayerTalkClass->SendGossipMenu(3,_Creature->GetGUID());

    return true;
}

bool GossipSelect_catapultmaster(Player *player, Creature *_Creature, uint32 sender, uint32 action )
{
    switch(action)
    {
    case ACTION_GO:
        _Creature->Say("YOLO !",LANG_UNIVERSAL,NULL);

        WorldPacket data(SMSG_MOVE_KNOCK_BACK, (8+4+4+4+4+4));
        data.append(player->GetPackGUID());
        data << uint32(0);                                      // Sequence
        data << X_DIRECTION;                                    // x direction
        data << Y_DIRECTION;                                    // y direction
        data << ((catapultmasterAI*)_Creature->AI())->horizontal_speed;      // Horizontal speed
        data << ((catapultmasterAI*)_Creature->AI())->vertical_speed;                              // Z Movement speed (vertical)

        player->GetSession()->SendPacket(&data);
        break;
    }

    return true;
}

bool GossipSelectWithCode_catapultmaster( Player *player, Creature *_Creature, uint32 sender, uint32 action, const char* Code )
{
    switch(action)
    {
    case ACTION_SPEED:
        ((catapultmasterAI*)_Creature->AI())->horizontal_speed = -atof(Code);
        break;
    case ACTION_ANGLE:
        ((catapultmasterAI*)_Creature->AI())->vertical_speed = -atof(Code);
        break;
    }

    player->PlayerTalkClass->CloseGossip();
    return true;
}

CreatureAI* GetAI_catapultmaster(Creature *_Creature)
{
    return new catapultmasterAI (_Creature);
}

void AddSC_catapultmaster()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="npc_catapultmaster";
    newscript->GetAI = &GetAI_catapultmaster;
    newscript->pGossipHello = &GossipHello_catapultmaster;
    newscript->pGossipSelect = &GossipSelect_catapultmaster;
    newscript->pGossipSelectWithCode = &GossipSelectWithCode_catapultmaster;
    newscript->RegisterSelf();
}
