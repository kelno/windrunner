/* Copyright (C) 2006 - 2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

/* ScriptData
SDName: FollowerAI
SD%Complete: 50
SDComment: This AI is under development
SDCategory: Npc
EndScriptData */

#include "precompiled.h"
#include "FollowerAI.h"

const float MAX_PLAYER_DISTANCE = 100.0f;

enum ePoints
{
    POINT_COMBAT_START  = 0xFFFFFF
};

FollowerAI::FollowerAI(Creature* pCreature) : ScriptedAI(pCreature),
    m_uiLeaderGUID(0),
    m_pQuestForFollow(NULL),
    m_uiUpdateFollowTimer(2500),
    m_uiFollowState(STATE_FOLLOW_NONE)
{}

void FollowerAI::AttackStart(Unit* pWho)
{
    if (!pWho)
        return;

    if (m_creature->Attack(pWho, true))
    {
        m_creature->AddThreat(pWho, 0.0f);
        m_creature->SetInCombatWith(pWho);
        pWho->SetInCombatWith(m_creature);

        if (m_creature->HasUnitState(UNIT_STAT_FOLLOW))
            m_creature->clearUnitState(UNIT_STAT_FOLLOW);

        if (IsCombatMovementAllowed())
            m_creature->GetMotionMaster()->MoveChase(pWho);
    }
}

//This part provides assistance to a player that are attacked by pWho, even if out of normal aggro range
//It will cause m_creature to attack pWho that are attacking _any_ player (which has been confirmed may happen also on offi)
//The flag (type_flag) is unconfirmed, but used here for further research and is a good candidate.
bool FollowerAI::AssistPlayerInCombat(Unit* pWho)
{
    if (!pWho || !pWho->GetVictim())
        return false;

    //experimental (unknown) flag not present
    if (!(m_creature->GetCreatureInfo()->type_flags & CREATURE_TYPEFLAGS_AID_PLAYERS))
        return false;

    //not a player
    if (!pWho->GetVictim()->GetCharmerOrOwnerPlayerOrPlayerItself())
        return false;

    //never attack friendly
    if (m_creature->IsFriendlyTo(pWho))
        return false;

    //too far away and no free sight?
    if (m_creature->IsWithinDistInMap(pWho, MAX_PLAYER_DISTANCE) && m_creature->IsWithinLOSInMap(pWho))
    {
        //already fighting someone?
        if (!m_creature->GetVictim())
        {
            AttackStart(pWho);
            return true;
        }
        else
        {
            pWho->SetInCombatWith(m_creature);
            m_creature->AddThreat(pWho, 0.0f);
            return true;
        }
    }

    return false;
}

void FollowerAI::MoveInLineOfSight(Unit* pWho)
{
    if (!m_creature->HasUnitState(UNIT_STAT_STUNNED) && m_creature->canAttack(pWho) && pWho->isInAccessiblePlaceFor(m_creature))
    {
        if (HasFollowState(STATE_FOLLOW_INPROGRESS) && AssistPlayerInCombat(pWho))
            return;

        if (!m_creature->canFly() && m_creature->GetDistanceZ(pWho) > CREATURE_Z_ATTACK_RANGE)
            return;

        if (m_creature->IsHostileTo(pWho))
        {
            float fAttackRadius = m_creature->GetAttackDistance(pWho);
            if (m_creature->IsWithinDistInMap(pWho, fAttackRadius) && m_creature->IsWithinLOSInMap(pWho))
            {
                if (!m_creature->GetVictim())
                {
                    pWho->RemoveAurasDueToSpell(SPELL_AURA_MOD_STEALTH);
                    AttackStart(pWho);
                }
                else if (m_creature->GetMap()->IsDungeon())
                {
                    pWho->SetInCombatWith(m_creature);
                    m_creature->AddThreat(pWho, 0.0f);
                }
            }
        }
    }
}

void FollowerAI::JustDied(Unit* pKiller)
{
    if (!HasFollowState(STATE_FOLLOW_INPROGRESS) || !m_uiLeaderGUID || !m_pQuestForFollow)
        return;

    //TODO: need a better check for quests with time limit.
    if (Player* pPlayer = GetLeaderForFollower())
    {
        if (Group* pGroup = pPlayer->GetGroup())
        {
            for (GroupReference* pRef = pGroup->GetFirstMember(); pRef != NULL; pRef = pRef->next())
            {
                if (Player* pMember = pRef->getSource())
                {
                    if (pPlayer->GetQuestStatus(m_pQuestForFollow->GetQuestId()) == QUEST_STATUS_INCOMPLETE)
                        pPlayer->FailQuest(m_pQuestForFollow->GetQuestId());
                }
            }
        }
        else
        {
            if (pPlayer->GetQuestStatus(m_pQuestForFollow->GetQuestId()) == QUEST_STATUS_INCOMPLETE)
                pPlayer->FailQuest(m_pQuestForFollow->GetQuestId());
        }
    }
}

void FollowerAI::JustRespawned()
{
    m_uiFollowState = STATE_FOLLOW_NONE;

    if (!IsCombatMovementAllowed())
        SetCombatMovementAllowed(true);

    if (m_creature->getFaction() != m_creature->GetCreatureInfo()->faction_A)
        m_creature->setFaction(m_creature->GetCreatureInfo()->faction_A);

    Reset();
}

void FollowerAI::EnterEvadeMode()
{
    m_creature->RemoveAllAuras();
    m_creature->DeleteThreatList();
    m_creature->CombatStop(true);
    m_creature->SetLootRecipient(NULL);

    if (HasFollowState(STATE_FOLLOW_INPROGRESS))
    {
        if (m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() == TARGETED_MOTION_TYPE)
        {
            float fPosX, fPosY, fPosZ;
            m_creature->GetPosition(fPosX, fPosY, fPosZ);
            m_creature->GetMotionMaster()->MovePoint(POINT_COMBAT_START, fPosX, fPosY, fPosZ);
        }
    }
    else
    {
        if (m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() == TARGETED_MOTION_TYPE)
            m_creature->GetMotionMaster()->MoveTargetedHome();
    }

    Reset();
}

void FollowerAI::UpdateAI(const uint32 uiDiff)
{
    if (HasFollowState(STATE_FOLLOW_INPROGRESS) && !m_creature->GetVictim())
    {
        if (m_uiUpdateFollowTimer <= uiDiff)
        {
            if (HasFollowState(STATE_FOLLOW_COMPLETE) && !HasFollowState(STATE_FOLLOW_POSTEVENT))
            {
                m_creature->ForcedDespawn();
                return;
            }

            bool bIsMaxRangeExceeded = true;

            if (Player* pPlayer = GetLeaderForFollower())
            {
                if (HasFollowState(STATE_FOLLOW_RETURNING))
                {
                    RemoveFollowState(STATE_FOLLOW_RETURNING);
                    m_creature->GetMotionMaster()->MoveFollow(pPlayer, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
                    return;
                }

                if (Group* pGroup = pPlayer->GetGroup())
                {
                    for (GroupReference* pRef = pGroup->GetFirstMember(); pRef != NULL; pRef = pRef->next())
                    {
                        Player* pMember = pRef->getSource();

                        if (pMember && m_creature->IsWithinDistInMap(pMember, MAX_PLAYER_DISTANCE))
                        {
                            bIsMaxRangeExceeded = false;
                            break;
                        }
                    }
                }
                else
                {
                    if (m_creature->IsWithinDistInMap(pPlayer, MAX_PLAYER_DISTANCE))
                        bIsMaxRangeExceeded = false;
                }
            }

            if (bIsMaxRangeExceeded)
            {
                m_creature->ForcedDespawn();
                return;
            }

            m_uiUpdateFollowTimer = 1000;
        }
        else
            m_uiUpdateFollowTimer -= uiDiff;
    }

    UpdateFollowerAI(uiDiff);
}

void FollowerAI::UpdateFollowerAI(const uint32 uiDiff)
{
    if (!UpdateVictim())
        return;

    DoMeleeAttackIfReady();
}

void FollowerAI::MovementInform(uint32 uiMotionType, uint32 uiPointId)
{
    if (uiMotionType != POINT_MOTION_TYPE || !HasFollowState(STATE_FOLLOW_INPROGRESS))
        return;

    if (uiPointId == POINT_COMBAT_START)
    {
        if (GetLeaderForFollower())
        {
            if (!HasFollowState(STATE_FOLLOW_PAUSED))
                AddFollowState(STATE_FOLLOW_RETURNING);
        }
        else
            m_creature->ForcedDespawn();
    }
}

void FollowerAI::StartFollow(Player* pLeader, uint32 uiFactionForFollower, const Quest* pQuest)
{
    if (m_creature->GetVictim())
        return;

    if (HasFollowState(STATE_FOLLOW_INPROGRESS))
    {
        sLog.outError("FollowerAI attempt to StartFollow while already following for creature %u.", me->GetEntry());
        return;
    }

    //set variables
    m_uiLeaderGUID = pLeader->GetGUID();

    if (uiFactionForFollower)
        m_creature->setFaction(uiFactionForFollower);

    m_pQuestForFollow = pQuest;

    if (m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() == WAYPOINT_MOTION_TYPE)
    {
        m_creature->GetMotionMaster()->Clear();
        m_creature->GetMotionMaster()->MoveIdle();
    }

    m_creature->SetUInt32Value(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_NONE);

    AddFollowState(STATE_FOLLOW_INPROGRESS);

    m_creature->GetMotionMaster()->MoveFollow(pLeader, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
}

Player* FollowerAI::GetLeaderForFollower()
{
    if (Player* pLeader = Unit::GetPlayer(m_uiLeaderGUID))
    {
        if (pLeader->IsAlive())
            return pLeader;
        else
        {
            if (Group* pGroup = pLeader->GetGroup())
            {
                for (GroupReference* pRef = pGroup->GetFirstMember(); pRef != NULL; pRef = pRef->next())
                {
                    Player* pMember = pRef->getSource();

                    if (pMember && pMember->IsAlive() && m_creature->IsWithinDistInMap(pMember, MAX_PLAYER_DISTANCE))
                    {
                        m_uiLeaderGUID = pMember->GetGUID();
                        return pMember;
                        break;
                    }
                }
            }
        }
    }

    sLog.outError("FollowerAI GetLeader can not find suitable leader for creature %u.", me->GetEntry());
    return NULL;
}

void FollowerAI::SetFollowComplete(bool bWithEndEvent)
{
    if (m_creature->HasUnitState(UNIT_STAT_FOLLOW))
    {
        m_creature->clearUnitState(UNIT_STAT_FOLLOW);

        m_creature->StopMoving();
        m_creature->GetMotionMaster()->Clear();
        m_creature->GetMotionMaster()->MoveIdle();
    }

    if (bWithEndEvent)
        AddFollowState(STATE_FOLLOW_POSTEVENT);
    else
    {
        if (HasFollowState(STATE_FOLLOW_POSTEVENT))
            RemoveFollowState(STATE_FOLLOW_POSTEVENT);
    }

    AddFollowState(STATE_FOLLOW_COMPLETE);
}

void FollowerAI::SetFollowPaused(bool bPaused)
{
    if (!HasFollowState(STATE_FOLLOW_INPROGRESS) || HasFollowState(STATE_FOLLOW_COMPLETE))
        return;

    if (bPaused)
    {
        AddFollowState(STATE_FOLLOW_PAUSED);

        if (m_creature->HasUnitState(UNIT_STAT_FOLLOW))
        {
            m_creature->clearUnitState(UNIT_STAT_FOLLOW);

            m_creature->StopMoving();
            m_creature->GetMotionMaster()->Clear();
            m_creature->GetMotionMaster()->MoveIdle();
        }
    }
    else
    {
        RemoveFollowState(STATE_FOLLOW_PAUSED);

        if (Player* pLeader = GetLeaderForFollower())
            m_creature->GetMotionMaster()->MoveFollow(pLeader, PET_FOLLOW_DIST, PET_FOLLOW_ANGLE);
    }
}
