/* Copyright (C) 2006 - 2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: mechanar
SD%Complete: 60
SDComment: Warden Mellichar, event controller for Skyriss event. Millhouse Manastorm. TODO: make better combatAI for Millhouse.
SDCategory: Tempest Keep, The mechanar
EndScriptData */

/* ContentData
npc_bridge_event
npc_warden_mellichar
mob_zerekethvoidzone
EndContentData */

#include "precompiled.h"
#include "def_mechanar.h"

/*#####
# npc_bridge_event
#####*/

//this npc send waves on the upper bridge. Doesn't spawn the npc's of the wave, this just use already placed ones if they're correctly placed

struct Locations
{
    float x, y, z;
};

static Locations WavePositions[]=
{
    {237.026993, -22.774099, 26.328400},
    {205.573212, -22.678530, 24.874506},
    {182.748077, -23.815599, 24.874506},
    {137.858368, 36.658611, 24.874506},
    {137.947998, 66.315300, 24.873100},
    {136.928543, 104.653252, 26.373301},
};

#define AGGRO_RANGE 15
#define MAX_WAVE 6

#define INVISIBLE_DUMMY 9

enum Phases
{
    PHASE_WAITING_FOR_PULL,
    PHASE_WAITING_FOR_WAVE_DEATH,
};

struct npc_bridge_eventAI : public ScriptedAI
{
    npc_bridge_eventAI(Creature *c) : ScriptedAI(c)
    {
        me->SetReactState(REACT_PASSIVE);
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        me->SetVisibility(VISIBILITY_OFF);
    }

    std::list<uint64> currentGroup;
    uint8 phase;
    uint8 wave; //from 1 to MAX_WAVE
    Unit* target; //wave target
    uint32 checkTimer; //cd for PHASE_WAITING_FOR_WAVE_DEATH checks
    uint64 dummies[MAX_WAVE]; //dummies at each wave start position

    void FillGroup(Creature* dummy, float radius)
    {
        currentGroup.clear();
        std::list<Creature*> creatureList;

        CellPair pair(Trinity::ComputeCellPair(dummy->GetPositionX(), dummy->GetPositionY()));
        Cell cell(pair);
        cell.data.Part.reserved = ALL_DISTRICT;
        cell.SetNoCreate();

        Trinity::AnyFriendlyUnitInObjectRangeCheck check(dummy,me,radius);
        Trinity::CreatureListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(creatureList, check);
        TypeContainerVisitor<Trinity::CreatureListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck>, GridTypeMapContainer> visitor(searcher);

        cell.Visit(pair, visitor, *me->GetMap());

        for(auto itr : creatureList)
        {
            if(itr == me || itr->GetEntry() == INVISIBLE_DUMMY || itr->GetPositionZ() < 24.0f) continue; //Z check to be sure not taking any creatures under the bridge
            currentGroup.push_back(itr->GetGUID());
        }
    }

    void Reset()
    {
        target = nullptr;
        checkTimer = 0;
        wave = 0;
        Cleanup(false);
        for(uint8 i = 0; i < MAX_WAVE; i++)
            if(Creature* c = me->SummonCreature(INVISIBLE_DUMMY,WavePositions[i].x,WavePositions[i].y,WavePositions[i].z,0,TEMPSUMMON_MANUAL_DESPAWN,0))
                dummies[i] = c->GetGUID();
        phase = PHASE_WAITING_FOR_PULL;
    }

    bool IsGroupAlive()
    {
        for(auto itr : currentGroup)
            if(Creature* c = me->GetCreature(*me,itr))
                if(c->IsAlive())
                    return true;

        return false;
    }

    void UpdateAI(const uint32 diff)
    {
        switch(phase)
        {
        case PHASE_WAITING_FOR_PULL:
            //Send attackers then step++, see MoveInLineOfSight
            break;
        case PHASE_WAITING_FOR_WAVE_DEATH:
            if(checkTimer > diff)
            {
                checkTimer -= diff;
                return;
            }

            if(!IsGroupAlive())
            {
                // must send next wave
                if(wave == 1 || wave == 4)
                {
                    SendNextWave(target);
                } else { //must prepare next wave only
                    target = nullptr;
                    phase = PHASE_WAITING_FOR_PULL;
                }
            }
            checkTimer = 1500;
        }
    }

    void Cleanup(bool killSelf)
    {
        for(uint8 i = 0; i < MAX_WAVE; i++)
        {
            if(Creature* dummy = me->GetCreature(*me,dummies[i]))
                dummy->DisappearAndDie();
            dummies[i] = 0;
        }

        if(killSelf)
            me->DisappearAndDie();
    }

    //fill currentGroup with nearby creatures and send those on target
    void SendNextWave(Unit* who)
    {
        Creature* dummy = me->GetCreature(*me,dummies[wave]);
        wave++;
        if(!dummy) return;
        FillGroup(dummy,10.0f);
        for(auto itr : currentGroup)
        {
            if(Creature* c = me->GetCreature(*me,itr))
                if(c->AI())
                    c->AI()->AttackStart(who);
        }

        //Last wave was sent, destroy npc and dummies
        if(wave == MAX_WAVE) 
            Cleanup(true);
    }

    void MoveInLineOfSight2(Unit *who)
    {
        if(!who->IsHostileTo(me)
           || !me->canAttack(who))
            return;

        if(phase == PHASE_WAITING_FOR_PULL)
        {
            if(who->GetDistance(me) < AGGRO_RANGE)
            {
                target = who;
                SendNextWave(target);
                phase = PHASE_WAITING_FOR_WAVE_DEATH;
            }
        }
    }
};

CreatureAI* GetAI_npc_bridge_event(Creature *_Creature)
{
    return new npc_bridge_eventAI (_Creature);
}

void AddSC_mechanar()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name="npc_bridge_event";
    newscript->GetAI = &GetAI_npc_bridge_event;
    newscript->RegisterSelf();
}

