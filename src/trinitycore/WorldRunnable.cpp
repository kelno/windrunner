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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/** \file
    \ingroup Trinityd
*/

#include "WorldSocketMgr.h"
#include "Common.h"
#include "World.h"
#include "WorldRunnable.h"
#include "Timer.h"
#include "ObjectAccessor.h"
#include "MapManager.h"
#include "BattleGroundMgr.h"

#include "Database/DatabaseEnv.h"

#if (defined(SHORT_SLEEP) || defined(WIN32))
#define WORLD_SLEEP_CONST 50
#else
#define WORLD_SLEEP_CONST 100                            //Is this still needed?? [On linux some time ago not working 50ms]
#endif

/// Heartbeat for the World
void WorldRunnable::run()
{
    ///- Init MySQL threads or connections
    bool needInit = true;
    if (!(sWorld.getConfig(CONFIG_MYSQL_BUNDLE_WORLDDB) & MYSQL_BUNDLE_RA))
    {
        WorldDatabase.Init_MySQL_Connection();
        needInit = false;
    }
    if (!(sWorld.getConfig(CONFIG_MYSQL_BUNDLE_LOGINDB) & MYSQL_BUNDLE_RA))
    {
        LoginDatabase.Init_MySQL_Connection();
        needInit = false;
    }

    if (!(sWorld.getConfig(CONFIG_MYSQL_BUNDLE_CHARDB) & MYSQL_BUNDLE_RA))
    {
        CharacterDatabase.Init_MySQL_Connection();
        needInit = false;
    }

    if (needInit)
        MySQL::Thread_Init();

    sWorld.InitResultQueue();

    uint32 realCurrTime = 0;
    uint32 realPrevTime = getMSTime();

    uint32 prevSleepTime = 0;                               // used for balanced full tick time length near WORLD_SLEEP_CONST

    ///- While we have not World::m_stopEvent, update the world
    while (!World::IsStopped())
    {
        ++World::m_worldLoopCounter;
        realCurrTime = getMSTime();

        uint32 diff = getMSTimeDiff(realPrevTime,realCurrTime);

        sWorld.Update( diff );
        realPrevTime = realCurrTime;

        // diff (D0) include time of previous sleep (d0) + tick time (t0)
        // we want that next d1 + t1 == WORLD_SLEEP_CONST
        // we can't know next t1 and then can use (t0 + d1) == WORLD_SLEEP_CONST requirement
        // d1 = WORLD_SLEEP_CONST - t0 = WORLD_SLEEP_CONST - (D0 - d0) = WORLD_SLEEP_CONST + d0 - D0
        if (diff <= WORLD_SLEEP_CONST+prevSleepTime)
        {
            prevSleepTime = WORLD_SLEEP_CONST+prevSleepTime-diff;
            ZThread::Thread::sleep(prevSleepTime);
        }
        else
            prevSleepTime = 0;
    }

    sWorld.KickAll();                                       // save and kick all players
    sWorld.UpdateSessions( 1 );                             // real players unload required UpdateSessions call

    // unload battleground templates before different singletons destroyed
    sBattleGroundMgr.DeleteAlllBattleGrounds();

    sWorldSocketMgr->StopNetwork();

    MapManager::Instance().UnloadAll();                     // unload all grids (including locked in memory)

    ///- Free MySQL thread resources and deallocate lingering connections
    if (!(sWorld.getConfig(CONFIG_MYSQL_BUNDLE_WORLDDB) & MYSQL_BUNDLE_RA))
        WorldDatabase.End_MySQL_Connection();

    if (!(sWorld.getConfig(CONFIG_MYSQL_BUNDLE_LOGINDB) & MYSQL_BUNDLE_RA))
        LoginDatabase.End_MySQL_Connection();

    if (!(sWorld.getConfig(CONFIG_MYSQL_BUNDLE_CHARDB) & MYSQL_BUNDLE_RA))
        CharacterDatabase.End_MySQL_Connection();

    if (needInit)
        MySQL::Thread_End();
}

