#include "precompiled.h"

#define GOSSIP_TELEPORT "Teleportez-moi"
#define QUEST_TELEPORT_HORDE    80000
#define QUEST_TELEPORT_ALLIANCE 80001


struct npc_pack58_teleporterAI : public ScriptedAI
{
    npc_pack58_teleporterAI(Creature* c) : ScriptedAI(c) 
    {
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);  
        me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
    }
};

bool GossipHello_npc_pack58_teleporter(Player *player, Creature *creature)
{
    if(player->getLevel() > 57)
    {
        creature->Whisper("Vous êtes trop haut niveau pour utiliser le pack 58.", player->GetGUID());
        return true;
    }

    if (creature->isQuestGiver())
        player->PrepareQuestMenu(creature->GetGUID());

    bool alliance = player->TeamForRace(player->GetRace()) == ALLIANCE;

    if(player->GetQuestStatus(alliance ? QUEST_TELEPORT_ALLIANCE : QUEST_TELEPORT_HORDE) == QUEST_STATUS_COMPLETE)
        player->ADD_GOSSIP_ITEM(0, GOSSIP_TELEPORT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+(alliance ? 1 : 2));

    player->SEND_GOSSIP_MENU(creature->GetNpcTextId(), creature->GetGUID());
    return true;
}

bool GossipSelect_npc_pack58_teleporter(Player *player, Creature *creature, uint32 sender, uint32 action )
{
    switch(action)
    {
    case GOSSIP_ACTION_INFO_DEF+1: //alliance
        player->TeleportTo(0, -8921.09, -119.16, 82.0, 6.0); //Northshire
        break;
    case GOSSIP_ACTION_INFO_DEF+2: //horde
        player->TeleportTo(1, -618.52, -4251.67, 38.72, 0); //Valley of Trials
        break;
    }       

    player->CLOSE_GOSSIP_MENU();
    return true;
}

CreatureAI* GetAI_npc_pack58_teleporterAI(Creature *_Creature)
{
    return new npc_pack58_teleporterAI (_Creature);
}

#define QUEST_DUEL_HORDE    80004
#define QUEST_DUEL_ALLIANCE 80005
#define GOSSIP_DUEL "Defier en duel"

enum DuelGuyMessages
{
    MESSAGE_START_DUEL,
    MESSAGE_STOP_DUEL,
};

struct npc_pack58_duelguyAI : public ScriptedAI
{
    npc_pack58_duelguyAI(Creature* c) : ScriptedAI(c) 
    {
        originalFaction = me->getFaction();
    }

    uint64 PlayerGUID;
    uint32 originalFaction;

    uint64 message(uint32 id, uint64 data) 
    { 
        switch(id)
        {
        case MESSAGE_START_DUEL:
            PlayerGUID = data;

            m_creature->setFaction(14);
            m_creature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            
            if(Player* player = Unit::GetPlayer(PlayerGUID))
                AttackStart(player);

            break;
        case MESSAGE_STOP_DUEL:
            m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            m_creature->setFaction(originalFaction);
            m_creature->DeleteThreatList();
            m_creature->CombatStop();
            m_creature->GetMotionMaster()->MoveTargetedHome();
            if(Player* player = Unit::GetPlayer(PlayerGUID))
            {
                player->GroupEventHappens(QUEST_DUEL_HORDE, me);
                player->GroupEventHappens(QUEST_DUEL_ALLIANCE, me);
            }
            break;
        }
        return 0;     
    }

    void Reset()
    {
        PlayerGUID = 0;

        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        m_creature->setFaction(originalFaction);
    }
    
    void DamageTaken(Unit *pSource, uint32 &damage)
    {
        if (damage > m_creature->GetHealth()) {
            damage = 0;
            message(MESSAGE_STOP_DUEL,PlayerGUID);
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim())
            return;

        //some attacks ?

        DoMeleeAttackIfReady();
    }
};

bool GossipHello_npc_pack58_duelguy(Player *player, Creature *creature)
{
    if (creature->isQuestGiver())
        player->PrepareQuestMenu(creature->GetGUID());

    uint32 questid;
    if(player->TeamForRace(player->GetRace()) == ALLIANCE)
        questid = QUEST_DUEL_ALLIANCE;
    else
        questid = QUEST_DUEL_HORDE;

    if(player->GetQuestStatus(questid) == QUEST_STATUS_INCOMPLETE)
        player->ADD_GOSSIP_ITEM(0, GOSSIP_DUEL, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    player->SEND_GOSSIP_MENU(creature->GetNpcTextId(), creature->GetGUID());
    return true;
}

bool GossipSelect_npc_pack58_duelguy(Player *player, Creature *creature, uint32 sender, uint32 action )
{
    if (action == GOSSIP_ACTION_INFO_DEF+1)
        (creature->AI())->message(MESSAGE_START_DUEL,player->GetGUID());
        
    player->CLOSE_GOSSIP_MENU();
    return true;
}

CreatureAI* GetAI_npc_pack58_duelguyAI(Creature *_Creature)
{
    return new npc_pack58_duelguyAI (_Creature);
}

void AddSC_pack58()
{
    Script *newscript; 
 
    newscript = new Script;
    newscript->Name="npc_pack58_teleporter";
    newscript->GetAI = &GetAI_npc_pack58_teleporterAI;
    newscript->pGossipHello =  &GossipHello_npc_pack58_teleporter;
    newscript->pGossipSelect = &GossipSelect_npc_pack58_teleporter;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_pack58_duelguy";
    newscript->GetAI = &GetAI_npc_pack58_duelguyAI;
    newscript->pGossipHello =  &GossipHello_npc_pack58_duelguy;
    newscript->pGossipSelect = &GossipSelect_npc_pack58_duelguy;
    newscript->RegisterSelf();
}