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

#include "Common.h"

#include "Transports.h"
#include "MapManager.h"
#include "ObjectMgr.h"
#include "Path.h"

#include "WorldPacket.h"
#include "Database/DBCStores.h"

#include "World.h"

void MapManager::LoadTransports()
{
    QueryResult *result = WorldDatabase.Query("SELECT entry, name, period FROM transports");

    uint32 count = 0;

    if( !result )
    {
        sLog.outString();
        sLog.outString( ">> Loaded %u transports", count );
        return;
    }

    do
    {
        Transport *t = new Transport;

        Field *fields = result->Fetch();

        uint32 entry = fields[0].GetUInt32();
        std::string name = fields[1].GetCppString();
        t->m_period = fields[2].GetUInt32();

        const GameObjectInfo *goinfo = objmgr.GetGameObjectInfo(entry);

        if(!goinfo)
        {
            sLog.outErrorDb("Transport ID:%u, Name: %s, will not be loaded, gameobject_template missing", entry, name.c_str());
            delete t;
            continue;
        }

        if(goinfo->type != GAMEOBJECT_TYPE_MO_TRANSPORT)
        {
            sLog.outErrorDb("Transport ID:%u, Name: %s, will not be loaded, gameobject_template type wrong", entry, name.c_str());
            delete t;
            continue;
        }

        // sLog.outString("Loading transport %d between %s, %s", entry, name.c_str(), goinfo->name);

        std::set<uint32> mapsUsed;

        if(!t->GenerateWaypoints(goinfo->moTransport.taxiPathId, mapsUsed))
            // skip transports with empty waypoints list
        {
            sLog.outErrorDb("Transport (path id %u) path size = 0. Transport ignored, check DBC files or transport GO data0 field.",goinfo->moTransport.taxiPathId);
            delete t;
            continue;
        }

        t->m_name = goinfo->name;

        float x, y, z, o;
        uint32 mapid;
        x = t->m_WayPoints[0].x; y = t->m_WayPoints[0].y; z = t->m_WayPoints[0].z; mapid = t->m_WayPoints[0].mapid; o = 1;

                                                            // creates the Gameobject
        if(!t->Create(entry, mapid, x, y, z, o, 100, 0))
        {
            delete t;
            continue;
        }

        m_Transports.insert(t);

        for (std::set<uint32>::iterator i = mapsUsed.begin(); i != mapsUsed.end(); ++i)
            m_TransportsByMap[*i].insert(t);

        //If we someday decide to use the grid to track transports, here:
        //MapManager::Instance().LoadGrid(mapid,x,y,true);
        //MapManager::Instance().GetMap(t->GetMapId())->Add<GameObject>((GameObject *)t);
        ++count;
    } while(result->NextRow());
    delete result;

    sLog.outString();
    sLog.outString( ">> Loaded %u transports", count );

    // check transport data DB integrity
    result = WorldDatabase.Query("SELECT gameobject.guid,gameobject.id,transports.name FROM gameobject,transports WHERE gameobject.id = transports.entry");
    if(result)                                              // wrong data found
    {
        do
        {
            Field *fields = result->Fetch();

            uint32 guid  = fields[0].GetUInt32();
            uint32 entry = fields[1].GetUInt32();
            std::string name = fields[2].GetCppString();
            sLog.outErrorDb("Transport %u '%s' have record (GUID: %u) in `gameobject`. Transports DON'T must have any records in `gameobject` or its behavior will be unpredictable/bugged.",entry,name.c_str(),guid);
        }
        while(result->NextRow());

        delete result;
    }
}

Transport::Transport() : GameObject()
{
                                                            // 2.3.2 - 0x5A
    m_updateFlag = (UPDATEFLAG_TRANSPORT | UPDATEFLAG_LOWGUID | UPDATEFLAG_HIGHGUID | UPDATEFLAG_HASPOSITION);
}

bool Transport::Create(uint32 guidlow, uint32 mapid, float x, float y, float z, float ang, uint32 animprogress, uint32 dynflags)
{
    Relocate(x,y,z,ang);

    SetMapId(mapid);

    if(!IsPositionValid())
    {
        sLog.outError("ERROR: Transport (GUID: %u) not created. Suggested coordinates isn't valid (X: %f Y: %f)",
            guidlow,x,y);
        return false;
    }

    Object::_Create(guidlow, 0, HIGHGUID_MO_TRANSPORT);

    GameObjectInfo const* goinfo = objmgr.GetGameObjectInfo(guidlow);

    if (!goinfo)
    {
        sLog.outErrorDb("Transport not created: entry in `gameobject_template` not found, guidlow: %u map: %u  (X: %f Y: %f Z: %f) ang: %f",guidlow, mapid, x, y, z, ang);
        return false;
    }

    m_goInfo = goinfo;

    SetFloatValue(OBJECT_FIELD_SCALE_X, goinfo->size);

    SetUInt32Value(GAMEOBJECT_FACTION, goinfo->faction);
    SetUInt32Value(GAMEOBJECT_FLAGS, goinfo->flags);

    SetUInt32Value(OBJECT_FIELD_ENTRY, goinfo->id);

    SetUInt32Value(GAMEOBJECT_DISPLAYID, goinfo->displayId);

    SetGoState(1);
    SetGoType(GameobjectTypes(goinfo->type));

    SetGoAnimProgress(animprogress);
    if(dynflags)
        SetUInt32Value(GAMEOBJECT_DYN_FLAGS, dynflags);

    return true;
}

struct keyFrame
{
    keyFrame(float _x, float _y, float _z, uint32 _mapid, int _actionflag, int _delay)
    {
        x = _x; y = _y; z = _z; mapid = _mapid; actionflag = _actionflag; delay = _delay; distFromPrev = -1; distSinceStop = -1; distUntilStop = -1;
        tFrom = 0; tTo = 0;
    }

    float x;
    float y;
    float z;
    uint32 mapid;
    int actionflag;
    int delay;
    float distSinceStop;
    float distUntilStop;
    float distFromPrev;
    float tFrom, tTo;
};

bool Transport::GenerateWaypoints(uint32 pathid, std::set<uint32> &mapids)
{
    TransportPath path;
    objmgr.GetTransportPathNodes(pathid, path);

    if (path.Empty())
        return false;

    std::vector<keyFrame> keyFrames;
    int mapChange = 0;
    mapids.clear();
    for (size_t i = 1; i < path.Size() - 1; i++)
    {
        if (mapChange == 0)
        {
            if ((path[i].mapid == path[i+1].mapid))
            {
                keyFrame k(path[i].x, path[i].y, path[i].z, path[i].mapid, path[i].actionFlag, path[i].delay);
                keyFrames.push_back(k);
                mapids.insert(k.mapid);
            }
            else
            {
                mapChange = 1;
            }
        }
        else
        {
            --mapChange;
        }
    }

    int lastStop = -1;
    int firstStop = -1;

    // first cell is arrived at by teleportation :S
    keyFrames[0].distFromPrev = 0;
    if (keyFrames[0].actionflag == 2)
    {
        lastStop = 0;
    }

    // find the rest of the distances between key points
    for (size_t i = 1; i < keyFrames.size(); i++)
    {
        if ((keyFrames[i].actionflag == 1) || (keyFrames[i].mapid != keyFrames[i-1].mapid))
        {
            keyFrames[i].distFromPrev = 0;
        }
        else
        {
            keyFrames[i].distFromPrev =
                sqrt(pow(keyFrames[i].x - keyFrames[i - 1].x, 2) +
                pow(keyFrames[i].y - keyFrames[i - 1].y, 2) +
                pow(keyFrames[i].z - keyFrames[i - 1].z, 2));
        }
        if (keyFrames[i].actionflag == 2)
        {
            // remember first stop frame
            if(firstStop == -1)
                firstStop = i;
            lastStop = i;
        }
    }

    float tmpDist = 0;
    for (size_t i = 0; i < keyFrames.size(); i++)
    {
        int j = (i + lastStop) % keyFrames.size();
        if (keyFrames[j].actionflag == 2)
            tmpDist = 0;
        else
            tmpDist += keyFrames[j].distFromPrev;
        keyFrames[j].distSinceStop = tmpDist;
    }

    for (int i = int(keyFrames.size()) - 1; i >= 0; i--)
    {
        int j = (i + (firstStop+1)) % keyFrames.size();
        tmpDist += keyFrames[(j + 1) % keyFrames.size()].distFromPrev;
        keyFrames[j].distUntilStop = tmpDist;
        if (keyFrames[j].actionflag == 2)
            tmpDist = 0;
    }

    for (size_t i = 0; i < keyFrames.size(); i++)
    {
        if (keyFrames[i].distSinceStop < (30 * 30 * 0.5f))
            keyFrames[i].tFrom = sqrt(2 * keyFrames[i].distSinceStop);
        else
            keyFrames[i].tFrom = ((keyFrames[i].distSinceStop - (30 * 30 * 0.5f)) / 30) + 30;

        if (keyFrames[i].distUntilStop < (30 * 30 * 0.5f))
            keyFrames[i].tTo = sqrt(2 * keyFrames[i].distUntilStop);
        else
            keyFrames[i].tTo = ((keyFrames[i].distUntilStop - (30 * 30 * 0.5f)) / 30) + 30;

        keyFrames[i].tFrom *= 1000;
        keyFrames[i].tTo *= 1000;
    }

    //    for (int i = 0; i < keyFrames.size(); i++) {
    //        sLog.outString("%f, %f, %f, %f, %f, %f, %f", keyFrames[i].x, keyFrames[i].y, keyFrames[i].distUntilStop, keyFrames[i].distSinceStop, keyFrames[i].distFromPrev, keyFrames[i].tFrom, keyFrames[i].tTo);
    //    }

    // Now we're completely set up; we can move along the length of each waypoint at 100 ms intervals
    // speed = max(30, t) (remember x = 0.5s^2, and when accelerating, a = 1 unit/s^2
    int t = 0;
    bool teleport = false;
    if (keyFrames[keyFrames.size() - 1].mapid != keyFrames[0].mapid)
        teleport = true;

    WayPoint pos(keyFrames[0].mapid, keyFrames[0].x, keyFrames[0].y, keyFrames[0].z, teleport, 0);
    m_WayPoints[0] = pos;
    t += keyFrames[0].delay * 1000;

    uint32 cM = keyFrames[0].mapid;
    for (size_t i = 0; i < keyFrames.size() - 1; i++)
    {
        float d = 0;
        float tFrom = keyFrames[i].tFrom;
        float tTo = keyFrames[i].tTo;

        // keep the generation of all these points; we use only a few now, but may need the others later
        if (((d < keyFrames[i + 1].distFromPrev) && (tTo > 0)))
        {
            while ((d < keyFrames[i + 1].distFromPrev) && (tTo > 0))
            {
                tFrom += 100;
                tTo -= 100;

                if (d > 0)
                {
                    float newX, newY, newZ;
                    newX = keyFrames[i].x + (keyFrames[i + 1].x - keyFrames[i].x) * d / keyFrames[i + 1].distFromPrev;
                    newY = keyFrames[i].y + (keyFrames[i + 1].y - keyFrames[i].y) * d / keyFrames[i + 1].distFromPrev;
                    newZ = keyFrames[i].z + (keyFrames[i + 1].z - keyFrames[i].z) * d / keyFrames[i + 1].distFromPrev;

                    bool teleport = false;
                    if (keyFrames[i].mapid != cM)
                    {
                        teleport = true;
                        cM = keyFrames[i].mapid;
                    }

                    //                    sLog.outString("T: %d, D: %f, x: %f, y: %f, z: %f", t, d, newX, newY, newZ);
                    WayPoint pos(keyFrames[i].mapid, newX, newY, newZ, teleport, i);
                    if (teleport)
                        m_WayPoints[t] = pos;
                }

                if (tFrom < tTo)                            // caught in tFrom dock's "gravitational pull"
                {
                    if (tFrom <= 30000)
                    {
                        d = 0.5f * (tFrom / 1000) * (tFrom / 1000);
                    }
                    else
                    {
                        d = 0.5f * 30 * 30 + 30 * ((tFrom - 30000) / 1000);
                    }
                    d = d - keyFrames[i].distSinceStop;
                }
                else
                {
                    if (tTo <= 30000)
                    {
                        d = 0.5f * (tTo / 1000) * (tTo / 1000);
                    }
                    else
                    {
                        d = 0.5f * 30 * 30 + 30 * ((tTo - 30000) / 1000);
                    }
                    d = keyFrames[i].distUntilStop - d;
                }
                t += 100;
            }
            t -= 100;
        }

        if (keyFrames[i + 1].tFrom > keyFrames[i + 1].tTo)
            t += 100 - ((long)keyFrames[i + 1].tTo % 100);
        else
            t += (long)keyFrames[i + 1].tTo % 100;

        bool teleport = false;
        if ((keyFrames[i + 1].actionflag == 1) || (keyFrames[i + 1].mapid != keyFrames[i].mapid))
        {
            teleport = true;
            cM = keyFrames[i + 1].mapid;
        }

        WayPoint pos(keyFrames[i + 1].mapid, keyFrames[i + 1].x, keyFrames[i + 1].y, keyFrames[i + 1].z, teleport, i);

        //        sLog.outString("T: %d, x: %f, y: %f, z: %f, t:%d", t, pos.x, pos.y, pos.z, teleport);
/*
        if(keyFrames[i+1].delay > 5)
            pos.delayed = true;
*/
        //if (teleport)
        m_WayPoints[t] = pos;

        t += keyFrames[i + 1].delay * 1000;
        //        sLog.outString("------");
    }

    uint32 timer = t;

    //    sLog.outDetail("    Generated %d waypoints, total time %u.", m_WayPoints.size(), timer);

    m_curr = m_WayPoints.begin();
    m_curr = GetNextWayPoint();
    m_next = GetNextWayPoint();
    m_pathTime = timer;

    m_nextNodeTime = m_curr->first;

    return true;
}

Transport::WayPointMap::iterator Transport::GetNextWayPoint()
{
    WayPointMap::iterator iter = m_curr;
    ++iter;
    if (iter == m_WayPoints.end())
        iter = m_WayPoints.begin();
    return iter;
}

void Transport::TeleportTransport(uint32 newMapid, float x, float y, float z)
{
    //MapManager::Instance().GetMap(oldMapid)->Remove((GameObject *)this, false);
    SetMapId(newMapid);
    //MapManager::Instance().LoadGrid(newMapid,x,y,true);
    Relocate(x, y, z);
    //MapManager::Instance().GetMap(newMapid)->Add<GameObject>((GameObject *)this);

    for(PlayerSet::iterator itr = m_passengers.begin(); itr != m_passengers.end();)
    {
        PlayerSet::iterator it2 = itr;
        ++itr;

        Player *plr = *it2;
        if(!plr)
        {
            m_passengers.erase(it2);
            continue;
        }

        if (plr->isDead() && !plr->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST))
        {
            plr->ResurrectPlayer(1.0);
        }
        plr->TeleportTo(newMapid, x, y, z, GetOrientation(), TELE_TO_NOT_LEAVE_TRANSPORT);

        //WorldPacket data(SMSG_811, 4);
        //data << uint32(0);
        //plr->GetSession()->SendPacket(&data);
    }
}

bool Transport::AddPassenger(Player* passenger)
{
    if (m_passengers.find(passenger) == m_passengers.end())
    {
        sLog.outDetail("Player %s boarded transport %s.", passenger->GetName(), this->m_name.c_str());
        m_passengers.insert(passenger);
    }
    return true;
}

bool Transport::RemovePassenger(Player* passenger)
{
    if (m_passengers.find(passenger) != m_passengers.end())
    {
        sLog.outDetail("Player %s removed from transport %s.", passenger->GetName(), this->m_name.c_str());
        m_passengers.erase(passenger);
    }
    return true;
}

void Transport::CheckForEvent(uint32 entry, uint32 wp_id)
{
    uint32 key = entry*100+wp_id;
    if(objmgr.TransportEventMap.find(key) != objmgr.TransportEventMap.end())
        sWorld.ScriptsStart(sEventScripts, objmgr.TransportEventMap[key], this, NULL);
}

void Transport::Update(uint32 /*p_time*/)
{
    if (m_WayPoints.size() <= 1)
        return;

    m_timer = getMSTime() % m_period;
    while (((m_timer - m_curr->first) % m_pathTime) > ((m_next->first - m_curr->first) % m_pathTime))
    {
        m_curr = GetNextWayPoint();
        m_next = GetNextWayPoint();

        // first check help in case client-server transport coordinates de-synchronization
        if (m_curr->second.mapid != GetMapId() || m_curr->second.teleport)
        {
            TeleportTransport(m_curr->second.mapid, m_curr->second.x, m_curr->second.y, m_curr->second.z);
        }
        else
        {
            //MapManager::Instance().GetMap(m_curr->second.mapid)->GameobjectRelocation((GameObject *)this, m_curr->second.x, m_curr->second.y, m_curr->second.z, this->m_orientation);
            Relocate(m_curr->second.x, m_curr->second.y, m_curr->second.z);
        }
/*
        if(m_curr->second.delayed)
        {
            switch (GetEntry())
            {
                case 176495:
                case 164871:
                case 175080:
                    SendPlaySound(11804, false); break;     // ZeppelinDocked
                case 20808:
                case 181646:
                case 176231:
                case 176244:
                case 176310:
                case 177233:
                    SendPlaySound(5495, false);break;       // BoatDockingWarning
                default:
                    SendPlaySound(5154, false); break;      // ShipDocked
            }
        }
*/
        /*
        for(PlayerSet::iterator itr = m_passengers.begin(); itr != m_passengers.end();)
        {
            PlayerSet::iterator it2 = itr;
            ++itr;
            //(*it2)->SetPosition( m_curr->second.x + (*it2)->GetTransOffsetX(), m_curr->second.y + (*it2)->GetTransOffsetY(), m_curr->second.z + (*it2)->GetTransOffsetZ(), (*it2)->GetTransOffsetO() );
        }
        */

        m_nextNodeTime = m_curr->first;

        if (m_curr == m_WayPoints.begin() && (sLog.getLogFilter() & LOG_FILTER_TRANSPORT_MOVES)==0)
            sLog.outDetail(" ************ BEGIN ************** %s", this->m_name.c_str());

        //        MapManager::Instance().GetMap(m_curr->second.mapid)->Add(&this); // -> // ->Add(t);
        //MapManager::Instance().GetMap(m_curr->second.mapid)->Remove((GameObject *)this, false); // -> // ->Add(t);
        //MapManager::Instance().GetMap(m_curr->second.mapid)->Add((GameObject *)this); // -> // ->Add(t);

        //if ((sLog.getLogFilter() & LOG_FILTER_TRANSPORT_MOVES)==0)
            //sLog.outDetail("%s moved to %d %f %f %f %d", this->m_name.c_str(), m_curr->second.id, m_curr->second.x, m_curr->second.y, m_curr->second.z, m_curr->second.mapid);

        //Transport Event System
        CheckForEvent(this->GetEntry(), m_curr->second.id);
    }
}

