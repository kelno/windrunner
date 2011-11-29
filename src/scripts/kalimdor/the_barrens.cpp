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
SDName: The_Barrens
SD%Complete: 90
SDComment: Quest support: 863, 898, 1719, 2458, 4921, 6981, 4021
SDCategory: Barrens
EndScriptData */

/* ContentData
npc_beaten_corpse
npc_sputtervalve
npc_taskmaster_fizzule
npc_twiggy_flathead
npc_wizzlecrank_shredder
npc_regthar_deathgate
EndContentData */

#include "precompiled.h"
#include "EscortAI.h"

/*######
## npc_beaten_corpse
######*/

#define GOSSIP_CORPSE "Examine corpse in detail..."

bool GossipHello_npc_beaten_corpse(Player* pPlayer, Creature* pCreature)
{
    if( pPlayer->GetQuestStatus(4921) == QUEST_STATUS_INCOMPLETE || pPlayer->GetQuestStatus(4921) == QUEST_STATUS_COMPLETE)
        pPlayer->ADD_GOSSIP_ITEM(0, GOSSIP_CORPSE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF+1);

    pPlayer->SEND_GOSSIP_MENU(3557, pCreature->GetGUID());
    return true;
}

bool GossipSelect_npc_beaten_corpse(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action )
{
    if(action == GOSSIP_ACTION_INFO_DEF +1)
    {
        pPlayer->SEND_GOSSIP_MENU(3558, pCreature->GetGUID());
        pPlayer->KilledMonster( 10668, pCreature->GetGUID() );
    }
    return true;
}

/*######
# npc_gilthares
######*/

enum eGilthares
{
    SAY_GIL_START               = -1000370,
    SAY_GIL_AT_LAST             = -1000371,
    SAY_GIL_PROCEED             = -1000372,
    SAY_GIL_FREEBOOTERS         = -1000373,
    SAY_GIL_AGGRO_1             = -1000374,
    SAY_GIL_AGGRO_2             = -1000375,
    SAY_GIL_AGGRO_3             = -1000376,
    SAY_GIL_AGGRO_4             = -1000377,
    SAY_GIL_ALMOST              = -1000378,
    SAY_GIL_SWEET               = -1000379,
    SAY_GIL_FREED               = -1000380,

    QUEST_FREE_FROM_HOLD        = 898,
    AREA_MERCHANT_COAST         = 391,
    FACTION_ESCORTEE            = 232                       //guessed, possible not needed for this quest
};

struct npc_giltharesAI : public npc_escortAI
{
    npc_giltharesAI(Creature* pCreature) : npc_escortAI(pCreature) { }

    void Reset() { }

    void WaypointReached(uint32 uiPointId)
    {
        Player* pPlayer = GetPlayerForEscort();

        if (!pPlayer)
            return;

        switch(uiPointId)
        {
            case 16:
                DoScriptText(SAY_GIL_AT_LAST, m_creature, pPlayer);
                break;
            case 17:
                DoScriptText(SAY_GIL_PROCEED, m_creature, pPlayer);
                break;
            case 18:
                DoScriptText(SAY_GIL_FREEBOOTERS, m_creature, pPlayer);
                break;
            case 37:
                DoScriptText(SAY_GIL_ALMOST, m_creature, pPlayer);
                break;
            case 47:
                DoScriptText(SAY_GIL_SWEET, m_creature, pPlayer);
                break;
            case 53:
                DoScriptText(SAY_GIL_FREED, m_creature, pPlayer);
                pPlayer->GroupEventHappens(QUEST_FREE_FROM_HOLD, m_creature);
                break;
        }
    }

    void Aggro(Unit* pWho)
    {
        //not always use
        if (rand()%4)
            return;

        //only aggro text if not player and only in this area
        if (pWho->GetTypeId() != TYPEID_PLAYER && m_creature->GetAreaId() == AREA_MERCHANT_COAST)
        {
            //appears to be pretty much random (possible only if escorter not in combat with pWho yet?)
            DoScriptText(RAND(SAY_GIL_AGGRO_1, SAY_GIL_AGGRO_2, SAY_GIL_AGGRO_3, SAY_GIL_AGGRO_4), m_creature, pWho);
        }
    }
};

CreatureAI* GetAI_npc_gilthares(Creature* pCreature)
{
    return new npc_giltharesAI(pCreature);
}

bool QuestAccept_npc_gilthares(Player* pPlayer, Creature* pCreature, const Quest* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_FREE_FROM_HOLD)
    {
        pCreature->setFaction(FACTION_ESCORTEE);
        pCreature->SetStandState(PLAYER_STATE_NONE);

        DoScriptText(SAY_GIL_START, pCreature, pPlayer);

        /*if (npc_giltharesAI* pEscortAI = CAST_AI(npc_giltharesAI, pCreature->AI()))
            pEscortAI->Start(false, false, pPlayer->GetGUID(), pQuest);*/
        ((npc_escortAI*)(pCreature->AI()))->Start(false, false, pPlayer->GetGUID());
    }
    return true;
}

/*######
## npc_sputtervalve
######*/

#define GOSSIP_SPUTTERVALVE "Can you tell me about this shard?"

bool GossipHello_npc_sputtervalve(Player* pPlayer, Creature* pCreature)
{
    if (pCreature->isQuestGiver())
        pPlayer->PrepareQuestMenu(pCreature->GetGUID());

    if( pPlayer->GetQuestStatus(6981) == QUEST_STATUS_INCOMPLETE)
        pPlayer->ADD_GOSSIP_ITEM(0, GOSSIP_SPUTTERVALVE, GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);

    pPlayer->SEND_GOSSIP_MENU(pCreature->GetNpcTextId(), pCreature->GetGUID());
    return true;
}

bool GossipSelect_npc_sputtervalve(Player* pPlayer, Creature* pCreature, uint32 sender, uint32 action )
{
    if(action == GOSSIP_ACTION_INFO_DEF)
    {
        pPlayer->SEND_GOSSIP_MENU(2013, pCreature->GetGUID());
        pPlayer->AreaExploredOrEventHappens(6981);
    }
    return true;
}

/*######
## npc_taskmaster_fizzule
######*/

enum eTaskmasterFizzule
{
FACTION_HOSTILE_F       = 16,
FACTION_FRIENDLY_F      = 35,

SPELL_FLARE             = 10113,
SPELL_FOLLY             = 10137
};

struct npc_taskmaster_fizzuleAI : public ScriptedAI
{
    npc_taskmaster_fizzuleAI(Creature* c) : ScriptedAI(c) {}

    bool IsFriend;
    uint32 Reset_Timer;
    uint32 FlareCount;

    void Reset()
    {
        IsFriend = false;
        Reset_Timer = 120000;
        FlareCount = 0;
        m_creature->setFaction(FACTION_HOSTILE_F);
        m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
    }

    //This is a hack. Spellcast will make creature aggro but that is not
    //supposed to happen (Trinity not implemented/not found way to detect this spell kind)
    void DoUglyHack()
    {
        m_creature->RemoveAllAuras();
        m_creature->DeleteThreatList();
        m_creature->CombatStop();
    }

    void SpellHit(Unit* caster, const SpellEntry* spell)
    {
        if( spell->Id == SPELL_FLARE || spell->Id == SPELL_FOLLY )
        {
            DoUglyHack();
            ++FlareCount;
            if( FlareCount >= 2 )
            {
                m_creature->setFaction(FACTION_FRIENDLY_F);
                IsFriend = true;
            }
        }
    }

    void Aggro(Unit* pWho) { }

    void UpdateAI(const uint32 diff)
    {
        if( IsFriend )
        {
            if( Reset_Timer < diff )
            {
                EnterEvadeMode();
                return;
            } else Reset_Timer -= diff;
        }

        if (!UpdateVictim())
            return;

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_npc_taskmaster_fizzule(Creature* pCreature)
{
    return new npc_taskmaster_fizzuleAI (pCreature);
}

bool ReciveEmote_npc_taskmaster_fizzule(Player* pPlayer, Creature* pCreature, uint32 emote)
{
    if( emote == TEXTEMOTE_SALUTE )
    {
        if ((CAST_AI(npc_taskmaster_fizzuleAI, (pCreature->AI())))->FlareCount >= 2 )
        {
            pCreature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
            pCreature->HandleEmoteCommand(EMOTE_ONESHOT_SALUTE);
        }
    }
    return true;
}
/*#####
## npc_twiggy_flathead
#####*/

enum eTwiggyFlathead
{
    NPC_BIG_WILL                = 6238,
    NPC_AFFRAY_CHALLENGER       = 6240,

    SAY_BIG_WILL_READY          = -1000123,
    SAY_TWIGGY_FLATHEAD_BEGIN   = -1000124,
    SAY_TWIGGY_FLATHEAD_FRAY    = -1000125,
    SAY_TWIGGY_FLATHEAD_DOWN    = -1000126,
    SAY_TWIGGY_FLATHEAD_OVER    = -1000127,
};

float AffrayChallengerLoc[6][4]=
{
    {-1683, -4326, 2.79, 0},
    {-1682, -4329, 2.79, 0},
    {-1683, -4330, 2.79, 0},
    {-1680, -4334, 2.79, 1.49},
    {-1674, -4326, 2.79, 3.49},
    {-1677, -4334, 2.79, 1.66}
};

struct npc_twiggy_flatheadAI : public ScriptedAI
{
    npc_twiggy_flatheadAI(Creature *c) : ScriptedAI(c) {}

    bool EventInProgress;
    bool EventGrate;
    bool EventBigWill;
    bool Challenger_down[6];
    uint32 Wave;
    uint32 Wave_Timer;
    uint32 Challenger_checker;
    uint64 PlayerGUID;
    uint64 AffrayChallenger[6];
    uint64 BigWill;

    void Reset()
    {
        EventInProgress = false;
        EventGrate = false;
        EventBigWill = false;
        Wave_Timer = 600000;
        Challenger_checker = 0;
        Wave = 0;
        PlayerGUID = 0;

        for(uint8 i = 0; i < 6; ++i)
        {
            AffrayChallenger[i] = 0;
            Challenger_down[i] = false;
        }
        BigWill = 0;
    }

    void Aggro(Unit* pWho) { }

    void MoveInLineOfSight(Unit* pWho)
    {
        if(!pWho || (!pWho->isAlive())) return;

        if (m_creature->IsWithinDistInMap(pWho, 10.0f) && (pWho->GetTypeId() == TYPEID_PLAYER) && CAST_PLR(pWho)->GetQuestStatus(1719) == QUEST_STATUS_INCOMPLETE && !EventInProgress)
        {
            PlayerGUID = pWho->GetGUID();
            EventInProgress = true;
        }
    }

    void KilledUnit(Unit* pVictim) { }

    void UpdateAI(const uint32 diff)
    {
        if (EventInProgress) {
            Player* pWarrior = NULL;

            if(PlayerGUID)
                pWarrior = Unit::GetPlayer(PlayerGUID);

            if(!pWarrior)
                return;

            if(!pWarrior->isAlive() && pWarrior->GetQuestStatus(1719) == QUEST_STATUS_INCOMPLETE) {
                EventInProgress = false;
                DoScriptText(SAY_TWIGGY_FLATHEAD_DOWN, m_creature);
                pWarrior->FailQuest(1719);

                for(uint8 i = 0; i < 6; ++i)
                {
                    if (AffrayChallenger[i])
                    {
                        Creature* pCreature = Unit::GetCreature((*m_creature), AffrayChallenger[i]);
                        if(pCreature) {
                            if(pCreature->isAlive())
                            {
                                pCreature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);
                                pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                                pCreature->setDeathState(JUST_DIED);
                            }
                        }
                    }
                    AffrayChallenger[i] = 0;
                    Challenger_down[i] = false;
                }

                if (BigWill)
                {
                    Creature* pCreature = Unit::GetCreature((*m_creature), BigWill);
                    if(pCreature) {
                        if(pCreature->isAlive()) {
                            pCreature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_IN_COMBAT);
                            pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            pCreature->setDeathState(JUST_DIED);
                        }
                    }
                }
                BigWill = 0;
            }

            if (!EventGrate && EventInProgress)
            {
                float x,y,z;
                pWarrior->GetPosition(x, y, z);

                if (x >= -1684 && x <= -1674 && y >= -4334 && y <= -4324) {
                    pWarrior->AreaExploredOrEventHappens(1719);
                    DoScriptText(SAY_TWIGGY_FLATHEAD_BEGIN, m_creature);

                    for(uint8 i = 0; i < 6; ++i)
                    {
                        Creature* pCreature = m_creature->SummonCreature(NPC_AFFRAY_CHALLENGER, AffrayChallengerLoc[i][0], AffrayChallengerLoc[i][1], AffrayChallengerLoc[i][2], AffrayChallengerLoc[i][3], TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 600000);
                        if(!pCreature)
                            continue;
                        pCreature->setFaction(35);
                        pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                        pCreature->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        pCreature->HandleEmoteCommand(EMOTE_ONESHOT_ROAR);
                        AffrayChallenger[i] = pCreature->GetGUID();
                    }
                    Wave_Timer = 5000;
                    Challenger_checker = 1000;
                    EventGrate = true;
                }
            }
            else if (EventInProgress)
            {
                if (Challenger_checker < diff)
                {
                    for(uint8 i = 0; i < 6; ++i)
                    {
                        if (AffrayChallenger[i])
                        {
                            Creature* pCreature = Unit::GetCreature((*m_creature), AffrayChallenger[i]);
                            if((!pCreature || (!pCreature->isAlive())) && !Challenger_down[i])
                            {
                                DoScriptText(SAY_TWIGGY_FLATHEAD_DOWN, m_creature);
                                Challenger_down[i] = true;
                            }
                        }
                    }
                    Challenger_checker = 1000;
                } else Challenger_checker -= diff;

                if(Wave_Timer < diff)
                {
                    if (AffrayChallenger[Wave] && Wave < 6 && !EventBigWill)
                    {
                        DoScriptText(SAY_TWIGGY_FLATHEAD_FRAY, m_creature);
                        Creature* pCreature = Unit::GetCreature((*m_creature), AffrayChallenger[Wave]);
                        if(pCreature && (pCreature->isAlive()))
                        {
                            pCreature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                            pCreature->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                            pCreature->HandleEmoteCommand(EMOTE_ONESHOT_ROAR);
                            pCreature->setFaction(14);
                            ((CreatureAI*)pCreature->AI())->AttackStart(pWarrior);
                            ++Wave;
                            Wave_Timer = 20000;
                        }
                    }
                    else if (Wave >= 6 && !EventBigWill) {
                        if(Creature* pCreature = m_creature->SummonCreature(NPC_BIG_WILL, -1722, -4341, 6.12, 6.26, TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 480000))
                        {
                            BigWill = pCreature->GetGUID();
                            //pCreature->GetMotionMaster()->MovePoint(0, -1693, -4343, 4.32);
                            //pCreature->GetMotionMaster()->MovePoint(1, -1684, -4333, 2.78);
                            pCreature->GetMotionMaster()->MovePoint(2, -1682, -4329, 2.79);
                            //pCreature->HandleEmoteCommand(EMOTE_ONESHOT_ROAR);
                            pCreature->HandleEmoteCommand(EMOTE_STATE_READYUNARMED);
                            EventBigWill = true;
                            Wave_Timer = 1000;
                        }
                    }
                    else if (Wave >= 6 && EventBigWill && BigWill)
                    {
                        Creature* pCreature = Unit::GetCreature((*m_creature), BigWill);
                        if (!pCreature || !pCreature->isAlive())
                        {
                            DoScriptText(SAY_TWIGGY_FLATHEAD_OVER, m_creature);
                            EventInProgress = false;
                            EventBigWill = false;
                            EventGrate = false;
                            PlayerGUID = 0;
                            Wave = 0;
                        }
                    }
                } else Wave_Timer -= diff;
            }
        }
    }
};

CreatureAI* GetAI_npc_twiggy_flathead(Creature* pCreature)
{
    return new npc_twiggy_flatheadAI (pCreature);
}

/*#####
## npc_wizzlecrank_shredder
#####*/

enum eEnums_Wizzlecrank
{
    SAY_START           = -1000272,
    SAY_STARTUP1        = -1000273,
    SAY_STARTUP2        = -1000274,
    SAY_MERCENARY       = -1000275,
    SAY_PROGRESS_1      = -1000276,
    SAY_PROGRESS_2      = -1000277,
    SAY_PROGRESS_3      = -1000278,
    SAY_END             = -1000279,

    QUEST_ESCAPE        = 863,
    FACTION_RATCHET     = 637,
    NPC_PILOT_WIZZ      = 3451,
    NPC_MERCENARY       = 3282,
};

struct npc_wizzlecrank_shredderAI : public npc_escortAI
{
    npc_wizzlecrank_shredderAI(Creature* pCreature) : npc_escortAI(pCreature)
    {
        m_bIsPostEvent = false;
        m_uiPostEventTimer = 1000;
        m_uiPostEventCount = 0;
    }

    bool m_bIsPostEvent;
    uint32 m_uiPostEventTimer;
    uint32 m_uiPostEventCount;

    void Reset()
    {
        if (IsBeingEscorted)
        {
            if (m_creature->getStandState() == UNIT_STAND_STATE_DEAD)
                 m_creature->SetStandState(UNIT_STAND_STATE_STAND);

            m_bIsPostEvent = false;
            m_uiPostEventTimer = 1000;
            m_uiPostEventCount = 0;
        }
    }
    
    void Aggro(Unit* pWho) {}

    void WaypointReached(uint32 uiPointId)
    {
        Player* pPlayer = GetPlayerForEscort();

        if (!pPlayer)
            return;

        switch(uiPointId)
        {
        case 0:
            DoScriptText(SAY_STARTUP1, m_creature);
            break;
        case 9:
            SetRun(false);
            break;
        case 17:
            if (Creature* pTemp = m_creature->SummonCreature(NPC_MERCENARY, 1128.489f, -3037.611f, 92.701f, 1.472f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000))
            {
                DoScriptText(SAY_MERCENARY, pTemp);
                m_creature->SummonCreature(NPC_MERCENARY, 1160.172f, -2980.168f, 97.313f, 3.690f, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 120000);
            }
            break;
        case 24:
            m_bIsPostEvent = true;
            break;
        }
    }

    void WaypointStart(uint32 uiPointId)
    {
        Player* pPlayer = GetPlayerForEscort();

        if (!pPlayer)
            return;

        switch(uiPointId)
        {
            case 9:
                DoScriptText(SAY_STARTUP2, m_creature, pPlayer);
                break;
            case 18:
                DoScriptText(SAY_PROGRESS_1, m_creature, pPlayer);
                SetRun();
                break;
        }
    }

    void JustSummoned(Creature* pSummoned)
    {
        if (pSummoned->GetEntry() == NPC_PILOT_WIZZ)
            m_creature->SetStandState(UNIT_STAND_STATE_DEAD);

        if (pSummoned->GetEntry() == NPC_MERCENARY)
            pSummoned->AI()->AttackStart(m_creature);
    }

    void UpdateEscortAI(const uint32 uiDiff)
    {
        if (!UpdateVictim())
        {
            if (m_bIsPostEvent)
            {
                if (m_uiPostEventTimer <= uiDiff)
                {
                    switch(m_uiPostEventCount)
                    {
                        case 0:
                            DoScriptText(SAY_PROGRESS_2, m_creature);
                            break;
                        case 1:
                            DoScriptText(SAY_PROGRESS_3, m_creature);
                            break;
                        case 2:
                            DoScriptText(SAY_END, m_creature);
                            break;
                        case 3:
                            if (Player* pPlayer = GetPlayerForEscort())
                            {
                                pPlayer->GroupEventHappens(QUEST_ESCAPE, m_creature);
                                m_creature->SummonCreature(NPC_PILOT_WIZZ, 0.0f, 0.0f, 0.0f, 0.0f, TEMPSUMMON_TIMED_DESPAWN, 180000);
                            }
                            break;
                    }

                    ++m_uiPostEventCount;
                    m_uiPostEventTimer = 5000;
                }
                else
                    m_uiPostEventTimer -= uiDiff;
            }

            return;
        }

        DoMeleeAttackIfReady();
    }
};

bool QuestAccept_npc_wizzlecrank_shredder(Player* pPlayer, Creature* pCreature, Quest const* pQuest)
{
    if (pQuest->GetQuestId() == QUEST_ESCAPE)
    {
        if (npc_escortAI* pEscortAI = CAST_AI(npc_wizzlecrank_shredderAI, (pCreature->AI())))
            pEscortAI->Start(true, true, false, pPlayer->GetGUID(), pCreature->GetEntry());
            
        pCreature->setFaction(113);
    }
    return true;
}

CreatureAI* GetAI_npc_wizzlecrank_shredderAI(Creature* pCreature)
{
    return new npc_wizzlecrank_shredderAI(pCreature);
}

/*######
## npc_regthar_deathgate
######*/

enum RegtharDeathgateData {
    QUEST_COUNTERATTACK     = 4021,
    
    NPC_HORDE_DEFENDER      = 9457,
    NPC_HORDE_AXE_THROWER   = 9458,
    NPC_KOLKAR_INVADER      = 9524,
    NPC_KOLKAR_STORMSEER    = 9523,
    NPC_LANTI_GAH           = 9990,
    NPC_WARLOCK_KROMZAR     = 9456,
    
    YELL_COUNTERATTACK_LANTIGAH     = -1000800,
    SAY_COUNTERATTACK_DEFENDER      = -1000801,
    SAY_COUNTERATTACK_BEWARE        = -1000802,
    YELL_COUNTERATTACK_KROMZAR      = -1000803
};

float attackersPos[][4] = { { -301.483246, -1864.453125, 92.982010, 5.107399 },
                            { -296.849884, -1864.198120, 92.681602, 4.956865 },
                            { -292.218994, -1863.122681, 92.529182, 4.685903 },
                            { -288.826233, -1863.246704, 92.524986, 4.675867 },
                            { -284.970764, -1858.442993, 92.458336, 4.645762 },
                            { -280.549591, -1858.738037, 92.583649, 4.645762 },
                            { -286.257660, -1865.968018, 92.625084, 4.645762 },
                            { -272.915497, -1846.779907, 93.371109, 4.164052 },
                            { -282.390839, -1843.288086, 92.586586, 4.489207 },
                            { -290.774078, -1842.121582, 92.882210, 4.736608 },
                            { -300.243317, -1842.350952, 93.933723, 4.736608 },
                            { -307.937622, -1843.978638, 94.907982, 5.060192 },
                            { -316.356171, -1848.410156, 95.201050, 5.319373 },
                            { -229.504166, -1917.720459, 92.358574, 2.399001 },
                            { -234.851791, -1921.778442, 92.701218, 2.219930 },
                            { -238.310852, -1927.177368, 92.959702, 2.571789 },
                            { -241.936066, -1932.835693, 92.934319, 2.571789 },
                            { -251.544861, -1933.647949, 91.972511, 2.571789 },
                            { -252.290527, -1927.605347, 91.928032, 2.571789 },
                            { -246.580887, -1923.654297, 92.278069, 2.571789 },
                            { -243.556091, -1918.933105, 92.251320, 2.571789 },
                            { -243.148834, -1912.324341, 91.964806, 2.571789 },
                            { -251.019424, -1911.610718, 91.883850, 2.571789 },
                            { -255.817886, -1916.005737, 91.875778, 2.571789 },
                            { -260.920074, -1921.308594, 91.817719, 2.571789 },
                            { -265.865906, -1925.523193, 91.897148, 2.571789 }};

    
float defendersPos[][4] = {   { -292.977020, -1892.080566, 91.995491, 1.584019 },
                                { -287.853455, -1892.012817, 92.257652, 1.584019 },
                                { -280.889069, -1891.920776, 92.060722, 1.584019 },
                                { -276.943176, -1876.993286, 92.618614, 1.584019 },
                                { -287.600189, -1872.038208, 92.740288, 1.584019 },
                                { -247.661148, -1911.826416, 91.863655, 6.151107 },
                                { -241.494400, -1915.744019, 92.160278, 6.151107 },
                                { -236.513535, -1912.012329, 92.013985, 6.151107 },
                                { -235.899582, -1907.390991, 91.823219, 6.151107 },
                                { -236.531174, -1917.745117, 92.372627, 6.151107 },
                                { -273.915649, -1925.971069, 92.491234, 1.333126 },
                                { -276.039703, -1922.492432, 92.560356, 1.333126 },
                                { -272.844818, -1917.105469, 92.344337, 1.333126 }};

struct npc_regthar_deathgateAI : public ScriptedAI
{
    npc_regthar_deathgateAI(Creature* c) : ScriptedAI(c), summons(me) {}
    
    bool eventRunning;
    bool lantigahYelled;
    bool kromzar;
    
    uint8 maxDefenders;
    uint8 maxAttackers;
    uint8 defendersCount;
    uint8 attackersCount;
    uint8 killsNeeded;
    uint8 killsCounter;
    
    uint32 despawnAllTimer;
    uint32 defendersTimer;
    uint32 attackersTimer;
    
    uint64 starterGUID;
    
    SummonList summons;
    
    void Reset()
    {
        maxDefenders = 15;
        maxAttackers = 20;
        despawnAllTimer = 0;
        starterGUID = 0;
        eventRunning = false;
        lantigahYelled = false;
        killsNeeded = 0;
        killsCounter = 0;
        kromzar = false;
    }
    
    void Aggro(Unit* who) {}
    
    void JustSummoned(Creature* summoned)
    {
        summons.Summon(summoned);
        
        switch (summoned->GetEntry()) {
            case NPC_KOLKAR_INVADER:
            case NPC_KOLKAR_STORMSEER:
                attackersCount++;
                break;
            case NPC_HORDE_DEFENDER:
            case NPC_HORDE_AXE_THROWER:
                defendersCount++;
                break;
            case NPC_WARLOCK_KROMZAR:
            {
                if (Player* player = Unit::GetPlayer(starterGUID))
                    player->CastSpell(player, 13965, true);
                break;
            }
        }
    }
    
    void SummonedCreatureDespawn(Creature* summon)
    {
        summons.Despawn(summon);
        
        switch (summon->GetEntry()) {
            case NPC_KOLKAR_INVADER:
            case NPC_KOLKAR_STORMSEER:
                attackersCount--;
                killsCounter++;
                break;
            case NPC_HORDE_DEFENDER:
            case NPC_HORDE_AXE_THROWER:
                defendersCount--;
                killsCounter++;
                if (rand()%10 > 3)
                    DoScriptText(SAY_COUNTERATTACK_DEFENDER, me, NULL);
                break;
            case NPC_WARLOCK_KROMZAR:
            {
                if (Player* player = Unit::GetPlayer(starterGUID))
                    player->CastSpell(player, 13965, true);

                EndEvent();
                break;
            }
        }
    }
    
    void StartEvent(uint64 playerGUID)
    {
        if (eventRunning)
            return;
            
        if (Player* player = Unit::GetPlayer(playerGUID))
            DoScriptText(SAY_COUNTERATTACK_BEWARE, me, player);
            
        despawnAllTimer = 0;
        starterGUID = playerGUID;
            
        eventRunning = true;
        lantigahYelled = false;
        killsNeeded = 18 + rand()%6;
        killsCounter = 0;
        kromzar = false;
        defendersTimer = 10000;
        attackersTimer = 8000;
        
        for (uint8 i = 0; i < 8; i++)
            me->SummonCreature(RAND(NPC_HORDE_DEFENDER, NPC_HORDE_AXE_THROWER), defendersPos[rand()%13][0], defendersPos[rand()%13][1], defendersPos[rand()%13][2], defendersPos[rand()%13][3], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 4000);
            
        for (uint8 i = 0; i < 6; i++)
            me->SummonCreature(RAND(NPC_KOLKAR_INVADER, NPC_KOLKAR_STORMSEER), attackersPos[rand()%25][0], attackersPos[rand()%25][1], attackersPos[rand()%25][2], attackersPos[rand()%25][3], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 4000);
            
        for (uint8 i = 13; i < 17; i++)
            me->SummonCreature(RAND(NPC_KOLKAR_INVADER, NPC_KOLKAR_STORMSEER), attackersPos[rand()%25][0], attackersPos[rand()%25][1], attackersPos[rand()%25][2], attackersPos[rand()%25][3], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 4000);
    }
    
    void EndEvent()
    {
        despawnAllTimer = 15000;
        eventRunning = false;
        killsNeeded = 0;
        killsCounter = 0;
        kromzar = false;
    }
    
    void UpdateAI(uint32 const diff)
    {
        if (despawnAllTimer) {
            if (despawnAllTimer <= diff) {
                summons.DespawnAll();
                despawnAllTimer = 0;
            }
            else
                despawnAllTimer -= diff;
        }
        
        if (!eventRunning)
            return;

            
        Player* player = Unit::GetPlayer(starterGUID);
        if (!player || player->isDead()) {
            EndEvent();
            return;
        }
            
        if (defendersTimer <= diff) {
            if (defendersCount < maxDefenders)
                me->SummonCreature(RAND(NPC_HORDE_DEFENDER, NPC_HORDE_AXE_THROWER), defendersPos[rand()%13][0], defendersPos[rand()%13][1], defendersPos[rand()%13][2], defendersPos[rand()%13][3], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 4000);
            
            defendersTimer = 10000 + rand()%2000;
        }
        else
            defendersTimer -= diff;
            
        if (attackersTimer <= diff) {
            if (attackersCount < maxAttackers)
                me->SummonCreature(RAND(NPC_KOLKAR_INVADER, NPC_KOLKAR_STORMSEER), attackersPos[rand()%25][0], attackersPos[rand()%25][1], attackersPos[rand()%25][2], attackersPos[rand()%25][3], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 4000);

            attackersTimer = 8000 + rand()%2000;
        }
        else
            attackersTimer -= diff;
            
        if (killsCounter >= killsNeeded/2 && !lantigahYelled) {
            if (Creature* lantigah = me->FindCreatureInGrid(NPC_LANTI_GAH, 100.0f, true)) {
                lantigahYelled = true;
                DoScriptText(YELL_COUNTERATTACK_LANTIGAH, lantigah, NULL);
            }
        }
        
        if (killsCounter >= killsNeeded && !kromzar) {
            if (Creature* warlordKromzar = me->SummonCreature(NPC_WARLOCK_KROMZAR, -307.937622, -1843.978638, 94.907982, 5.060192, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 8000)) {
                kromzar = true;
                DoScriptText(YELL_COUNTERATTACK_KROMZAR, warlordKromzar, NULL);
            }
        }
    }
};

CreatureAI* GetAI_npc_regthar_deathgate(Creature* creature)
{
    return new npc_regthar_deathgateAI(creature);
}

bool GossipHello_npc_regthar_deathgate(Player* player, Creature* creature)
{
    if (creature->isQuestGiver())
        player->PrepareQuestMenu(creature->GetGUID());

    if (player->GetQuestStatus(QUEST_COUNTERATTACK) == QUEST_STATUS_INCOMPLETE)
        player->ADD_GOSSIP_ITEM(0, "Où est le Seigneur de guerre Krom'zar ?", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF);
        
    player->SEND_GOSSIP_MENU(creature->GetNpcTextId(), creature->GetGUID());
    
    return true;
}

bool GossipSelect_npc_regthar_deathgate(Player* player, Creature* creature, uint32 sender, uint32 action)
{
    if (action == GOSSIP_ACTION_INFO_DEF)
        ((npc_regthar_deathgateAI*)creature->AI())->StartEvent(player->GetGUID());
        
    return true;
}

bool QuestAccept_npc_regthar_deathgate(Player* player, Creature* creature, const Quest* quest)
{
    if (quest->GetQuestId() == QUEST_COUNTERATTACK)
        ((npc_regthar_deathgateAI*)creature->AI())->StartEvent(player->GetGUID());
        
    return true;
}

void AddSC_the_barrens()
{
    Script* newscript;

    newscript = new Script;
    newscript->Name="npc_beaten_corpse";
    newscript->pGossipHello = &GossipHello_npc_beaten_corpse;
    newscript->pGossipSelect = &GossipSelect_npc_beaten_corpse;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_gilthares";
    newscript->GetAI = &GetAI_npc_gilthares;
    newscript->pQuestAccept = &QuestAccept_npc_gilthares;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_sputtervalve";
    newscript->pGossipHello = &GossipHello_npc_sputtervalve;
    newscript->pGossipSelect = &GossipSelect_npc_sputtervalve;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_taskmaster_fizzule";
    newscript->GetAI = &GetAI_npc_taskmaster_fizzule;
    newscript->pReceiveEmote = &ReciveEmote_npc_taskmaster_fizzule;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_twiggy_flathead";
    newscript->GetAI = &GetAI_npc_twiggy_flathead;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name="npc_wizzlecrank_shredder";
    newscript->GetAI = &GetAI_npc_wizzlecrank_shredderAI;
    newscript->pQuestAccept = &QuestAccept_npc_wizzlecrank_shredder;
    newscript->RegisterSelf();
    
    newscript = new Script;
    newscript->Name = "npc_regthar_deathgate";
    newscript->GetAI = &GetAI_npc_regthar_deathgate;
    newscript->pGossipHello = &GossipHello_npc_regthar_deathgate;
    newscript->pGossipSelect = &GossipSelect_npc_regthar_deathgate;
    newscript->pQuestAccept = &QuestAccept_npc_regthar_deathgate;
    newscript->RegisterSelf();
}

