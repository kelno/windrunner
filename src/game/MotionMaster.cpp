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
#include "Traveller.h"

#include "ConfusedMovementGenerator.h"
#include "FleeingMovementGenerator.h"
#include "HomeMovementGenerator.h"
#include "IdleMovementGenerator.h"
#include "PointMovementGenerator.h"
#include "TargetedMovementGenerator.h"
#include "WaypointMovementGenerator.h"
#include "RandomMovementGenerator.h"
#include "ChargeMovementGenerator.h"
#include "World.h"

#include <cassert>

inline bool isStatic(MovementGenerator *mv)
{
    return (mv == &si_idleMovement);
}

void
MotionMaster::Initialize()
{
    // clear ALL movement generators (including default)
    while(!empty())
    {
        MovementGenerator *curr = top();
        pop();
        if(curr) DirectDelete(curr);
    }

    // set new default movement generator
    if(i_owner->GetTypeId() == TYPEID_UNIT)
    {
        MovementGenerator* movement = FactorySelector::selectMovementGenerator(i_owner->ToCreature());
        push(  movement == NULL ? &si_idleMovement : movement );
        InitTop();
    }
    else
    {
        push(&si_idleMovement);
        needInit[MOTION_SLOT_IDLE] = false;
    }
}

MotionMaster::~MotionMaster()
{
    // clear ALL movement generators (including default)
    while(!empty())
    {
        MovementGenerator *curr = top();
        pop();
        if(curr) DirectDelete(curr);
    }
}

void
MotionMaster::UpdateMotion(uint32 diff)
{
    if( i_owner->hasUnitState(UNIT_STAT_ROOT | UNIT_STAT_STUNNED) ) {
        // cancel charge if owner is not dead
        if (!i_owner->hasUnitState(UNIT_STAT_DIED) && top()->GetMovementGeneratorType() == CHARGE_MOTION_TYPE)
            DirectExpire(true);
        
        return;
    }
    assert( !empty() );
    m_cleanFlag |= MMCF_UPDATE;
    if (!top()->Update(*i_owner, diff))
    {
        m_cleanFlag &= ~MMCF_UPDATE;
        MovementExpired();
    }
    else
        m_cleanFlag &= ~MMCF_UPDATE;

    if (m_expList)
    {
        for (int i = 0; i < m_expList->size(); ++i)
        {
            MovementGenerator* mg = (*m_expList)[i];
            DirectDelete(mg);
        }

        delete m_expList;
        m_expList = NULL;

        if(empty())
            Initialize();
        else if(needInitTop())
            InitTop();
        else if (m_cleanFlag & MMCF_RESET)
            top()->Reset(*i_owner);

        m_cleanFlag &= ~MMCF_RESET;
    }
}

void
MotionMaster::DirectClean(bool reset)
{
    while(size() > 1)
    {
        MovementGenerator *curr = top();
        pop();
        if(curr) DirectDelete(curr);
    }

    if(needInitTop())
        InitTop();
    else if(reset)
        top()->Reset(*i_owner);
}

void
MotionMaster::DelayedClean()
{
    while(size() > 1)
    {
        MovementGenerator *curr = top();
        pop();
        if(curr) DelayedDelete(curr);
    }
}

void
MotionMaster::DirectExpire(bool reset)
{
    if(size() > 1 )
    {
        MovementGenerator *curr = top();
        pop();
        DirectDelete(curr);
    }

    while(!top())
        --i_top;

    if(empty())
        Initialize();
    else if(needInitTop())
        InitTop();
    else if(reset)
        top()->Reset(*i_owner);
}

void
MotionMaster::DelayedExpire()
{
    if(size() > 1 )
    {
        MovementGenerator *curr = top();
        pop();
        DelayedDelete(curr);
    }

    while(!top())
        --i_top;
}

void MotionMaster::MoveIdle(MovementSlot slot)
{
    //if( empty() || !isStatic( top() ) )
    //    push( &si_idleMovement );
    if(!isStatic(Impl[slot]))
        Mutate(&si_idleMovement, slot);
}

void
MotionMaster::MoveRandom(float spawndist)
{
    if(i_owner->GetTypeId()==TYPEID_UNIT)
    {
        DEBUG_LOG("Creature (GUID: %u) start moving random", i_owner->GetGUIDLow() );
        Mutate(new RandomMovementGenerator<Creature>(spawndist), MOTION_SLOT_IDLE);
    }
}

void
MotionMaster::MoveTargetedHome()
{
    //if(i_owner->hasUnitState(UNIT_STAT_FLEEING))
    //    return;

    Clear(false);

    if(i_owner->GetTypeId()==TYPEID_UNIT && !(i_owner->ToCreature())->GetCharmerOrOwnerGUID())
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) targeted home", i_owner->GetEntry(), i_owner->GetGUIDLow());
        Mutate(new HomeMovementGenerator<Creature>(), MOTION_SLOT_ACTIVE);
    }
    else if(i_owner->GetTypeId()==TYPEID_UNIT && (i_owner->ToCreature())->GetCharmerOrOwnerGUID())
    {
        DEBUG_LOG("Pet or controlled creature (Entry: %u GUID: %u) targeting home",
            i_owner->GetEntry(), i_owner->GetGUIDLow() );
        Unit *target = (i_owner->ToCreature())->GetCharmerOrOwner();
        if(target)
        {
            i_owner->addUnitState(UNIT_STAT_FOLLOW);
            DEBUG_LOG("Following %s (GUID: %u)",
                target->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
                target->GetTypeId()==TYPEID_PLAYER ? target->GetGUIDLow() : (target->ToCreature())->GetDBTableGUIDLow() );
            Mutate(new TargetedMovementGenerator<Creature>(*target,PET_FOLLOW_DIST,PET_FOLLOW_ANGLE), MOTION_SLOT_ACTIVE);
        }
    }
    else
    {
        sLog.outError("Player (GUID: %u) attempt targeted home", i_owner->GetGUIDLow() );
    }
}

void
MotionMaster::MoveConfused()
{
    if(i_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) move confused", i_owner->GetGUIDLow() );
        Mutate(new ConfusedMovementGenerator<Player>(), MOTION_SLOT_CONTROLLED);
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) move confused",
            i_owner->GetEntry(), i_owner->GetGUIDLow() );
        Mutate(new ConfusedMovementGenerator<Creature>(), MOTION_SLOT_CONTROLLED);
    }
}

void
MotionMaster::MoveChase(Unit* target, float dist, float angle)
{
    // ignore movement request if target not exist
    if(!target || target == i_owner)
        return;

    if (i_owner->ToCreature() && i_owner->ToCreature()->GetReactState() == REACT_PASSIVE && !i_owner->isPet())
        return;

    i_owner->clearUnitState(UNIT_STAT_FOLLOW);
    if(i_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) chase to %s (GUID: %u)",
            i_owner->GetGUIDLow(),
            target->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
            target->GetTypeId()==TYPEID_PLAYER ? target->GetGUIDLow() : (target->ToCreature())->GetDBTableGUIDLow() );
        Mutate(new TargetedMovementGenerator<Player>(*target,dist,angle), MOTION_SLOT_ACTIVE);
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) chase to %s (GUID: %u)",
            i_owner->GetEntry(), i_owner->GetGUIDLow(),
            target->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
            target->GetTypeId()==TYPEID_PLAYER ? target->GetGUIDLow() : (target->ToCreature())->GetDBTableGUIDLow() );
        Mutate(new TargetedMovementGenerator<Creature>(*target,dist,angle), MOTION_SLOT_ACTIVE);
    }
}

void
MotionMaster::MoveFollow(Unit* target, float dist, float angle, bool onPoint)
{
    // ignore movement request if target not exist
    if(!target || target == i_owner)
        return;

    i_owner->addUnitState(UNIT_STAT_FOLLOW);
    if(i_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) follow to %s (GUID: %u)", i_owner->GetGUIDLow(),
            target->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
            target->GetTypeId()==TYPEID_PLAYER ? target->GetGUIDLow() : (target->ToCreature())->GetDBTableGUIDLow() );
        if(!onPoint)
            Mutate(new TargetedMovementGenerator<Player>(*target,dist,angle), MOTION_SLOT_ACTIVE);
        else
            Mutate(new TargetedMovementGenerator<Player>(*target,true), MOTION_SLOT_ACTIVE);
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) follow to %s (GUID: %u)",
            i_owner->GetEntry(), i_owner->GetGUIDLow(),
            target->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
            target->GetTypeId()==TYPEID_PLAYER ? target->GetGUIDLow() : (target->ToCreature())->GetDBTableGUIDLow() );
        if(!onPoint)
            Mutate(new TargetedMovementGenerator<Creature>(*target,dist,angle), MOTION_SLOT_ACTIVE);
        else
            Mutate(new TargetedMovementGenerator<Creature>(*target,true), MOTION_SLOT_ACTIVE);
    }
}

void 
MotionMaster::MoveFollowOnPoint(Unit* target)
{
    MoveFollow(target, 0, 0, true);
}

void MotionMaster::MovePoint(uint32 id, float x, float y, float z, bool usePathfinding)
{
    if(i_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) targeted point (Id: %u X: %f Y: %f Z: %f)", i_owner->GetGUIDLow(), id, x, y, z );
        Mutate(new PointMovementGenerator<Player>(id,x,y,z,usePathfinding), MOTION_SLOT_ACTIVE);
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) targeted point (ID: %u X: %f Y: %f Z: %f)",
            i_owner->GetEntry(), i_owner->GetGUIDLow(), id, x, y, z );
        Mutate(new PointMovementGenerator<Creature>(id,x,y,z,usePathfinding), MOTION_SLOT_ACTIVE);
    }
}

void MotionMaster::MovePoint(uint32 id, Unit* target)
{
    float x,y,z;
    target->GetPosition(x,y,z);
    MovePoint(id,x,y,z,true);
}

void
MotionMaster::MoveCharge(float x, float y, float z)
{
    if(Impl[MOTION_SLOT_CONTROLLED] && Impl[MOTION_SLOT_CONTROLLED]->GetMovementGeneratorType() != DISTRACT_MOTION_TYPE)
        return;

    i_owner->addUnitState(UNIT_STAT_CHARGING);
    if(i_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) charge point (X: %f Y: %f Z: %f)", i_owner->GetGUIDLow(), x, y, z );
        if (sWorld.getConfig(CONFIG_CHARGEMOVEGEN))
            Mutate(new PointMovementGenerator<Player>(0,x,y,z,true,42.0f), MOTION_SLOT_CONTROLLED);
        else
            Mutate(new PointMovementGenerator<Player>(0,x,y,z,false), MOTION_SLOT_CONTROLLED);
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) charge point (X: %f Y: %f Z: %f)",
            i_owner->GetEntry(), i_owner->GetGUIDLow(), x, y, z );
        if (sWorld.getConfig(CONFIG_CHARGEMOVEGEN))
            Mutate(new PointMovementGenerator<Creature>(0,x,y,z,true,42.0f), MOTION_SLOT_CONTROLLED);
        else
            Mutate(new PointMovementGenerator<Creature>(0,x,y,z,false), MOTION_SLOT_CONTROLLED);
    }
}

void
MotionMaster::MoveCharge(Unit* target, uint32 triggeredSpellId/* = 0*/, uint32 triggeredSpellId2/* = 0*/)
{
    if (!target)
        return;

    if (i_owner->GetTypeId() == TYPEID_PLAYER)
        Mutate(new ChargeMovementGenerator<Player>(target->GetGUID(), triggeredSpellId, triggeredSpellId2), MOTION_SLOT_CONTROLLED);
    else
        Mutate(new ChargeMovementGenerator<Creature>(target->GetGUID(), triggeredSpellId, triggeredSpellId2), MOTION_SLOT_CONTROLLED);
}

void
MotionMaster::MoveFleeing(Unit* enemy)
{
    if(!enemy)
        return;

    if(i_owner->HasAuraType(SPELL_AURA_PREVENTS_FLEEING))
        return;

    if(i_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) flee from %s (GUID: %u)", i_owner->GetGUIDLow(),
            enemy->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
            enemy->GetTypeId()==TYPEID_PLAYER ? enemy->GetGUIDLow() : (enemy->ToCreature())->GetDBTableGUIDLow() );
        Mutate(new FleeingMovementGenerator<Player>(enemy->GetGUID()), MOTION_SLOT_CONTROLLED);
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) flee from %s (GUID: %u)",
            i_owner->GetEntry(), i_owner->GetGUIDLow(),
            enemy->GetTypeId()==TYPEID_PLAYER ? "player" : "creature",
            enemy->GetTypeId()==TYPEID_PLAYER ? enemy->GetGUIDLow() : (enemy->ToCreature())->GetDBTableGUIDLow() );
        Mutate(new FleeingMovementGenerator<Creature>(enemy->GetGUID()), MOTION_SLOT_CONTROLLED);
    }
}

void
MotionMaster::MoveTaxiFlight(uint32 path, uint32 pathnode)
{
    if(i_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) taxi to (Path %u node %u)", i_owner->GetGUIDLow(), path, pathnode);
        FlightPathMovementGenerator* mgen = new FlightPathMovementGenerator(path,pathnode);
        Mutate(mgen, MOTION_SLOT_CONTROLLED);
    }
    else
    {
        sLog.outError("Creature (Entry: %u GUID: %u) attempt taxi to (Path %u node %u)",
            i_owner->GetEntry(), i_owner->GetGUIDLow(), path, pathnode );
    }
}

void
MotionMaster::MoveDistract(uint32 timer)
{
    if(Impl[MOTION_SLOT_CONTROLLED])
        return;

    if(i_owner->GetTypeId()==TYPEID_PLAYER)
    {
        DEBUG_LOG("Player (GUID: %u) distracted (timer: %u)", i_owner->GetGUIDLow(), timer);
    }
    else
    {
        DEBUG_LOG("Creature (Entry: %u GUID: %u) (timer: %u)",
            i_owner->GetEntry(), i_owner->GetGUIDLow(), timer);
    }

    DistractMovementGenerator* mgen = new DistractMovementGenerator(timer);
    Mutate(mgen, MOTION_SLOT_CONTROLLED);
}

void MotionMaster::Mutate(MovementGenerator *m, MovementSlot slot)
{
    if(MovementGenerator *curr = Impl[slot])
    {
        if(i_top == slot && (m_cleanFlag & MMCF_UPDATE))
            DelayedDelete(curr);
        else
            DirectDelete(curr);
    }
    else if(i_top < slot)
    {
        i_top = slot;
    }

    if(i_top > slot)
        needInit[slot] = true;
    else
    {
        m->Initialize(*i_owner);
        needInit[slot] = false;
    }
    Impl[slot] = m;
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
        i_owner->GetTypeId()==TYPEID_PLAYER ? "Player" : "Creature",
        i_owner->GetGUIDLow(), path_id, repeatable ? "YES" : "NO" );
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
    for ( ;it != end(); ++it)
    {
        (*it)->unitSpeedChanged();
    }*/
    for(int i = 0; i <= i_top; ++i)
    {
        if(Impl[i])
            Impl[i]->unitSpeedChanged();
    }
}

MovementGeneratorType MotionMaster::GetCurrentMovementGeneratorType() const
{
   if(empty())
       return IDLE_MOTION_TYPE;

   return top()->GetMovementGeneratorType();
}

MovementGeneratorType MotionMaster::GetMotionSlotType(int slot) const
{
    if(!Impl[slot])
        return NULL_MOTION_TYPE;
    else
        return Impl[slot]->GetMovementGeneratorType();
}

void MotionMaster::InitTop()
{
    top()->Initialize(*i_owner);
    needInit[i_top] = false;
}

void MotionMaster::DirectDelete(_Ty curr)
{
    if(isStatic(curr))
        return;
    curr->Finalize(*i_owner);
    delete curr;
}

void MotionMaster::DelayedDelete(_Ty curr)
{
    sLog.outError("CRASH ALARM! Unit (Entry %u) is trying to delete its updating MG (Type %u)!", i_owner->GetEntry(), curr->GetMovementGeneratorType());
    if(isStatic(curr))
        return;
    if(!m_expList)
        m_expList = new ExpireList();
    m_expList->push_back(curr);
}

bool MotionMaster::GetDestination(float &x, float &y, float &z)
{
   if(empty())
       return false;

   return top()->GetDestination(x,y,z);
}

