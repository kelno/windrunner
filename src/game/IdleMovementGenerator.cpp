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
#include "CreatureAINew.h"
#include "Creature.h"

IdleMovementGenerator si_idleMovement;

// StopMoving is needed to make unit stop if its last movement generator expires
// But it should not be sent otherwise there are many redundent packets
void IdleMovementGenerator::Initialize(Unit* owner)
{
    Reset(owner);
}

void
IdleMovementGenerator::Reset(Unit* owner)
{
    if (!owner->IsStopped())
        owner->StopMoving();
}

void RotateMovementGenerator::Initialize(Unit* owner)
{
    if (!owner->IsStopped())
        owner->StopMoving();

    if (owner->getVictim())
        owner->SetInFront(owner->getVictim());

    owner->addUnitState(UNIT_STAT_ROTATING);

    owner->AttackStop();
}

bool RotateMovementGenerator::Update(Unit* owner, const uint32& diff)
{
    float angle = owner->GetOrientation();
    if (m_direction == ROTATE_DIRECTION_LEFT)
    {
        angle += (float)diff * M_PI * 2 / m_maxDuration; 
        while(angle >= (M_PI * 2) ) angle -= M_PI * 2; 
    }
    else
    {
        angle -= (float)diff * M_PI * 2 / m_maxDuration; 
        while(angle < 0) angle += M_PI * 2;
    }

    owner->SetFacingTo(angle);

    if (m_duration > diff)
        m_duration -= diff;
    else
        return false;
        
    return true;
}

void RotateMovementGenerator::Finalize(Unit* unit)
{
    unit->clearUnitState(UNIT_STAT_ROTATING);
    if (unit->GetTypeId() == TYPEID_UNIT)
    {
    	if (unit->GetSummoner())
        {
    	    if (unit->GetSummoner()->ToCreature())
    	        if (unit->GetSummoner()->ToCreature()->getAI())
    	            unit->GetSummoner()->ToCreature()->getAI()->summonedMovementInform(unit->ToCreature(), ROTATE_MOTION_TYPE, 0);
    	}

        if (unit->ToCreature()->getAI())
            unit->ToCreature()->getAI()->onMovementInform(ROTATE_MOTION_TYPE, 0);
        else
            unit->ToCreature()->AI()->MovementInform(ROTATE_MOTION_TYPE, 0);
    }
}

void
DistractMovementGenerator::Initialize(Unit* owner)
{
    owner->addUnitState(UNIT_STAT_DISTRACTED);
}

void
DistractMovementGenerator::Finalize(Unit* owner)
{
    owner->clearUnitState(UNIT_STAT_DISTRACTED);
}

bool
DistractMovementGenerator::Update(Unit* /*owner*/, const uint32& time_diff)
{
    if(time_diff > m_timer)
        return false;

    m_timer -= time_diff;
    return true;
}

void AssistanceDistractMovementGenerator::Finalize(Unit* unit)
{
    unit->clearUnitState(UNIT_STAT_DISTRACTED);
    unit->ToCreature()->SetReactState(REACT_AGGRESSIVE);
}
