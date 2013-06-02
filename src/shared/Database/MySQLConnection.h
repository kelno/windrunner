/*
 * Copyright (C) 2008-2010 Trinity <http://www.trinitycore.org/>
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

#ifndef _MYSQLCONNECTION_H
#define _MYSQLCONNECTION_H

#include <ace/Activation_Queue.h>
#include <mysql.h>
#include <string>

#include "Platform/Define.h"

//#include "DatabaseWorkerPool.h"

class DatabaseWorker;
class QueryResult;

class MySQLConnection
{
    friend class DatabaseWorkerPool;

    public:
        MySQLConnection();                                  //! Constructor for synchroneous connections.
        MySQLConnection(ACE_Activation_Queue* queue);       //! Constructor for asynchroneous connections.
        ~MySQLConnection();

        bool Open(const std::string& infoString);           //! Connection details.

    public:
        bool Execute(const char* sql);
        QueryResult* Query(const char* sql);
        bool _Query(const char *sql, MYSQL_RES **pResult, MYSQL_FIELD **pFields, uint64* pRowCount, uint32* pFieldCount);

        void BeginTransaction();
        void RollbackTransaction();
        void CommitTransaction();

        operator bool () const { return m_Mysql != NULL; }

    protected:
        MYSQL* GetHandle()  { return m_Mysql; }

    private:
        ACE_Activation_Queue* m_queue;                      //! Queue shared with other asynchroneous connections.
        DatabaseWorker*       m_worker;                     //! Core worker task.
        MYSQL *               m_Mysql;                      //! MySQL Handle.
        ACE_Thread_Mutex      m_Mutex;
};

#endif

