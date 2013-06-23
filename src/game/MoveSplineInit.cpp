/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "MovementPacketBuilder.h"
#include "Unit.h"
#include "WorldPacket.h"
#include "Opcodes.h"

namespace Movement
{
    UnitMoveType SelectSpeedType(uint32 moveFlags)
    {
        if (moveFlags & MOVEMENTFLAG_FLYING)
        {
            if (moveFlags & MOVEMENTFLAG_BACKWARD /*&& speed_obj.flight >= speed_obj.flight_back*/)
                return MOVE_FLIGHT_BACK;
            else
                return MOVE_FLIGHT;
        }
        else if (moveFlags & MOVEMENTFLAG_SWIMMING)
        {
            if (moveFlags & MOVEMENTFLAG_BACKWARD /*&& speed_obj.swim >= speed_obj.swim_back*/)
                return MOVE_SWIM_BACK;
            else
                return MOVE_SWIM;
        }
        else if (moveFlags & MOVEMENTFLAG_WALK_MODE)
        {
            //if (speed_obj.run > speed_obj.walk)
            return MOVE_WALK;
        }
        else if (moveFlags & MOVEMENTFLAG_BACKWARD /*&& speed_obj.run >= speed_obj.run_back*/)
            return MOVE_RUN_BACK;

        // Flying creatures use MOVEMENTFLAG_CAN_FLY or MOVEMENTFLAG_DISABLE_GRAVITY
        // Run speed is their default flight speed.
        return MOVE_RUN;
    }

    int32 MoveSplineInit::Launch()
    {
        MoveSpline& move_spline = *unit->movespline;

        bool transport = false;
        Location real_position(unit->GetPositionX(), unit->GetPositionY(), unit->GetPositionZ(), unit->GetOrientation());

        // there is a big chance that current position is unknown if current state is not finalized, need compute it
        // this also allows CalculatePath spline position and update map position in much greater intervals
        // Don't compute for transport movement if the unit is in a motion between two transports
        if (!move_spline.Finalized() && move_spline.onTransport == transport)
            real_position = move_spline.ComputePosition();

        // should i do the things that user should do? - no.
        if (args.path.empty())
            return 0;

        // corrent first vertex
        args.path[0] = real_position;
        args.initialOrientation = real_position.orientation;
        move_spline.onTransport = transport;

        uint32 moveFlags = unit->GetUnitMovementFlags();
        moveFlags |= (MOVEMENTFLAG_SPLINE_ENABLED|MOVEMENTFLAG_FORWARD);

        if (moveFlags & MOVEMENTFLAG_ROOT)
            moveFlags &= ~MOVEMENTFLAG_MOVING;

        if (!args.HasVelocity)
        {
            // If spline is initialized with SetWalk method it only means we need to select
            // walk move speed for it but not add walk flag to unit
            uint32 moveFlagsForSpeed = moveFlags;
            if (args.flags.walkmode)
                moveFlagsForSpeed |= MOVEMENTFLAG_WALK_MODE;
            else
                moveFlagsForSpeed &= ~MOVEMENTFLAG_WALK_MODE;

            args.velocity = unit->GetSpeed(SelectSpeedType(moveFlagsForSpeed));
        }

        if (!args.Validate(unit))
            return 0;

        unit->SetUnitMovementFlags((MovementFlags)moveFlags);
        move_spline.Initialize(args);

        WorldPacket data(SMSG_MONSTER_MOVE, 64);
        data.append(unit->GetPackGUID());

        PacketBuilder::WriteMonsterMove(move_spline, data);
        unit->SendMessageToSet(&data, true);

        return move_spline.Duration();
    }

    MoveSplineInit::MoveSplineInit(Unit* m) : unit(m)
    {
        args.splineId = splineIdGen.NewId();
        // Elevators also use MOVEMENTFLAG_ONTRANSPORT but we do not keep track of their position changes

        args.TransformForTransport = unit->HasUnitMovementFlag(MOVEMENTFLAG_ONTRANSPORT);

        // mix existing state into new
        args.flags.walkmode = unit->HasUnitMovementFlag(MOVEMENTFLAG_WALK_MODE);
        args.flags.flying = unit->HasUnitMovementFlag((MovementFlags)(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_LEVITATING));
    }

    void MoveSplineInit::SetFacing(const Unit* target)
    {
        args.flags.EnableFacingTarget();
        args.facing.target = target->GetGUID();
    }

    void MoveSplineInit::SetFacing(float angle)
    {
        args.facing.angle = G3D::wrap(angle, 0.f, (float)G3D::twoPi());
        args.flags.EnableFacingAngle();
    }

    void MoveSplineInit::MoveTo(const Vector3& dest, bool generatePath, bool forceDestination)
    {
        if (generatePath)
        {
        	PathInfo path(unit);
            bool result = path.Update(dest.x, dest.y, dest.z, false, forceDestination);
            if (result && !(path.getPathType() & PATHFIND_NOPATH))
            {
                MovebyPath(path.getFullPath());
                return;
            }
        }

        args.path_Idx_offset = 0;
        args.path.resize(2);
        TransportPathTransform transform(unit, args.TransformForTransport);
        args.path[1] = transform(dest);
    }

    Vector3 TransportPathTransform::operator()(Vector3 input)
    {
        return input;
    }
}
