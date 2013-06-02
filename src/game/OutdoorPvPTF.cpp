/*
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

#include "OutdoorPvPTF.h"
#include "OutdoorPvPMgr.h"
#include "WorldPacket.h"
#include "Player.h"
#include "ObjectMgr.h"
#include "Language.h"
#include "World.h"

OutdoorPvPTF::OutdoorPvPTF()
{
    m_TypeId = OUTDOOR_PVP_TF;
}

OutdoorPvPObjectiveTF::OutdoorPvPObjectiveTF(OutdoorPvP *pvp, OutdoorPvPTF_TowerType type)
: OutdoorPvPObjective(pvp), m_TowerType(type), m_TowerState(TF_TOWERSTATE_N)
{
    AddCapturePoint(TFCapturePoints[type].entry,TFCapturePoints[type].map,TFCapturePoints[type].x,TFCapturePoints[type].y,TFCapturePoints[type].z,TFCapturePoints[type].o,TFCapturePoints[type].rot0,TFCapturePoints[type].rot1,TFCapturePoints[type].rot2,TFCapturePoints[type].rot3);
}

void OutdoorPvPObjectiveTF::FillInitialWorldStates(WorldPacket &data)
{
    data << uint32(TFTowerWorldStates[m_TowerType].n) << uint32(bool(m_TowerState & TF_TOWERSTATE_N));
    data << uint32(TFTowerWorldStates[m_TowerType].h) << uint32(bool(m_TowerState & TF_TOWERSTATE_H));
    data << uint32(TFTowerWorldStates[m_TowerType].a) << uint32(bool(m_TowerState & TF_TOWERSTATE_A));
}

void OutdoorPvPTF::FillInitialWorldStates(WorldPacket &data)
{
    data << TF_UI_TOWER_SLIDER_POS << uint32(50);
    data << TF_UI_TOWER_SLIDER_N << uint32(100);
    data << TF_UI_TOWER_SLIDER_DISPLAY << uint32(0);

    data << TF_UI_TOWER_COUNT_H << m_HordeTowersControlled;
    data << TF_UI_TOWER_COUNT_A << m_AllianceTowersControlled;
    data << TF_UI_TOWERS_CONTROLLED_DISPLAY << uint32(!m_IsLocked);

    data << TF_UI_LOCKED_TIME_MINUTES_FIRST_DIGIT << first_digit;
    data << TF_UI_LOCKED_TIME_MINUTES_SECOND_DIGIT << second_digit;
    data << TF_UI_LOCKED_TIME_HOURS << hours_left;

    data << TF_UI_LOCKED_DISPLAY_NEUTRAL << uint32(m_IsLocked && !m_HordeTowersControlled && !m_AllianceTowersControlled);
    data << TF_UI_LOCKED_DISPLAY_HORDE << uint32(m_IsLocked && (m_HordeTowersControlled > m_AllianceTowersControlled));
    data << TF_UI_LOCKED_DISPLAY_ALLIANCE << uint32(m_IsLocked && (m_HordeTowersControlled < m_AllianceTowersControlled));

    for(OutdoorPvPObjectiveSet::iterator itr = m_OutdoorPvPObjectives.begin(); itr != m_OutdoorPvPObjectives.end(); ++itr)
    {
        (*itr)->FillInitialWorldStates(data);
    }
}

void OutdoorPvPTF::SendRemoveWorldStates(Player * plr)
{
    plr->SendUpdateWorldState(TF_UI_TOWER_SLIDER_POS,uint32(0));
    plr->SendUpdateWorldState(TF_UI_TOWER_SLIDER_N,uint32(0));
    plr->SendUpdateWorldState(TF_UI_TOWER_SLIDER_DISPLAY,uint32(0));

    plr->SendUpdateWorldState(TF_UI_TOWER_COUNT_H,uint32(0));
    plr->SendUpdateWorldState(TF_UI_TOWER_COUNT_A,uint32(0));
    plr->SendUpdateWorldState(TF_UI_TOWERS_CONTROLLED_DISPLAY,uint32(0));

    plr->SendUpdateWorldState(TF_UI_LOCKED_TIME_MINUTES_FIRST_DIGIT,uint32(0));
    plr->SendUpdateWorldState(TF_UI_LOCKED_TIME_MINUTES_SECOND_DIGIT,uint32(0));
    plr->SendUpdateWorldState(TF_UI_LOCKED_TIME_HOURS,uint32(0));

    plr->SendUpdateWorldState(TF_UI_LOCKED_DISPLAY_NEUTRAL,uint32(0));
    plr->SendUpdateWorldState(TF_UI_LOCKED_DISPLAY_HORDE,uint32(0));
    plr->SendUpdateWorldState(TF_UI_LOCKED_DISPLAY_ALLIANCE,uint32(0));

    for(int i = 0; i < TF_TOWER_NUM; ++i)
    {
        plr->SendUpdateWorldState(uint32(TFTowerWorldStates[i].n),uint32(0));
        plr->SendUpdateWorldState(uint32(TFTowerWorldStates[i].h),uint32(0));
        plr->SendUpdateWorldState(uint32(TFTowerWorldStates[i].a),uint32(0));
    }
}

void OutdoorPvPObjectiveTF::UpdateTowerState()
{
    m_PvP->SendUpdateWorldState(uint32(TFTowerWorldStates[m_TowerType].n),uint32(bool(m_TowerState & TF_TOWERSTATE_N)));
    m_PvP->SendUpdateWorldState(uint32(TFTowerWorldStates[m_TowerType].h),uint32(bool(m_TowerState & TF_TOWERSTATE_H)));
    m_PvP->SendUpdateWorldState(uint32(TFTowerWorldStates[m_TowerType].a),uint32(bool(m_TowerState & TF_TOWERSTATE_A)));
}

bool OutdoorPvPObjectiveTF::HandlePlayerEnter(Player *plr)
{
    if(OutdoorPvPObjective::HandlePlayerEnter(plr))
    {
        plr->SendUpdateWorldState(TF_UI_TOWER_SLIDER_DISPLAY, 1);
        uint32 phase = (uint32)ceil(( m_ShiftPhase + m_ShiftMaxPhase) / ( 2 * m_ShiftMaxPhase ) * 100.0f);
        plr->SendUpdateWorldState(TF_UI_TOWER_SLIDER_POS, phase);
        plr->SendUpdateWorldState(TF_UI_TOWER_SLIDER_N, m_NeutralValue);
        return true;
    }
    return false;
}

void OutdoorPvPObjectiveTF::HandlePlayerLeave(Player *plr)
{
    plr->SendUpdateWorldState(TF_UI_TOWER_SLIDER_DISPLAY, 0);
    OutdoorPvPObjective::HandlePlayerLeave(plr);
}

bool OutdoorPvPObjectiveTF::HandleCapturePointEvent(Player *plr, uint32 eventId)
{
    if(eventId == TFTowerPlayerEnterEvents[m_TowerType])
    {
        this->HandlePlayerEnter(plr);
        return true;
    }
    else if (eventId == TFTowerPlayerLeaveEvents[m_TowerType])
    {
        this->HandlePlayerLeave(plr);
        return true;
    }
    return false;
}

void OutdoorPvPTF::BuffTeam(uint32 team)
{
    if(team == ALLIANCE)
    {
        for(std::set<uint64>::iterator itr = m_PlayerGuids[0].begin(); itr != m_PlayerGuids[0].end(); ++itr)
        {
            if(Player * plr = objmgr.GetPlayer(*itr))
                if(plr->IsInWorld()) plr->CastSpell(plr,TF_CAPTURE_BUFF,true);
        }
        for(std::set<uint64>::iterator itr = m_PlayerGuids[1].begin(); itr != m_PlayerGuids[1].end(); ++itr)
        {
            if(Player * plr = objmgr.GetPlayer(*itr))
                if(plr->IsInWorld()) plr->RemoveAurasDueToSpell(TF_CAPTURE_BUFF);
        }
    }
    else if(team == HORDE)
    {
        for(std::set<uint64>::iterator itr = m_PlayerGuids[1].begin(); itr != m_PlayerGuids[1].end(); ++itr)
        {
            if(Player * plr = objmgr.GetPlayer(*itr))
                if(plr->IsInWorld()) plr->CastSpell(plr,TF_CAPTURE_BUFF,true);
        }
        for(std::set<uint64>::iterator itr = m_PlayerGuids[0].begin(); itr != m_PlayerGuids[0].end(); ++itr)
        {
            if(Player * plr = objmgr.GetPlayer(*itr))
                if(plr->IsInWorld()) plr->RemoveAurasDueToSpell(TF_CAPTURE_BUFF);
        }
    }
    else
    {
        for(std::set<uint64>::iterator itr = m_PlayerGuids[0].begin(); itr != m_PlayerGuids[0].end(); ++itr)
        {
            if(Player * plr = objmgr.GetPlayer(*itr))
                if(plr->IsInWorld()) plr->RemoveAurasDueToSpell(TF_CAPTURE_BUFF);
        }
        for(std::set<uint64>::iterator itr = m_PlayerGuids[1].begin(); itr != m_PlayerGuids[1].end(); ++itr)
        {
            if(Player * plr = objmgr.GetPlayer(*itr))
                if(plr->IsInWorld()) plr->RemoveAurasDueToSpell(TF_CAPTURE_BUFF);
        }
    }
}

void OutdoorPvPObjectiveTF::RewardDailyQuest(uint32 team)
{
    if (team == ALLIANCE)
    {
        for(std::set<uint64>::iterator itr = m_ActivePlayerGuids[0].begin(); itr != m_ActivePlayerGuids[0].end(); ++itr)
        {
            if(Player * plr = objmgr.GetPlayer(*itr)) {
                if(plr->IsInWorld() && plr->GetQuestStatus(11505) == QUEST_STATUS_INCOMPLETE)
                    plr->AreaExploredOrEventHappens(11505);
            }
        }
    }
    else if (team == HORDE)
    {
        for(std::set<uint64>::iterator itr = m_ActivePlayerGuids[1].begin(); itr != m_ActivePlayerGuids[1].end(); ++itr)
        {
            if(Player * plr = objmgr.GetPlayer(*itr)) {
                if(plr->IsInWorld() && plr->GetQuestStatus(11506) == QUEST_STATUS_INCOMPLETE)
                    plr->AreaExploredOrEventHappens(11506);
            }
        }
    }
    else
        sLog.outError("OutdoorPvPTF::RewardDailyQuest() invalid team ID: %u", team);
}

bool OutdoorPvPTF::Update(uint32 diff)
{
    bool changed = false;

    if(changed = OutdoorPvP::Update(diff))
    {
        if(m_AllianceTowersControlled == TF_TOWER_NUM)
        {
            BuffTeam(ALLIANCE);
            m_IsLocked = true;
            SendUpdateWorldState(TF_UI_LOCKED_DISPLAY_NEUTRAL,uint32(0));
            SendUpdateWorldState(TF_UI_LOCKED_DISPLAY_HORDE,uint32(0));
            SendUpdateWorldState(TF_UI_LOCKED_DISPLAY_ALLIANCE,uint32(1));
            SendUpdateWorldState(TF_UI_TOWERS_CONTROLLED_DISPLAY, uint32(0));
        }
        else if(m_HordeTowersControlled == TF_TOWER_NUM)
        {
            BuffTeam(HORDE);
            m_IsLocked = true;
            SendUpdateWorldState(TF_UI_LOCKED_DISPLAY_NEUTRAL,uint32(0));
            SendUpdateWorldState(TF_UI_LOCKED_DISPLAY_HORDE,uint32(1));
            SendUpdateWorldState(TF_UI_LOCKED_DISPLAY_ALLIANCE,uint32(0));
            SendUpdateWorldState(TF_UI_TOWERS_CONTROLLED_DISPLAY, uint32(0));
        }
        else
            BuffTeam(NULL);
        SendUpdateWorldState(TF_UI_TOWER_COUNT_A, m_AllianceTowersControlled);
        SendUpdateWorldState(TF_UI_TOWER_COUNT_H, m_HordeTowersControlled);
    }
    if(m_IsLocked)
    {
        // lock timer is down, release lock
        if(m_LockTimer < diff)
        {
            m_LockTimer = TF_LOCK_TIME;
            m_LockTimerUpdate = 0;
            m_IsLocked = false;
            SendUpdateWorldState(TF_UI_TOWERS_CONTROLLED_DISPLAY, uint32(1));
            SendUpdateWorldState(TF_UI_LOCKED_DISPLAY_NEUTRAL,uint32(0));
            SendUpdateWorldState(TF_UI_LOCKED_DISPLAY_HORDE,uint32(0));
            SendUpdateWorldState(TF_UI_LOCKED_DISPLAY_ALLIANCE,uint32(0));
        }
        else
        {
            // worldstateui update timer is down, update ui with new time data
            if(m_LockTimerUpdate < diff)
            {
                m_LockTimerUpdate = TF_LOCK_TIME_UPDATE;
                uint32 minutes_left = m_LockTimer / 60000;
                hours_left = minutes_left / 60;
                minutes_left -= hours_left * 60;
                second_digit = minutes_left % 10;
                first_digit = minutes_left / 10;

                SendUpdateWorldState(TF_UI_LOCKED_TIME_MINUTES_FIRST_DIGIT,first_digit);
                SendUpdateWorldState(TF_UI_LOCKED_TIME_MINUTES_SECOND_DIGIT,second_digit);
                SendUpdateWorldState(TF_UI_LOCKED_TIME_HOURS,hours_left);
            } else m_LockTimerUpdate -= diff;
            m_LockTimer -= diff;
        }
    }
    return changed;
}

void OutdoorPvPTF::HandlePlayerEnterZone(Player * plr, uint32 zone)
{
    if(plr->GetTeam() == ALLIANCE)
    {
        if(m_AllianceTowersControlled >= TF_TOWER_NUM)
            plr->CastSpell(plr,TF_CAPTURE_BUFF,true);
    }
    else
    {
        if(m_HordeTowersControlled >= TF_TOWER_NUM)
            plr->CastSpell(plr,TF_CAPTURE_BUFF,true);
    }
    OutdoorPvP::HandlePlayerEnterZone(plr,zone);
}

void OutdoorPvPTF::HandlePlayerLeaveZone(Player * plr, uint32 zone)
{
    // remove buffs
    plr->RemoveAurasDueToSpell(TF_CAPTURE_BUFF);
    OutdoorPvP::HandlePlayerLeaveZone(plr, zone);
}

bool OutdoorPvPTF::SetupOutdoorPvP()
{
    m_AllianceTowersControlled = 0;
    m_HordeTowersControlled = 0;

    m_IsLocked = false;
    m_LockTimer = TF_LOCK_TIME;
    m_LockTimerUpdate = 0;
    hours_left = 6;
    second_digit = 0;
    first_digit = 0;

    // add the zones affected by the pvp buff
    for(int i = 0; i < OutdoorPvPTFBuffZonesNum; ++i)
        sOutdoorPvPMgr.AddZone(OutdoorPvPTFBuffZones[i],this);

    m_OutdoorPvPObjectives.push_back(new OutdoorPvPObjectiveTF(this,TF_TOWER_NW));
    m_OutdoorPvPObjectives.push_back(new OutdoorPvPObjectiveTF(this,TF_TOWER_N));
    m_OutdoorPvPObjectives.push_back(new OutdoorPvPObjectiveTF(this,TF_TOWER_NE));
    m_OutdoorPvPObjectives.push_back(new OutdoorPvPObjectiveTF(this,TF_TOWER_SE));
    m_OutdoorPvPObjectives.push_back(new OutdoorPvPObjectiveTF(this,TF_TOWER_S));

    return true;
}

bool OutdoorPvPObjectiveTF::Update(uint32 diff)
{
    // can update even in locked state if gathers the controlling faction
    bool canupdate = ((((OutdoorPvPTF*)m_PvP)->m_AllianceTowersControlled > 0) && this->m_ActivePlayerGuids[0].size() > this->m_ActivePlayerGuids[1].size()) ||
            ((((OutdoorPvPTF*)m_PvP)->m_HordeTowersControlled > 0) && this->m_ActivePlayerGuids[0].size() < this->m_ActivePlayerGuids[1].size());
    // if gathers the other faction, then only update if the pvp is unlocked
    canupdate = canupdate || !((OutdoorPvPTF*)m_PvP)->m_IsLocked;
    if(canupdate && OutdoorPvPObjective::Update(diff))
    {
        if(m_OldState != m_State)
        {
            // if changing from controlling alliance to horde
            if( m_OldState == OBJECTIVESTATE_ALLIANCE )
            {
                if(((OutdoorPvPTF*)m_PvP)->m_AllianceTowersControlled)
                    ((OutdoorPvPTF*)m_PvP)->m_AllianceTowersControlled--;
                sWorld.SendZoneText(OutdoorPvPTFBuffZones[0],objmgr.GetTrinityStringForDBCLocale(LANG_OPVP_TF_LOOSE_A));
            }
            // if changing from controlling horde to alliance
            else if ( m_OldState == OBJECTIVESTATE_HORDE )
            {
                if(((OutdoorPvPTF*)m_PvP)->m_HordeTowersControlled)
                    ((OutdoorPvPTF*)m_PvP)->m_HordeTowersControlled--;
                sWorld.SendZoneText(OutdoorPvPTFBuffZones[0],objmgr.GetTrinityStringForDBCLocale(LANG_OPVP_TF_LOOSE_H));
            }

            uint32 artkit = 21;

            switch(m_State)
            {
            case OBJECTIVESTATE_ALLIANCE:
                m_TowerState = TF_TOWERSTATE_A;
                artkit = 2;
                if(((OutdoorPvPTF*)m_PvP)->m_AllianceTowersControlled<TF_TOWER_NUM)
                    ((OutdoorPvPTF*)m_PvP)->m_AllianceTowersControlled++;
                sWorld.SendZoneText(OutdoorPvPTFBuffZones[0],objmgr.GetTrinityStringForDBCLocale(LANG_OPVP_TF_CAPTURE_A));
                RewardDailyQuest(ALLIANCE);
                break;
            case OBJECTIVESTATE_HORDE:
                m_TowerState = TF_TOWERSTATE_H;
                artkit = 1;
                if(((OutdoorPvPTF*)m_PvP)->m_HordeTowersControlled<TF_TOWER_NUM)
                    ((OutdoorPvPTF*)m_PvP)->m_HordeTowersControlled++;
                sWorld.SendZoneText(OutdoorPvPTFBuffZones[0],objmgr.GetTrinityStringForDBCLocale(LANG_OPVP_TF_CAPTURE_H));
                RewardDailyQuest(HORDE);
                break;
            case OBJECTIVESTATE_NEUTRAL:
            case OBJECTIVESTATE_NEUTRAL_ALLIANCE_CHALLENGE:
            case OBJECTIVESTATE_NEUTRAL_HORDE_CHALLENGE:
            case OBJECTIVESTATE_ALLIANCE_HORDE_CHALLENGE:
            case OBJECTIVESTATE_HORDE_ALLIANCE_CHALLENGE:
                m_TowerState = TF_TOWERSTATE_N;
                break;
            }

            GameObject* flag = HashMapHolder<GameObject>::Find(m_CapturePoint);
            if(flag)
            {
                flag->SetGoArtKit(artkit);
                flag->SendUpdateObjectToAllExcept(NULL);
            }

            UpdateTowerState();
        }

        if(m_ShiftPhase != m_OldPhase)
        {
            // send this too, sometimes the slider disappears, dunno why :(
            SendUpdateWorldState(TF_UI_TOWER_SLIDER_DISPLAY, 1);
            // send these updates to only the ones in this objective
            uint32 phase = (uint32)ceil(( m_ShiftPhase + m_ShiftMaxPhase) / ( 2 * m_ShiftMaxPhase ) * 100.0f);
            SendUpdateWorldState(TF_UI_TOWER_SLIDER_POS, phase);
            // send this too, sometimes it resets :S
            SendUpdateWorldState(TF_UI_TOWER_SLIDER_N, m_NeutralValue);
        }
        return m_OldState != m_State;
    }
    return false;
}

