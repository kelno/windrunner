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
npc_demoniac_scryer
npc_fel_guard_hound
npc_anchorite_relic
npc_living_flare
npc_ancestral_spirit_wolf
npc_anchorite_barada
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
## npc_demoniac_scryer
######*/

enum eDemoniacScryer
{
HELLFIRE_WANDLING   = 22259,
FEL_WARDEN          = 22273,

QUEST_DEMO_SCRYER   = 10838
};

struct npc_demoniac_scryerAI : public Scripted_NoMovementAI
{
    npc_demoniac_scryerAI(Creature* c) : Scripted_NoMovementAI(c) {}
    
    uint32 WandlingTimer;
    uint32 WardenTimer;
    uint8 WandlingCount;
    bool WardenSpawned;
    bool sayComeOn;
    
    void Reset()
    {
        WandlingTimer = 12000;
        WardenTimer = 100000;
        WardenSpawned = false;
        WandlingCount = 0;
        sayComeOn = false;
    }
    
    void Aggro(Unit* pWho) {}
    
    void UpdateAI(const uint32 diff)
    {
        //player is gone ?
        if (!((TemporarySummon*)m_creature)->GetSummoner())
            return;
            
        Player* player = (((TemporarySummon*)m_creature)->GetSummoner())->ToPlayer();
        if (!player)
            return;
            
        if (WandlingCount >= 14)
        {
            //set GOSSIP flag on creature, then player has to speak to it to obtain quest item
            if (!sayComeOn)
            {
                m_creature->Say("Relevé établi. Prêt à être retiré.", LANG_UNIVERSAL, 0);
                m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
                sayComeOn = true;
            }
            
            return;
        }
        
        if (WandlingTimer < diff)
        {
            if (player && player->IsInWorld()) {
                if (Creature *tmpSpawn = m_creature->SummonCreature(HELLFIRE_WANDLING, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 15000))
                    tmpSpawn->AI()->AttackStart(player);
            }
            WandlingTimer = 12000;
            WandlingCount++;
        }else WandlingTimer -= diff;
        
        if (WardenTimer < diff && !WardenSpawned)
        {
            if (player && player->IsInWorld()) {
                if (Creature *tmpSpawn = m_creature->SummonCreature(FEL_WARDEN, m_creature->GetPositionX(), m_creature->GetPositionY(), m_creature->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 15000))
                    tmpSpawn->AI()->AttackStart(player);
            }
            WardenSpawned = true;
        }else WardenTimer -= diff;
    }
};

bool GossipHello_npc_demoniac_scryer(Player* pPlayer, Creature* pCreature)
{
    if (!pPlayer)
        return false;
    ItemPosCountVec dest;
    uint8 msg = pPlayer->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, 31607, 1);
    if (msg == EQUIP_ERR_OK)
    {
       Item* item = pPlayer->StoreNewItem( dest, 31607, true);
       pPlayer->SendNewItem(item, 1, true, false);
    }
    
    return true;
}

CreatureAI* GetAI_npc_demoniac_scryer(Creature* pCreature)
{
    return new npc_demoniac_scryerAI(pCreature);
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
}
