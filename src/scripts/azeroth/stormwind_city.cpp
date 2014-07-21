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
SDName: Stormwind_City
SD%Complete: 100
SDComment: Quest support: 1640, 1447, 4185, 8356, 11223. Receive emote General Marcus
SDCategory: Stormwind City
EndScriptData */

/* ContentData
npc_archmage_malin
npc_bartleby
npc_dashel_stonefist
npc_general_marcus_jonathan
npc_lady_katrana_prestor
npc_innkeeper_allison
npc_monty
npc_bolvar_fordragon
EndContentData */

#include "precompiled.h"

/*######
## npc_archmage_malin
######*/

#define GOSSIP_ITEM_MALIN "Pouvez-vous m'envoyer à Theramore ? J'ai un message urgent pour Jaina, de la part de Bolvar."

bool GossipHello_npc_archmage_malin(Player *player, Creature *_Creature)
{
    if(_Creature->isQuestGiver())
        player->PrepareQuestMenu( _Creature->GetGUID() );

    if(player->GetQuestStatus(11223) == QUEST_STATUS_COMPLETE && !player->GetQuestRewardStatus(11223))
        player->ADD_GOSSIP_ITEM(0, GOSSIP_ITEM_MALIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);

    player->SEND_GOSSIP_MENU(_Creature->GetNpcTextId(), _Creature->GetGUID());

    return true;
}

bool GossipSelect_npc_archmage_malin(Player *player, Creature *_Creature, uint32 sender, uint32 action)
{
    if(action = GOSSIP_ACTION_INFO_DEF)
    {
        player->CLOSE_GOSSIP_MENU();
        _Creature->CastSpell(player, 42711, true);
    }

    return true;
}

/*######
## npc_bartleby
######*/

struct npc_bartlebyAI : public ScriptedAI
{
    npc_bartlebyAI(Creature *c) : ScriptedAI(c) {}

    uint64 PlayerGUID;

    void Reset()
    {
        m_creature->setFaction(11);
        m_creature->setEmoteState(7);

        PlayerGUID = 0;
    }

    void JustDied(Unit *who)
    {
        m_creature->setFaction(11);
    }

    void DamageTaken(Unit *done_by, uint32 & damage)
    {
        if(damage > m_creature->GetHealth() || ((m_creature->GetHealth() - damage)*100 / m_creature->GetMaxHealth() < 15))
        {
            //Take 0 damage
            damage = 0;

            if (done_by->GetTypeId() == TYPEID_PLAYER && done_by->GetGUID() == PlayerGUID)
            {
                (done_by->ToPlayer())->AttackStop();
                (done_by->ToPlayer())->AreaExploredOrEventHappens(1640);
            }
            m_creature->CombatStop();
            EnterEvadeMode();
        }
    }

    void EnterCombat(Unit *who) {}
};

bool QuestAccept_npc_bartleby(Player *player, Creature *_Creature, Quest const *_Quest)
{
    if(_Quest->GetQuestId() == 1640)
    {
        _Creature->setFaction(168);
        ((npc_bartlebyAI*)_Creature->AI())->PlayerGUID = player->GetGUID();
        ((npc_bartlebyAI*)_Creature->AI())->AttackStart(player);
    }
    return true;
}

CreatureAI* GetAI_npc_bartleby(Creature *_creature)
{
    return new npc_bartlebyAI(_creature);
}

/*######
## npc_dashel_stonefist
######*/

struct npc_dashel_stonefistAI : public ScriptedAI
{
    npc_dashel_stonefistAI(Creature *c) : ScriptedAI(c) {}

    void Reset()
    {
        m_creature->setFaction(11);
        m_creature->setEmoteState(7);
    }

    void DamageTaken(Unit *done_by, uint32 & damage)
    {
        if((damage > m_creature->GetHealth()) || (m_creature->GetHealth() - damage)*100 / m_creature->GetMaxHealth() < 15)
        {
            //Take 0 damage
            damage = 0;

            if (done_by->GetTypeId() == TYPEID_PLAYER)
            {
                (done_by->ToPlayer())->AttackStop();
                (done_by->ToPlayer())->AreaExploredOrEventHappens(1447);
            }
            //m_creature->CombatStop();
            EnterEvadeMode();
        }
    }

    void EnterCombat(Unit *who) {}
};

bool QuestAccept_npc_dashel_stonefist(Player *player, Creature *_Creature, Quest const *_Quest)
{
    if(_Quest->GetQuestId() == 1447)
    {
        _Creature->setFaction(168);
        ((npc_dashel_stonefistAI*)_Creature->AI())->AttackStart(player);
    }
    return true;
}

CreatureAI* GetAI_npc_dashel_stonefist(Creature *_creature)
{
    return new npc_dashel_stonefistAI(_creature);
}

/*######
## npc_general_marcus_jonathan
######*/

bool ReceiveEmote_npc_general_marcus_jonathan(Player *player, Creature *_Creature, uint32 emote)
{
    if(player->GetTeam() == ALLIANCE)
    {
        if (emote == TEXTEMOTE_SALUTE)
        {
            _Creature->SetOrientation(_Creature->GetAngle(player));
            _Creature->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);
        }
        if (emote == TEXTEMOTE_WAVE)
        {
            _Creature->MonsterSay("Greetings citizen",LANG_COMMON,0);
        }
    }
    return true;
}

/*######
## npc_lady_katrana_prestor
######*/

#define GOSSIP_ITEM_KAT_1 "Excusez mon intrusion, Dame Prestor, mais le Seigneur Bolvar a suggéré que je vous demande conseil."
#define GOSSIP_ITEM_KAT_2 "Mes excuses, Dame Prestor."
#define GOSSIP_ITEM_KAT_3 "Je vous demande pardon, Dame Prestor. Ce n'était pas mon intention."
#define GOSSIP_ITEM_KAT_4 "Merci pour votre temps, Dame Prestor."

bool GossipHello_npc_lady_katrana_prestor(Player *player, Creature *_Creature)
{
    if (_Creature->isQuestGiver())
        player->PrepareQuestMenu( _Creature->GetGUID() );

    if (player->GetQuestStatus(4185) == QUEST_STATUS_INCOMPLETE)
        player->ADD_GOSSIP_ITEM( 0, GOSSIP_ITEM_KAT_1, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);

    player->SEND_GOSSIP_MENU(2693, _Creature->GetGUID());

    return true;
}

bool GossipSelect_npc_lady_katrana_prestor(Player *player, Creature *_Creature, uint32 sender, uint32 action)
{
    switch (action)
    {
        case GOSSIP_ACTION_INFO_DEF:
            player->ADD_GOSSIP_ITEM( 0, GOSSIP_ITEM_KAT_2, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            player->SEND_GOSSIP_MENU(2694, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+1:
            player->ADD_GOSSIP_ITEM( 0, GOSSIP_ITEM_KAT_3, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            player->SEND_GOSSIP_MENU(2695, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+2:
            player->ADD_GOSSIP_ITEM( 0, GOSSIP_ITEM_KAT_4, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            player->SEND_GOSSIP_MENU(2696, _Creature->GetGUID());
            break;
        case GOSSIP_ACTION_INFO_DEF+3:
            player->CLOSE_GOSSIP_MENU();
            player->AreaExploredOrEventHappens(4185);
            break;
    }
    return true;
}

/*######
## npc_innkeeper_allison
######*/

#define QUEST_FLEXING_NOUGAT    8356

bool ReceiveEmote_npc_innkeeper_allison(Player *player, Creature *_Creature, uint32 emote)
{
    if (emote == TEXTEMOTE_FLEX)
    {
        if (player->GetQuestStatus(QUEST_FLEXING_NOUGAT) == QUEST_STATUS_INCOMPLETE)
            player->AreaExploredOrEventHappens(QUEST_FLEXING_NOUGAT);
    }
    
    return true;
}

/*######
## npc_monty
######*/

bool ChooseReward_npc_monty(Player* player, Creature* creature, const Quest* quest, uint32 option)
{
    if (quest->GetQuestId() == 6661) {
        DoScriptText(-1000765, creature, NULL);
        Creature* rat = creature->FindCreatureInGrid(13017, 15.0f, true);
        while (rat) {
            rat->DisappearAndDie();
            rat->Respawn();
            rat = creature->FindCreatureInGrid(13017, 15.0f, true);
        }
    }
    
    return true;
}

/*######
## npc_bolvar_fordragon
######*/

#define GOSSIP_PACK58 "Teleportez moi a la porte des Tenebres."

bool GossipHello_npc_bolvar_fordragon(Player *player, Creature *_Creature)
{
    if (_Creature->isQuestGiver())
        player->PrepareQuestMenu( _Creature->GetGUID() );

    if (player->GetQuestStatus(80017) > QUEST_STATUS_NONE) //pack58, tp vers la porte des tenebres
        player->ADD_GOSSIP_ITEM(0, GOSSIP_PACK58, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    player->SEND_GOSSIP_MENU(_Creature->GetNpcTextId(), _Creature->GetGUID());
    return true;
}

bool GossipSelect_npc_bolvar_fordragon(Player *player, Creature *_Creature, uint32 sender, uint32 action)
{
    switch (action)
    {
        case GOSSIP_ACTION_INFO_DEF+1:
            player->TeleportTo(0, -11741.21, -3173.35, -16.49, 3.35); //Porte des tenebres
            player->CLOSE_GOSSIP_MENU();
            break;
    }
    return true;
}

bool QuestComplete_npc_bolvar_fordragon(Player* pPlayer, Creature* pCreature, Quest const* pQuest)
{
    switch(pQuest->GetQuestId())
    {
        case 80003:
            pPlayer->DoPack58(PACK58_STEP1);
            break;
        case 80009:
            pPlayer->DoPack58(PACK58_HEAL);
            break;
        case 80011:
            pPlayer->DoPack58(PACK58_MELEE);
            break;
        case 80013:
            pPlayer->DoPack58(PACK58_TANK);
            break;
        case 80015:
            pPlayer->DoPack58(PACK58_MAGIC);
            break;
    }
    return true;
}

/*######
## AddSC
######*/

void AddSC_stormwind_city()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="npc_archmage_malin";
    newscript->pGossipHello = &GossipHello_npc_archmage_malin;
    newscript->pGossipSelect = &GossipSelect_npc_archmage_malin;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_bartleby";
    newscript->GetAI = &GetAI_npc_bartleby;
    newscript->pQuestAccept = &QuestAccept_npc_bartleby;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_dashel_stonefist";
    newscript->GetAI = &GetAI_npc_dashel_stonefist;
    newscript->pQuestAccept = &QuestAccept_npc_dashel_stonefist;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_general_marcus_jonathan";
    newscript->pReceiveEmote = &ReceiveEmote_npc_general_marcus_jonathan;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_lady_katrana_prestor";
    newscript->pGossipHello = &GossipHello_npc_lady_katrana_prestor;
    newscript->pGossipSelect = &GossipSelect_npc_lady_katrana_prestor;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name="npc_innkeeper_allison";
    newscript->pReceiveEmote = &ReceiveEmote_npc_innkeeper_allison;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_monty";
    newscript->pChooseReward = &ChooseReward_npc_monty;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_bolvar_fordragon";
    newscript->pGossipHello =  &GossipHello_npc_bolvar_fordragon;
    newscript->pGossipSelect = &GossipSelect_npc_bolvar_fordragon;
    newscript->pQuestComplete = &QuestComplete_npc_bolvar_fordragon;
    newscript->RegisterSelf();
}

