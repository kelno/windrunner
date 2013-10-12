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

#include "SqlOperations.h"
#include "SqlDelayThread.h"
#include "DatabaseEnv.h"
#include "DatabaseImpl.h"

/// ---- ASYNC STATEMENTS / TRANSACTIONS ----

void SqlStatement::Execute(Database *db)
{
    /// just do it
    db->DirectExecute(m_sql);
}

void SqlTransaction::Execute(Database *db)
{
    if(m_queue.empty())
        return;
    db->DirectExecute("START TRANSACTION");
    while(!m_queue.empty())
    {
        char const *sql = m_queue.front();
        m_queue.pop();

        if(!db->DirectExecute(sql))
        {
            free((void*)const_cast<char*>(sql));
            db->DirectExecute("ROLLBACK");
            while(!m_queue.empty())
            {
                free((void*)const_cast<char*>(m_queue.front()));
                m_queue.pop();
            }
            return;
        }

        free((void*)const_cast<char*>(sql));
    }
    db->DirectExecute("COMMIT");
}

/// ---- ASYNC QUERIES ----

void SqlQuery::Execute(Database *db)
{
    if(!m_callback || !m_queue)
        return;
    /// execute the query and store the result in the callback
    m_callback->SetResult(db->Query(m_sql));
    /// add the callback to the sql result queue of the thread it originated from
    m_queue->add(m_callback);
}

void SqlResultQueue::Update()
{
    /// execute the callbacks waiting in the synchronization queue
    while(!empty())
    {
        Trinity::IQueryCallback * callback = next();
        callback->Execute();
        delete callback;
    }
}

bool SqlQueryHolder::Execute(Trinity::IQueryCallback * callback, SqlDelayThread *thread, SqlResultQueue *queue)
{
    if(!callback || !thread || !queue)
        return false;

    /// delay the execution of the queries, sync them with the delay thread
    /// which will in turn resync on execution (via the queue) and call back
    SqlQueryHolderEx *holderEx = new SqlQueryHolderEx(this, callback, queue);
    thread->Delay(holderEx);
    return true;
}

bool SqlQueryHolder::SetQuery(size_t index, const char *sql)
{
    if(m_queries.size() <= index)
    {
        sLog.outError("Query index (%u) out of range (size: %u) for query: %s",index,m_queries.size(),sql);
        return false;
    }

    if(m_queries[index].first != NULL)
    {
        sLog.outError("Attempt assign query to holder index (%u) where other query stored (Old: [%s] New: [%s])",
            index,m_queries[index].first,sql);
        return false;
    }

    /// not executed yet, just stored (it's not called a holder for nothing)
    m_queries[index] = SqlResultPair(strdup(sql), NULL);
    return true;
}

bool SqlQueryHolder::SetPQuery(size_t index, const char *format, ...)
{
    if(!format)
    {
        sLog.outError("Query (index: %u) is empty.",index);
        return false;
    }

    va_list ap;
    char szQuery [MAX_QUERY_LEN];
    va_start(ap, format);
    int res = vsnprintf( szQuery, MAX_QUERY_LEN, format, ap );
    va_end(ap);

    if(res==-1)
    {
        sLog.outError("SQL Query truncated (and not execute) for format: %s",format);
        return false;
    }

    return SetQuery(index,szQuery);
}

QueryResult* SqlQueryHolder::GetResult(size_t index)
{
    if(index < m_queries.size())
    {
        /// the query strings are freed on the first GetResult or in the destructor
        if(m_queries[index].first != NULL)
        {
            free((void*)(const_cast<char*>(m_queries[index].first)));
            m_queries[index].first = NULL;
        }
        /// when you get a result aways remember to delete it!
        return m_queries[index].second;
    }
    else
        return NULL;
}

void SqlQueryHolder::SetResult(size_t index, QueryResult *result)
{
    /// store the result in the holder
    if(index < m_queries.size())
        m_queries[index].second = result;
}

SqlQueryHolder::~SqlQueryHolder()
{
    for(size_t i = 0; i < m_queries.size(); i++)
    {
        /// if the result was never used, free the resources
        /// results used already (getresult called) are expected to be deleted
        if(m_queries[i].first != NULL)
        {
            free((void*)(const_cast<char*>(m_queries[i].first)));
            if(m_queries[i].second)
                delete m_queries[i].second;
        }
    }
}

void SqlQueryHolder::SetSize(size_t size)
{
    /// to optimize push_back, reserve the number of queries about to be executed
    m_queries.resize(size);
}

void SqlQueryHolderEx::Execute(Database *db)
{
    if(!m_holder || !m_callback || !m_queue)
        return;

    /// we can do this, we are friends
    std::vector<SqlQueryHolder::SqlResultPair> &queries = m_holder->m_queries;

    for(size_t i = 0; i < queries.size(); i++)
    {
        /// execute all queries in the holder and pass the results
        char const *sql = queries[i].first;
        if(sql) m_holder->SetResult(i, db->Query(sql));
    }

    /// sync with the caller thread
    m_queue->add(m_callback);
}

