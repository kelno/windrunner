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
SDName: Darkshore
SD%Complete: 100
SDComment: Quest support: 731, 2078, 5321
SDCategory: Darkshore
EndScriptData */

/* ContentData
npc_kerlonian
npc_threshwackonator
npc_prospector_remtravel
npc_rabid_thistle_bear
npc_tharnariun_treetender
EndContentData */

#include "precompiled.h"
#include "EscortAI.h"
#include "FollowerAI.h"

/*######
## npc_kerlonian
######*/

enum eKerlonian
{
    SAY_KER_START               = -1000434,

    EMOTE_KER_SLEEP_1           = -1000435,
    EMOTE_KER_SLEEP_2           = -1000436,
    EMOTE_KER_SLEEP_3           = -1000437,

    SAY_KER_SLEEP_1             = -1000438,
    SAY_KER_SLEEP_2             = -1000439,
    SAY_KER_SLEEP_3             = -1000440,
    SAY_KER_SLEEP_4             = -1000441,

    EMOTE_KER_AWAKEN            = -1000445,

    SAY_KER_ALERT_1             = -1000442,
    SAY_KER_ALERT_2             = -1000443,

    SAY_KER_END                 = -1000444,

    SPELL_SLEEP_VISUAL          = 25148,
    SPELL_AWAKEN                = 17536,
    QUEST_SLEEPER_AWAKENED      = 5321,
    NPC_LILADRIS                = 11219,                    //attackers entries unknown
    FACTION_KER_ESCORTEE        = 113
};

//TODO: make concept similar as "ringo" -escort. Find a way to run the scripted attacks, _if_ player are choosing road.
struct npc_kerlonianAI : public FollowerAI
{
    npc_kerlonianAI(Creature* pCreature) : FollowerAI(pCreature) { }

    uint32 m_uiFallAsleepTimer;

    void Reset()
    {
        m_uiFallAsleepTimer = urand(10000, 45000);
    }

    void MoveInLineOfSight(Unit* pWho)
    {
        FollowerAI::MoveInLineOfSight(pWho);

        if (!m_creature->getVictim() && !HasFollowState(STATE_FOLLOW_COMPLETE) && pWho->GetEntry() == NPC_LILADRIS)
        {
            if (m_creature->IsWithinDistInMap(pWho, INTERACTION_DISTANCE*5))
            {
                if (Player* pPlayer = GetLeaderForFollower())
                {
                    if (pPlayer->GetQuestStatus(QUEST_SLEEPER_AWAKENED) == QUEST_STATUS_INCOMPLETE)
                        pPlayer->GroupEventHappens(QUEST_SLEEPER_AWAKENED, m_creature);

                    DoScriptText(SAY_KER_END, m_creature);
                }

                SetFollowComplete();
            }
        }
    }

    void SpellHit(Unit* pCaster, const SpellEntry* pSpell)
    {
        if (HasFollowState(STATE_FOLLOW_INPROGRESS | STATE_FOLLOW_PAUSED) && pSpell->Id == SPELL_AWAKEN)
            ClearSleeping();
    }

    void SetSleeping()
    {
        SetFollowPaused(true);

        DoScriptText(RAND(EMOTE_KER_SLEEP_1,EMOTE_KER_SLEEP_2,EMOTE_KER_SLEEP_3), m_creature);

        DoScriptText(RAND(SAY_KER_SLEEP_1,SAY_KER_SLEEP_2,SAY_KER_SLEEP_3,SAY_KER_SLEEP_4), m_creature);

        m_creature->SetStandState(UNIT_STAND_STATE_SLEEP);
        m_creature->CastSpell(m_creature, SPELL_SLEEP_VISUAL, false);
    }

    void ClearSleeping()
    {
        m_creature->RemoveAurasDueToSpell(SPELL_SLEEP_VISUAL);
        m_creature->SetStandState(UNIT_STAND_STATE_STAND);

        DoScriptText(EMOTE_KER_AWAKEN, m_creature);

        SetFollowPaused(false);
    }

    void Aggro(Unit* pWho) {}

    void UpdateFollowerAI(const uint32 uiDiff)
    {
        if (!UpdateVictim())
        {
            if (!HasFollowState(STATE_FOLLOW_INPROGRESS))
                return;

            if (!HasFollowState(STATE_FOLLOW_PAUSED))
            {
                if (m_uiFallAsleepTimer <= uiDiff)
                {
                    SetSleeping();
                    m_uiFallAsleepTimer = urand(25000, 90000);
                }
                else
                    m_uiFallAsleepTimer -= uiDiff;
            }

            return;
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_kerlonian(Creature* pCreature)
{
    return new npc_kerlonianAI(pCreature);
}

bool QuestAccept_npc_kerlonian(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_SLEEPER_AWAKENED)
    {
        if (npc_kerlonianAI* pKerlonianAI = CAST_AI(npc_kerlonianAI, pCreature->AI()))
        {
            pCreature->SetStandState(UNIT_STAND_STATE_STAND);
            DoScriptText(SAY_KER_START, pCreature, pPlayer);
            pKerlonianAI->StartFollow(pPlayer, FACTION_KER_ESCORTEE, pQuest);
        }
    }

    return true;
}

/*####
# npc_threshwackonator
####*/

enum eThreshwackonator
{
    EMOTE_START             = -1000413, //signed for 4966
    SAY_AT_CLOSE            = -1000414, //signed for 4966
    QUEST_GYROMAST_REV      = 2078,
    NPC_GELKAK              = 6667,
    FACTION_HOSTILE         = 14
};

#define GOSSIP_ITEM_INSERT_KEY  "[PH] Insert key"

struct npc_threshwackonatorAI : public FollowerAI
{
    npc_threshwackonatorAI(Creature* pCreature) : FollowerAI(pCreature) { }

    void Reset() { }
    
    void Aggro(Unit* pWho) {}

    void MoveInLineOfSight(Unit* pWho)
    {
        FollowerAI::MoveInLineOfSight(pWho);

        if (!m_creature->getVictim() && !HasFollowState(STATE_FOLLOW_COMPLETE) && pWho->GetEntry() == NPC_GELKAK)
        {
            if (m_creature->IsWithinDistInMap(pWho, 10.0f))
            {
                DoScriptText(SAY_AT_CLOSE, pWho);
                DoAtEnd();
            }
        }
    }

    void DoAtEnd()
    {
        me->setFaction(FACTION_HOSTILE);

        SetFollowComplete(true);

        if (Player* pHolder = GetLeaderForFollower()) {
            me->AI()->AttackStart(pHolder);
            me->GetMotionMaster()->MoveChase(pHolder);
        }
    }
};

CreatureAI* GetAI_npc_threshwackonator(Creature* pCreature)
{
    return new npc_threshwackonatorAI(pCreature);
}

bool GossipHello_npc_threshwackonator(Player* pPlayer, Creature* pCreature)
{
    if (pPlayer->GetQuestStatus(QUEST_GYROMAST_REV) == QUEST_STATUS_INCOMPLETE)
        pPlayer->ADD_GOSSIP_ITEM(0, GOSSIP_ITEM_INSERT_KEY, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    pPlayer->SEND_GOSSIP_MENU(pCreature->GetNpcTextId(), pCreature->GetGUID());
    return true;
}

bool GossipSelect_npc_threshwackonator(Player* pPlayer, Creature* pCreature, uint32 uiSender, uint32 uiAction)
{
    if (uiAction == GOSSIP_ACTION_INFO_DEF+1)
    {
        pPlayer->CLOSE_GOSSIP_MENU();

        if (npc_threshwackonatorAI* pThreshAI = CAST_AI(npc_threshwackonatorAI, pCreature->AI()))
        {
            DoScriptText(EMOTE_START, pCreature);
            pThreshAI->StartFollow(pPlayer);
        }
    }

    return true;
}

/*####
# npc_prospector_remtravel
####*/

enum eRemtravel
{
    SAY_REM_START               = -1000327,
    SAY_REM_RAMP1_1             = -1000328,
    SAY_REM_RAMP1_2             = -1000329,
    SAY_REM_BOOK                = -1000330,
    SAY_REM_TENT1_1             = -1000331,
    SAY_REM_TENT1_2             = -1000332,
    SAY_REM_MOSS                = -1000333,
    EMOTE_REM_MOSS              = -1000334,
    SAY_REM_MOSS_PROGRESS       = -1000335,
    SAY_REM_PROGRESS            = -1000336,
    SAY_REM_REMEMBER            = -1000337,
    EMOTE_REM_END               = -1000338,
    SAY_REM_AGGRO               = -1000339,

    FACTION_ESCORTEE            = 10,
    QUEST_ABSENT_MINDED_PT2     = 731,
    NPC_GRAVEL_SCOUT            = 2158,
    NPC_GRAVEL_BONE             = 2159,
    NPC_GRAVEL_GEO              = 2160
};

struct npc_prospector_remtravelAI : public npc_escortAI
{
    npc_prospector_remtravelAI(Creature* pCreature) : npc_escortAI(pCreature) {}

    void WaypointReached(uint32 i)
    {
        Player* pPlayer = GetPlayerForEscort();

        if (!pPlayer)
            return;

        switch(i)
        {
            case 0:
                DoScriptText(SAY_REM_START, me, pPlayer);
                break;
            case 1:
                DoScriptText(SAY_REM_RAMP1_1, me, pPlayer);
                break;
            case 2:
                DoSpawnCreature(NPC_GRAVEL_SCOUT, -10.0f, 5.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000);
                DoSpawnCreature(NPC_GRAVEL_BONE, -10.0f, 7.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000);
                break;
            case 4:
                DoScriptText(SAY_REM_RAMP1_2, me, pPlayer);
                break;
            case 7:
                DoScriptText(SAY_REM_TENT1_1, me, pPlayer);
                DoSpawnCreature(NPC_GRAVEL_SCOUT, -10.0f, 5.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000);
                DoSpawnCreature(NPC_GRAVEL_BONE, -10.0f, 7.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000);
                break;
            case 8:
                DoScriptText(SAY_REM_TENT1_2, me, pPlayer);
                break;
            case 14:
                DoScriptText(SAY_REM_MOSS_PROGRESS, me, pPlayer);
                break;
            case 15:
                DoSpawnCreature(NPC_GRAVEL_SCOUT, -15.0f, 3.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000);
                DoSpawnCreature(NPC_GRAVEL_BONE, -15.0f, 5.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000);
                DoSpawnCreature(NPC_GRAVEL_GEO, -15.0f, 7.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 30000);
                break;
            case 18:
                DoScriptText(SAY_REM_BOOK, me, pPlayer);
                break;
            case 24:
                DoScriptText(SAY_REM_REMEMBER, me, pPlayer);
                break;
            case 25:
                DoScriptText(EMOTE_REM_END, me, pPlayer);
                pPlayer->GroupEventHappens(QUEST_ABSENT_MINDED_PT2, me);
                break;
        }
    }

    void Reset() {}

    void Aggro(Unit* who)
    {
        if (rand()%2)
            DoScriptText(SAY_REM_AGGRO, me, who);
    }
};

CreatureAI* GetAI_npc_prospector_remtravel(Creature* pCreature)
{
    return new npc_prospector_remtravelAI(pCreature);
}

bool QuestAccept_npc_prospector_remtravel(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_ABSENT_MINDED_PT2)
    {
        ((npc_escortAI*)(pCreature->AI()))->Start(true, true, false, pPlayer->GetGUID(), pCreature->GetEntry());
        pCreature->setFaction(FACTION_ESCORTEE);
    }

    return true;
}

/*######
## npc_rabid_thistle_bear
######*/

struct npc_rabid_thistle_bearAI : public ScriptedAI
{
    npc_rabid_thistle_bearAI(Creature* pCreature) : ScriptedAI(pCreature) {}
    
    uint64 guid;

    void Reset() {}

    void Aggro(Unit* who) {}
    
    bool sOnDummyEffect(Unit* caster, uint32 spellId, uint32 effIndex)
    {
        if (spellId == 9439) {
            if (Player* plr = Unit::GetPlayer(guid)) {
                me->UpdateEntry(11836);
                me->GetMotionMaster()->MoveFollow(plr, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                plr->CastedCreatureOrGO(11836, me->GetGUID(), 9437);
                me->CombatStop();
                plr->CombatStop();
            }
        }
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        if (me->getVictim()->ToPlayer())
            guid = me->getVictim()->GetGUID();
        
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_rabid_thistle_bear(Creature* pCreature)
{
    return new npc_rabid_thistle_bearAI(pCreature);
}

/*######
## npc_tharnariun_treetender
######*/

struct npc_tharnariun_treetenderAI : public ScriptedAI
{
    npc_tharnariun_treetenderAI(Creature* c) : ScriptedAI(c) {}
    
    void Aggro(Unit* who) {}
    
    void MoveInLineOfSight(Unit* who)
    {
        if (who->ToCreature() && who->GetEntry() == 11836)
            who->ToCreature()->ForcedDespawn();
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (!UpdateVictim())
            return;
            
        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_tharnariun_treetender(Creature* creature)
{
    return new npc_tharnariun_treetenderAI(creature);
}

/*######
## AddSC
######*/

void AddSC_darkshore()
{
    Script* newscript;

    newscript = new Script;
    newscript->Name = "npc_kerlonian";
    newscript->GetAI = &GetAI_npc_kerlonian;
    newscript->pQuestAccept = &QuestAccept_npc_kerlonian;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_threshwackonator";
    newscript->GetAI = &GetAI_npc_threshwackonator;
    newscript->pGossipHello = &GossipHello_npc_threshwackonator;
    newscript->pGossipSelect = &GossipSelect_npc_threshwackonator;
    newscript->pGossipSelect = &GossipSelect_npc_threshwackonator;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_prospector_remtravel";
    newscript->GetAI = &GetAI_npc_prospector_remtravel;
    newscript->pQuestAccept = &QuestAccept_npc_prospector_remtravel;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_rabid_thistle_bear";
    newscript->GetAI = &GetAI_npc_rabid_thistle_bear;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_tharnariun_treetender";
    newscript->GetAI = &GetAI_npc_tharnariun_treetender;
    newscript->RegisterSelf();
}
