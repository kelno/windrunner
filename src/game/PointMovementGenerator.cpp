/*
 * Copyright (C) 2005-2008 MaNGOS <http://www.mangosproject.org/>
 *
 * Copyright (C) 2008 Trinity <http://www.trinitycore.org/>
 *
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

#include "PointMovementGenerator.h"
#include "Errors.h"
#include "CreatureAINew.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "MapManager.h"
#include "DestinationHolderImp.h"

//----- Point Movement Generator
template<class T>
void PointMovementGenerator<T>::Initialize(T &unit)
{
    unit.StopMoving();
    Traveller<T> traveller(unit);
    float travSpeed = traveller.Speed();
    if(!travSpeed)
        return;

    i_destinationHolder.SetDestination(traveller, i_x, i_y, i_z, !m_usePathfinding);

    if(m_usePathfinding)
    {
        PathInfo path(&unit, i_x, i_y, i_z);
        PointPath pointPath = path.getFullPath();

        float speed;
        if (m_speed > 0.0f)
            speed = m_speed;
        else
            speed = travSpeed * 0.001f; // in ms
        uint32 traveltime = uint32(pointPath.GetTotalLength() / speed);
        SplineFlags flags = (unit.GetTypeId() == TYPEID_UNIT) ? ((SplineFlags)((Creature*)&unit)->GetUnitMovementFlags()) : SPLINEFLAG_WALKMODE;   // TODOMMAPS: Merge SplineFlags
        unit.SendMonsterMoveByPath(pointPath, 1, pointPath.size(), flags, traveltime);
    }

    if (unit.GetTypeId() == TYPEID_UNIT && ((Unit*)&unit)->ToCreature()->canFly())
        unit.AddUnitMovementFlag(MOVEMENTFLAG_FLYING2);
}

template<class T>
bool PointMovementGenerator<T>::Update(T &unit, const uint32 &diff)
{
    if(!&unit)
        return false;

    if(unit.HasUnitState(UNIT_STAT_ROOT | UNIT_STAT_STUNNED))
    {
        if(unit.HasUnitState(UNIT_STAT_CHARGING))
            return false;
        else
            return true;
    }

    Traveller<T> traveller(unit);

    i_destinationHolder.UpdateTraveller(traveller, diff);

    if(i_destinationHolder.HasArrived())
    {
        unit.clearUnitState(UNIT_STAT_MOVE);
        arrived = true;
        return false;
    }

    return true;
}

template<class T>
void PointMovementGenerator<T>:: Finalize(T &unit)
{
    if(unit.HasUnitState(UNIT_STAT_CHARGING))
        unit.clearUnitState(UNIT_STAT_CHARGING);
    else if(arrived)
        MovementInform(unit);
}

template<class T>
void PointMovementGenerator<T>::MovementInform(T &unit)
{
}

template <> void PointMovementGenerator<Creature>::MovementInform(Creature &unit)
{
	if (unit.GetSummoner())
	{
		if (unit.GetSummoner()->ToCreature())
	        if (unit.GetSummoner()->ToCreature()->getAI())
	        	unit.GetSummoner()->ToCreature()->getAI()->summonedMovementInform(&unit, POINT_MOTION_TYPE, id);
	}

    if (unit.getAI())
        unit.getAI()->onMovementInform(POINT_MOTION_TYPE, id);
    else
        unit.AI()->MovementInform(POINT_MOTION_TYPE, id);
}

template void PointMovementGenerator<Player>::Initialize(Player&);
template bool PointMovementGenerator<Player>::Update(Player &, const uint32 &diff);
template void PointMovementGenerator<Player>::MovementInform(Player&);
template void PointMovementGenerator<Player>::Finalize(Player&);

template void PointMovementGenerator<Creature>::Initialize(Creature&);
template bool PointMovementGenerator<Creature>::Update(Creature&, const uint32 &diff);
template void PointMovementGenerator<Creature>::Finalize(Creature&);
