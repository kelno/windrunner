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

#include "IdleMovementGenerator.h"
#include "Creature.h"

IdleMovementGenerator si_idleMovement;

// StopMoving is needed to make unit stop if its last movement generator expires
// But it should not be sent otherwise there are many redundent packets
void IdleMovementGenerator::Initialize(Unit &owner)
{
    if(owner.hasUnitState(UNIT_STAT_MOVE))
        owner.StopMoving();
}

void
IdleMovementGenerator::Reset(Unit& owner)
{
    if(owner.hasUnitState(UNIT_STAT_MOVE))
        owner.StopMoving();
    // Set Flying movement flag, so flying mobs do not fall down on ground when placed in air
    if(owner.GetTypeId() == TYPEID_UNIT && ((Creature*)&owner)->canFly())
        owner.AddUnitMovementFlag(MOVEMENTFLAG_FLYING2);
}

void
DistractMovementGenerator::Initialize(Unit& owner)
{
    owner.addUnitState(UNIT_STAT_DISTRACTED);
}

void
DistractMovementGenerator::Finalize(Unit& owner)
{
    owner.clearUnitState(UNIT_STAT_DISTRACTED);
}

bool
DistractMovementGenerator::Update(Unit& owner, const uint32& time_diff)
{
    if(time_diff > m_timer)
        return false;

    m_timer -= time_diff;
    return true;
}

