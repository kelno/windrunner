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

#ifndef TRINITY_FLEEINGMOVEMENTGENERATOR_H
#define TRINITY_FLEEINGMOVEMENTGENERATOR_H

#include "MovementGenerator.h"

template<class T>
class FleeingMovementGenerator
: public MovementGeneratorMedium< T, FleeingMovementGenerator<T> >
{
    public:
        FleeingMovementGenerator(uint64 fright) : i_frightGUID(fright), i_nextCheckTime(0) {}

        void Initialize(T*);
        void Finalize(T*);
        void Reset(T*);
        bool Update(T*, const uint32 &);

        MovementGeneratorType GetMovementGeneratorType() { return FLEEING_MOTION_TYPE; }

    private:
        void _setTargetLocation(T*);
        void _getPoint(T*, float &x, float &y, float &z);

        TimeTracker i_nextCheckTime;
        uint64 i_frightGUID;
};

class TimedFleeingMovementGenerator : public FleeingMovementGenerator<Creature>
{
    public:
        TimedFleeingMovementGenerator(uint64 fright, uint32 time) :
            FleeingMovementGenerator<Creature>(fright),
            i_totalFleeTime(time) {}

        MovementGeneratorType GetMovementGeneratorType() { return TIMED_FLEEING_MOTION_TYPE; }
        bool Update(Unit*, uint32);
        void Finalize(Unit*);

    private:
        TimeTracker i_totalFleeTime;
};

#endif

