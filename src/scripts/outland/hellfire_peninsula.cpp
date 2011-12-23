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
SDName: Hellfire_Peninsula
SD%Complete: 100
SDComment: Quest support: 9375, 9418, 10129, 10146, 10162, 10163, 10340, 10346, 10347, 10382 (Special flight paths), 10629, 10838, 10909, 11516
SDCategory: Hellfire Peninsula
EndScriptData */

/* ContentData
npc_aeranas
go_haaleshi_altar
npc_wing_commander_dabiree
npc_gryphoneer_windbellow
npc_wing_commander_brack
npc_wounded_blood_elf
npc_fel_guard_hound
npc_anchorite_relic
npc_living_flare
npc_ancestral_spirit_wolf
npc_anchorite_barada
npc_pathaleon_image
npc_demoniac_scryer
npc_magic_sucker_device_spawner
npc_sedai_quest_credit_marker
npc_vindicator_sedai
EndContentData */

#include "precompiled.h"
#include "EscortAI.h"

/*######
## npc_aeranas
######*/

enum eAeranas
{
SAY_SUMMON                      = -1000138,
SAY_FREE                        = -1000139,

FACTION_HOSTILE                 = 16,
FACTION_FRIENDLY                = 35,

SPELL_ENVELOPING_WINDS          = 15535,
SPELL_SHOCK                     = 12553,

C_AERANAS                       = 17085
};

struct npc_aeranasAI : public ScriptedAI
{
    npc_aeranasAI(Creature* c) : ScriptedAI(c) {}

    uint32 Faction_Timer;
    uint32 EnvelopingWinds_Timer;
    uint32 Shock_Timer;

    void Reset()
    {
        Faction_Timer = 8000;
        EnvelopingWinds_Timer = 9000;
        Shock_Timer = 5000;

        m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
        m_creature->setFaction(FACTION_FRIENDLY);

        DoScriptText(SAY_SUMMON, m_creature);
    }

    void Aggro(Unit* pWho) {}

    void UpdateAI(const uint32 diff)
    {
        if (Faction_Timer)
        {
            if (Faction_Timer < diff)
            {
                m_creature->setFaction(FACTION_HOSTILE);
                Faction_Timer = 0;
            } else Faction_Timer -= diff;
        }

        if (!UpdateVictim())
            return;

        if ((m_creature->GetHealth()*100) / m_creature->GetMaxHealth() < 30)
        {
            m_creature->setFaction(FACTION_FRIENDLY);
            m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
            m_creature->RemoveAllAuras();
            m_creature->DeleteThreatList();
            m_creature->CombatStop();
            DoScriptText(SAY_FREE, m_creature);
            return;
        }

        if (Shock_Timer < diff)
        {
            DoCast(m_creature->getVictim(),SPELL_SHOCK);
            Shock_Timer = 10000;
        } else Shock_Timer -= diff;

        if (EnvelopingWinds_Timer < diff)
        {
            DoCast(m_creature->getVictim(),SPELL_ENVELOPING_WINDS);
            EnvelopingWinds_Timer = 25000;
        } else EnvelopingWinds_Timer -= diff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_aeranas(Creature* pCreature)
{
    return new npc_aeranasAI(pCreature);
}

/*######
## go_haaleshi_altar
######*/

bool GOHello_go_haaleshi_altar(Player* pPlayer, GameObject* pGo)
{
    pGo->SummonCreature(C_AERANAS,-1321.79, 4043.80, 116.24, 1.25, TEMPSUMMON_TIMED_DESPAWN, 180000);
    return false;
}

/*######
## npc_wing_commander_dabiree
######*/

#define GOSSIP_ITEM1_DAB "Fly me to Murketh and Shaadraz Gateways"
#define GOSSIP_ITEM2_DAB "Fly me to Shatter Point"

bool GossipHello_npc_wing_commander_dabiree(Player* pPlayer, Creature* pCreature)
{
    if (pCreature->isQuestGiver())
        pPlayer->PrepareQuestMenu(pCreature->GetGUID());

    //Mission: The Murketh and Shaadraz Gateways
    if (pPlayer->GetQuestStatus(10146) == QUEST_STATUS_INCOMPLETE)
        pPlayer->ADD_GOSSIP_ITEM(2, GOSSIP_ITEM1_DAB, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

    //Shatter Point
    if (!pPlayer->GetQuestRewardStatus(10340))
        pPlayer->ADD_GOSSIP_ITEM(2, GOSSIP_ITEM2_DAB, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);

    pPlayer->SEND_GOSSIP_MENU(pCreature->GetNpcTextId(), pCreature->GetGUID());

    return true;
}

bool GossipSelect_npc_wing_commander_dabiree(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action )
{
    if (action == GOSSIP_ACTION_INFO_DEF + 1)
    {
        pPlayer->CLOSE_GOSSIP_MENU();
        pPlayer->CastSpell(pPlayer, 33768, true);               //TaxiPath 585 (Gateways Murket and Shaadraz)
    }
    if (action == GOSSIP_ACTION_INFO_DEF + 2)
    {
        pPlayer->CLOSE_GOSSIP_MENU();
        pPlayer->CastSpell(pPlayer, 35069, true);               //TaxiPath 612 (Taxi - Hellfire Peninsula - Expedition Point to Shatter Point)
    }
    return true;
}

/*######
## npc_gryphoneer_windbellow
######*/

#define GOSSIP_ITEM1_WIN "Fly me to The Abyssal Shelf"
#define GOSSIP_ITEM2_WIN "Fly me to Honor Point"

bool GossipHello_npc_gryphoneer_windbellow(Player* pPlayer, Creature* pCreature)
{
    if (pCreature->isQuestGiver())
        pPlayer->PrepareQuestMenu( pCreature->GetGUID() );

    //Mission: The Abyssal Shelf || Return to the Abyssal Shelf
    if (pPlayer->GetQuestStatus(10163) == QUEST_STATUS_INCOMPLETE || pPlayer->GetQuestStatus(10346) == QUEST_STATUS_INCOMPLETE)
        pPlayer->ADD_GOSSIP_ITEM(2, GOSSIP_ITEM1_WIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

    //Go to the Front
    if (pPlayer->GetQuestStatus(10382) != QUEST_STATUS_NONE && !pPlayer->GetQuestRewardStatus(10382))
        pPlayer->ADD_GOSSIP_ITEM(2, GOSSIP_ITEM2_WIN, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);

    pPlayer->SEND_GOSSIP_MENU(pCreature->GetNpcTextId(), pCreature->GetGUID());

    return true;
}

bool GossipSelect_npc_gryphoneer_windbellow(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action )
{
    if (action == GOSSIP_ACTION_INFO_DEF + 1)
    {
        pPlayer->CLOSE_GOSSIP_MENU();
        pPlayer->CastSpell(pPlayer, 33899, true);               //TaxiPath 589 (Aerial Assault Flight (Alliance))
    }
    if (action == GOSSIP_ACTION_INFO_DEF + 2)
    {
        pPlayer->CLOSE_GOSSIP_MENU();
        pPlayer->CastSpell(pPlayer, 35065, true);               //TaxiPath 607 (Taxi - Hellfire Peninsula - Shatter Point to Beach Head)
    }
    return true;
}

/*######
## npc_gryphoneer_leafbeard
######*/

enum
{
    SPELL_TAXI_TO_SHATTERP      = 35066
};

#define GOSSIP_ITEM1_LEAF       "Fly me to Shatter Point"

bool GossipHello_npc_gryphoneer_leafbeard(Player* pPlayer, Creature *pCreature)
{
    //Go back to Shatter Point if player has completed the quest 10340 - Shatter Point
    if (pPlayer->GetQuestStatus(10340) == QUEST_STATUS_COMPLETE)
        pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM1_LEAF, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    pPlayer->SEND_GOSSIP_MENU(pCreature->GetNpcTextId(), pCreature->GetGUID());
    return true;
}

bool GossipSelect_npc_gryphoneer_leafbeard(Player* pPlayer, Creature *pCreature, uint32 sender, uint32 action)
{
    if (action == GOSSIP_ACTION_INFO_DEF+1)
    {
        pPlayer->CLOSE_GOSSIP_MENU();
        //TaxiPath 609 (3.x.x)
        pPlayer->CastSpell(pPlayer, SPELL_TAXI_TO_SHATTERP, true);
    }
    return true;
}

/*######
## npc_wing_commander_brack
######*/

#define GOSSIP_ITEM1_BRA "Fly me to Murketh and Shaadraz Gateways"
#define GOSSIP_ITEM2_BRA "Fly me to The Abyssal Shelf"
#define GOSSIP_ITEM3_BRA "Fly me to Spinebreaker Post"

bool GossipHello_npc_wing_commander_brack(Player* pPlayer, Creature* pCreature)
{
    if (pCreature->isQuestGiver())
        pPlayer->PrepareQuestMenu( pCreature->GetGUID() );

    //Mission: The Murketh and Shaadraz Gateways
    if (pPlayer->GetQuestStatus(10129) == QUEST_STATUS_INCOMPLETE)
        pPlayer->ADD_GOSSIP_ITEM(2, GOSSIP_ITEM1_BRA, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

    //Mission: The Abyssal Shelf || Return to the Abyssal Shelf
    if (pPlayer->GetQuestStatus(10162) == QUEST_STATUS_INCOMPLETE || pPlayer->GetQuestStatus(10347) == QUEST_STATUS_INCOMPLETE)
        pPlayer->ADD_GOSSIP_ITEM(2, GOSSIP_ITEM2_BRA, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);

    //Spinebreaker Post
    if (pPlayer->GetQuestStatus(10242) == QUEST_STATUS_COMPLETE && !pPlayer->GetQuestRewardStatus(10242))
        pPlayer->ADD_GOSSIP_ITEM(2, GOSSIP_ITEM3_BRA, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);

    pPlayer->SEND_GOSSIP_MENU(pCreature->GetNpcTextId(), pCreature->GetGUID());

    return true;
}

bool GossipSelect_npc_wing_commander_brack(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action )
{
    switch(action)
    {
    case GOSSIP_ACTION_INFO_DEF + 1:
        pPlayer->CLOSE_GOSSIP_MENU();
        pPlayer->CastSpell(pPlayer,33659,true);               //TaxiPath 584 (Gateways Murket and Shaadraz)
        break;
    case GOSSIP_ACTION_INFO_DEF + 2:
        pPlayer->CLOSE_GOSSIP_MENU();
        pPlayer->CastSpell(pPlayer,33825,true);               //TaxiPath 587 (Aerial Assault Flight (Horde))
        break;
    case GOSSIP_ACTION_INFO_DEF + 3:
        pPlayer->CLOSE_GOSSIP_MENU();
        pPlayer->CastSpell(pPlayer,34578,true);               //TaxiPath 604 (Taxi - Reaver's Fall to Spinebreaker Ridge)
        break;
    }
        return true;
}

/*######
## npc_wounded_blood_elf
######*/

enum eWoundedElf
{
SAY_ELF_START               = -1000117,
SAY_ELF_SUMMON1             = -1000118,
SAY_ELF_RESTING             = -1000119,
SAY_ELF_SUMMON2             = -1000120,
SAY_ELF_COMPLETE            = -1000121,
SAY_ELF_AGGRO               = -1000122,

QUEST_ROAD_TO_FALCON_WATCH  = 9375
};

struct npc_wounded_blood_elfAI : public npc_escortAI
{
    npc_wounded_blood_elfAI(Creature *c) : npc_escortAI(c) {}

    void WaypointReached(uint32 i)
    {
        Player* player = GetPlayerForEscort();

        if (!player)
            return;

        switch (i)
        {
        case 0:
            DoScriptText(SAY_ELF_START, m_creature, player);
            break;
        case 9:
            DoScriptText(SAY_ELF_SUMMON1, m_creature, player);
            // Spawn two Haal'eshi Talonguard
            DoSpawnCreature(16967, -15, -15, 0, 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000);
            DoSpawnCreature(16967, -17, -17, 0, 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000);
            break;
        case 13:
            DoScriptText(SAY_ELF_RESTING, m_creature, player);
            // make the NPC kneel
            m_creature->HandleEmoteCommand(EMOTE_ONESHOT_KNEEL);
            break;
        case 14:
            DoScriptText(SAY_ELF_SUMMON2, m_creature, player);
            // Spawn two Haal'eshi Windwalker
            DoSpawnCreature(16966, -15, -15, 0, 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000);
            DoSpawnCreature(16966, -17, -17, 0, 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 5000);
            break;
        case 27:
            DoScriptText(SAY_ELF_COMPLETE, m_creature, player);
            // Award quest credit
            Player* player = GetPlayerForEscort();
            if (player)
                player->GroupEventHappens(QUEST_ROAD_TO_FALCON_WATCH,m_creature);
            break;
        }
    }

    void Reset()
    {
        if (!IsBeingEscorted)
            m_creature->setFaction(1604);
    }

    void Aggro(Unit* pWho)
    {
        if (IsBeingEscorted)
            DoScriptText(SAY_ELF_AGGRO, m_creature);
    }

    void JustSummoned(Creature* pSummoned)
    {
        pSummoned->AI()->AttackStart(m_creature);
    }

    void JustDied(Unit* pKiller)
    {
        if (!IsBeingEscorted)
            return;

        if (PlayerGUID)
        {
            // If NPC dies, player fails the quest
            Player* player = GetPlayerForEscort();
            if (player)
                player->FailQuest(QUEST_ROAD_TO_FALCON_WATCH);
        }
    }

    void UpdateAI(const uint32 diff)
    {
        npc_escortAI::UpdateAI(diff);
    }
};

CreatureAI* GetAI_npc_wounded_blood_elf(Creature* pCreature)
{
    return new npc_wounded_blood_elfAI(pCreature);
}

bool QuestAccept_npc_wounded_blood_elf(Player* pPlayer, Creature* pCreature, Quest const* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_ROAD_TO_FALCON_WATCH)
    {
        CAST_AI(npc_escortAI, (pCreature->AI()))->Start(true, true, false, pPlayer->GetGUID(), pCreature->GetEntry());
        // Change faction so mobs attack
        pCreature->setFaction(775);
    }

    return true;
}

/*######
## npc_fel_guard_hound
######*/

enum eFelGuard
{
SPELL_SUMMON_POO    = 37688,
SPELL_STANKY        = 37695,

DERANGED_HELBOAR    = 16863
};

struct npc_fel_guard_houndAI : public ScriptedAI
{
    npc_fel_guard_houndAI(Creature* c) : ScriptedAI(c) {}
    
    uint32 checkTimer;
    uint64 lastHelboar; //store last helboar GUID to prevent multiple spawns of poo with the same mob
    
    void Reset()
    {
        m_creature->GetMotionMaster()->MoveFollow(m_creature->GetOwner(), PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
        checkTimer = 5000; //check for creature every 5 sec
    }
    
    void Aggro(Unit* pWho) {}
    
    void UpdateAI(const uint32 diff)
    {
        if (checkTimer < diff)
        {
            Creature* helboar = m_creature->FindCreatureInGrid(DERANGED_HELBOAR, 10, false);
            if (helboar && helboar->GetGUID() != lastHelboar)
            {
                lastHelboar = helboar->GetGUID();
                DoCast(m_creature, SPELL_SUMMON_POO);
                DoCast(m_creature->GetOwner(), SPELL_STANKY);
                helboar->RemoveCorpse();
                checkTimer = 5000;
            }
        }else checkTimer -= diff;
    }
};

CreatureAI* GetAI_npc_fel_guard_hound(Creature* pCreature)
{
    return new npc_fel_guard_houndAI(pCreature);
}

/*######
## npc_anchorite_relic
######*/

enum eAnchoriteRelic
{
MOB_BERSERKER   = 16878,
MOB_FEL_SPIRIT  = 22454
};

struct npc_anchorite_relicAI : public ScriptedAI
{
    npc_anchorite_relicAI(Creature* c) : ScriptedAI(c) {}
    
    uint32 checkTimer;
    bool hasTarget;
    Creature* berserker;
    
    void Reset()
    {
        checkTimer = 5000;
        hasTarget = false;
        berserker = NULL;
    }
    
    void Aggro(Unit* pWho) {}
    
    Creature* SelectCreatureInGrid(uint32 entry, float range)
    {
        Creature* pCreature = NULL;

        CellPair pair(Trinity::ComputeCellPair(m_creature->GetPositionX(), m_creature->GetPositionY()));
        Cell cell(pair);
        cell.data.Part.reserved = ALL_DISTRICT;
        cell.SetNoCreate();

        Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck creature_check(*m_creature, entry, true, range); //true, as it should check only for alive creatures
        Trinity::CreatureLastSearcher<Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck> searcher(pCreature, creature_check);

        TypeContainerVisitor<Trinity::CreatureLastSearcher<Trinity::NearestCreatureEntryWithLiveStateInObjectRangeCheck>, GridTypeMapContainer> creature_searcher(searcher);

        cell.Visit(pair, creature_searcher, *m_creature->GetMap());
        
        return pCreature;
    }
    
    void UpdateAI(const uint32 diff)
    {
        if (checkTimer < diff)
        {
            if (!hasTarget) //can handle only one orc at a time, dunno if it's blizzlike, but it's easier :p
            {
                berserker = SelectCreatureInGrid(MOB_BERSERKER, 20);
                if (berserker)
                    hasTarget = true;
                //here, m_creature should cast a channelling spell on the select orc, but I don't know which one...
            }
        }else checkTimer -= diff;
        
        //at each update, check if the orc is dead; if he is, summon a fel spirit (npc 22454) at his position
        if (berserker && !berserker->isAlive())
        {
            m_creature->SummonCreature(MOB_FEL_SPIRIT, berserker->GetPositionX(), berserker->GetPositionY(), berserker->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 15000)->AI()->AttackStart(m_creature->GetOwner());
            hasTarget = false;
            berserker = NULL; //unset berserker
        }
    }
};

CreatureAI* GetAI_npc_anchorite_relic(Creature* pCreature)
{
    return new npc_anchorite_relicAI(pCreature);
}

/*######
## npc_living_flare
######*/
enum
{
    SPELL_COSMETIC             = 44880,
    SPELL_UNSTABLE_COSMETIC    = 46196,
    SPELL_ABSORBED             = 44944,
    NPC_FEL_SPARK              = 22323,
    NPC_TRIGGER                = 24959,
    GO_FIRE                    = 185319
};

struct npc_living_flareAI : public ScriptedAI
{
    npc_living_flareAI(Creature* pCreature) : ScriptedAI(pCreature) {}
    uint32 uiCheckTimer;
    uint64 uiSparkGUID;
    uint32 Count;

    void Reset()
    {
        DoCast(me, SPELL_COSMETIC);
        me->GetMotionMaster()->MoveFollow(me->GetOwner(), PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
        uiCheckTimer = 8000;
        uiSparkGUID = 0;
        Count = 0;
    }

    void AttackedBy(Unit* pWho) {}
    void AttackStart(Unit* pWho) {}
    void Aggro(Unit* who) {}

    void UpdateAI(const uint32 uiDiff)
    {
        if (uiCheckTimer <= uiDiff)
        {
            if (Creature* pSpark = me->FindNearestCreature(NPC_FEL_SPARK, 10.0f, false))
            {
                if (pSpark->GetGUID() != uiSparkGUID && CAST_PLR(me->GetOwner())->GetQuestStatus(11516) == QUEST_STATUS_INCOMPLETE)
                {
                    if (Count <= 7)
                    {
                        ++Count;
                        DoCast(me, SPELL_ABSORBED);
                        uiSparkGUID = pSpark->GetGUID();
                    }
                    else DoCast(me, SPELL_UNSTABLE_COSMETIC);
                }
            }

            if (Creature* pTrigger = me->FindNearestCreature(NPC_TRIGGER, 8.0f, true))
            {
                pTrigger->SummonGameObject(GO_FIRE, pTrigger->GetPositionX(), pTrigger->GetPositionY(), pTrigger->GetPositionZ(), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 60);
                CAST_PLR(me->GetOwner())->AreaExploredOrEventHappens(11516);
                me->setDeathState(CORPSE);
            }
        }
        else uiCheckTimer -= uiDiff;
    }
};

CreatureAI* GetAI_npc_living_flare(Creature* pCreature)
{
    return new npc_living_flareAI(pCreature);
}

/*######
## npc_ancestral_wolf
######*/

enum eAncestralWolf
{
    EMOTE_WOLF_LIFT_HEAD            = -1000496,
    EMOTE_WOLF_HOWL                 = -1000497,
    SAY_WOLF_WELCOME                = -1000498,

    SPELL_ANCESTRAL_WOLF_BUFF       = 29981,

    NPC_RYGA                        = 17123
};

struct npc_ancestral_wolfAI : public npc_escortAI
{
    npc_ancestral_wolfAI(Creature* pCreature) : npc_escortAI(pCreature)
    {
        if (pCreature->GetOwner() && pCreature->GetOwner()->GetTypeId() == TYPEID_PLAYER)
            Start(false, false, true, pCreature->GetOwner()->GetGUID(), pCreature->GetEntry());
        else
            sLog.outError("TRINITY: npc_ancestral_wolf can not obtain owner or owner is not a player.");

        pCreature->SetSpeed(MOVE_WALK, 1.5f);
        Reset();
    }

    Unit* pRyga;

    void Reset()
    {
        pRyga = NULL;
        DoCast(me, SPELL_ANCESTRAL_WOLF_BUFF, true);
    }

    void Aggro(Unit *pWho) {}

    void MoveInLineOfSight(Unit* pWho)
    {
        if (!pRyga && pWho->GetTypeId() == TYPEID_UNIT && pWho->GetEntry() == NPC_RYGA && me->IsWithinDistInMap(pWho, 15.0f))
            pRyga = pWho;

        npc_escortAI::MoveInLineOfSight(pWho);
    }

    void WaypointReached(uint32 uiPointId)
    {
        switch(uiPointId)
        {
            case 0:
                DoScriptText(EMOTE_WOLF_LIFT_HEAD, me);
                break;
            case 2:
                DoScriptText(EMOTE_WOLF_HOWL, me);
                break;
            case 50:
                if (pRyga && pRyga->isAlive() && !pRyga->isInCombat())
                    DoScriptText(SAY_WOLF_WELCOME, pRyga);
                break;
        }
    }
};

CreatureAI* GetAI_npc_ancestral_wolf(Creature* pCreature)
{
    return new npc_ancestral_wolfAI(pCreature);
}

/*######
## npc_anchorite_barada
######*/

#define QUEST_COLONEL_JULES     10935

#define GOSSIP_START_EVENT      "It is time... The rite of exorcism will now commence..."

bool GossipHello_npc_anchorite_barada(Player* player, Creature* creature)
{
    sLog.outString("Pom1");
    if (player->GetQuestStatus(QUEST_COLONEL_JULES) == QUEST_STATUS_INCOMPLETE) {
        sLog.outString("Pom2");
        player->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_START_EVENT, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);
    }
        
    sLog.outString("Pom3");
    player->SEND_GOSSIP_MENU(creature->GetNpcTextId(), creature->GetGUID());
    
    return true;
}

/*######
## npc_pathaleon_image
######*/

enum
{
    SAY_PATHALEON1         = -1900165,
    SAY_PATHALEON2         = -1900166,
    SAY_PATHALEON3         = -1900167,
    SAY_PATHALEON4         = -1900168,
    SPELL_ROOTS            = 35468,
    SPELL_INSECT           = 35471,
    SPELL_LIGHTING         = 35487,
    SPELL_TELE             = 7741,
    NPC_TARGET_TRIGGER     = 20781,
    NPC_CRYSTAL_TRIGGER    = 20617,
    NPC_GOLIATHON          = 19305,
};

struct npc_pathaleon_imageAI : public ScriptedAI
{
    npc_pathaleon_imageAI(Creature* pCreature) : ScriptedAI(pCreature) {}

    bool Event;
    bool SummonTrigger;

    uint32 uiSumTimer;
    uint32 uiStepsTimer;
    uint32 uiSteps;

    void Reset()
    {
        uiSumTimer = 5000;
        uiStepsTimer = 0;
        uiSteps = 0;
        Event = true;
        SummonTrigger = false;
    }
    
    void Aggro(Unit* who) {}

    void DoSpawnGoliathon()
    {
        me->SummonCreature(NPC_GOLIATHON, 113.29f, 4858.19f, 74.37f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000);
    }

    void DoSpawnTrigger()
    {
        me->SummonCreature(NPC_TARGET_TRIGGER, 81.20f, 4806.26f, 51.75f, 2.0f, TEMPSUMMON_TIMED_DESPAWN, 120000);
    }

    void DoSpawnCtrigger()
    {
        me->SummonCreature(NPC_CRYSTAL_TRIGGER, 106.21f, 4834.39f, 79.56f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 7000);
        me->SummonCreature(NPC_CRYSTAL_TRIGGER, 124.98f, 4813.17f, 79.66f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 7000);
        me->SummonCreature(NPC_CRYSTAL_TRIGGER, 124.01f, 4778.61f, 77.86f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 7000);
        me->SummonCreature(NPC_CRYSTAL_TRIGGER, 46.37f, 4795.72f, 66.73f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 7000);
        me->SummonCreature(NPC_CRYSTAL_TRIGGER, 60.14f, 4830.46f, 77.83f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 7000);
    }

    void JustSummoned(Creature* pSummoned)
    {
        if (pSummoned->GetEntry() == NPC_GOLIATHON)
        {
            pSummoned->CastSpell(pSummoned, SPELL_TELE, false);
            pSummoned->GetMotionMaster()->MovePoint(0, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ());
        }

        if (pSummoned->GetEntry() == NPC_CRYSTAL_TRIGGER)
        {
            pSummoned->CastSpell(pSummoned, SPELL_INSECT, false);
            pSummoned->CastSpell(pSummoned, SPELL_LIGHTING, false);
        }
        else
        {
            if (pSummoned->GetEntry() == NPC_TARGET_TRIGGER)
            {
                pSummoned->CastSpell(pSummoned, SPELL_ROOTS, false);
            }
        }
    }

    int32 NextStep(uint32 uiSteps)
    {              
        switch (uiSteps)
        {
            case 1:
                return 10000;
            case 2:
                DoSpawnTrigger();
                SummonTrigger = true;
                return 2000;
            case 3:
                DoScriptText(SAY_PATHALEON1, me, 0);
                return 15000;
            case 4:
                DoScriptText(SAY_PATHALEON2, me, 0);
                return 15000;
            case 5:
                DoScriptText(SAY_PATHALEON3, me, 0);
                return 15000;
            case 6:
                DoScriptText(SAY_PATHALEON4, me, 0);
                return 5000;
            case 7:
                DoSpawnGoliathon();
                return 1000;
            case 8:
                DoCast(me, SPELL_TELE);
                return 500;
            case 9:
                me->SetVisibility(VISIBILITY_OFF);
                return 60000;
            case 10:
                me->setDeathState(CORPSE);
            default: return 0;
        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (uiStepsTimer <= uiDiff)
        {
            if (Event)
                uiStepsTimer = NextStep(++uiSteps);
        }
        else uiStepsTimer -= uiDiff;

        if (SummonTrigger)
        {
            if (uiSumTimer <= uiDiff)
            {
                DoSpawnCtrigger();
                uiSumTimer = 5000;
            }
            else uiSumTimer -= uiDiff;
        }
    }
};

CreatureAI* GetAI_npc_pathaleon_image(Creature* pCreature)
{
    return new npc_pathaleon_imageAI(pCreature);
}

/*######
## npc_demoniac_scryer
######*/

#define GOSSIP_ITEM_ATTUNE          "Oui, scrutateur. Vous pouvez me posséder."

enum
{
    GOSSIP_TEXTID_PROTECT           = 10659,
    GOSSIP_TEXTID_ATTUNED           = 10643,
    QUEST_DEMONIAC                  = 10838,
    NPC_HELLFIRE_WARDLING           = 22259,
    NPC_ORC_HA                      = 22273,
    NPC_BUTTRESS                    = 22267,
    NPC_BUTTRESS_SPAWNER            = 22260,

    MAX_BUTTRESS                    = 4,

    TIME_TOTAL                      = 10*MINUTE*IN_MILLISECONDS,

    SPELL_SUMMONED                  = 7741,
    SPELL_DEMONIAC_VISITATION       = 38708,
    SPELL_BUTTRESS_APPERANCE        = 38719,
    SPELL_SUCKER_CHANNEL            = 38721,
    SPELL_SUCKER_DESPAWN_MOB        = 38691,
};

struct npc_demoniac_scryerAI : public ScriptedAI
{
    npc_demoniac_scryerAI(Creature* pCreature) : ScriptedAI(pCreature)
    {
        IfIsComplete = false;
        uiSpawnDemonTimer = 15000;
        uiSpawnOrcTimer = 30000;
        uiSpawnButtressTimer = 45000;
        uiEndTimer = 262000;
        uiButtressCount = 0;
        Reset();
    }

    bool IfIsComplete;

    uint32 uiSpawnDemonTimer;
    uint32 uiSpawnOrcTimer;
    uint32 uiSpawnButtressTimer;
    uint32 uiEndTimer;
    uint32 uiButtressCount;

    void Reset() {}

    void AttackedBy(Unit* pEnemy) {}

    void AttackStart(Unit* pEnemy) {}

    void Aggro(Unit* who) {}

    void DoSpawnButtress()
    {
        ++uiButtressCount;
        float fAngle;
        switch (uiButtressCount)
        {
            case 1: fAngle = 0.0f; break;
            case 2: fAngle = 4.6f; break;
            case 3: fAngle = 1.5f; break;
            case 4: fAngle = 3.1f; break;
        }

        float fX, fY, fZ;
        me->GetNearPoint(me, fX, fY, fZ, 0.0f, 7.0f, fAngle);
        uint32 uiTime = TIME_TOTAL - (uiSpawnButtressTimer * uiButtressCount);
        me->SummonCreature(NPC_BUTTRESS, fX, fY, fZ, me->GetAngle(fX, fY), TEMPSUMMON_TIMED_DESPAWN, uiTime);
        me->SummonCreature(NPC_BUTTRESS_SPAWNER, fX, fY, fZ, me->GetAngle(fX, fY), TEMPSUMMON_TIMED_DESPAWN, uiTime);
    }

    void DoSpawnDemon()
    {
        float fX, fY, fZ;
        me->GetNearPoint(me,fX, fY, fZ, 5.0f, 5.0f, -13.0f);
        me->SummonCreature(NPC_HELLFIRE_WARDLING, fX, fY, fZ, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
    }

    void DospawnOrc()
    {
        float fX, fY, fZ;
        me->GetNearPoint(me,fX, fY, fZ, 5.0f, 5.0f, -13.0f);
        me->SummonCreature(NPC_ORC_HA, fX, fY, fZ, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 10000);
    }

    void JustSummoned(Creature* pSummoned)
    {
        if (pSummoned->GetEntry() == NPC_HELLFIRE_WARDLING)
        {
            pSummoned->CastSpell(pSummoned, SPELL_SUMMONED, false);
            pSummoned->AI()->AttackStart(me);
        }

        if (pSummoned->GetEntry() == NPC_ORC_HA)
        {
            pSummoned->CastSpell(pSummoned, SPELL_SUMMONED, false);
            pSummoned->AI()->AttackStart(me);
        }

        if (pSummoned->GetEntry() == NPC_BUTTRESS)
        {
            pSummoned->CastSpell(pSummoned, SPELL_BUTTRESS_APPERANCE, false);
        }
        else
        {
            if (pSummoned->GetEntry() == NPC_BUTTRESS_SPAWNER)
            {
                pSummoned->CastSpell(me, SPELL_SUCKER_CHANNEL, true);
            }
        }
    }

    void SpellHitTarget(Unit* pTarget, const SpellEntry* pSpell)
    {
        if (pTarget->GetEntry() == NPC_BUTTRESS && pSpell->Id == SPELL_SUCKER_DESPAWN_MOB)
            ((Creature*)pTarget)->setDeathState(CORPSE);

        if (pTarget->GetEntry() == NPC_BUTTRESS_SPAWNER && pSpell->Id == SPELL_SUCKER_DESPAWN_MOB)
            ((Creature*)pTarget)->setDeathState(CORPSE);
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (uiEndTimer <= uiDiff)
        {
            me->ForcedDespawn();
            uiEndTimer = 262000;
        }
        else uiEndTimer -= uiDiff;

        if (IfIsComplete || !me->isAlive())
            return;

        if (uiSpawnButtressTimer <= uiDiff)
        {
            if (uiButtressCount >= MAX_BUTTRESS)
            {
                DoCast(me, SPELL_SUCKER_DESPAWN_MOB);
                IfIsComplete = true;
                return;
            }

            uiSpawnButtressTimer = 45000;
            DoSpawnButtress();
        }
        else uiSpawnButtressTimer -= uiDiff;

        if (uiSpawnDemonTimer <= uiDiff)
        {
            DoSpawnDemon();
            uiSpawnDemonTimer = 15000;
        }
        else uiSpawnDemonTimer -= uiDiff;

        if (uiSpawnOrcTimer <= uiDiff)
        {
            DospawnOrc();
            uiSpawnOrcTimer = 30000;
        }
        else uiSpawnOrcTimer -= uiDiff;
    }
};

CreatureAI* GetAI_npc_demoniac_scryer(Creature* pCreature)
{
    return new npc_demoniac_scryerAI(pCreature);
}

bool GossipHello_npc_demoniac_scryer(Player* pPlayer, Creature* pCreature)
{
    if (npc_demoniac_scryerAI* pScryerAI = dynamic_cast<npc_demoniac_scryerAI*>(pCreature->AI()))
    {
        if (pScryerAI->IfIsComplete)
        {
            if (pPlayer->GetQuestStatus(QUEST_DEMONIAC) == QUEST_STATUS_INCOMPLETE)
                pPlayer->ADD_GOSSIP_ITEM(GOSSIP_ICON_CHAT, GOSSIP_ITEM_ATTUNE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);

            pPlayer->SEND_GOSSIP_MENU(GOSSIP_TEXTID_ATTUNED, pCreature->GetGUID());

            return true;
        }
    }

    pPlayer->SEND_GOSSIP_MENU(GOSSIP_TEXTID_PROTECT, pCreature->GetGUID());

    return true;
}

bool GossipSelect_npc_demoniac_scryer(Player* pPlayer, Creature* pCreature, uint32 uiSender, uint32 uiAction)
{
    if (uiAction == GOSSIP_ACTION_INFO_DEF + 1)
    {
        pPlayer->CLOSE_GOSSIP_MENU();
        pCreature->CastSpell(pPlayer, SPELL_DEMONIAC_VISITATION, false);
    }

    return true;
}

/*######
## npc_magic_sucker_device_spawner
######*/

enum
{
    SPELL_EFFECT    = 38724,
    NPC_SCRYER      = 22258,
    NPC_BUTTRES     = 22267
};

struct npc_magic_sucker_device_spawnerAI : public ScriptedAI
{
    npc_magic_sucker_device_spawnerAI(Creature* pCreature) : ScriptedAI(pCreature) {}

    uint32 uiCastTimer;
    uint32 uiCheckTimer;

    void Reset()
    {
        uiCastTimer = 1800;
        uiCheckTimer = 5000;
    }
    
    void Aggro(Unit* who) {}

    void UpdateAI(const uint32 uiDiff)
    {
        if (uiCastTimer <= uiDiff)
        {
            DoCast(me, SPELL_EFFECT);
            uiCastTimer = 1800;
        }
        else uiCastTimer -= uiDiff;

        if (uiCheckTimer <= uiDiff)
        {
            if (Creature* pScr = me->FindNearestCreature(NPC_SCRYER, 15.0f, false))
            {
                if (Creature* pBut = me->FindNearestCreature(NPC_BUTTRES, 5.0f))
                {
                    pBut->setDeathState(CORPSE);
                    me->setDeathState(CORPSE);
                }
            }
            else return;

            uiCheckTimer = 5000;
        }
        else uiCheckTimer -= uiDiff;
    }
};

CreatureAI* GetAI_npc_magic_sucker_device_spawner(Creature* pCreature)
{
    return new npc_magic_sucker_device_spawnerAI(pCreature);
}

/*######
## npc_sedai_quest_credit_marker
######*/

enum
{
    NPC_ESCORT1    = 17417,
    NPC_SEDAI      = 17404
};

struct npc_sedai_quest_credit_markerAI : public ScriptedAI
{
    npc_sedai_quest_credit_markerAI(Creature* pCreature) : ScriptedAI(pCreature) {}

    void Reset() 
    {
        DoSpawn();
    }

    void Aggro(Unit* who) {}

    void DoSpawn()
    {
        me->SummonCreature(NPC_SEDAI, 225.908, 4124.034, 82.505, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 100000);
        me->SummonCreature(NPC_ESCORT1, 229.257, 4125.271, 83.388, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 40000);
    }

    void JustSummoned(Creature* pSummoned)
    {
        if (pSummoned->GetEntry() == NPC_ESCORT1)
        {
            pSummoned->SetUnitMovementFlags(MOVEMENTFLAG_WALK_MODE);
            pSummoned->GetMotionMaster()->MovePoint(0, 208.029f, 4134.618f, 77.763f);
        }
    }
};

CreatureAI* GetAI_npc_sedai_quest_credit_marker(Creature* pCreature)
{
    return new npc_sedai_quest_credit_markerAI(pCreature);
}

/*######
## npc_vindicator_sedai
######*/

#define SAY_MAG_ESSCORT    -1900125
#define SAY_SEDAI1         -1900126
#define SAY_SEDAI2         -1900127
#define SAY_KRUN           -1900128

enum
{
    NPC_ESCORT        = 17417,
    NPC_AMBUSHER      = 17418,
    NPC_KRUN          = 17405,
    SPELL_STUN        = 13005,
    SPELL_HOLYFIRE    = 17141
};

struct npc_vindicator_sedaiAI : public ScriptedAI
{
    npc_vindicator_sedaiAI(Creature* pCreature) : ScriptedAI(pCreature) {}

    bool Vision;

    uint32 uiStepsTimer;
    uint32 uiSteps;

    void Reset()
    {
        Vision = true;
        uiStepsTimer =0;
        uiSteps = 0;
    }
    
    void Aggro(Unit* who) {}

    void DoSpawnEscort()
    {
        me->SummonCreature(NPC_ESCORT, 227.188f, 4121.116f, 82.745f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 40000);
    }

    void DoSpawnAmbusher()
    {
        me->SummonCreature(NPC_AMBUSHER, 223.408f, 4120.086f, 81.843f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 30000);
    }

    void DoSpawnKrun()
    {
        me->SummonCreature(NPC_KRUN, 192.872f, 4129.417f, 73.655f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 6000);
    }

    void JustSummoned(Creature* pSummoned)
    {
        if (pSummoned->GetEntry() == NPC_ESCORT)
        {
            pSummoned->SetUnitMovementFlags(MOVEMENTFLAG_WALK_MODE);
            pSummoned->GetMotionMaster()->MovePoint(0, 205.660f, 4130.663f, 77.175f);
        }

        if (pSummoned->GetEntry() == NPC_AMBUSHER)
        {
            Creature* pEscort = pSummoned->FindNearestCreature(NPC_ESCORT, 15);
            pSummoned->AI()->AttackStart(pEscort);
        }
        else
        {
            if (pSummoned->GetEntry() == NPC_KRUN)
            {
                pSummoned->SetUnitMovementFlags(MOVEMENTFLAG_WALK_MODE);
                pSummoned->GetMotionMaster()->MovePoint(0, 194.739868f, 4143.145996f, 73.798088f);
                DoScriptText(SAY_KRUN, pSummoned,0);
                pSummoned->AI()->AttackStart(me);
            }
        }
    }

    void MoveInLineOfSight(Unit *who)
    {
        if (who->GetTypeId() == TYPEID_PLAYER)
        {
            if (CAST_PLR(who)->GetQuestStatus(9545) == QUEST_STATUS_INCOMPLETE)
            {
                if (Creature * pCr = me->FindNearestCreature(17413, 6.0f))
                {
                    float Radius = 10.0;
                    if (me->IsWithinDistInMap(who, Radius))
                    {
                        CAST_PLR(who)->KilledMonster(17413, pCr->GetGUID());
                    }
                }
                else return;
            }
        }
    }

    uint32 NextStep(uint32 uiSteps)
    {
        Creature* pEsc = me->FindNearestCreature(NPC_ESCORT, 20);
        Creature* pAmb = me->FindNearestCreature(NPC_AMBUSHER, 35);
        Creature* pKrun = me->FindNearestCreature(NPC_KRUN, 20);

        switch(uiSteps)
        {
            case 1:
                DoSpawnEscort();
            case 2:
                me->SetUnitMovementFlags(MOVEMENTFLAG_WALK_MODE);
            case 3:
                me->GetMotionMaster()->MovePoint(0, 204.877f, 4133.172f, 76.897f);
                return 2900;
            case 4:
                DoScriptText(SAY_MAG_ESSCORT, pEsc,0);
                return 1000;
            case 5:
                if (pEsc)
                    pEsc->GetMotionMaster()->MovePoint(0, 229.257f, 4125.271f, 83.388f);
                return 1500;
            case 6:
                if (pEsc)
                    pEsc->GetMotionMaster()->MovePoint(0, 227.188f, 4121.116f, 82.745f);
                return 1000;
            case 7:
                DoScriptText(SAY_SEDAI1, me,0);
                return 1000;
            case 8:
                DoSpawnAmbusher();
                return 3000;
            case 9:
                DoSpawnAmbusher();
                return 1000;
            case 10:
                if (pAmb)
                    me->AI()->AttackStart(pAmb);
                return 2000;
            case 11:
                if (pAmb)
                    me->CastSpell(pAmb, SPELL_STUN , false);
                return 2000;
            case 12:
                if (pAmb)
                    pAmb->DealDamage(pAmb, pAmb->GetHealth(), 0, DIRECT_DAMAGE);
                return 1500;
            case 13:
                if (pEsc)
                    pEsc->DealDamage(pEsc, pEsc->GetHealth(), 0, DIRECT_DAMAGE);
            case 14:
                me->AI()->AttackStart(pAmb);
            case 15:
                if (pEsc && pAmb)
                    pEsc->AI()->AttackStart(pAmb);
                return 1000;
            case 16:
                if (pAmb)
                    me->CastSpell(pAmb, SPELL_HOLYFIRE , false);
                return 6000;
            case 17:
                if (pAmb)
                    pAmb->DealDamage(pAmb, pAmb->GetHealth(), 0, DIRECT_DAMAGE);
                return 1000;
            case 18:
                if (pEsc)
                    pEsc->GetMotionMaster()->MovePoint(0, 235.063f, 4117.826f, 84.471f);
                return 1000;
            case 19:
                me->SetUnitMovementFlags(MOVEMENTFLAG_WALK_MODE);
                me->GetMotionMaster()->MovePoint(0, 199.706f, 4134.302f, 75.404f);
                return 6000;       
            case 20:
                me->GetMotionMaster()->MovePoint(0, 193.524f, 4147.451f, 73.605f);
                return 7000;              
            case 21:
                me->SetStandState(UNIT_STAND_STATE_KNEEL);
                DoScriptText(SAY_SEDAI2, me,0);
                return 5000;
            case 22:
                DoSpawnKrun();
                return 1000;
            case 23:
                if (pKrun)
                    me->CastSpell(pKrun, SPELL_HOLYFIRE, false);
                return 3000;
            case 24:
                me->DealDamage(me, me->GetHealth(), 0, DIRECT_DAMAGE);
            default:
                return 0;
        }
    }

    void UpdateAI(const uint32 uiDiff)
    {
        if (uiStepsTimer <= uiDiff)
        {
            if (Vision)
                uiStepsTimer = NextStep(++uiSteps);
        }
        else uiStepsTimer -= uiDiff;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_vindicator_sedai(Creature* pCreature)
{
    return new npc_vindicator_sedaiAI(pCreature);
}

/*######
## AddSC
######*/

void AddSC_hellfire_peninsula()
{
    Script* newscript;

    newscript = new Script;
    newscript->Name = "npc_aeranas";
    newscript->GetAI = &GetAI_npc_aeranas;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "go_haaleshi_altar";
    newscript->pGOHello = &GOHello_go_haaleshi_altar;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_wing_commander_dabiree";
    newscript->pGossipHello =   &GossipHello_npc_wing_commander_dabiree;
    newscript->pGossipSelect =  &GossipSelect_npc_wing_commander_dabiree;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_gryphoneer_leafbeard";
    newscript->pGossipHello = &GossipHello_npc_gryphoneer_leafbeard;
    newscript->pGossipSelect = &GossipSelect_npc_gryphoneer_leafbeard;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_gryphoneer_windbellow";
    newscript->pGossipHello =   &GossipHello_npc_gryphoneer_windbellow;
    newscript->pGossipSelect =  &GossipSelect_npc_gryphoneer_windbellow;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_wing_commander_brack";
    newscript->pGossipHello =   &GossipHello_npc_wing_commander_brack;
    newscript->pGossipSelect =  &GossipSelect_npc_wing_commander_brack;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_wounded_blood_elf";
    newscript->GetAI = &GetAI_npc_wounded_blood_elf;
    newscript->pQuestAccept = &QuestAccept_npc_wounded_blood_elf;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name="npc_demoniac_scryer";
    newscript->GetAI = &GetAI_npc_demoniac_scryer;
    newscript->pGossipHello = &GossipHello_npc_demoniac_scryer;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name="npc_fel_guard_hound";
    newscript->GetAI = &GetAI_npc_fel_guard_hound;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name="npc_anchorite_relic";
    newscript->GetAI = &GetAI_npc_anchorite_relic;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_living_flare";
    newscript->GetAI = &GetAI_npc_living_flare;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_ancestral_wolf";
    newscript->GetAI = &GetAI_npc_ancestral_wolf;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_anchorite_barada";
    newscript->pGossipHello = &GossipHello_npc_anchorite_barada;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_pathaleon_image";
    newscript->GetAI = &GetAI_npc_pathaleon_image;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_demoniac_scryer";
    newscript->pGossipHello =  &GossipHello_npc_demoniac_scryer;
    newscript->pGossipSelect = &GossipSelect_npc_demoniac_scryer;
    newscript->GetAI = &GetAI_npc_demoniac_scryer;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_magic_sucker_device_spawner";
    newscript->GetAI = &GetAI_npc_magic_sucker_device_spawner;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_sedai_quest_credit_marker";
    newscript->GetAI = &GetAI_npc_sedai_quest_credit_marker;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "npc_vindicator_sedai";
    newscript->GetAI = &GetAI_npc_vindicator_sedai;
    newscript->RegisterSelf();
}
