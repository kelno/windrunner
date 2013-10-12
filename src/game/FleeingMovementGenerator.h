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
#include "DestinationHolder.h"
#include "Traveller.h"
#include "MapManager.h"

template<class T>
class FleeingMovementGenerator
: public MovementGeneratorMedium< T, FleeingMovementGenerator<T> >
{
    public:
        FleeingMovementGenerator(uint64 fright) : i_frightGUID(fright), i_nextCheckTime(0) {}

        void Initialize(T &);
        void Finalize(T &);
        void Reset(T &);
        bool Update(T &, const uint32 &);
        bool GetDestination(float &x, float &y, float &z) const;

        MovementGeneratorType GetMovementGeneratorType() { return FLEEING_MOTION_TYPE; }

    private:
        void _setTargetLocation(T &owner);
        bool _getPoint(T &owner, float &x, float &y, float &z);
        bool _setMoveData(T &owner);
        void _Init(T &);

        bool is_water_ok   :1;
        bool is_land_ok    :1;
        bool i_only_forward:1;

        float i_caster_x;
        float i_caster_y;
        float i_caster_z;
        float i_last_distance_from_caster;
        float i_to_distance_from_caster;
        float i_cur_angle;
        TimeTracker i_nextCheckTime;
        uint64 i_frightGUID;

        DestinationHolder< Traveller<T> > i_destinationHolder;
};
#endif

