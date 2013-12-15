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

#ifndef TRINITY_MOTIONMASTER_H
#define TRINITY_MOTIONMASTER_H

#include "Common.h"
#include "Object.h"
#include "Utilities/EventProcessor.h"
#include <vector>

class MovementGenerator;
class Unit;
class PathInfo;

// Creature Entry ID used for waypoints show, visible only for GMs
#define VISUAL_WAYPOINT 1

// values 0 ... MAX_DB_MOTION_TYPE-1 used in DB
enum MovementGeneratorType
{
    IDLE_MOTION_TYPE      = 0,                              // IdleMovementGenerator.h
    RANDOM_MOTION_TYPE    = 1,                              // RandomMovementGenerator.h
    WAYPOINT_MOTION_TYPE  = 2,                              // WaypointMovementGenerator.h
    MAX_DB_MOTION_TYPE    = 3,                              // *** this and below motion types can't be set in DB.
    ANIMAL_RANDOM_MOTION_TYPE = MAX_DB_MOTION_TYPE,         // AnimalRandomMovementGenerator.h
    CONFUSED_MOTION_TYPE  = 4,                              // ConfusedMovementGenerator.h
    CHASE_MOTION_TYPE     = 5,                              // TargetedMovementGenerator.h
    HOME_MOTION_TYPE      = 6,                              // HomeMovementGenerator.h
    FLIGHT_MOTION_TYPE    = 7,                              // WaypointMovementGenerator.h
    POINT_MOTION_TYPE     = 8,                              // PointMovementGenerator.h
    FLEEING_MOTION_TYPE   = 9,                              // FleeingMovementGenerator.h
    DISTRACT_MOTION_TYPE  = 10,                             // IdleMovementGenerator.h
    ASSISTANCE_MOTION_TYPE= 11,                             // PointMovementGenerator.h (first part of flee for assistance)
    ASSISTANCE_DISTRACT_MOTION_TYPE = 12,                   // IdleMovementGenerator.h (second part of flee for assistance)
    TIMED_FLEEING_MOTION_TYPE = 13,                         // FleeingMovementGenerator.h (alt.second part of flee for assistance)
    FOLLOW_MOTION_TYPE    = 14,
    ROTATE_MOTION_TYPE    = 15,
    EFFECT_MOTION_TYPE    = 16,
    NULL_MOTION_TYPE      = 17
};

enum MovementSlot
{
    MOTION_SLOT_IDLE,
    MOTION_SLOT_ACTIVE,
    MOTION_SLOT_CONTROLLED,
    MAX_MOTION_SLOT,
};

enum MMCleanFlag
{
    MMCF_NONE   = 0,
    MMCF_UPDATE = 1, // Clear or Expire called from update
    MMCF_RESET  = 2  // Flag if need top()->Reset()
};

enum RotateDirection
{
    ROTATE_DIRECTION_LEFT,
    ROTATE_DIRECTION_RIGHT
};

class MotionMaster //: private std::stack<MovementGenerator *>
{
    private:
        //typedef std::stack<MovementGenerator *> Impl;
        typedef MovementGenerator* _Ty;

        void pop()
        {
            if (empty())
                return;

            Impl[_top] = NULL;
            while (!empty() && !top())
                --_top;
        }
        void push(_Ty _Val) { ++_top; Impl[_top] = _Val; }

        bool needInitTop() const
        {
            if (empty())
                return false;

            return _needInit[_top]; 
        }
        void InitTop();
    public:

        explicit MotionMaster(Unit* unit) : _expList(NULL), _top(-1), _owner(unit), _cleanFlag(MMCF_NONE)
        {
            for (uint8 i = 0; i < MAX_MOTION_SLOT; ++i)
            {
                Impl[i] = NULL;
                _needInit[i] = true;
            }
        }
        ~MotionMaster();

        void Initialize();
        void InitDefault();

        bool empty() const { return (_top < 0); }
        int size() const { return _top + 1; }
        _Ty top() const 
        { 
            ASSERT(!empty());
            return Impl[_top]; 
        }
        _Ty GetMotionSlot(int slot) const 
        { 
            ASSERT(slot >= 0);
            return Impl[slot]; 
        }

        void DirectDelete(_Ty curr);
        void DelayedDelete(_Ty curr);

        bool IsReachable();

        void UpdateMotion(uint32 diff);
        void Clear(bool reset = true)
        {
            if (_cleanFlag & MMCF_UPDATE)
            {
                if (reset)
                    _cleanFlag |= MMCF_RESET;
                else
                    _cleanFlag &= ~MMCF_RESET;
                DelayedClean();
            }
            else
                DirectClean(reset);
        }
        void MovementExpired(bool reset = true)
        {
            if (_cleanFlag & MMCF_UPDATE)
            {
                if (reset)
                    _cleanFlag |= MMCF_RESET;
                else
                    _cleanFlag &= ~MMCF_RESET;
                DelayedExpire();
            }
            else
                DirectExpire(reset);
        }

        void MoveIdle(MovementSlot slot = MOTION_SLOT_ACTIVE);
        void MoveTargetedHome();
        void MoveRandom(float spawndist = 0.0f);
        void MoveFollow(Unit* target, float dist, float angle, MovementSlot slot = MOTION_SLOT_ACTIVE);
        void MoveChase(Unit* target, float dist = 0.0f, float angle = 0.0f, bool strictDist = false);
        void MoveConfused();
        void MoveFleeing(Unit* enemy, uint32 time = 0);
        void MovePoint(uint32 id, Unit* target);
        void MovePoint(uint32 id, const Position &pos, bool usePathfinding = true, uint32 delay = 0)
            { MovePoint(id, pos.m_positionX, pos.m_positionY, pos.m_positionZ, usePathfinding, delay); }
        void MovePoint(uint32 id, float x, float y, float z, bool usePathfinding = true, uint32 delay = 0);

        void MoveCharge(float x, float y, float z, uint32 id, float speed = 42.0f);
        void MoveCharge(PathInfo const& path);
        void MoveFall(uint32 id = 0);

        void MoveSeekAssistance(float x, float y, float z);
        void MoveSeekAssistanceDistract(uint32 timer);
        void MoveTaxiFlight(uint32 path, uint32 pathnode);
        void MoveDistract(uint32 time);
        void MovePath(uint32 path_id, bool repeatable);
        void MoveRotate(uint32 time, RotateDirection direction);

        MovementGeneratorType GetCurrentMovementGeneratorType() const;
        MovementGeneratorType GetMotionSlotType(int slot) const;

        void propagateSpeedChange();

        bool GetDestination(float &x, float &y, float &z);

        // Event handler
        EventProcessor m_Events;

    private:
        void Mutate(MovementGenerator *m, MovementSlot slot);                  // use Move* functions instead

        void DirectClean(bool reset);
        void DelayedClean();

        void DirectExpire(bool reset);
        void DelayedExpire();

        typedef std::vector<_Ty> ExpireList;
        ExpireList* _expList;
        _Ty Impl[MAX_MOTION_SLOT];
        int _top;
        Unit* _owner;
        bool _needInit[MAX_MOTION_SLOT];
        uint8 _cleanFlag;
};

class MotionDelayEvent : public BasicEvent
{
    public:
	MotionDelayEvent(Unit* owner, uint32 id, float x, float y, float z, bool usePathfinding)
    {
		BasicEvent();
		m_owner = owner;
		m_x = x;
		m_y = y;
		m_z = z;
		m_id = id;
		m_usePathfinding = usePathfinding;
    }
        bool Execute(uint64 e_time, uint32 p_time);

    private:
        Unit* m_owner;
        float m_x, m_y, m_z;
        uint32 m_id;
        bool m_usePathfinding;
};

#endif

