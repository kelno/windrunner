/* Copyright (C) 2006 - 2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * This program is free software licensed under GPL version 2
 * Please see the included DOCS/LICENSE.TXT for more information */

/* ScriptData
SDName: Npc_EscortAI
SD%Complete: 100
SDComment:
SDCategory: Npc
EndScriptData */

#include "precompiled.h"
#include "EscortAI.h"

#define WP_LAST_POINT   -1

bool npc_escortAI::IsVisible(Unit* who) const
{
    if (!who)
        return false;

    return (m_creature->GetDistance(who) < VISIBLE_RANGE) && who->isVisibleForOrDetect(m_creature,true);
}

void npc_escortAI::AttackStart(Unit *who)
{
    if (!who)
        return;

    if (IsBeingEscorted && !Defend)
        return;

    if ( m_creature->Attack(who, true) )
    {
        m_creature->AddThreat(who, 0.0f);

        if (!InCombat)
        {
            InCombat = true;

            if (IsBeingEscorted)
            {
                //Store last position
                m_creature->GetPosition(LastPos.x, LastPos.y, LastPos.z);
            }

            EnterCombat(who);
        }

        m_creature->GetMotionMaster()->MovementExpired();
        m_creature->GetMotionMaster()->MoveChase(who);
    }
}

void npc_escortAI::MoveInLineOfSight(Unit *who)
{
    if (IsBeingEscorted && AssistPlayerInCombat(who))
        return;

    if (IsBeingEscorted && !Attack)
        return;

    if(m_creature->GetVictim() || !m_creature->canStartAttack(who))
        return;

    AttackStart(who);
}

void npc_escortAI::JustRespawned()
{
    InCombat = false;
    IsBeingEscorted = false;
    IsOnHold = false;

    //Re-Enable questgiver flag
    m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);

    Reset();
}

void npc_escortAI::EnterEvadeMode()
{
    InCombat = false;

    m_creature->RemoveAllAuras();
    m_creature->DeleteThreatList();
    m_creature->CombatStop();
    m_creature->SetLootRecipient(NULL);

    if (IsBeingEscorted || !DespawnAtEnd)
    {
        Returning = true;
        m_creature->GetMotionMaster()->MovementExpired();
        m_creature->GetMotionMaster()->MovePoint(WP_LAST_POINT, LastPos.x, LastPos.y, LastPos.z);

    }else
    {
        m_creature->GetMotionMaster()->MovementExpired();
        m_creature->GetMotionMaster()->MoveTargetedHome();
    }
    m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

    Reset();
}

void npc_escortAI::UpdateAI(const uint32 diff)
{
    me->SetEscorted(IsBeingEscorted);
    
    /*
    if (InCombat) {
        if (me->getAttackers().size() == 0)
            EnterEvadeMode();
    }*/

    //Waypoint Updating
    if (IsBeingEscorted && !InCombat && WaitTimer && !Returning)
    {
        if (WaitTimer <= diff)
        {
            if (ReconnectWP)
            {
                //Correct movement speed
                if (Run)
                    m_creature->RemoveUnitMovementFlag(MOVEMENTFLAG_WALK_MODE);
                else
                    m_creature->AddUnitMovementFlag(MOVEMENTFLAG_WALK_MODE);

                //Continue with waypoints
                if( !IsOnHold )
                {
                    m_creature->GetMotionMaster()->MovePoint(CurrentWP->id, CurrentWP->x, CurrentWP->y, CurrentWP->z );
                    WaitTimer = 0;
                    ReconnectWP = false;
                    return;
                }
            }

            //End of the line, Despawn self then immediatly respawn
            if (CurrentWP == WaypointList.end())
            {
                if(DespawnAtEnd)
                {
                    m_creature->setDeathState(JUST_DIED);
                    m_creature->SetHealth(0);
                    m_creature->CombatStop();
                    m_creature->DeleteThreatList();
                    m_creature->Respawn();
                    m_creature->GetMotionMaster()->Clear(true);

                    //Re-Enable gossip
                    m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);

                    IsBeingEscorted = false;
                    WaitTimer = 0;
                    return;
                }else{
                    if (DespawnAtEnd)
                        IsBeingEscorted = false;
                    WaitTimer = 0;
                    return;
                }
            }

            if( !IsOnHold )
            {
                m_creature->GetMotionMaster()->MovePoint(CurrentWP->id, CurrentWP->x, CurrentWP->y, CurrentWP->z );
                WaitTimer = 0;
            }
        }else WaitTimer -= diff;
    }

    //Check if player is within range if he's a player
    Player* player = Unit::GetPlayer(PlayerGUID);
    if (IsBeingEscorted && !InCombat && PlayerGUID && player && player->GetSession()->GetSecurity() <= SEC_PLAYER)
    {
        if (PlayerTimer < diff)
        {
            Unit* p = Unit::GetUnit(*m_creature, PlayerGUID);

            if (DespawnAtFar && (!p || m_creature->GetDistance(p) > GetMaxPlayerDistance()))
            {
                JustDied(m_creature);
                IsBeingEscorted = false;

                m_creature->setDeathState(JUST_DIED);
                m_creature->SetHealth(0);
                m_creature->CombatStop();
                m_creature->DeleteThreatList();
                m_creature->Respawn();
                m_creature->GetMotionMaster()->Clear(true);

                //Re-Enable gossip
                m_creature->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_GOSSIP);
            }

            PlayerTimer = 1000;
        }else PlayerTimer -= diff;
    }

    if(CanMelee)
    {
        //Check if we have a current target
        if( m_creature->IsAlive() && UpdateVictim())
            DoMeleeAttackIfReady();
    }
}

void npc_escortAI::MovementInform(uint32 type, uint32 id)
{
    if (type != POINT_MOTION_TYPE || !IsBeingEscorted)
        return;

    //Original position reached, continue waypoint movement
    if (id == WP_LAST_POINT)
    {
        ReconnectWP = true;
        Returning = false;
        WaitTimer = 1;

    }else
    {
        //Make sure that we are still on the right waypoint
        if (CurrentWP->id != id)
        {
            return;
        }

        //Call WP function
        WaypointReached(CurrentWP->id);

        WaitTimer = CurrentWP->WaitTimeMs + 1;

        ++CurrentWP;
    }
}

void npc_escortAI::OnPossess(bool apply)
{
    // We got possessed in the middle of being escorted, store the point
    // where we left off to come back to when possess is removed
    if (IsBeingEscorted)
    {
        if (apply)
            m_creature->GetPosition(LastPos.x, LastPos.y, LastPos.z);
        else
        {
            Returning = true;
            m_creature->GetMotionMaster()->MovementExpired();
            m_creature->GetMotionMaster()->MovePoint(WP_LAST_POINT, LastPos.x, LastPos.y, LastPos.z);
        }
    }
}

void npc_escortAI::AddWaypoint(uint32 id, float x, float y, float z, uint32 WaitTimeMs)
{
    Escort_Waypoint t(id, x, y, z, WaitTimeMs);

    WaypointList.push_back(t);
}

void npc_escortAI::SetRun(bool bRun)
{
    if (bRun)
    {
        if (!Run)
            m_creature->RemoveUnitMovementFlag(MOVEMENTFLAG_WALK_MODE);
        else
            sLog.outError("EscortAI attempt to set run mode for creature %u, but is already running.", me->GetEntry());
    }
    else
    {
        if (Run)
            m_creature->AddUnitMovementFlag(MOVEMENTFLAG_WALK_MODE);
        else
            sLog.outError("EscortAI attempt to set walk mode for creature %u, but is already walking.", me->GetEntry());
    }
    Run = bRun;
}

void npc_escortAI::GetWaypointListFromDB(uint32 entry)
{
    if (entry == 0)
        return; //entry = 0 means the script use old waypoint system, with AddWaypoint in the script constructor
    QueryResult* result = WorldDatabase.PQuery("SELECT pointid, location_x, location_y, location_z, waittime FROM escort_waypoints WHERE entry = '%u' ORDER BY pointid", entry);
    
    if (!result)
    {
        sLog.outError("SD2 ERROR: EscortAI: Attempt to GetWaypointListFromDB for creature entry %u, but no record found in DB !", entry);
        return;
    }
    
    uint16 pointid;
    float location_x, location_y, location_z;
    uint32 waittime;
    
    do
    {
        Field* fields = result->Fetch();
        pointid = fields[0].GetUInt16();
        location_x = fields[1].GetFloat();
        location_y = fields[2].GetFloat();
        location_z = fields[3].GetFloat();
        waittime = fields[4].GetUInt32();
        
        AddWaypoint(pointid, location_x, location_y, location_z, waittime);        
    }while (result->NextRow());
}

void npc_escortAI::Start(bool bAttack, bool bDefend, bool bRun, uint64 pGUID, uint32 entry)
{
    if (InCombat)
        return;

    GetWaypointListFromDB(entry);

    if (WaypointList.empty()) {
        sLog.outError("Call to escortAI::Start with 0 waypoints for creature %u", me->GetEntry());
        return;
    }

    Attack = bAttack;
    Defend = bDefend;
    Run = bRun;
    PlayerGUID = pGUID;

    CurrentWP = WaypointList.begin();

    //Set initial speed
    if (Run)
        m_creature->RemoveUnitMovementFlag(MOVEMENTFLAG_WALK_MODE);
    else m_creature->AddUnitMovementFlag(MOVEMENTFLAG_WALK_MODE);

    //Start WP
    m_creature->GetMotionMaster()->MovePoint(CurrentWP->id, CurrentWP->x, CurrentWP->y, CurrentWP->z );
    IsBeingEscorted = true;
    ReconnectWP = false;
    Returning = false;
    IsOnHold = false;

    //Disable questgiver flag
    m_creature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
    
    AddEscortState(STATE_ESCORT_ESCORTING);
}

void npc_escortAI::SetEscortPaused(bool bPaused)
{
    if (!HasEscortState(STATE_ESCORT_ESCORTING))
        return;
        
    IsOnHold = bPaused;

    if (bPaused)
        AddEscortState(STATE_ESCORT_PAUSED);
    else
        RemoveEscortState(STATE_ESCORT_PAUSED);
}

bool npc_escortAI::AssistPlayerInCombat(Unit* who)
{
    if (!who || !who->GetVictim())
        return false;

    //experimental (unknown) flag not present
    if (!(me->GetCreatureInfo()->type_flags & 0x001000/*CREATURE_TYPEFLAGS_AID_PLAYERS*/))
        return false;

    //not a player
    if (!who->GetVictim()->GetCharmerOrOwnerPlayerOrPlayerItself())
        return false;

    //never attack friendly
    if (me->IsFriendlyTo(who))
        return false;

    //too far away and no free sight?
    if (me->IsWithinDistInMap(who, 25.0f) && me->IsWithinLOSInMap(who))
    {
        //already fighting someone?
        if (!me->GetVictim())
        {
            EnterCombat(who);
            AttackStart(who);
            return true;
        }
        else
        {
            who->SetInCombatWith(me);
            me->AddThreat(who, 0.0f);
            return true;
        }
    }

    return false;
}
