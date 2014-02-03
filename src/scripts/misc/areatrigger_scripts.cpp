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
SDName: Areatrigger_Scripts
SD%Complete: 100
SDComment: Scripts for areatriggers
SDCategory: Areatrigger
EndScriptData */

/* ContentData
at_legion_teleporter    4560 Teleporter TO Invasion Point: Cataclysm
at_test                 script test only
at_coilfang_waterfall   4591
at_mechanar             4614
at_botanica             4612
at_orb_of_command
at_childweek_quest911   3549
at_quest_whispers_raven_god
at_quest_ravenholdt     3066
EndContentData */

#include "precompiled.h"

/*#####
## at_legion_teleporter
#####*/

#define SPELL_TELE_A_TO   37387
#define SPELL_TELE_H_TO   37389

bool AreaTrigger_at_legion_teleporter(Player *player, AreaTriggerEntry const *at)
{
    if (player->IsAlive() && !player->IsInCombat())
    {
        if (player->GetTeam()== ALLIANCE && player->GetQuestRewardStatus(10589))
        {
            player->CastSpell(player,SPELL_TELE_A_TO,false);
            return true;
        }

        if (player->GetTeam()== HORDE && player->GetQuestRewardStatus(10604))
        {
            player->CastSpell(player,SPELL_TELE_H_TO,false);
            return true;
        }

        return false;
    }
    return false;
}

bool ATtest(Player *player, AreaTriggerEntry const *at)
{
    player->Say("Hi!",LANG_UNIVERSAL);
    return true;
}

/*######
## at_coilfang_waterfall
######*/

enum
{
    GO_COILFANG_WATERFALL   = 184212
};

bool AreaTrigger_at_coilfang_waterfall(Player* pPlayer, AreaTriggerEntry const *pAt)
{
    if (GameObject* pGo = pPlayer->FindGOInGrid(GO_COILFANG_WATERFALL, 50.0f))
    {
        if (pGo->getLootState() == GO_READY)
            pGo->UseDoorOrButton();
    }

    return false;
}

/*######
## at_scent_larkorwi
######*/

enum eScentLarkorwi
{
    QUEST_SCENT_OF_LARKORWI                     = 4291,
    NPC_LARKORWI_MATE                           = 9683
};

bool AreaTrigger_at_scent_larkorwi(Player* pPlayer, AreaTriggerEntry const *pAt)
{
    if (!pPlayer->isDead() && pPlayer->GetQuestStatus(QUEST_SCENT_OF_LARKORWI) == QUEST_STATUS_INCOMPLETE)
    {
        if (!pPlayer->FindCreatureInGrid(NPC_LARKORWI_MATE,15,true))
            pPlayer->SummonCreature(NPC_LARKORWI_MATE, pPlayer->GetPositionX()+5, pPlayer->GetPositionY(), pPlayer->GetPositionZ(), 3.3, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 100000);
    }


    return false;
}

/*######
## at_mechanar
######*/

#define DATA_PATHALEON  5

bool AreaTrigger_at_mechanar(Player* pPlayer, AreaTriggerEntry const *pAt) {
    if (ScriptedInstance* pInstance = ((ScriptedInstance*)pPlayer->GetInstanceData())) {
        if (pInstance->GetData(DATA_PATHALEON) == DONE)
            pPlayer->TeleportTo(554, -25.639709, 0.213936, -1.812820, 3.076223);
        
        return true;
    }
    return false;
}

/*######
## at_botanica
######*/

bool AreaTrigger_at_botanica(Player *pPlayer, AreaTriggerEntry const *pAt) {
    pPlayer->TeleportTo(553, 41.069, -29.975, -1.12, 5.49);
    
    return true;
}

/*######
## at_orb_of_command
######*/

bool AreaTrigger_at_orb_of_command(Player* pPlayer, AreaTriggerEntry const *pAt) {
    if (pPlayer->isDead() && pPlayer->GetQuestRewardStatus(7761))       // TODO: Check that player's corpse is in instance
        pPlayer->TeleportTo(469, -7673.03, -1106.08, 396.65, 0.7);

    return true;
}

/*######
## at_childweek_quest911
######*/

bool AreaTrigger_at_childweek_quest911(Player* pPlayer, AreaTriggerEntry const *pAt) {
    if (pPlayer->GetQuestStatus(911) == QUEST_STATUS_INCOMPLETE) {
        if (Pet* pet = pPlayer->GetMiniPet()) {
            if (pet->GetEntry() == 14444)
                pPlayer->AreaExploredOrEventHappens(911);
        }
    }
    
    return true;
}

/*######
## at_childweek_quest1800
######*/

bool AreaTrigger_at_childweek_quest1800(Player* pPlayer, AreaTriggerEntry const *pAt) {
    if (pPlayer->GetQuestStatus(1800) == QUEST_STATUS_INCOMPLETE) {
        if (Pet* pet = pPlayer->GetMiniPet()) {
            if (pet->GetEntry() == 14444)
                pPlayer->AreaExploredOrEventHappens(1800);
        }
    }
    
    return true;
}

/*######
## at_childweek_quest910
######*/

bool AreaTrigger_at_childweek_quest910(Player* pPlayer, AreaTriggerEntry const *pAt) {
    if (pPlayer->GetQuestStatus(910) == QUEST_STATUS_INCOMPLETE) {
        if (Pet* pet = pPlayer->GetMiniPet()) {
            if (pet->GetEntry() == 14444)
                pPlayer->AreaExploredOrEventHappens(910);
        }
    }
    
    return true;
}

/*######
## at_childweek_quest1479
######*/

bool AreaTrigger_at_childweek_quest1479(Player* pPlayer, AreaTriggerEntry const *pAt) {
    if (pPlayer->GetQuestStatus(1479) == QUEST_STATUS_INCOMPLETE) {
        if (Pet* pet = pPlayer->GetMiniPet()) {
            if (pet->GetEntry() == 14305)
                pPlayer->AreaExploredOrEventHappens(1479);
        }
    }
    
    return true;
}

/*######
## at_childweek_quest1558
######*/

bool AreaTrigger_at_childweek_quest1558(Player* pPlayer, AreaTriggerEntry const *pAt) {
    if (pPlayer->GetQuestStatus(1558) == QUEST_STATUS_INCOMPLETE) {
        if (Pet* pet = pPlayer->GetMiniPet()) {
            if (pet->GetEntry() == 14305)
                pPlayer->AreaExploredOrEventHappens(1558);
        }
    }
    
    return true;
}

/*######
## at_childweek_quest1687
######*/

bool AreaTrigger_at_childweek_quest1687(Player* pPlayer, AreaTriggerEntry const *pAt) {
    if (pPlayer->GetQuestStatus(1687) == QUEST_STATUS_INCOMPLETE) {
        if (Pet* pet = pPlayer->GetMiniPet()) {
            if (pet->GetEntry() == 14305)
                pPlayer->AreaExploredOrEventHappens(1687);
        }
    }
    
    return true;
}

/*######
## at_childweek_quest10951
######*/

bool AreaTrigger_at_childweek_quest10951(Player* pPlayer, AreaTriggerEntry const *pAt) {
    if (pPlayer->GetQuestStatus(10951) == QUEST_STATUS_INCOMPLETE || pPlayer->GetQuestStatus(10952) == QUEST_STATUS_INCOMPLETE) {
        if (Pet* pet = pPlayer->GetMiniPet()) {
            if (pet->GetEntry() == 22817)
                pPlayer->AreaExploredOrEventHappens(10951); // Horde
            else if (pet->GetEntry() == 22818)
                pPlayer->AreaExploredOrEventHappens(10952); // Alliance
        }
    }
    
    return true;
}

/*######
## at_shade_of_eranikus
######*/

bool AreaTrigger_at_shade_of_eranikus(Player* pPlayer, AreaTriggerEntry const *pAt) {
    if (pPlayer->GetQuestRewardStatus(8555) && pPlayer->GetReputationRank(910) >= REP_NEUTRAL) {
        if (!pPlayer->FindCreatureInGrid(15362, 100, true))
            pPlayer->SummonCreature(15362, -672.965149, 6.674169, -90.835663, 1.539716, TEMPSUMMON_TIMED_DESPAWN, 3600000);
    }
	return true;
}

/*######
## at_crashing_wickerman
######*/

bool AreaTrigger_at_crashing_wickerman(Player* player, AreaTriggerEntry const* pAt)
{
    if (player->GetQuestStatus(1658) == QUEST_STATUS_INCOMPLETE)
        player->AreaExploredOrEventHappens(1658);
        
    return true;
}

/*######
## at_quest_whispers_raven_god
######*/

bool AreaTrigger_at_quest_whispers_raven_god(Player* player, AreaTriggerEntry const* at)
{
    if (player->GetQuestStatus(10607) != QUEST_STATUS_INCOMPLETE)
        return true;
        
    if (!player->HasAura(37466))
        return true;
        
    uint32 credit = 0;
    switch (at->id) {
    case 4613:
        credit = 22798;
        break;
    case 4615:
        credit = 22799;
        break;
    case 4616:
        credit = 22800;
        break;
    case 4617:
        credit = 22801;
        break;
    default:
        sLog.outError("AreaTrigger_at_quest_whispers_raven_god: wrong areatrigger %u", at->id);
        credit = 0;
        break;
    }
    
    player->KilledMonster(credit, 0);
    
    return true;
}

/*######
## at_quest_ravenholdt
######*/

bool AreaTrigger_at_quest_ravenholdt(Player* player, AreaTriggerEntry const* at)
{
    if (player->GetQuestStatus(6681) != QUEST_STATUS_INCOMPLETE)
        return true;
        
    player->KilledMonster(13936, 0);
    return true;
}

/*######
## at_quest_simmer_down_now
######*/

bool AreaTrigger_at_quest_simmer_down_now(Player* player, AreaTriggerEntry const* at)
{
    if (player->GetQuestStatus(25) != QUEST_STATUS_INCOMPLETE)
        return true;

    player->AreaExploredOrEventHappens(25);
    return true;
}

void AddSC_areatrigger_scripts()
{
    Script* newscript;

    newscript = new Script;
    newscript->Name = "at_legion_teleporter";
    newscript->pAreaTrigger = &AreaTrigger_at_legion_teleporter;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="at_test";
    newscript->pAreaTrigger = &ATtest;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "at_coilfang_waterfall";
    newscript->pAreaTrigger = &AreaTrigger_at_coilfang_waterfall;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "at_scent_larkorwi";
    newscript->pAreaTrigger = &AreaTrigger_at_scent_larkorwi;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "at_mechanar";
    newscript->pAreaTrigger = &AreaTrigger_at_mechanar;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "at_botanica";
    newscript->pAreaTrigger = &AreaTrigger_at_botanica;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "at_orb_of_command";
    newscript->pAreaTrigger = &AreaTrigger_at_orb_of_command;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "at_childweek_quest911";
    newscript->pAreaTrigger = &AreaTrigger_at_childweek_quest911;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "at_childweek_quest1800";
    newscript->pAreaTrigger = &AreaTrigger_at_childweek_quest1800;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "at_childweek_quest910";
    newscript->pAreaTrigger = &AreaTrigger_at_childweek_quest910;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "at_childweek_quest1479";
    newscript->pAreaTrigger = &AreaTrigger_at_childweek_quest1479;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "at_childweek_quest1558";
    newscript->pAreaTrigger = &AreaTrigger_at_childweek_quest1558;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "at_childweek_quest1687";
    newscript->pAreaTrigger = &AreaTrigger_at_childweek_quest1687;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "at_childweek_quest10951";
    newscript->pAreaTrigger = &AreaTrigger_at_childweek_quest10951;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "at_shade_of_eranikus";
    newscript->pAreaTrigger = &AreaTrigger_at_shade_of_eranikus;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "at_crashing_wickerman";
    newscript->pAreaTrigger = &AreaTrigger_at_crashing_wickerman;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "at_quest_whispers_raven_god";
    newscript->pAreaTrigger = &AreaTrigger_at_quest_whispers_raven_god;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "at_quest_ravenholdt";
    newscript->pAreaTrigger = &AreaTrigger_at_quest_ravenholdt;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "at_quest_simmer_down_now";
    newscript->pAreaTrigger = &AreaTrigger_at_quest_simmer_down_now;
    newscript->RegisterSelf();
}

