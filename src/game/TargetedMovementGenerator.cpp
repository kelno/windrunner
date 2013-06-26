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
#include "TargetedMovementGenerator.h"
#include "Errors.h"
#include "Creature.h"
#include "MapManager.h"
#include "World.h"

#include <cmath>

template<class T, typename D>
void TargetedMovementGeneratorMedium<T, D>::_setTargetLocation(T* owner, bool updateDestination)
{
	if (!i_target.isValid() || !i_target->IsInWorld())
        return;

    if( owner->hasUnitState(UNIT_STAT_NOT_MOVE) )
        return;

    if (owner->GetTypeId() == TYPEID_UNIT && !i_target->isInAccessiblePlaceFor(owner->ToCreature()))
        return;

    float x, y, z;

    if (updateDestination || !i_path)
    {
        if (!i_offset)
        {
            // to nearest contact position
            i_target->GetContactPoint(owner, x, y, z);
        }
        else
        {
            float dist;
            float size;

            // Pets need special handling.
            // We need to subtract GetObjectSize() because it gets added back further down the chain
            //  and that makes pets too far away. Subtracting it allows pets to properly
            //  be (GetCombatReach() + i_offset) away.
            // Only applies when i_target is pet's owner otherwise pets and mobs end up
            //   doing a "dance" while fighting
            if (owner->IsPet() && i_target->GetTypeId() == TYPEID_PLAYER)
            {
                dist = i_target->GetCombatReach();
                size = i_target->GetCombatReach() - i_target->GetObjectSize();
            }
            else
            {
                dist = i_offset + 1.0f;
                size = owner->GetObjectSize();
            }

            if (i_target->IsWithinDistInMap(owner, dist))
                return;

            // to at i_offset distance from target and i_angle from target facing
            i_target->GetClosePoint(x, y, z, size, i_offset, i_angle);
        }
    }
    else
    {
        // the destination has not changed, we just need to refresh the path (usually speed change)
        G3D::Vector3 end = i_path->getEndPosition();
        x = end.x;
        y = end.y;
        z = end.z;
    }

    if (!i_path)
        i_path = new PathInfo(owner);

    // allow pets to use shortcut if no path found when following their master
    bool forceDest = (owner->GetTypeId() == TYPEID_UNIT && owner->ToCreature()->IsPet()
        && owner->HasUnitState(UNIT_STATE_FOLLOW));

    bool result = i_path->Update(x, y, z, false, forceDest);
    if (!result || (i_path->getPathType() & PATHFIND_NOPATH))
    {
        // Cant reach target
        i_recalculateTravel = true;
        return;
    }

    D::_addUnitStateMove(owner);
    i_targetReached = false;
    i_recalculateTravel = false;
    owner->addUnitState(UNIT_STAT_CHASE);

    Movement::MoveSplineInit init(owner);
    init.MovebyPath(i_path->getFullPath());
    init.SetWalk(((D*)this)->EnableWalking());
    // Using the same condition for facing target as the one that is used for SetInFront on movement end
    // - applies to ChaseMovementGenerator mostly
    if (i_angle == 0.f)
        init.SetFacing(i_target.getTarget());

    init.Launch();
}

template<class T, typename D>
bool TargetedMovementGeneratorMedium<T, D>::Update(T* owner, const uint32 & time_diff)
{
	if (!i_target.isValid() || !i_target->IsInWorld())
	    return false;

	if (!owner || !owner->IsAlive())
	    return false;

	if (owner->hasUnitState(UNIT_STAT_NOT_MOVE))
    {
	    D::_clearUnitStateMove(owner);
	    return true;
    }

	// prevent movement while casting spells with cast time or channel time
    if (owner->hasUnitState(UNIT_STAT_CASTING))
    {
	    if (!owner->IsStopped())
	        owner->StopMoving();
	    return true;
	}

	// prevent crash after creature killed pet
	if (static_cast<D*>(this)->_lostTarget(owner))
    {
	    D::_clearUnitStateMove(owner);
	    return true;
	}

	bool targetMoved = false;
    i_recheckDistance.Update(time_diff);
	if (i_recheckDistance.Passed())
    {
	    i_recheckDistance.Reset(100);
	    //More distance let have better performance, less distance let have more sensitive reaction at target move.
	    float allowed_dist = owner->GetCombatReach() + sWorld.getRate(RATE_TARGET_POS_RECALCULATION_RANGE);
	    G3D::Vector3 dest = owner->movespline->FinalDestination();

	    if (owner->GetTypeId() == TYPEID_UNIT && owner->ToCreature()->CanFly())
	        targetMoved = !i_target->IsWithinDist3d(dest.x, dest.y, dest.z, allowed_dist);
	    else
	        targetMoved = !i_target->IsWithinDist2d(dest.x, dest.y, allowed_dist);
	}

	if (i_recalculateTravel || targetMoved)
	    _setTargetLocation(owner, targetMoved);

	if (owner->movespline->Finalized())
	{
	    static_cast<D*>(this)->MovementInform(owner);
	    if (i_angle == 0.f && !owner->HasInArc(0.01f, i_target.getTarget()))
	        owner->SetInFront(i_target.getTarget());

	    if (!i_targetReached)
	    {
	        i_targetReached = true;
	        static_cast<D*>(this)->_reachTarget(owner);
	    }
	}

	return true;
}

//-----------------------------------------------//
template<class T>
void ChaseMovementGenerator<T>::_reachTarget(T* owner)
{
    if (owner->IsWithinMeleeRange(this->i_target.getTarget()))
        owner->Attack(this->i_target.getTarget(), true);
}

template<>
void ChaseMovementGenerator<Player>::Initialize(Player* owner)
{
    owner->addUnitState(UNIT_STAT_CHASE | UNIT_STAT_CHASE_MOVE);
    _setTargetLocation(owner, true);
}

template<>
void ChaseMovementGenerator<Creature>::Initialize(Creature* owner)
{
    owner->SetWalk(false);
    owner->addUnitState(UNIT_STAT_CHASE | UNIT_STAT_CHASE_MOVE);
    _setTargetLocation(owner, true);
}

template<class T>
void ChaseMovementGenerator<T>::Finalize(T* owner)
{
    owner->clearUnitState(UNIT_STAT_CHASE | UNIT_STAT_CHASE_MOVE);
}

template<class T>
void ChaseMovementGenerator<T>::Reset(T* owner)
{
	Initialize(owner);
}

template<class T>
void ChaseMovementGenerator<T>::MovementInform(T* /*unit*/)
{
}

template<>
void ChaseMovementGenerator<Creature>::MovementInform(Creature* unit)
{
    // Pass back the GUIDLow of the target. If it is pet's owner then PetAI will handle
	if (unit->getAI())
		unit->getAI()->onMovementInform(CHASE_MOTION_TYPE, i_target.getTarget()->GetGUIDLow());
	else if (unit->AI())
        unit->AI()->MovementInform(CHASE_MOTION_TYPE, i_target.getTarget()->GetGUIDLow());
}

//-----------------------------------------------//
template<>
bool FollowMovementGenerator<Creature>::EnableWalking() const
{
    return i_target.isValid() && i_target->IsWalking();
}

template<>
bool FollowMovementGenerator<Player>::EnableWalking() const
{
    return false;
}

template<>
void FollowMovementGenerator<Player>::_updateSpeed(Player* /*owner*/)
{
    // nothing to do for Player
}

template<>
void FollowMovementGenerator<Creature>::_updateSpeed(Creature* owner)
{
    // pet only sync speed with owner
    /// Make sure we are not in the process of a map change (IsInWorld)
    if (!owner->isPet() || !owner->IsInWorld() || !i_target.isValid() || i_target->GetGUID() != owner->GetOwnerGUID())
        return;

    owner->UpdateSpeed(MOVE_RUN, true);
    owner->UpdateSpeed(MOVE_WALK, true);
    owner->UpdateSpeed(MOVE_SWIM, true);
}

template<>
void FollowMovementGenerator<Player>::Initialize(Player* owner)
{
    owner->addUnitState(UNIT_STAT_FOLLOW | UNIT_STAT_FOLLOW_MOVE);
    _updateSpeed(owner);
    _setTargetLocation(owner, true);
}

template<>
void FollowMovementGenerator<Creature>::Initialize(Creature* owner)
{
    owner->addUnitState(UNIT_STAT_FOLLOW | UNIT_STAT_FOLLOW_MOVE);
    _updateSpeed(owner);
    _setTargetLocation(owner, true);
}

template<class T>
void FollowMovementGenerator<T>::Finalize(T* owner)
{
    owner->clearUnitState(UNIT_STAT_FOLLOW | UNIT_STAT_FOLLOW_MOVE);
    _updateSpeed(owner);
}

template<class T>
void FollowMovementGenerator<T>::Reset(T* owner)
{
    DoInitialize(owner);
}

template<class T>
void FollowMovementGenerator<T>::MovementInform(T* /*unit*/)
{
}

template<>
void FollowMovementGenerator<Creature>::MovementInform(Creature* unit)
{
    // Pass back the GUIDLow of the target. If it is pet's owner then PetAI will handle
	if (unit->getAI())
		unit->getAI()->onMovementInform(CHASE_MOTION_TYPE, i_target.getTarget()->GetGUIDLow());
    if (unit->AI())
        unit->AI()->MovementInform(FOLLOW_MOTION_TYPE, i_target.getTarget()->GetGUIDLow());
}

//-----------------------------------------------//
template void TargetedMovementGeneratorMedium<Player, ChaseMovementGenerator<Player> >::_setTargetLocation(Player*, bool);
template void TargetedMovementGeneratorMedium<Player, FollowMovementGenerator<Player> >::_setTargetLocation(Player*, bool);
template void TargetedMovementGeneratorMedium<Creature, ChaseMovementGenerator<Creature> >::_setTargetLocation(Creature*, bool);
template void TargetedMovementGeneratorMedium<Creature, FollowMovementGenerator<Creature> >::_setTargetLocation(Creature*, bool);
template bool TargetedMovementGeneratorMedium<Player, ChaseMovementGenerator<Player> >::Update(Player*, const uint32 &diff);
template bool TargetedMovementGeneratorMedium<Player, FollowMovementGenerator<Player> >::Update(Player*, const uint32 &diff);
template bool TargetedMovementGeneratorMedium<Creature, ChaseMovementGenerator<Creature> >::Update(Creature*, const uint32 &diff);
template bool TargetedMovementGeneratorMedium<Creature, FollowMovementGenerator<Creature> >::Update(Creature*, const uint32 &diff);

template void ChaseMovementGenerator<Player>::_reachTarget(Player*);
template void ChaseMovementGenerator<Creature>::_reachTarget(Creature*);
template void ChaseMovementGenerator<Player>::Finalize(Player*);
template void ChaseMovementGenerator<Creature>::Finalize(Creature*);
template void ChaseMovementGenerator<Player>::Reset(Player*);
template void ChaseMovementGenerator<Creature>::Reset(Creature*);
template void ChaseMovementGenerator<Player>::MovementInform(Player*);

template void FollowMovementGenerator<Player>::Finalize(Player*);
template void FollowMovementGenerator<Creature>::Finalize(Creature*);
template void FollowMovementGenerator<Player>::Reset(Player*);
template void FollowMovementGenerator<Creature>::Reset(Creature*);
template void FollowMovementGenerator<Player>::MovementInform(Player*);
