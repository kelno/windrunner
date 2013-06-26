/*
 * Copyright (C) 2005-2008 MaNGOS <http://www.mangosproject.org/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
//Basic headers
#include "WaypointMovementGenerator.h"
//Extended headers
#include "ObjectMgr.h"
#include "World.h"
//Creature-specific headers
#include "CreatureAINew.h"
#include "Creature.h"
#include "CreatureAI.h"
//Player-specific
#include "Player.h"
#include "PathFinder.h"

void WaypointMovementGenerator<Creature>::LoadPath(Creature* creature)
{
    if (!path_id)
        path_id = creature->GetWaypointPath();

    i_path = WaypointMgr.GetPath(path_id);

    if (!i_path)
    {
        // No path id found for entry
    	sLog.outError("WaypointMovementGenerator::LoadPath: creature %s (Entry: %u GUID: %u DB GUID: %u) doesn't have waypoint path id: %u", creature->GetName(), creature->GetEntry(), creature->GetGUIDLow(), creature->GetDBTableGUIDLow(), path_id);
        return;
    }

    StartMoveNow(creature);
}

void WaypointMovementGenerator<Creature>::Initialize(Creature* creature)
{
	LoadPath(creature);
	creature->addUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);
}

void WaypointMovementGenerator<Creature>::Finalize(Creature* creature)
{
	creature->clearUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);
	creature->SetWalk(false);
}

void WaypointMovementGenerator<Creature>::Reset(Creature* creature)
{
	creature->addUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);
	StartMoveNow(creature);
}

void WaypointMovementGenerator<Creature>::OnArrived(Creature* creature)
{
    if (!i_path || i_path->empty())
        return;
    if (m_isArrivalDone)
        return;

    creature->clearUnitState(UNIT_STAT_ROAMING_MOVE);
    m_isArrivalDone = true;

    if (i_path->at(i_currentNode)->event_id && urand(0, 99) < i_path->at(i_currentNode)->event_chance)
    {
    	sLog.outDebug("Creature movement start script %u at point %u for %u", i_path->at(i_currentNode)->event_id, i_currentNode, creature->GetGUID());
        sWorld.ScriptsStart(sWaypointScripts, i_path->at(i_currentNode)->event_id, creature, NULL);
    }

    // Inform script
    MovementInform(creature);
    creature->UpdateWaypointID(i_currentNode);
    Stop(i_path->at(i_currentNode)->delay);
}

bool WaypointMovementGenerator<Creature>::StartMove(Creature* creature)
{
    if (!i_path || i_path->empty())
        return false;
    if (Stopped())
        return true;

    if (m_isArrivalDone)
    {
        if ((i_currentNode == i_path->size() - 1) && !repeating) // If that's our last waypoint
        {
            creature->SetHomePosition(i_path->at(i_currentNode)->x, i_path->at(i_currentNode)->y, i_path->at(i_currentNode)->z, creature->GetOrientation());
            creature->GetMotionMaster()->Initialize();
            return false;
        }

        i_currentNode = (i_currentNode+1) % i_path->size();
    }

    WaypointData const* node = i_path->at(i_currentNode);

    m_isArrivalDone = false;

    creature->addUnitState(UNIT_STAT_ROAMING_MOVE);

    Movement::MoveSplineInit init(creature);
    init.MoveTo(node->x, node->y, node->z);

    init.SetWalk(!node->run);
    init.Launch();

    //Call for creature group update
    if (creature->GetFormation() && creature->GetFormation()->getLeader() == creature)
        creature->GetFormation()->LeaderMoveTo(node->x, node->y, node->z);

    return true;
}

bool WaypointMovementGenerator<Creature>::Update(Creature* creature, const uint32 &diff)
{
	// Waypoint movement can be switched on/off
	// This is quite handy for escort quests and other stuff
	if (creature->hasUnitState(UNIT_STAT_NOT_MOVE))
	{
	    creature->clearUnitState(UNIT_STAT_ROAMING_MOVE);
	    return true;
    }
	// prevent a crash at empty waypoint path.
	if (!i_path || i_path->empty())
	    return false;

	if (Stopped())
	{
	    if (CanMove(diff))
	        return StartMove(creature);
	}
	else
	{
	    if (creature->IsStopped())
	        Stop(STOP_TIME_FOR_PLAYER);
	    else if (creature->movespline->Finalized())
	    {
	        OnArrived(creature);
	        return StartMove(creature);
	    }
	}
	return true;
}

void WaypointMovementGenerator<Creature>::MovementInform(Creature* creature)
{
	if (creature->GetSummoner())
	{
	    if (creature->GetSummoner()->ToCreature())
	        if (creature->GetSummoner()->ToCreature()->getAI())
	        	creature->GetSummoner()->ToCreature()->getAI()->summonedMovementInform(creature, WAYPOINT_MOTION_TYPE, i_currentNode);
	}

    if (creature->getAI())
    	creature->getAI()->onMovementInform(WAYPOINT_MOTION_TYPE, i_currentNode);
    else
    	creature->AI()->MovementInform(WAYPOINT_MOTION_TYPE, i_currentNode);
}

bool WaypointMovementGenerator<Creature>::GetResetPos(Creature*, float& x, float& y, float& z)
{
    // prevent a crash at empty waypoint path.
    if (!i_path || i_path->empty())
        return false;

    const WaypointData* node = i_path->at(i_currentNode);
    x = node->x; y = node->y; z = node->z;
    return true;
}


//----------------------------------------------------//

uint32 FlightPathMovementGenerator::GetPathAtMapEnd() const
{
    if (i_currentNode >= i_path->size())
        return i_path->size();

    uint32 curMapId = (*i_path)[i_currentNode].mapid;
    for (uint32 i = i_currentNode; i < i_path->size(); ++i)
    {
        if ((*i_path)[i].mapid != curMapId)
            return i;
    }

    return i_path->size();
}

void FlightPathMovementGenerator::Initialize(Player* player)
{
	Reset(player);
	InitEndGridInfo();
}

void FlightPathMovementGenerator::Finalize(Player* player)
{
	// remove flag to prevent send object build movement packets for flight state and crash (movement generator already not at top of stack)
	player->clearUnitState(UNIT_STAT_IN_FLIGHT);

	player->Unmount();
	player->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_TAXI_FLIGHT);

	if (player->m_taxi.empty())
	{
	    player->getHostilRefManager().setOnlineOfflineState(true);
	    // update z position to ground and orientation for landing point
	    // this prevent cheating with landing  point at lags
	    // when client side flight end early in comparison server side
	    player->StopMoving();
    }
}

#define PLAYER_FLIGHT_SPEED 32.0f

void FlightPathMovementGenerator::Reset(Player* player)
{
    player->getHostilRefManager().setOnlineOfflineState(false);
    player->addUnitState(UNIT_STAT_IN_FLIGHT);
    player->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE | UNIT_FLAG_TAXI_FLIGHT);

    Movement::MoveSplineInit init(player);
    uint32 end = GetPathAtMapEnd();
    for (uint32 i = GetCurrentNode(); i != end; ++i)
    {
        G3D::Vector3 vertice((*i_path)[i].x, (*i_path)[i].y, (*i_path)[i].z);
        init.Path().push_back(vertice);
    }
    init.SetFirstPointId(GetCurrentNode());
    init.SetFly();
    init.SetVelocity(PLAYER_FLIGHT_SPEED);
    init.Launch();
}

bool FlightPathMovementGenerator::Update(Player* player, const uint32 &diff)
{
	uint32 pointId = (uint32)player->movespline->currentPathIdx();
	if (pointId > i_currentNode)
	{
	    bool departureEvent = true;
	    do
	    {
	        DoEventIfAny(player, (*i_path)[i_currentNode], departureEvent);
	        if (pointId == i_currentNode)
	            break;
	        if (i_currentNode == _preloadTargetNode)
	            PreloadEndGrid();
	        i_currentNode += (uint32)departureEvent;
	        departureEvent = !departureEvent;
	    }
	    while (true);
	}

	return i_currentNode < (i_path->size()-1);
}

void
FlightPathMovementGenerator::SetCurrentNodeAfterTeleport()
{
	if (i_path->empty())
	    return;

	uint32 map0 = (*i_path)[0].mapid;
	for (size_t i = 1; i < i_path->size(); ++i)
	{
	    if ((*i_path)[i].mapid != map0)
	    {
	        i_currentNode = i;
	        return;
	    }
	}
}

void FlightPathMovementGenerator::DoEventIfAny(Player* player, TaxiPathNodeEntry const& node, bool departure)
{
    if (uint32 eventid = departure ? node.departureEventID : node.arrivalEventID)
    {
    	sLog.outDebug("Taxi %s event %u of node %u of path %u for player %s", departure ? "departure" : "arrival", eventid, node.index, node.path, player->GetName());
        sWorld.ScriptsStart(sEventScripts, eventid, player, player);
    }
}

bool FlightPathMovementGenerator::GetResetPos(Player*, float& x, float& y, float& z)
{
    const TaxiPathNodeEntry& node = (*i_path)[i_currentNode];
    x = node.x; y = node.y; z = node.z;
    return true;
}

void FlightPathMovementGenerator::InitEndGridInfo()
{
    /*! Storage to preload flightmaster grid at end of flight. For multi-stop flights, this will
       be reinitialized for each flightmaster at the end of each spline (or stop) in the flight. */
    uint32 nodeCount = (*i_path).size();        //! Number of nodes in path.
    _endMapId = (*i_path)[nodeCount - 1].mapid; //! MapId of last node
    _preloadTargetNode = nodeCount - 3;
    _endGridX = (*i_path)[nodeCount - 1].x;
    _endGridY = (*i_path)[nodeCount - 1].y;
}

void FlightPathMovementGenerator::PreloadEndGrid()
{
    // used to preload the final grid where the flightmaster is
    Map* endMap = MapManager::Instance().FindBaseNonInstanceMap(_endMapId);

    // Load the grid
    if (endMap)
    {
    	sLog.outDebug("Preloading rid (%f, %f) for map %u at node index %u/%u", _endGridX, _endGridY, _endMapId, _preloadTargetNode, (uint32)(i_path->size()-1));
        endMap->LoadGrid(_endGridX, _endGridY);
    }
    else
    	sLog.outDebug("Unable to determine map to preload flightmaster grid");
}
