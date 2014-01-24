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

#include <ace/OS_NS_signal.h>

#include "WorldSocketMgr.h"
#include "Common.h"
#include "Master.h"
#include "WorldSocket.h"
#include "WorldRunnable.h"
#include "World.h"
#include "Log.h"
#include "Timer.h"
#include "Policies/SingletonImp.h"
#include "SystemConfig.h"
#include "Config/ConfigEnv.h"
#include "Database/DatabaseEnv.h"
#include "CliRunnable.h"
#include "RASocket.h"
#include "ScriptCalls.h"
#include "Util.h"
#include "IRCMgr.h"

#include "sockets/TcpSocket.h"
#include "sockets/Utility.h"
#include "sockets/Parse.h"
#include "sockets/Socket.h"
#include "sockets/SocketHandler.h"
#include "sockets/ListenSocket.h"

#ifdef WIN32
#include "ServiceWin32.h"
extern int m_ServiceStatus;
#endif

/// \todo Warning disabling not useful under VC++2005. Can somebody say on which compiler it is useful?
#pragma warning(disable:4305)

INSTANTIATE_SINGLETON_1( Master );

volatile uint32 Master::m_masterLoopCounter = 0;

class FreezeDetectorRunnable : public ZThread::Runnable
{
public:
    FreezeDetectorRunnable() { _delaytime = 0; }
    uint32 m_loops, m_lastchange;
    uint32 w_loops, w_lastchange;
    uint32 _delaytime;
    void SetDelayTime(uint32 t) { _delaytime = t; }
    void run(void)
    {
        if(!_delaytime)
            return;
        sLog.outString("Starting up anti-freeze thread (%u seconds max stuck time)...",_delaytime/1000);
        m_loops = 0;
        w_loops = 0;
        m_lastchange = 0;
        w_lastchange = 0;
        while(!World::IsStopped())
        {
            ZThread::Thread::sleep(1000);
            uint32 curtime = getMSTime();
            //DEBUG_LOG("anti-freeze: time=%u, counters=[%u; %u]",curtime,Master::m_masterLoopCounter,World::m_worldLoopCounter);

            // There is no Master anymore
            // TODO: clear the rest of the code
//            // normal work
//            if(m_loops != Master::m_masterLoopCounter)
//            {
//                m_lastchange = curtime;
//                m_loops = Master::m_masterLoopCounter;
//            }
//            // possible freeze
//            else if(GetMSTimeDiff(m_lastchange,curtime) > _delaytime)
//            {
//                sLog.outError("Main/Sockets Thread hangs, kicking out server!");
//                *((uint32 volatile*)NULL) = 0;                       // bang crash
//            }

            // normal work
            if(w_loops != World::m_worldLoopCounter)
            {
                w_lastchange = curtime;
                w_loops = World::m_worldLoopCounter;
            }
            // possible freeze
            else if(GetMSTimeDiff(w_lastchange,curtime) > _delaytime)
            {
                sLog.outError("World Thread hangs, kicking out server!");
                *((uint32 volatile*)NULL) = 0;                       // bang crash
            }
        }
        sLog.outString("Anti-freeze thread exiting without problems.");
    }
};

class RARunnable : public ZThread::Runnable
{
public:
  uint32 numLoops, loopCounter;

  RARunnable ()
  {
    uint32 socketSelecttime = sWorld.getConfig (CONFIG_SOCKET_SELECTTIME);
    numLoops = (sConfig.GetIntDefault ("MaxPingTime", 30) * (MINUTE * 1000000 / socketSelecttime));
    loopCounter = 0;
  }

  void
  checkping ()
  {
    // ping if need
    if ((++loopCounter) == numLoops)
      {
        loopCounter = 0;
        sLog.outDetail ("Ping MySQL to keep connection alive");
        delete WorldDatabase.Query ("SELECT 1 FROM command LIMIT 1");
        delete LoginDatabase.Query ("SELECT 1 FROM realmlist LIMIT 1");
        delete CharacterDatabase.Query ("SELECT 1 FROM bugreport LIMIT 1");
      }
  }

  void
  run (void)
  {
    SocketHandler h;

    // Launch the RA listener socket
    ListenSocket<RASocket> RAListenSocket (h);
    bool usera = sConfig.GetBoolDefault ("Ra.Enable", false);

    if (usera)
      {
        port_t raport = sConfig.GetIntDefault ("Ra.Port", 3443);
        std::string stringip = sConfig.GetStringDefault ("Ra.IP", "0.0.0.0");
        ipaddr_t raip;
        if (!Utility::u2ip (stringip, raip))
          sLog.outError ("Trinity RA can not bind to ip %s", stringip.c_str ());
        else if (RAListenSocket.Bind (raip, raport))
          sLog.outError ("Trinity RA can not bind to port %d on %s", raport, stringip.c_str ());
        else
          {
            h.Add (&RAListenSocket);

            sLog.outString ("Starting Remote access listner on port %d on %s", raport, stringip.c_str ());
          }
      }

    // Socket Selet time is in microseconds , not miliseconds!!
    uint32 socketSelecttime = sWorld.getConfig (CONFIG_SOCKET_SELECTTIME);

    // if use ra spend time waiting for io, if not use ra ,just sleep
    if (usera)
      while (!World::IsStopped())
        {
          h.Select (0, socketSelecttime);
          checkping ();
        }
    else
      while (!World::IsStopped())
        {
          ZThread::Thread::sleep (static_cast<unsigned long> (socketSelecttime / 1000));
          checkping ();
        }
  }
};

Master::Master()
{
}

Master::~Master()
{
}

/// Main function
int Master::Run()
{
    sLog.outString( "%s (core-daemon)", _FULLVERSION );
    sLog.outString( "<Ctrl-C> to stop.\n" );

    sLog.outTitle(" __          ___           _                                  ");
    sLog.outTitle(" \\ \\        / (_)         | |");
    sLog.outTitle("  \\ \\  /\\  / / _ _ __   __| |_ __ _   _ _ __  _ __   ___ _ __ ");
    sLog.outTitle("   \\ \\/  \\/ / | | '_ \\ / _` | '__| | | | '_ \\| '_ \\ / _ \\ '__|");
    sLog.outTitle("    \\  /\\  /  | | | | | (_| | |  | |_| | | | | | | |  __/ |   ");
    sLog.outTitle("     \\/  \\/   |_|_| |_|\\__,_|_|   \\__,_|_| |_|_| |_|\\___|_|   ");
    sLog.outTitle("");
    sLog.outTitle("");


    /// worldd PID file creation
    std::string pidfile = sConfig.GetStringDefault("PidFile", "");
    if(!pidfile.empty())
    {
        uint32 pid = CreatePIDFile(pidfile);
        if( !pid )
        {
            sLog.outError( "Cannot create PID file %s.\n", pidfile.c_str() );
            return 1;
        }

        sLog.outString( "Daemon PID: %u\n", pid );
    }

    ///- Start the databases
    if (!_StartDB())
        return 1;

    ///- Initialize the World
    sWorld.SetInitialWorldSettings();

    ///- Catch termination signals
    _HookSignals();

    ///- Launch WorldRunnable thread
    ZThread::Thread t(new WorldRunnable);
    t.setPriority ((ZThread::Priority )2);

    // set server online
    LoginDatabase.PExecute("UPDATE realmlist SET color = 0, population = 0 WHERE id = '%d'",realmID);

#ifdef WIN32
    if (sConfig.GetBoolDefault("Console.Enable", true) && (m_ServiceStatus == -1)/* need disable console in service mode*/)
#else
    if (sConfig.GetBoolDefault("Console.Enable", true))
#endif
    {
        ///- Launch CliRunnable thread
        ZThread::Thread td1(new CliRunnable);
    }

    ZThread::Thread td2(new RARunnable);

    ///- Handle affinity for multiple processors and process priority on Windows
    #ifdef WIN32
    {
        HANDLE hProcess = GetCurrentProcess();

        uint32 Aff = sConfig.GetIntDefault("UseProcessors", 0);
        if(Aff > 0)
        {
            ULONG_PTR appAff;
            ULONG_PTR sysAff;

            if(GetProcessAffinityMask(hProcess,&appAff,&sysAff))
            {
                ULONG_PTR curAff = Aff & appAff;            // remove non accessible processors

                if(!curAff )
                {
                    sLog.outError("Processors marked in UseProcessors bitmask (hex) %x not accessible for Trinityd. Accessible processors bitmask (hex): %x",Aff,appAff);
                }
                else
                {
                    if(SetProcessAffinityMask(hProcess,curAff))
                        sLog.outString("Using processors (bitmask, hex): %x", curAff);
                    else
                        sLog.outError("Can't set used processors (hex): %x",curAff);
                }
            }
            sLog.outString();
        }

        bool Prio = sConfig.GetBoolDefault("ProcessPriority", false);

//        if(Prio && (m_ServiceStatus == -1)/* need set to default process priority class in service mode*/)
        if(Prio)
        {
            if(SetPriorityClass(hProcess,HIGH_PRIORITY_CLASS))
                sLog.outString("TrinityCore process priority class set to HIGH");
            else
                sLog.outError("ERROR: Can't set Trinityd process priority class.");
            sLog.outString();
        }
    }
    #endif

    uint32 realCurrTime, realPrevTime;
    realCurrTime = realPrevTime = getMSTime();

    uint32 socketSelecttime = sWorld.getConfig(CONFIG_SOCKET_SELECTTIME);

    // maximum counter for next ping
    uint32 numLoops = (sConfig.GetIntDefault( "MaxPingTime", 30 ) * (MINUTE * 1000000 / socketSelecttime));
    uint32 loopCounter = 0;

    ///- Start up freeze catcher thread
    uint32 freeze_delay = sConfig.GetIntDefault("MaxCoreStuckTime", 0);
    if(freeze_delay)
    {
        FreezeDetectorRunnable *fdr = new FreezeDetectorRunnable();
        fdr->SetDelayTime(freeze_delay*1000);
        ZThread::Thread t(fdr);
        t.setPriority(ZThread::High);
    }
    
    ///- Start up the IRC client
    if (sConfig.GetBoolDefault("IRC.Enabled", false))
        ACE_Based::Thread thwrchat(&sIRCMgr);
    
    ///- Launch the world listener socket
  port_t wsport = sWorld.getConfig (CONFIG_PORT_WORLD);
  std::string bind_ip = sConfig.GetStringDefault ("BindIP", "0.0.0.0");

  if (sWorldSocketMgr->StartNetwork (wsport, bind_ip.c_str ()) == -1)
    {
      sLog.outError ("Failed to start network");
      World::StopNow(ERROR_EXIT_CODE);
      // go down and shutdown the server
    }

    sWorldSocketMgr->Wait ();

    // set server offline
    LoginDatabase.PExecute("UPDATE realmlist SET color = 2 WHERE id = '%d'",realmID);

    ///- Remove signal handling before leaving
    _UnhookSignals();

    // when the main thread closes the singletons get unloaded
    // since worldrunnable uses them, it will crash if unloaded after master
    t.wait();
    td2.wait ();

    ///- Clean database before leaving
    clearOnlineAccounts();

    ///- Wait for delay threads to end
    CharacterDatabase.Close();
    WorldDatabase.Close();
    LoginDatabase.Close();

    sLog.outString( "Halting process..." );

    #ifdef WIN32
    if (sConfig.GetBoolDefault("Console.Enable", true))
    {
        // this only way to terminate CLI thread exist at Win32 (alt. way exist only in Windows Vista API)
        //_exit(1);
        // send keyboard input to safely unblock the CLI thread
        INPUT_RECORD b[5];
        HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
        b[0].EventType = KEY_EVENT;
        b[0].Event.KeyEvent.bKeyDown = TRUE;
        b[0].Event.KeyEvent.uChar.AsciiChar = 'X';
        b[0].Event.KeyEvent.wVirtualKeyCode = 'X';
        b[0].Event.KeyEvent.wRepeatCount = 1;

        b[1].EventType = KEY_EVENT;
        b[1].Event.KeyEvent.bKeyDown = FALSE;
        b[1].Event.KeyEvent.uChar.AsciiChar = 'X';
        b[1].Event.KeyEvent.wVirtualKeyCode = 'X';
        b[1].Event.KeyEvent.wRepeatCount = 1;

        b[2].EventType = KEY_EVENT;
        b[2].Event.KeyEvent.bKeyDown = TRUE;
        b[2].Event.KeyEvent.dwControlKeyState = 0;
        b[2].Event.KeyEvent.uChar.AsciiChar = '\r';
        b[2].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
        b[2].Event.KeyEvent.wRepeatCount = 1;
        b[2].Event.KeyEvent.wVirtualScanCode = 0x1c;

        b[3].EventType = KEY_EVENT;
        b[3].Event.KeyEvent.bKeyDown = FALSE;
        b[3].Event.KeyEvent.dwControlKeyState = 0;
        b[3].Event.KeyEvent.uChar.AsciiChar = '\r';
        b[3].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
        b[3].Event.KeyEvent.wVirtualScanCode = 0x1c;
        b[3].Event.KeyEvent.wRepeatCount = 1;
        DWORD numb;
        BOOL ret = WriteConsoleInput(hStdIn, b, 4, &numb);
    }
    #endif

    // Exit the process with specified return value
    return World::GetExitCode();
}

/// Initialize connection to the databases
bool Master::_StartDB()
{
    uint8 num_threads;
    
    std::string dbstring;
    if(!sConfig.GetString("WorldDatabaseInfo", &dbstring))
    {
        sLog.outError("Database not specified in configuration file");
        return false;
    }
    sLog.outDetail("World Database: %s", dbstring.c_str());

    num_threads = sConfig.GetIntDefault("WorldDatabase.WorkerThreads", 1);
    if (num_threads < 1 || num_threads > 32) {
        sLog.outError("World database: invalid number of worker threads specified. "
            "Please pick a value between 1 and 32.");
        return false;
    }
    
    ///- Initialise the world database
    if (!WorldDatabase.Open(dbstring, num_threads)) {
        sLog.outError("Cannot connect to world database %s", dbstring.c_str());
        return false;
    }

    if(!sConfig.GetString("CharacterDatabaseInfo", &dbstring))
    {
        sLog.outError("Character Database not specified in configuration file");
        return false;
    }
    sLog.outDetail("Character Database: %s", dbstring.c_str());

    num_threads = sConfig.GetIntDefault("CharacterDatabase.WorkerThreads", 1);
    if (num_threads < 1 || num_threads > 32)
    {
        sLog.outError("Character database: invalid number of worker threads specified. "
            "Please pick a value between 1 and 32.");
        return false;
    }

    ///- Initialise the Character database
    if (!CharacterDatabase.Open(dbstring, num_threads))
    {
        sLog.outError("Cannot connect to Character database %s", dbstring.c_str());
        return false;
    }

    ///- Get login database info from configuration file
    if(!sConfig.GetString("LoginDatabaseInfo", &dbstring))
    {
        sLog.outError("Login database not specified in configuration file");
        return false;
    }

    ///- Initialise the login database
    sLog.outDetail("Login Database: %s", dbstring.c_str() );
    num_threads = sConfig.GetIntDefault("LoginDatabase.WorkerThreads", 1);
    if (num_threads < 1 || num_threads > 32) {
        sLog.outError("Login database: invalid number of worker threads specified. "
            "Please pick a value between 1 and 32.");
        return false;
    }
    
    ///- Initialise the login database
    if (!LoginDatabase.Open(dbstring, num_threads)) {
        sLog.outError("Cannot connect to login database %s", dbstring.c_str());
        return false;
    }
    
    ///- Get logs database info from configuration file
    if(!sConfig.GetString("LogsDatabaseInfo", &dbstring))
    {
        sLog.outError("Logs database not specified in configuration file");
        return false;
    }
    
    num_threads = sConfig.GetIntDefault("WorldDatabase.WorkerThreads", 1);
    if (num_threads < 1 || num_threads > 32) {
        sLog.outError("World database: invalid number of worker threads specified. "
            "Please pick a value between 1 and 32.");
        return false;
    }
    sLog.outDetail("Logs Database: %s", dbstring.c_str());
    
    ///- Initialise the logs database
    if (!LogsDatabase.Open(dbstring, num_threads)) {
        sLog.outError("Cannot connect to logs database %s", dbstring.c_str());
        return false;
    }

    ///- Get the realm Id from the configuration file
    realmID = sConfig.GetIntDefault("RealmID", 0);
    if(!realmID)
    {
        sLog.outError("Realm ID not defined in configuration file");
        return false;
    }
    sLog.outString("Realm running as realm ID %d", realmID);

    ///- Clean the database before starting
    clearOnlineAccounts();

    ///- Insert version info into DB
    WorldDatabase.PExecute("UPDATE `version` SET `core_version` = '%s', `core_revision` = '%s'", _FULLVERSION, _REVISION);

    sWorld.LoadDBVersion();

    sLog.outString("Using %s", sWorld.GetDBVersion());
    return true;
}

/// Clear 'online' status for all accounts with characters in this realm
void Master::clearOnlineAccounts()
{
    // Cleanup online status for characters hosted at current realm
    /// \todo Only accounts with characters logged on *this* realm should have online status reset. Move the online column from 'account' to 'realmcharacters'?
    LoginDatabase.PExecute(
        "UPDATE account SET online = 0 WHERE online > 0 "
        "AND id IN (SELECT acctid FROM realmcharacters WHERE realmid = '%d')",realmID);


    CharacterDatabase.Execute("UPDATE characters SET online = 0 WHERE online<>0");
}

/// Handle termination signals
void Master::_OnSignal(int s)
{
    switch (s)
    {
        case SIGINT:
            World::StopNow(RESTART_EXIT_CODE);
            break;
        case SIGTERM:
        #ifdef _WIN32
        case SIGBREAK:
        #endif
            World::StopNow(SHUTDOWN_EXIT_CODE);
            break;
    }

    signal(s, _OnSignal);
}

/// Define hook '_OnSignal' for all termination signals
void Master::_HookSignals()
{
    signal(SIGINT, _OnSignal);
    signal(SIGTERM, _OnSignal);
    #ifdef _WIN32
    signal(SIGBREAK, _OnSignal);
    #endif
}

/// Unhook the signals before leaving
void Master::_UnhookSignals()
{
    signal(SIGINT, 0);
    signal(SIGTERM, 0);
    #ifdef _WIN32
    signal(SIGBREAK, 0);
    #endif
}

