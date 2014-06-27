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
SDName: Orgrimmar
SD%Complete: 100
SDComment: Quest support: 2460, 5727, 6566
SDCategory: Orgrimmar
EndScriptData */

/* ContentData
npc_neeru_fireblade     npc_text + gossip options text missing
npc_shenthul
npc_thrall_warchief
npc_eitrigg
EndContentData */

#include "precompiled.h"
#include "Chat.h"

/*######
## npc_neeru_fireblade
######*/

#define QUEST_5727  5727

#define GOSSIP_HNF "You may speak frankly, Neeru..."
#define GOSSIP_SNF "[PH] ..."
bool GossipHello_npc_neeru_fireblade(Player *player, Creature *_Creature)
{
    if (_Creature->isQuestGiver())
        player->PrepareQuestMenu( _Creature->GetGUID() );

    if (player->GetQuestStatus(QUEST_5727) == QUEST_STATUS_INCOMPLETE)
        player->ADD_GOSSIP_ITEM(0, GOSSIP_HNF, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    player->SEND_GOSSIP_MENU(4513, _Creature->GetGUID());
    return true;
}

bool GossipSelect_npc_neeru_fireblade(Player *player, Creature *_Creature, uint32 sender, uint32 action)
{
    switch (action)
    {
        case GOSSIP_ACTION_INFO_DEF+1:
            player->ADD_GOSSIP_ITEM(0, GOSSIP_SNF, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
            player->SEND_GOSSIP_MENU(4513, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+2:
            player->CLOSE_GOSSIP_MENU();
            player->AreaExploredOrEventHappens(QUEST_5727);
            break;
    }
    return true;
}

/*######
## npc_shenthul
######*/

#define QUEST_2460  2460

struct npc_shenthulAI : public ScriptedAI
{
    npc_shenthulAI(Creature* c) : ScriptedAI(c) {}

    bool CanTalk;
    bool CanEmote;
    uint32 Salute_Timer;
    uint32 Reset_Timer;
    uint64 playerGUID;

    void Reset()
    {
        CanTalk = false;
        CanEmote = false;
        Salute_Timer = 6000;
        Reset_Timer = 0;
        playerGUID = 0;
    }

    void EnterCombat(Unit* who) { }

    void UpdateAI(const uint32 diff)
    {
        if( CanEmote )
            if( Reset_Timer < diff )
        {
            if( Player* temp = Unit::GetPlayer(playerGUID) )
                temp->FailQuest(QUEST_2460);
            Reset();
        } else Reset_Timer -= diff;

        if( CanTalk && !CanEmote )
            if( Salute_Timer < diff )
        {
            m_creature->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);
            CanEmote = true;
            Reset_Timer = 60000;
        } else Salute_Timer -= diff;

        if (!UpdateVictim())
            return;

        DoMeleeAttackIfReady();
    }
};
CreatureAI* GetAI_npc_shenthul(Creature *_Creature)
{
    return new npc_shenthulAI (_Creature);
}

bool QuestAccept_npc_shenthul(Player* player, Creature* creature, Quest const* quest)
{
    if( quest->GetQuestId() == QUEST_2460 )
    {
        ((npc_shenthulAI*)creature->AI())->CanTalk = true;
        ((npc_shenthulAI*)creature->AI())->playerGUID = player->GetGUID();
    }
    return true;
}

bool ReciveEmote_npc_shenthul(Player *player, Creature *_Creature, uint32 emote)
{
    if( emote == TEXTEMOTE_SALUTE && player->GetQuestStatus(QUEST_2460) == QUEST_STATUS_INCOMPLETE )
        if( ((npc_shenthulAI*)_Creature->AI())->CanEmote )
    {
        player->AreaExploredOrEventHappens(QUEST_2460);
        ((npc_shenthulAI*)_Creature->AI())->Reset();
    }
    return true;
}

/*######
## npc_thrall_warchief
######*/

#define QUEST_6566              6566

#define SPELL_CHAIN_LIGHTNING   16033
#define SPELL_SHOCK             16034

#define GOSSIP_HTW "Please share your wisdom with me, Warchief."
#define GOSSIP_STW1 "What discoveries?"
#define GOSSIP_STW2 "Usurper?"
#define GOSSIP_STW3 "With all due respect, Warchief - why not allow them to be destroyed? Does this not strengthen our position?"
#define GOSSIP_STW4 "I... I did not think of it that way, Warchief."
#define GOSSIP_STW5 "I live only to serve, Warchief! My life is empty and meaningless without your guidance."
#define GOSSIP_STW6 "Of course, Warchief!"
#define GOSSIP_PACK58 "Teleportez moi a la porte des Tenebres."

//TODO: verify abilities/timers
struct npc_thrall_warchiefAI : public ScriptedAI
{
    npc_thrall_warchiefAI(Creature* c) : ScriptedAI(c) {}

    uint32 ChainLightning_Timer;
    uint32 Shock_Timer;

    void Reset()
    {
        ChainLightning_Timer = 2000;
        Shock_Timer = 8000;
    }

    void EnterCombat(Unit *who) {}

    void UpdateAI(const uint32 diff)
    {
        if(!UpdateVictim())
            return;

        if( ChainLightning_Timer < diff )
        {
            DoCast(m_creature->GetVictim(),SPELL_CHAIN_LIGHTNING);
            ChainLightning_Timer = 9000;
        }else ChainLightning_Timer -= diff;

        if( Shock_Timer < diff )
        {
            DoCast(m_creature->GetVictim(),SPELL_SHOCK);
            Shock_Timer = 15000;
        }else Shock_Timer -= diff;

        DoMeleeAttackIfReady();
    }
};
CreatureAI* GetAI_npc_thrall_warchief(Creature *_Creature)
{
    return new npc_thrall_warchiefAI (_Creature);
}

bool GossipHello_npc_thrall_warchief(Player *player, Creature *_Creature)
{
    if (_Creature->isQuestGiver())
        player->PrepareQuestMenu( _Creature->GetGUID() );

    if (player->GetQuestStatus(QUEST_6566) == QUEST_STATUS_INCOMPLETE)
        player->ADD_GOSSIP_ITEM(0, GOSSIP_HTW, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    if (player->GetQuestStatus(80016) == QUEST_STATUS_COMPLETE) //pack58, tp vers la porte des tenebres
        player->ADD_GOSSIP_ITEM(0, GOSSIP_PACK58, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+8);

    player->SEND_GOSSIP_MENU(_Creature->GetNpcTextId(), _Creature->GetGUID());
    return true;
}

bool GossipSelect_npc_thrall_warchief(Player *player, Creature *_Creature, uint32 sender, uint32 action)
{
    switch (action)
    {
        case GOSSIP_ACTION_INFO_DEF+1:
            player->ADD_GOSSIP_ITEM(0, GOSSIP_STW1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+2);
            player->SEND_GOSSIP_MENU(5733, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+2:
            player->ADD_GOSSIP_ITEM(0, GOSSIP_STW2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+3);
            player->SEND_GOSSIP_MENU(5734, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+3:
            player->ADD_GOSSIP_ITEM(0, GOSSIP_STW3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+4);
            player->SEND_GOSSIP_MENU(5735, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+4:
            player->ADD_GOSSIP_ITEM(0, GOSSIP_STW4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+5);
            player->SEND_GOSSIP_MENU(5736, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+5:
            player->ADD_GOSSIP_ITEM(0, GOSSIP_STW5, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+6);
            player->SEND_GOSSIP_MENU(5737, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+6:
            player->ADD_GOSSIP_ITEM(0, GOSSIP_STW6, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+7);
            player->SEND_GOSSIP_MENU(5738, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+7:
            player->CLOSE_GOSSIP_MENU();
            player->AreaExploredOrEventHappens(QUEST_6566);
            break;
        case GOSSIP_ACTION_INFO_DEF+8:
            player->TeleportTo(0, -11741.21, -3173.35, -16.49, 3.35); //Porte des tenebres
            player->CLOSE_GOSSIP_MENU();
            break;
    }
    return true;
}

bool QuestComplete_npc_thrall_warchief(Player* pPlayer, Creature* pCreature, Quest const* pQuest)
{
    switch(pQuest->GetQuestId())
    {
        case 8485:
        {
            std::stringstream sst;
            sst << "Sachez tous que " << pPlayer->GetName() << " - " << pPlayer->getClass() << " de la Horde - a gagné le respect du Chef de guerre. Il a engagé la diplomatie avec les Grumegueules et accompli diverses actions en notre nom. Il est allé bien au delà de l'appel du devoir. Trois félicitations pour " << pPlayer->GetName() << " - un vrai héros de la Horde !";
            pCreature->MonsterYell(sst.str().c_str(), LANG_UNIVERSAL, 0);
            break;
        }
        case 80002:
            pPlayer->DoPack58(PACK58_STEP1);
            break;
        case 80008:
            pPlayer->DoPack58(PACK58_HEAL);
            break;
        case 80010:
            pPlayer->DoPack58(PACK58_MELEE);
            break;
        case 80012:
            pPlayer->DoPack58(PACK58_TANK);
            break;
        case 80014:
            pPlayer->DoPack58(PACK58_MAGIC);
            break;
    }
    return true;
}

/*######
## npc_eitrigg
######*/

bool GossipHello_npc_eitrigg(Player* player, Creature* creature)
{
    if (creature->isQuestGiver())
        player->PrepareQuestMenu(creature->GetGUID());
        
    if (player->GetQuestStatus(4941) == QUEST_STATUS_INCOMPLETE)
        player->ADD_GOSSIP_ITEM(0, "[PH] Valider la quête \"Sagesse d'Eitrigg\".", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    player->SEND_GOSSIP_MENU(creature->GetNpcTextId(), creature->GetGUID());
    return true;
}

bool GossipSelect_npc_eitrigg(Player* player, Creature* creature, uint32 sender, uint32 action)
{
    if (action == GOSSIP_ACTION_INFO_DEF + 1)
        player->AreaExploredOrEventHappens(4941);
        
    player->CLOSE_GOSSIP_MENU();
        
    return true;
}

void AddSC_orgrimmar()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="npc_neeru_fireblade";
    newscript->pGossipHello =  &GossipHello_npc_neeru_fireblade;
    newscript->pGossipSelect = &GossipSelect_npc_neeru_fireblade;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_shenthul";
    newscript->GetAI = &GetAI_npc_shenthul;
    newscript->pQuestAccept =  &QuestAccept_npc_shenthul;
    newscript->pReceiveEmote = &ReciveEmote_npc_shenthul;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_thrall_warchief";
    newscript->GetAI = &GetAI_npc_thrall_warchief;
    newscript->pGossipHello =  &GossipHello_npc_thrall_warchief;
    newscript->pGossipSelect = &GossipSelect_npc_thrall_warchief;
    newscript->pQuestComplete = &QuestComplete_npc_thrall_warchief;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_eitrigg";
    newscript->pGossipHello = &GossipHello_npc_eitrigg;
    newscript->pGossipSelect = &GossipSelect_npc_eitrigg;
    newscript->RegisterSelf();
}

