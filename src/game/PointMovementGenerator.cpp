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

#include "PointMovementGenerator.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "Errors.h"
#include "CreatureAINew.h"
#include "Player.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "MapManager.h"
#include "World.h"

//----- Point Movement Generator
template<class T>
void PointMovementGenerator<T>::Initialize(T* unit)
{
    if (!unit->IsStopped())
        unit->StopMoving();

    unit->addUnitState(UNIT_STAT_ROAMING);

    if (id == 1005)
        return;

    Movement::MoveSplineInit init(unit);
    init.MoveTo(i_x, i_y, i_z, m_usePathfinding);
    if (m_speed > 0.0f)
        init.SetVelocity(m_speed);
    init.Launch();

    // Call for creature group update
    if (Creature* creature = unit->ToCreature())
        if (creature->GetFormation() && creature->GetFormation()->getLeader() == creature)
            creature->GetFormation()->LeaderMoveTo(i_x, i_y, i_z);
}

template<class T>
bool PointMovementGenerator<T>::Update(T* unit, const uint32 &diff)
{
    if(!unit)
        return false;

    if(unit->hasUnitState(UNIT_STAT_NOT_MOVE))
    {
    	unit->clearUnitState(UNIT_STAT_ROAMING_MOVE);
        return true;
    }

    unit->addUnitState(UNIT_STAT_ROAMING_MOVE);

    if (id != 1005 && i_recalculateSpeed && !unit->movespline->Finalized())
    {
        i_recalculateSpeed = false;
        Movement::MoveSplineInit init(unit);
        init.MoveTo(i_x, i_y, i_z, m_usePathfinding);
        if (m_speed > 0.0f) // Default value for point motion type is 0.0, if 0.0 spline will use GetSpeed on unit
            init.SetVelocity(m_speed);
        init.Launch();

        // Call for creature group update
        if (Creature* creature = unit->ToCreature())
            if (creature->GetFormation() && creature->GetFormation()->getLeader() == creature)
               creature->GetFormation()->LeaderMoveTo(i_x, i_y, i_z);
    }

    return !unit->movespline->Finalized();
}

template<class T>
void PointMovementGenerator<T>:: Finalize(T* unit)
{
    if (unit->hasUnitState(UNIT_STAT_CHARGING))
        unit->clearUnitState(UNIT_STAT_ROAMING | UNIT_STAT_ROAMING_MOVE);

    if (unit->movespline->Finalized())
        MovementInform(unit);
}

template<class T>
void PointMovementGenerator<T>::Reset(T* unit)
{
    if (!unit->IsStopped())
        unit->StopMoving();

    unit->addUnitState(UNIT_STAT_ROAMING);
}

template<class T>
void PointMovementGenerator<T>::MovementInform(T* /*unit*/)
{
}

template <> void PointMovementGenerator<Creature>::MovementInform(Creature* unit)
{
    if (unit->GetSummoner())
    {
        if (unit->GetSummoner()->ToCreature())
            if (unit->GetSummoner()->ToCreature()->getAI())
                unit->GetSummoner()->ToCreature()->getAI()->summonedMovementInform(unit, POINT_MOTION_TYPE, id);
    }

    if (unit->getAI())
        unit->getAI()->onMovementInform(POINT_MOTION_TYPE, id);
    else
        unit->AI()->MovementInform(POINT_MOTION_TYPE, id);
}

template void PointMovementGenerator<Player>::Initialize(Player*);
template bool PointMovementGenerator<Player>::Update(Player*, const uint32 &diff);
template void PointMovementGenerator<Player>::Finalize(Player*);
template void PointMovementGenerator<Player>::Reset(Player*);

template void PointMovementGenerator<Creature>::Initialize(Creature*);
template bool PointMovementGenerator<Creature>::Update(Creature*, const uint32 &diff);
template void PointMovementGenerator<Creature>::Finalize(Creature*);
template void PointMovementGenerator<Creature>::Reset(Creature*);

void AssistanceMovementGenerator::Finalize(Unit* unit)
{
    unit->ToCreature()->SetNoCallAssistance(false);
    unit->ToCreature()->CallAssistance();
    if (unit->isAlive())
        unit->GetMotionMaster()->MoveSeekAssistanceDistract(sWorld.getConfig(CONFIG_CREATURE_FAMILY_ASSISTANCE_DELAY));
}

bool EffectMovementGenerator::Update(Unit* unit, const uint32 &diff)
{
    return !unit->movespline->Finalized();
}

void EffectMovementGenerator::Finalize(Unit* unit)
{
    if (unit->GetTypeId() != TYPEID_UNIT)
        return;

    // Need restore previous movement since we have no proper states system
    if (unit->isAlive() && !unit->hasUnitState(UNIT_STAT_CONFUSED | UNIT_STAT_FLEEING))
    {
        if (Unit* victim = unit->getVictim())
            unit->GetMotionMaster()->MoveChase(victim);
        else
            unit->GetMotionMaster()->Initialize();
    }

    if (unit->GetSummoner())
    {
        if (unit->GetSummoner()->ToCreature())
    	    if (unit->GetSummoner()->ToCreature()->getAI())
    	        unit->GetSummoner()->ToCreature()->getAI()->summonedMovementInform(unit->ToCreature(), POINT_MOTION_TYPE, m_Id);
    }

    if (unit->ToCreature()->getAI())
        unit->ToCreature()->getAI()->onMovementInform(POINT_MOTION_TYPE, m_Id);
    if (unit->ToCreature()->AI())
        unit->ToCreature()->AI()->MovementInform(EFFECT_MOTION_TYPE, m_Id);
}
