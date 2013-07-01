/*
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * Copyright (C) 2008-2009 Trinity <http://www.trinitycore.org/>
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

#include "MotionMaster.h"
#include "CreatureAISelector.h"
#include "Creature.h"

#include "ConfusedMovementGenerator.h"
#include "FleeingMovementGenerator.h"
#include "HomeMovementGenerator.h"
#include "IdleMovementGenerator.h"
#include "PointMovementGenerator.h"
#include "TargetedMovementGenerator.h"
#include "WaypointMovementGenerator.h"
#include "RandomMovementGenerator.h"
#include "MoveSpline.h"
#include "MoveSplineInit.h"
#include "World.h"

#include <cassert>

inline bool isStatic(MovementGenerator *mv)
{
    return (mv == &si_idleMovement);
}

void MotionMaster::Initialize()
{
	// clear ALL movement generators (including default)
	while (!empty())
	{
	    MovementGenerator *curr = top();
	    pop();
	    if (curr)
	        DirectDelete(curr);
	}

	InitDefault();
}

// set new default movement generator
void MotionMaster::InitDefault()
{
    if (_owner->GetTypeId() == TYPEID_UNIT)
    {
        MovementGenerator* movement = FactorySelector::selectMovementGenerator(_owner->ToCreature());
        Mutate(movement == NULL ? &si_idleMovement : movement, MOTION_SLOT_IDLE);
    }
    else
    {
        Mutate(&si_idleMovement, MOTION_SLOT_IDLE);
    }
}

MotionMaster::~MotionMaster()
{
	// clear ALL movement generators (including default)
	while (!empty())
	{
	    MovementGenerator *curr = top();
	    pop();
	    if (curr) DirectDelete(curr);
	}
}

void MotionMaster::UpdateMotion(uint32 diff)
{
	if (!_owner)
	    return;

	if (_owner->hasUnitState(UNIT_STAT_ROOT | UNIT_STAT_STUNNED)) // what about UNIT_STATE_DISTRACTED? Why is this not included?
	    return;

	ASSERT(!empty());

	_cleanFlag |= MMCF_UPDATE;
	if (!top()->Update(_owner, diff))
	{
	    _cleanFlag &= ~MMCF_UPDATE;
	    MovementExpired();
	}
	else
	    _cleanFlag &= ~MMCF_UPDATE;

	if (_expList)
	{
	    for (size_t i = 0; i < _expList->size(); ++i)
	    {
	        MovementGenerator* mg = (*_expList)[i];
	        DirectDelete(mg);
	    }

	    delete _expList;
	    _expList = NULL;

	    if (empty())
	        Initialize();
	    else if (needInitTop())
	        InitTop();
	    else if (_cleanFlag & MMCF_RESET)
	        top()->Reset(_owner);

	    _cleanFlag &= ~MMCF_RESET;
    }
}

void MotionMaster::DirectClean(bool reset)
{
	while (size() > 1)
	{
	    MovementGenerator *curr = top();
	    pop();
	    if (curr) DirectDelete(curr);
	}

	if (needInitTop())
	    InitTop();
    else if (reset)
	    top()->Reset(_owner);
}

void MotionMaster::DelayedClean()
{
    while(size() > 1)
    {
        MovementGenerator *curr = top();
        pop();
        if(curr)
        	DelayedDelete(curr);
    }
}

void MotionMaster::DirectExpire(bool reset)
{
	if (size() > 1)
	{
	    MovementGenerator *curr = top();
	    pop();
	    DirectDelete(curr);
    }

    while (!top())
	    --_top;

	if (empty())
	    Initialize();
	else if (needInitTop())
	    InitTop();
	else if (reset)
	    top()->Reset(_owner);
}

void MotionMaster::DelayedExpire()
{
	if (size() > 1)
	{
	    MovementGenerator *curr = top();
	    pop();
	    DelayedDelete(curr);
	}

	while (!top())
	    --_top;
}

void MotionMaster::MoveIdle(MovementSlot slot)
{
	//! Should be preceded by MovementExpired or Clear if there's an overlying movementgenerator active
	if (empty() || !isStatic(top()))
	    Mutate(&si_idleMovement, MOTION_SLOT_IDLE);
}

void MotionMaster::MoveRandom(float spawndist)
{
    if(_owner->GetTypeId()==TYPEID_UNIT)
    {
        DEBUG_LOG("Creature (GUID: %u) start moving random", _owner->GetGUIDLow() );
        Mutate(new RandomMovementGenerator<Creature>(spawndist), MOTION_SLOT_IDLE);
    }
}

void MotionMaster::MoveTargetedHome()
{
    Clear(false);

    if(_owner->GetTypeId()==TYPEID_UNIT && !(_owner->ToCreature())->GetCharmerOrOwnerGUID())
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) targeted home", _owner->GetEntry(), _owner->GetGUIDLow());
        Mutate(new HomeMovementGenerator<Creature>(), MOTION_SLOT_ACTIVE);
    }
    else if(_owner->GetTypeId()==TYPEID_UNIT && (_owner->ToCreature())->GetCharmerOrOwnerGUID())
    {
        DEBUG_LOG("Pet or controlled creature (Entry: %u GUID: %u) targeting home",
        		_owner->GetEntry(), _owner->GetGUIDLow() );
        Unit *target = _owner->ToCreature()->GetCharmerOrOwner();
        if(target)
        {
        	_owner->addUnitState(UNIT_STAT_FOLLOW);
            DEBUG_LOG("Following %s (GUID: %u)",
                target->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
                target->GetTypeId()==TYPEID_PLAYER ? target->GetGUIDLow() : (target->ToCreature())->GetDBTableGUIDLow() );
            Mutate(new FollowMovementGenerator<Creature>(target,PET_FOLLOW_DIST,PET_FOLLOW_ANGLE), MOTION_SLOT_ACTIVE);
        }
    }
    else
    {
        sLog.outError("Player (GUID: %u) attempt targeted home", _owner->GetGUIDLow() );
    }
}

void MotionMaster::MoveConfused()
{
    if(_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) move confused", _owner->GetGUIDLow() );
        Mutate(new ConfusedMovementGenerator<Player>(), MOTION_SLOT_CONTROLLED);
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) move confused",
        		_owner->GetEntry(), _owner->GetGUIDLow() );
        Mutate(new ConfusedMovementGenerator<Creature>(), MOTION_SLOT_CONTROLLED);
    }
}

void MotionMaster::MoveChase(Unit* target, float dist, float angle)
{
    // ignore movement request if target not exist
    if(!target || target == _owner || _owner->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE))
        return;

    //i_owner->clearUnitState(UNIT_STAT_FOLLOW);
    if(_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) chase to %s (GUID: %u)",
        		_owner->GetGUIDLow(),
            target->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
            target->GetTypeId()==TYPEID_PLAYER ? target->GetGUIDLow() : (target->ToCreature())->GetDBTableGUIDLow() );
        Mutate(new ChaseMovementGenerator<Player>(target, dist, angle), MOTION_SLOT_ACTIVE);
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) chase to %s (GUID: %u)",
        		_owner->GetEntry(), _owner->GetGUIDLow(),
            target->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
            target->GetTypeId()==TYPEID_PLAYER ? target->GetGUIDLow() : (target->ToCreature())->GetDBTableGUIDLow() );
        Mutate(new ChaseMovementGenerator<Creature>(target, dist, angle), MOTION_SLOT_ACTIVE);
    }
}

void MotionMaster::MoveFollow(Unit* target, float dist, float angle, MovementSlot slot)
{
    // ignore movement request if target not exist
    if(!target || target == _owner || _owner->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE))
        return;

    //i_owner->addUnitState(UNIT_STAT_FOLLOW);
    if(_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) follow to %s (GUID: %u)", _owner->GetGUIDLow(),
            target->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
            target->GetTypeId()==TYPEID_PLAYER ? target->GetGUIDLow() : (target->ToCreature())->GetDBTableGUIDLow() );
        Mutate(new FollowMovementGenerator<Player>(target, dist, angle), slot);
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) follow to %s (GUID: %u)",
        		_owner->GetEntry(), _owner->GetGUIDLow(),
            target->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
            target->GetTypeId()==TYPEID_PLAYER ? target->GetGUIDLow() : (target->ToCreature())->GetDBTableGUIDLow() );
        Mutate(new FollowMovementGenerator<Creature>(target, dist, angle), slot);
    }
}

void MotionMaster::MovePoint(uint32 id, float x, float y, float z, bool usePathfinding)
{
    if(_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) targeted point (Id: %u X: %f Y: %f Z: %f)", _owner->GetGUIDLow(), id, x, y, z );
        Mutate(new PointMovementGenerator<Player>(id,x,y,z,usePathfinding), MOTION_SLOT_ACTIVE);
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) targeted point (ID: %u X: %f Y: %f Z: %f)",
        		_owner->GetEntry(), _owner->GetGUIDLow(), id, x, y, z );
        Mutate(new PointMovementGenerator<Creature>(id,x,y,z,usePathfinding), MOTION_SLOT_ACTIVE);
    }
}

void MotionMaster::MoveFall(uint32 id /*=0*/)
{
    // use larger distance for vmap height search than in most other cases
    float tz = _owner->GetMap()->GetHeight(_owner->GetPositionX(), _owner->GetPositionY(), _owner->GetPositionZ(), true);
    if (tz <= INVALID_HEIGHT)
    {
    	DEBUG_LOG("MotionMaster::MoveFall: unable retrive a proper height at map %u (x: %f, y: %f, z: %f).",
    			_owner->GetMap()->GetId(), _owner->GetPositionX(), _owner->GetPositionY(), _owner->GetPositionZ());
        return;
    }

    // Abort too if the ground is very near
    if (fabs(_owner->GetPositionZ() - tz) < 0.1f)
        return;

    if (_owner->GetTypeId() == TYPEID_PLAYER)
    {
    	_owner->AddUnitMovementFlag(MOVEMENTFLAG_FALLING);
    	_owner->m_movementInfo.SetFallTime(0);
    }

    Movement::MoveSplineInit init(_owner);
    init.MoveTo(_owner->GetPositionX(), _owner->GetPositionY(), tz, false);
    init.SetFall();
    init.Launch();
    Mutate(new EffectMovementGenerator(id), MOTION_SLOT_CONTROLLED);
}

void
MotionMaster::MoveCharge(float x, float y, float z, uint32 id, float speed)
{
    if(Impl[MOTION_SLOT_CONTROLLED] && Impl[MOTION_SLOT_CONTROLLED]->GetMovementGeneratorType() != DISTRACT_MOTION_TYPE)
        return;

    if(_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) charge point (X: %f Y: %f Z: %f)", _owner->GetGUIDLow(), x, y, z );
        if (sWorld.getConfig(CONFIG_CHARGEMOVEGEN))
            Mutate(new PointMovementGenerator<Player>(id, x, y, z, true, speed), MOTION_SLOT_CONTROLLED);
        else
            Mutate(new PointMovementGenerator<Player>(id, x, y, z, false, speed), MOTION_SLOT_CONTROLLED);
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) charge point (X: %f Y: %f Z: %f)",
        		_owner->GetEntry(), _owner->GetGUIDLow(), x, y, z );
        if (sWorld.getConfig(CONFIG_CHARGEMOVEGEN))
            Mutate(new PointMovementGenerator<Creature>(id, x, y, z, true, speed), MOTION_SLOT_CONTROLLED);
        else
            Mutate(new PointMovementGenerator<Creature>(id, x, y, z, false, speed), MOTION_SLOT_CONTROLLED);
    }
}

void MotionMaster::MoveCharge(PathInfo const& path)
{
	G3D::Vector3 dest = path.getActualEndPosition();

    MoveCharge(dest.x, dest.y, dest.z, 1005, 42.0f);

    Movement::MoveSplineInit init(_owner);
    init.MovebyPath(path.getFullPath());
    init.SetVelocity(SPEED_CHARGE);
    init.Launch();
}

void MotionMaster::MoveSeekAssistance(float x, float y, float z)
{
    if (_owner->GetTypeId() == TYPEID_PLAYER)
    {
    	sLog.outError("Player (GUID: %u) attempt to seek assistance", _owner->GetGUIDLow());
    }
    else
    {
    	DEBUG_LOG("Creature (Entry: %u GUID: %u) seek assistance (X: %f Y: %f Z: %f)",
    			_owner->GetEntry(), _owner->GetGUIDLow(), x, y, z);
    	_owner->AttackStop();
    	_owner->ToCreature()->SetReactState(REACT_PASSIVE);
        Mutate(new AssistanceMovementGenerator(x, y, z), MOTION_SLOT_ACTIVE);
    }
}

void MotionMaster::MoveSeekAssistanceDistract(uint32 time)
{
    if (_owner->GetTypeId() == TYPEID_PLAYER)
    {
    	sLog.outError("Player (GUID: %u) attempt to call distract after assistance", _owner->GetGUIDLow());
    }
    else
    {
    	DEBUG_LOG("Creature (Entry: %u GUID: %u) is distracted after assistance call (Time: %u)",
    			_owner->GetEntry(), _owner->GetGUIDLow(), time);
        Mutate(new AssistanceDistractMovementGenerator(time), MOTION_SLOT_ACTIVE);
    }
}

void
MotionMaster::MoveFleeing(Unit* enemy, uint32 time)
{
    if(!enemy)
        return;

    if(_owner->HasAuraType(SPELL_AURA_PREVENTS_FLEEING))
        return;

    if(_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) flee from %s (GUID: %u)", _owner->GetGUIDLow(),
            enemy->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
            enemy->GetTypeId()==TYPEID_PLAYER ? enemy->GetGUIDLow() : (enemy->ToCreature())->GetDBTableGUIDLow() );
        Mutate(new FleeingMovementGenerator<Player>(enemy->GetGUID()), MOTION_SLOT_CONTROLLED);
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) flee from %s (GUID: %u)",
        		_owner->GetEntry(), _owner->GetGUIDLow(),
            enemy->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
            enemy->GetTypeId()==TYPEID_PLAYER ? enemy->GetGUIDLow() : (enemy->ToCreature())->GetDBTableGUIDLow() );
        if (time)
        	Mutate(new TimedFleeingMovementGenerator(enemy->GetGUID(), time), MOTION_SLOT_CONTROLLED);
        else
            Mutate(new FleeingMovementGenerator<Creature>(enemy->GetGUID()), MOTION_SLOT_CONTROLLED);
    }
}

void
MotionMaster::MoveTaxiFlight(uint32 path, uint32 pathnode)
{
    if(_owner->GetTypeId()==TYPEID_PLAYER)
    {
    	if (path < sTaxiPathNodesByPath.size())
    	{
            DEBUG_LOG("Player (GUID: %u) taxi to (Path %u node %u)", _owner->GetGUIDLow(), path, pathnode);
            FlightPathMovementGenerator* mgen = new FlightPathMovementGenerator(sTaxiPathNodesByPath[path], pathnode);
            Mutate(mgen, MOTION_SLOT_CONTROLLED);
    	}
    	else
    	{
    		sLog.outError("%s attempt taxi to (not existed Path %u node %u)",
    				_owner->GetName(), path, pathnode);
    	}
    }
    else
    {
        sLog.outError("Creature (Entry: %u GUID: %u) attempt taxi to (Path %u node %u)",
        		_owner->GetEntry(), _owner->GetGUIDLow(), path, pathnode );
    }
}

void
MotionMaster::MoveDistract(uint32 timer)
{
    if(Impl[MOTION_SLOT_CONTROLLED])
        return;

    if(_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) distracted (timer: %u)", _owner->GetGUIDLow(), timer);
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) (timer: %u)",
        		_owner->GetEntry(), _owner->GetGUIDLow(), timer);
    }

    DistractMovementGenerator* mgen = new DistractMovementGenerator(timer);
    Mutate(mgen, MOTION_SLOT_CONTROLLED);
}

void MotionMaster::Mutate(MovementGenerator *m, MovementSlot slot)
{
	if (MovementGenerator *curr = Impl[slot])
	{
	    Impl[slot] = NULL; // in case a new one is generated in this slot during directdelete
	    if (_top == slot && (_cleanFlag & MMCF_UPDATE))
	        DelayedDelete(curr);
	    else
	        DirectDelete(curr);
	}
	else if (_top < slot)
	{
	    _top = slot;
	}

	Impl[slot] = m;
	if (_top > slot)
	    _needInit[slot] = true;
    else
	{
	    _needInit[slot] = false;
	    m->Initialize(_owner);
	}
}

void MotionMaster::MovePath(uint32 path_id, bool repeatable)
{
    if(!path_id)
        return;
    //We set waypoint movement as new default movement generator
    // clear ALL movement generators (including default)
    /*while(!empty())
    {
        MovementGenerator *curr = top();
        curr->Finalize(*i_owner);
        pop();
        if( !isStatic( curr ) )
            delete curr;
    }*/

    //i_owner->GetTypeId()==TYPEID_PLAYER ?
        //Mutate(new WaypointMovementGenerator<Player>(path_id, repeatable)):
    Mutate(new WaypointMovementGenerator<Creature>(path_id, repeatable), MOTION_SLOT_IDLE);

    DEBUG_LOG("%s (GUID: %u) start moving over path(Id:%u, repeatable: %s)",
    		_owner->GetTypeId()==TYPEID_PLAYER ? "Player" : "Creature",
    				_owner->GetGUIDLow(), path_id, repeatable ? "YES" : "NO" );
}

void MotionMaster::MoveRotate(uint32 time, RotateDirection direction) 
{
    if(!time) 
        return;
        
    Mutate(new RotateMovementGenerator(time, direction), MOTION_SLOT_ACTIVE); 
}

void MotionMaster::propagateSpeedChange()
{
	/*Impl::container_type::iterator it = Impl::c.begin();
	for (; it != end(); ++it)
	{
	    (*it)->unitSpeedChanged();
	}*/
	for (int i = 0; i <= _top; ++i)
	{
	    if (Impl[i])
	        Impl[i]->unitSpeedChanged();
	}
}

MovementGeneratorType MotionMaster::GetCurrentMovementGeneratorType() const
{
	if (empty())
	   return IDLE_MOTION_TYPE;

    return top()->GetMovementGeneratorType();
}

MovementGeneratorType MotionMaster::GetMotionSlotType(int slot) const
{
	if (!Impl[slot])
	    return NULL_MOTION_TYPE;
	else
	    return Impl[slot]->GetMovementGeneratorType();
}

void MotionMaster::InitTop()
{
	top()->Initialize(_owner);
	_needInit[_top] = false;
}

void MotionMaster::DirectDelete(_Ty curr)
{
	if (isStatic(curr))
	    return;
	curr->Finalize(_owner);
	delete curr;
}

void MotionMaster::DelayedDelete(_Ty curr)
{
    sLog.outError("CRASH ALARM! Unit (Entry %u) is trying to delete its updating MG (Type %u)!", _owner->GetEntry(), curr->GetMovementGeneratorType());
    if (isStatic(curr))
        return;
    if (!_expList)
        _expList = new ExpireList();
    _expList->push_back(curr);
}

bool MotionMaster::GetDestination(float &x, float &y, float &z)
{
	if (_owner->movespline->Finalized())
	    return false;

	G3D::Vector3 const& dest = _owner->movespline->FinalDestination();
	x = dest.x;
	y = dest.y;
	z = dest.z;
	return true;
}

bool MotionMaster::IsReachable()
{
	if (!empty())
	    return top()->IsReachable();

	return false;
}
