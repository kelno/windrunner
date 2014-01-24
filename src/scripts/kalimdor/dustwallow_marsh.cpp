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
SDName: Dustwallow_Marsh
SD%Complete: 95
SDComment: Quest support: 558, 1173, 1324, 11126, 11180. Vendor Nat Pagle
SDCategory: Dustwallow Marsh
EndScriptData */

/* ContentData
mobs_risen_husk_spirit
npc_restless_apparition
npc_deserter_agitator
npc_lady_jaina_proudmoore
npc_nat_pagle
npc_overlord_mokmorokk
npc_private_hendel
npc_stinky
npc_cassa_crimsonwing
npc_ogron
npc_captured_totem
EndContentData */

#include "precompiled.h"
#include "EscortAI.h"
#include "SimpleCooldown.h"

/*######
## mobs_risen_husk_spirit
######*/

enum eHuskSpirit
{
SPELL_SUMMON_RESTLESS_APPARITION    = 42511,
SPELL_CONSUME_FLESH                 = 37933,          //Risen Husk
SPELL_INTANGIBLE_PRESENCE           = 43127           //Risen Spirit
};

struct mobs_risen_husk_spiritAI : public ScriptedAI
{
    mobs_risen_husk_spiritAI(Creature *c) : ScriptedAI(c) {}

    uint32 ConsumeFlesh_Timer;
    uint32 IntangiblePresence_Timer;

    void Reset()
    {
        ConsumeFlesh_Timer = 10000;
        IntangiblePresence_Timer = 5000;
    }

    void EnterCombat(Unit* pWho) { }

    void DamageTaken(Unit* pDoneBy, uint32 &damage)
    {
        if( pDoneBy->GetTypeId() == TYPEID_PLAYER )
            if (damage >= m_creature->GetHealth() && CAST_PLR(pDoneBy)->GetQuestStatus(11180) == QUEST_STATUS_INCOMPLETE)
                m_creature->CastSpell(pDoneBy, SPELL_SUMMON_RESTLESS_APPARITION, false);
    }

    void UpdateAI(const uint32 diff)
    {
        if (!UpdateVictim())
            return;

        if (ConsumeFlesh_Timer < diff)
        {
            if( m_creature->GetEntry() == 23555 )
                DoCast(m_creature->GetVictim(),SPELL_CONSUME_FLESH);
            ConsumeFlesh_Timer = 15000;
        } else ConsumeFlesh_Timer -= diff;

        if (IntangiblePresence_Timer < diff)
        {
            if( m_creature->GetEntry() == 23554 )
                DoCast(m_creature->GetVictim(),SPELL_INTANGIBLE_PRESENCE);
            IntangiblePresence_Timer = 20000;
        } else IntangiblePresence_Timer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_mobs_risen_husk_spirit(Creature* pCreature)
{
    return new mobs_risen_husk_spiritAI (pCreature);
}

/*######
## npc_restless_apparition
######*/

bool GossipHello_npc_restless_apparition(Player* pPlayer, Creature* pCreature)
{
    pPlayer->SEND_GOSSIP_MENU(pCreature->GetNpcTextId(), pCreature->GetGUID());

    pPlayer->TalkedToCreature(pCreature->GetEntry(), pCreature->GetGUID());
    pCreature->SetInt32Value(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);

    return true;
}

/*######
## npc_deserter_agitator
######*/

struct npc_deserter_agitatorAI : public ScriptedAI
{
    npc_deserter_agitatorAI(Creature *c) : ScriptedAI(c) {}

    void Reset()
    {
        m_creature->setFaction(894);
    }

    void EnterCombat(Unit* pWho) {}
};

CreatureAI* GetAI_npc_deserter_agitator(Creature* pCreature)
{
    return new npc_deserter_agitatorAI (pCreature);
}

bool GossipHello_npc_deserter_agitator(Player* pPlayer, Creature* pCreature)
{
    if (pPlayer->GetQuestStatus(11126) == QUEST_STATUS_INCOMPLETE)
    {
        pCreature->setFaction(1883);
        pCreature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
        pPlayer->TalkedToCreature(pCreature->GetEntry(), pCreature->GetGUID());
    }
    else
        pPlayer->SEND_GOSSIP_MENU(pCreature->GetNpcTextId(), pCreature->GetGUID());

    return true;
}

/*######
## npc_lady_jaina_proudmoore
######*/

#define GOSSIP_ITEM_JAINA "I know this is rather silly but i have a young ward who is a bit shy and would like your autograph."

bool GossipHello_npc_lady_jaina_proudmoore(Player* pPlayer, Creature* pCreature)
{
    if (pCreature->isQuestGiver())
        pPlayer->PrepareQuestMenu( pCreature->GetGUID() );

    if( pPlayer->GetQuestStatus(558) == QUEST_STATUS_INCOMPLETE )
        pPlayer->ADD_GOSSIP_ITEM( 0, GOSSIP_ITEM_JAINA, GOSSIP_SENDER_MAIN, GOSSIP_SENDER_INFO );

    pPlayer->SEND_GOSSIP_MENU(pCreature->GetNpcTextId(), pCreature->GetGUID());

    return true;
}

bool GossipSelect_npc_lady_jaina_proudmoore(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action)
{
    if (action == GOSSIP_SENDER_INFO)
    {
        pPlayer->SEND_GOSSIP_MENU(7012, pCreature->GetGUID());
        pPlayer->CastSpell(pPlayer, 23122, false);
    }
    return true;
}

/*######
## npc_nat_pagle
######*/

bool GossipHello_npc_nat_pagle(Player* pPlayer, Creature* pCreature)
{
    if (pCreature->isQuestGiver())
        pPlayer->PrepareQuestMenu(pCreature->GetGUID());

    if (pCreature->isVendor() && pPlayer->GetQuestRewardStatus(8227))
    {
        pPlayer->ADD_GOSSIP_ITEM(1, GOSSIP_TEXT_BROWSE_GOODS, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_TRADE);
        pPlayer->SEND_GOSSIP_MENU(7640, pCreature->GetGUID());
    }
    else
        pPlayer->SEND_GOSSIP_MENU(7638, pCreature->GetGUID());

    return true;
}

bool GossipSelect_npc_nat_pagle(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action)
{
    if (action == GOSSIP_ACTION_TRADE)
        pPlayer->SEND_VENDORLIST(pCreature->GetGUID());

    return true;
}

/*######
## npc_overlord_mokmorokk
######*/

enum eOverlordMokmorokk
{
QUEST_CHALLENGE_OVERLORD    = 1173,

FACTION_NEUTRAL             = 120,
FACTION_UNFRIENDLY          = 14    //guessed
};

struct npc_overlord_mokmorokkAI : public ScriptedAI
{
    npc_overlord_mokmorokkAI(Creature *c) : ScriptedAI(c) {}
    
    Player* player;
    
    void Reset()
    {
        m_creature->setFaction(FACTION_NEUTRAL);
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        m_creature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
        m_creature->SetHealth(m_creature->GetMaxHealth());
        m_creature->CombatStop();
        m_creature->DeleteThreatList();
    }
    
    void EnterCombat(Unit* pWho) {}
    
    void UpdateAI(const uint32 diff)
    {
        if (m_creature->getFaction() == FACTION_NEUTRAL) //if neutral, event is not running
            return;
            
        if (m_creature->GetHealth() < (m_creature->GetMaxHealth()/5.0f)) //at 20%, he stops fighting and complete the quest
        {
            player = CAST_PLR(m_creature->GetVictim());
            
            if (player && player->GetQuestStatus(QUEST_CHALLENGE_OVERLORD) == QUEST_STATUS_INCOMPLETE)
                player->KilledMonster(4500, m_creature->GetGUID());
            
            m_creature->MonsterSay("N'en jetez plus !", LANG_UNIVERSAL, 0);
            Reset();
            
            return;
        }
        
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_overlord_mokmorokk(Creature* pCreature)
{
    return new npc_overlord_mokmorokkAI(pCreature);
}

bool GossipHello_npc_overlord_mokmorokk(Player* pPlayer, Creature* pCreature)
{
    if (pCreature->isQuestGiver())
        pPlayer->PrepareQuestMenu(pCreature->GetGUID());
    
    if (pPlayer->GetQuestStatus(QUEST_CHALLENGE_OVERLORD) == QUEST_STATUS_INCOMPLETE)
        pPlayer->ADD_GOSSIP_ITEM(0, "Partez maintenant !", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
        
    pPlayer->SEND_GOSSIP_MENU(pCreature->GetNpcTextId(), pCreature->GetGUID());
    
    return true;
}

bool GossipSelect_npc_overlord_mokmorokk(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action)
{
    if (action == GOSSIP_ACTION_INFO_DEF+1)
    {
        pCreature->MonsterSay("C'est ce qu'on va voir !", LANG_UNIVERSAL, 0);
        pCreature->setFaction(FACTION_UNFRIENDLY);
        pCreature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        pCreature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE);
        pCreature->AI()->AttackStart(pPlayer);
    }
    
    return true;
}

bool QuestAccept_npc_overlord_mokmorokk(Player* player, Creature* creature, const Quest* quest) {
    if (quest->GetQuestId() == 1173) {
        creature->setFaction(FACTION_UNFRIENDLY);
        creature->AI()->AttackStart(player);
    }
    
    return true;
}

/*######
## npc_private_hendel
######*/

enum eHendel
{
    // looks like all this text ids are wrong.
    SAY_PROGRESS_1_TER          = -1000411, // signed for 3568
    SAY_PROGRESS_2_HEN          = -1000412, // signed for 3568
    SAY_PROGRESS_3_TER          = -1000413,
    SAY_PROGRESS_4_TER          = -1000414,
    EMOTE_SURRENDER             = -1000415,

    QUEST_MISSING_DIPLO_PT16    = 1324,
    FACTION_HOSTILE             = 168,                      //guessed, may be different

    NPC_SENTRY                  = 5184,                     //helps hendel
    NPC_JAINA                   = 4968,                     //appears once hendel gives up
    NPC_TERVOSH                 = 4967
};

//TODO: develop this further, end event not created
struct npc_private_hendelAI : public ScriptedAI
{
    npc_private_hendelAI(Creature* pCreature) : ScriptedAI(pCreature) { }

    void Reset()
    {
        me->RestoreFaction();
    }

    void AttackedBy(Unit* pAttacker)
    {
        if (m_creature->GetVictim())
            return;

        if (m_creature->IsFriendlyTo(pAttacker))
            return;

        AttackStart(pAttacker);
    }
    
    void EnterCombat(Unit* pWho) {}

    void DamageTaken(Unit* pDoneBy, uint32 &uiDamage)
    {
        if (uiDamage > m_creature->GetHealth() || ((m_creature->GetHealth() - uiDamage)*100 / m_creature->GetMaxHealth() < 20))
        {
            uiDamage = 0;

            if (Player* pPlayer = pDoneBy->GetCharmerOrOwnerPlayerOrPlayerItself())
                pPlayer->GroupEventHappens(QUEST_MISSING_DIPLO_PT16, m_creature);

            DoScriptText(EMOTE_SURRENDER, m_creature);
            EnterEvadeMode();
        }
    }
};

bool QuestAccept_npc_private_hendel(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_MISSING_DIPLO_PT16)
        pCreature->setFaction(FACTION_HOSTILE);

    return true;
}

CreatureAI* GetAI_npc_private_hendel(Creature* pCreature)
{
    return new npc_private_hendelAI(pCreature);
}

/*######
## npc_stinky
######*/

enum eStinky
{
    QUEST_STINKYS_ESCAPE_H                       = 1270,
    QUEST_STINKYS_ESCAPE_A                       = 1222,
    SAY_QUEST_ACCEPTED                           = -1000507,
    SAY_STAY_1                                   = -1000508,
    SAY_STAY_2                                   = -1000509,
    SAY_STAY_3                                   = -1000510,
    SAY_STAY_4                                   = -1000511,
    SAY_STAY_5                                   = -1000512,
    SAY_STAY_6                                   = -1000513,
    SAY_QUEST_COMPLETE                           = -1000514,
    SAY_ATTACKED_1                               = -1000515,
    EMOTE_DISAPPEAR                              = -1000516
};

struct npc_stinkyAI : public npc_escortAI
{
    npc_stinkyAI(Creature* pCreature) : npc_escortAI(pCreature)
    {
        completed = false;
    }

    bool completed;

    void WaypointReached(uint32 i)
    {
        Player* pPlayer = GetPlayerForEscort();
        if (!pPlayer)
            return;

        switch (i) {
        case 7:
            DoScriptText(SAY_STAY_1, me, pPlayer);
            break;
        case 11:
                DoScriptText(SAY_STAY_2, me, pPlayer);
            break;
        case 25:
                DoScriptText(SAY_STAY_3, me, pPlayer);
            break;
        case 26:
                DoScriptText(SAY_STAY_4, me, pPlayer);
            break;
        case 27:
                DoScriptText(SAY_STAY_5, me, pPlayer);
            break;
        case 28:
                DoScriptText(SAY_STAY_6, me, pPlayer);
            me->SetStandState(UNIT_STAND_STATE_KNEEL);
            break;
        case 29:
            me->SetStandState(UNIT_STAND_STATE_STAND);
            break;
        case 37:
            DoScriptText(SAY_QUEST_COMPLETE, me, pPlayer);
            me->SetSpeed(MOVE_RUN, 1.2f, true);
            //me->RemoveUnitMovementFlag(MOVEMENTFLAG_WALKING);
            if (pPlayer && pPlayer->GetQuestStatus(QUEST_STINKYS_ESCAPE_H))
                pPlayer->GroupEventHappens(QUEST_STINKYS_ESCAPE_H, me);
            if (pPlayer && pPlayer->GetQuestStatus(QUEST_STINKYS_ESCAPE_A))
                pPlayer->GroupEventHappens(QUEST_STINKYS_ESCAPE_A, me);
            completed = true;
            break;
        case 39:
            DoScriptText(EMOTE_DISAPPEAR, me);
            break;
        }
    }

    void EnterCombat(Unit* pWho)
    {
        DoScriptText(SAY_ATTACKED_1, me, pWho);
    }

    void Reset() {}

    void JustDied(Unit* /*pKiller*/)
    {
        if (completed)
            return;

        Player* pPlayer = GetPlayerForEscort();
        if (HasEscortState(STATE_ESCORT_ESCORTING) && pPlayer)
        {
            if (pPlayer->GetQuestStatus(QUEST_STINKYS_ESCAPE_H))
                pPlayer->FailQuest(QUEST_STINKYS_ESCAPE_H);
            if (pPlayer->GetQuestStatus(QUEST_STINKYS_ESCAPE_A))
                pPlayer->FailQuest(QUEST_STINKYS_ESCAPE_A);
        }
    }

   void UpdateAI(const uint32 uiDiff)
    {
        npc_escortAI::UpdateAI(uiDiff);

        if (!UpdateVictim())
            return;

        DoMeleeAttackIfReady();
    }
};

bool QuestAccept_npc_stinky(Player* pPlayer, Creature* pCreature, Quest const *quest)
{
    if (quest->GetQuestId() == QUEST_STINKYS_ESCAPE_H || QUEST_STINKYS_ESCAPE_A) {
        pCreature->setFaction(250);
        pCreature->SetStandState(UNIT_STAND_STATE_STAND);
        DoScriptText(SAY_QUEST_ACCEPTED, pCreature);
        ((npc_escortAI*)(pCreature->AI()))->Start(false, false, false, pPlayer->GetGUID(), pCreature->GetEntry());
    }
    return true;
}

CreatureAI* GetAI_npc_stinky(Creature* pCreature)
{
    return new npc_stinkyAI(pCreature);
}

/*######
## npc_cassa_crimsonwing
######*/

bool GossipHello_npc_cassa_crimsonwing(Player* player, Creature* creature)
{
    if (creature->isQuestGiver())
        player->PrepareQuestMenu(creature->GetGUID());

    if (player->GetQuestStatus(11142) == QUEST_STATUS_INCOMPLETE)
        player->ADD_GOSSIP_ITEM(0, "Je dois survoler l'île d'Alcaz.", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
        
    player->SEND_GOSSIP_MENU(creature->GetNpcTextId(), creature->GetGUID());
        
    return true;
}

bool GossipSelect_npc_cassa_crimsonwing(Player* player, Creature* creature, uint32 sender, uint32 action)
{
    player->CLOSE_GOSSIP_MENU();

    if (action == GOSSIP_ACTION_INFO_DEF) {
        player->CastSpell(player, 42316, true);
        std::vector<uint32> nodes;

        nodes.resize(2);
        nodes[0] = 180;                                     //from
        nodes[1] = 181;                                     //end at
        player->ActivateTaxiPathTo(nodes);
    }
        
    return true;
}

/*######
## npc_ogron
######*/

enum eOgron
{
    QUEST_QUESTIONING_REETHE                     = 1273
};

struct npc_ogronAI : public npc_escortAI
{
    npc_ogronAI(Creature* pCreature) : npc_escortAI(pCreature)
    {
        completed = false;
    }

    bool completed;
    uint32 step;
    uint32 timer;

    void WaypointReached(uint32 i)
    {
        Player* pPlayer = GetPlayerForEscort();
        if (!pPlayer)
            return;

        switch (i) {
        case 7:
            me->MonsterSay("C'est Reethe ! Allons voir ce qu'il a à nous dire, hein ?", LANG_UNIVERSAL, 0);
            break;
        case 8:
            step = 1;
            timer = 500;
            break;
        }
    }

    void EnterCombat(Unit* pWho)
    {
        
    }

    void Reset()
    {
        step = 0;
    }

    void JustDied(Unit* /*pKiller*/)
    {
        if (completed)
            return;

        Player* pPlayer = GetPlayerForEscort();
        if (HasEscortState(STATE_ESCORT_ESCORTING) && pPlayer)
        {
            if (pPlayer->GetQuestStatus(QUEST_QUESTIONING_REETHE))
                pPlayer->FailQuest(QUEST_QUESTIONING_REETHE);
        }
    }

   void UpdateAI(const uint32 uiDiff)
    {
        npc_escortAI::UpdateAI(uiDiff);
        
        if (step) {
            if (timer <= uiDiff) {
                Creature* reethe = me->FindCreatureInGrid(4980, 15.0f, true);
                Creature* caldwell = me->FindCreatureInGrid(5046, 20.0f, true);
                if (reethe && (caldwell || step < 8)) {
                    switch (step) {
                    case 1:
                        reethe->MonsterSay("Je vous jure, je n'ai rien volé ! Tenez, servez-vous dans mes affaires, et allez-vous en !", LANG_UNIVERSAL, 0);
                        ++step;
                        timer = 2000;
                        break;
                    case 2:
                        me->MonsterSay("Dis-nous simplement ce que tu sais de l'auberge du Repos Ombragé, et je ne vais pas t'exploser le crâne.", LANG_UNIVERSAL, 0);
                        ++step;
                        timer = 2000;
                        break;
                    case 3:
                        reethe->MonsterSay("Hé bien... Il se pourrait que j'aie pris quelques trucs à l'auberge... Mais pourquoi un ogre s'en inquiéterait-il ?", LANG_UNIVERSAL, 0);
                        ++step;
                        timer = 2000;
                        break;
                    case 4:
                        me->MonsterSay("Ecoute-moi bien, si tu ne m'en dis pas plus au sujet du feu...", LANG_UNIVERSAL, 0);
                        ++step;
                        timer = 800;
                        break;
                    case 5:
                        reethe->MonsterSay("Pas un pas de plus, ogre !", LANG_UNIVERSAL, 0);
                        ++step;
                        timer = 1500;
                        break;
                    case 6:
                        reethe->MonsterSay("Et je ne sais rien à propos de ton feu...", LANG_UNIVERSAL, 0);
                        ++step;
                        timer = 1000;
                        break;
                    case 7:
                        if (Creature* caldspawn = me->SummonCreature(5046, -3376.830322, -3208.791260, 35.163025, 5.946863, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 60000)) {
                            caldspawn->GetMotionMaster()->MovePoint(0, -3371.119385, -3212.487061, 34.137459);
                            caldspawn->setFaction(35);
                        }
                        reethe->MonsterSay("Qu'est-ce que c'était ? Avez-vous entendu quelque chose ?", LANG_UNIVERSAL, 0);
                        ++step;
                        timer = 2000;
                        break;
                    case 8:
                        if (Creature* add = me->SummonCreature(5044, -3373.989014, -3207.442871, 35.073441, 5.826434, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 60000))
                            me->setFaction(35);
                        if (Creature* add = me->SummonCreature(5044, -3374.850342, -3209.195557, 34.980534, 5.826434, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 60000))
                            me->setFaction(35);
                        if (Creature* add = me->SummonCreature(5045, -3375.389404, -3211.398682, 34.845543, 6.043204, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 60000))
                            me->setFaction(35);
                        caldwell->MonsterSay("Paval Reethe ! Enfin je vous trouve. Vous vous acoquinez avec des ogres maintenant ? N'ayez crainte, même les traîtres et les déserteurs méritent un peu de pitié.", LANG_UNIVERSAL, 0);
                        reethe->SetFlag(UNIT_FIELD_FLAGS, 0);
                        reethe->setFaction(me->getFaction());
                        ++step;
                        timer = 2000;
                        break;
                    case 9:
                        caldwell->MonsterSay("Soldat, montrez au Lieutement Reethe un peu de votre pitié.", LANG_UNIVERSAL, 0);
                        ++step;
                        timer = 800;
                        break;
                    case 10:
                        if (Creature* hallan = me->FindCreatureInGrid(5045, 20.0f, true))
                            hallan->CastSpell(reethe, 7105 /* Fake Shot*/, true);
                        ++step;
                        timer = 500;
                        break;
                    case 11:
                        reethe->MonsterSay("Hallan... Je ne pensais pas que tu avais ça en toi...", LANG_UNIVERSAL, 0);
                        ++step;
                        timer = 500;
                        break;
                    case 12:
                        caldwell->MonsterSay("Bien, maintenant, nettoyez le reste, messieurs !", LANG_UNIVERSAL, 0);
                        me->MonsterSay("Nom de... ! Tu ferais mieux de ne pas rendre l'âme devant moi, humain !", LANG_UNIVERSAL, 0);
                        ++step;
                        timer = 1000;
                        break;
                    case 13:
                    {
                        /*std::list<Creature*> skirms;
                        me->GetCreatureListWithEntryInGrid(skirms, 5044, 30.0f);
                        for (std::list<Creature*>::iterator it = skirms.begin(); it != skirms.end(); it++) {
                            (*it)->setFaction(14);
                            (*it)->AI()->AttackStart(reethe);
                        }
                        caldwell->setFaction(14);
                        caldwell->AI()->AttackStart(reethe);
                        if (Creature* hallan = me->FindCreatureInGrid(5045, 20.0f, true)) {
                            hallan->setFaction(14);
                            hallan->AI()->AttackStart(reethe);
                        }*/
                        ++step;
                        timer = 5000;
                        break;
                    }
                    case 14:
                        if (Player* pPlayer = GetPlayerForEscort())
                            pPlayer->AreaExploredOrEventHappens(QUEST_QUESTIONING_REETHE);
                        ++step;
                        timer = 999999;
                    }
                }
            }
            else
                timer -= uiDiff;
        }

        if (!UpdateVictim()) {
            if (step == 15) {
                if (Player* pPlayer = GetPlayerForEscort())
                    pPlayer->AreaExploredOrEventHappens(QUEST_QUESTIONING_REETHE);
                
                step = 0;
                timer = 0;
                completed = true;
                
                me->DisappearAndDie();
                me->Respawn();
            }
            
            return;
        }

        DoMeleeAttackIfReady();
    }
};

bool QuestAccept_npc_ogron(Player* pPlayer, Creature* pCreature, Quest const *quest)
{
    if (quest->GetQuestId() == QUEST_QUESTIONING_REETHE) {
        pCreature->setFaction(pPlayer->getFaction());
        pCreature->SetStandState(UNIT_STAND_STATE_STAND);
        pCreature->MonsterSay("J'ai remarqué du feu sur cette île, là-bas. Et un humain, aussi. Allons vérifier.", LANG_UNIVERSAL, 0);
        ((npc_escortAI*)(pCreature->AI()))->Start(true, true, false, pPlayer->GetGUID(), pCreature->GetEntry());
        ((npc_escortAI*)(pCreature->AI()))->SetDespawnAtEnd(false);
    }
    return true;
}

CreatureAI* GetAI_npc_ogron(Creature* pCreature)
{
    return new npc_ogronAI(pCreature);
}

/*######
## npc_captured_totem
######*/

struct npc_captured_totemAI : public Scripted_NoMovementAI
{
    npc_captured_totemAI(Creature* creature) : Scripted_NoMovementAI(creature)
    {
        me->SetReactState(REACT_PASSIVE);
        me->setFaction(35);
    }

    void EnterCombat(Unit* pWho)
    {
    }
    
    void MasterKilledUnit(Unit* victim)
    {
        if (!me->GetOwner())
            return;
        
        if (victim->GetEntry() == 4344 || victim->GetEntry() == 4345)
            me->GetOwner()->ToPlayer()->KilledMonster(23811, victim->GetGUID());
    }
};

CreatureAI* GetAI_npc_captured_totem(Creature* creature)
{
    return new npc_captured_totemAI(creature);
}

/*######
## npc_searingwhelp
######*/

#define TIMER_FIREBALL 6000
#define SPELL_FIREBALL 11021

struct SearingWhelpAI : public ScriptedAI
{
    SimpleCooldown* SCDBdf;

    SearingWhelpAI(Creature *c) : ScriptedAI(c)
    {
        SCDBdf = new SimpleCooldown(TIMER_FIREBALL);
    }
    
    void EnterCombat(Unit* who)
    {
        if(who)
            DoCast(who,SPELL_FIREBALL,false);
    }
    
    void Reset()
    {
        SCDBdf->reinitCD();
    }

    void UpdateAI(const uint32 diff)
    {
        if (!UpdateVictim())
            return;
        
        if(SCDBdf->CheckAndUpdate(diff) && me->GetVictim())
            DoCast(me->GetVictim(),SPELL_FIREBALL,false);
        
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_SearingWhelp(Creature *_Creature)
{
    return new SearingWhelpAI(_Creature);
}

/*######
## AddSC
######*/

void AddSC_dustwallow_marsh()
{
    Script* newscript;

    newscript = new Script;
    newscript->Name="mobs_risen_husk_spirit";
    newscript->GetAI = &GetAI_mobs_risen_husk_spirit;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_restless_apparition";
    newscript->pGossipHello =   &GossipHello_npc_restless_apparition;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_deserter_agitator";
    newscript->GetAI = &GetAI_npc_deserter_agitator;
    newscript->pGossipHello = &GossipHello_npc_deserter_agitator;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_lady_jaina_proudmoore";
    newscript->pGossipHello = &GossipHello_npc_lady_jaina_proudmoore;
    newscript->pGossipSelect = &GossipSelect_npc_lady_jaina_proudmoore;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_nat_pagle";
    newscript->pGossipHello = &GossipHello_npc_nat_pagle;
    newscript->pGossipSelect = &GossipSelect_npc_nat_pagle;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_overlord_mokmorokk";
    newscript->pGossipHello =  &GossipHello_npc_overlord_mokmorokk;
    newscript->pGossipSelect = &GossipSelect_npc_overlord_mokmorokk;
    newscript->pQuestAccept = &QuestAccept_npc_overlord_mokmorokk;
    newscript->GetAI = &GetAI_npc_overlord_mokmorokk;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_private_hendel";
    newscript->GetAI = &GetAI_npc_private_hendel;
    newscript->pQuestAccept = &QuestAccept_npc_private_hendel;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_stinky";
    newscript->GetAI = &GetAI_npc_stinky;
    newscript->pQuestAccept = &QuestAccept_npc_stinky;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_cassa_crimsonwing";
    newscript->pGossipHello = &GossipHello_npc_cassa_crimsonwing;
    newscript->pGossipSelect = &GossipSelect_npc_cassa_crimsonwing;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_ogron";
    newscript->GetAI = &GetAI_npc_ogron;
    newscript->pQuestAccept = &QuestAccept_npc_ogron;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_captured_totem";
    newscript->GetAI = &GetAI_npc_captured_totem;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_searingwhelp";
    newscript->GetAI = &GetAI_SearingWhelp;
    newscript->RegisterSelf();
}
