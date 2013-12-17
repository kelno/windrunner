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

#include "HomeMovementGenerator.h"
#include "Creature.h"
#include "CreatureAINew.h"
#include "WorldPacket.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"

void
HomeMovementGenerator<Creature>::Initialize(Creature* owner)
{
    _setTargetLocation(owner);
}

void
HomeMovementGenerator<Creature>::Finalize(Creature* owner)
{
    if (arrived)
    {
        owner->clearUnitState(UNIT_STAT_EVADE);
        owner->SetWalk(true);
        owner->LoadCreaturesAddon(true);
        if (owner->IsAIEnabled)
        {
            owner->AI()->JustReachedHome();
            if (owner->getAI())
                owner->getAI()->onReachedHome();
        }
    }
}

void
HomeMovementGenerator<Creature>::Reset(Creature*)
{
}

void
HomeMovementGenerator<Creature>::_setTargetLocation(Creature* owner)
{
    if( !owner )
        return;

    if( owner->hasUnitState(UNIT_STAT_NOT_MOVE) )
        return;

    Movement::MoveSplineInit init(owner);
    float x, y, z, o;
    // at apply we can select more nice return points base at current movegen
    if (owner->GetMotionMaster()->empty() || !owner->GetMotionMaster()->top()->GetResetPosition(owner, x, y, z))
    {
        owner->GetHomePosition(x, y, z, o);
        init.SetFacing(o);
    }
    init.MoveTo(x, y, z);
    init.SetWalk(false);
    init.Launch();

    arrived = false;

    owner->clearUnitState(uint32(UNIT_STAT_ALL_STATE & ~UNIT_STAT_EVADE));
}

bool
HomeMovementGenerator<Creature>::Update(Creature* owner, const uint32& time_diff)
{
    arrived = owner->movespline->Finalized();
    return !arrived;
}

