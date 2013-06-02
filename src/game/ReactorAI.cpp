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

#include "ByteBuffer.h"
#include "ReactorAI.h"
#include "Errors.h"
#include "Creature.h"
#include "Log.h"
#include "ObjectAccessor.h"

#define REACTOR_VISIBLE_RANGE (26.46f)

int
ReactorAI::Permissible(const Creature *creature)
{
    if( creature->isCivilian() || creature->IsNeutralToAll() )
        return PERMIT_BASE_REACTIVE;

    return PERMIT_BASE_NO;
}

void
ReactorAI::MoveInLineOfSight(Unit *)
{
}

bool
ReactorAI::IsVisible(Unit *) const
{
    return false;
}

void
ReactorAI::UpdateAI(const uint32 /*time_diff*/)
{
    // update i_victimGuid if i_creature.getVictim() !=0 and changed
    if(!UpdateVictim())
        return;

    i_victimGuid = i_creature.getVictim()->GetGUID();

    if( i_creature.isAttackReady() )
    {
        if( i_creature.IsWithinMeleeRange(i_creature.getVictim()))
        {
            i_creature.AttackerStateUpdate(i_creature.getVictim());
            i_creature.resetAttackTimer();
        }
    }
}

void
ReactorAI::EnterEvadeMode()
{
    if( !i_creature.isAlive() )
    {
        DEBUG_LOG("Creature stoped attacking cuz his dead [guid=%u]", i_creature.GetGUIDLow());
        i_creature.GetMotionMaster()->MovementExpired();
        i_creature.GetMotionMaster()->MoveIdle();
        i_victimGuid = 0;
        i_creature.CombatStop();
        i_creature.DeleteThreatList();
        return;
    }

    Unit* victim = ObjectAccessor::GetUnit(i_creature, i_victimGuid );

    if( !victim  )
    {
        DEBUG_LOG("Creature stopped attacking because victim is non exist [guid=%u]", i_creature.GetGUIDLow());
    }
    else if( victim->HasStealthAura() )
    {
        DEBUG_LOG("Creature stopped attacking cuz his victim is stealth [guid=%u]", i_creature.GetGUIDLow());
    }
    else if( victim->isInFlight() )
    {
        DEBUG_LOG("Creature stopped attacking cuz his victim is fly away [guid=%u]", i_creature.GetGUIDLow());
    }
    else
    {
        DEBUG_LOG("Creature stopped attacking due to target %s [guid=%u]", victim->isAlive() ? "out run him" : "is dead", i_creature.GetGUIDLow());
    }

    i_creature.RemoveAllAuras();
    i_creature.DeleteThreatList();
    i_victimGuid = 0;
    i_creature.CombatStop();
    i_creature.SetLootRecipient(NULL);
    i_creature.ResetPlayerDamageReq();

    if(!i_creature.GetCharmerOrOwner())
    {
        // Remove TargetedMovementGenerator from MotionMaster stack list, and add HomeMovementGenerator instead
        if( i_creature.GetMotionMaster()->GetCurrentMovementGeneratorType() == TARGETED_MOTION_TYPE )
            i_creature.GetMotionMaster()->MoveTargetedHome();
    }
    else if (i_creature.GetOwner() && i_creature.GetOwner()->isAlive())
        i_creature.GetMotionMaster()->MoveFollow(i_creature.GetOwner(),PET_FOLLOW_DIST,PET_FOLLOW_ANGLE);
}

