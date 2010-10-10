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

#include "AccountMgr.h"
#include "Database/DatabaseEnv.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Policies/SingletonImp.h"
#include "Util.h"

#ifdef DO_POSTGRESQL
extern DatabasePostgre LoginDatabase;
#else
extern DatabaseMysql LoginDatabase;
#endif

INSTANTIATE_SINGLETON_1(AccountMgr);

AccountMgr::AccountMgr()
{}

AccountMgr::~AccountMgr()
{}

AccountOpResult AccountMgr::CreateAccount(std::string username, std::string password)
{
    if(utf8length(username) > MAX_ACCOUNT_STR)
        return AOR_NAME_TOO_LONG;                           // username's too long

    normilizeString(username);
    normilizeString(password);

    LoginDatabase.escape_string(username);
    LoginDatabase.escape_string(password);

    QueryResult *result = LoginDatabase.PQuery("SELECT 1 FROM account WHERE username = '%s'", username.c_str());
    if(result)
    {
        delete result;
        return AOR_NAME_ALREDY_EXIST;                       // username does already exist
    }

    if(!LoginDatabase.PExecute("INSERT INTO account(username,sha_pass_hash,joindate,expansion) VALUES('%s',SHA1(CONCAT('%s',':','%s')),NOW(),%u)", username.c_str(), username.c_str(), password.c_str(), 8))
        return AOR_DB_INTERNAL_ERROR;                       // unexpected error
    LoginDatabase.Execute("INSERT INTO realmcharacters (realmid, acctid, numchars) SELECT realmlist.id, account.id, 0 FROM realmlist,account LEFT JOIN realmcharacters ON acctid=account.id WHERE acctid IS NULL");

    return AOR_OK;                                          // everything's fine
}

AccountOpResult AccountMgr::DeleteAccount(uint32 accid)
{
    QueryResult *result = LoginDatabase.PQuery("SELECT 1 FROM account WHERE id='%d'", accid);
    if(!result)
        return AOR_NAME_NOT_EXIST;                          // account doesn't exist
    delete result;

    result = CharacterDatabase.PQuery("SELECT guid FROM characters WHERE account='%d'",accid);
    if (result)
    {
        do
        {
            Field *fields = result->Fetch();
            uint32 guidlo = fields[0].GetUInt32();
            uint64 guid = MAKE_NEW_GUID(guidlo, 0, HIGHGUID_PLAYER);

            // kick if player currently
            if(Player* p = ObjectAccessor::FindPlayer(guid))
            {
                WorldSession* s = p->GetSession();
                s->KickPlayer();                            // mark session to remove at next session list update
                s->LogoutPlayer(false);                     // logout player without waiting next session list update
            }

            Player::DeleteFromDB(guid, accid, false);       // no need to update realm characters
        } while (result->NextRow());

        delete result;
    }

    // table realm specific but common for all characters of account for realm
    CharacterDatabase.PExecute("DELETE FROM character_tutorial WHERE account = '%u'",accid);

    LoginDatabase.BeginTransaction();

    bool res =
        LoginDatabase.PExecute("DELETE FROM account WHERE id='%d'", accid) &&
        LoginDatabase.PExecute("DELETE FROM realmcharacters WHERE acctid='%d'", accid);

    LoginDatabase.CommitTransaction();

    if(!res)
        return AOR_DB_INTERNAL_ERROR;                       // unexpected error;

    return AOR_OK;
}

AccountOpResult AccountMgr::ChangeUsername(uint32 accid, std::string new_uname, std::string new_passwd)
{
    QueryResult *result = LoginDatabase.PQuery("SELECT 1 FROM account WHERE id='%d'", accid);
    if(!result)
        return AOR_NAME_NOT_EXIST;                          // account doesn't exist
    delete result;

    if(utf8length(new_uname) > MAX_ACCOUNT_STR)
        return AOR_NAME_TOO_LONG;

    if(utf8length(new_passwd) > MAX_ACCOUNT_STR)
        return AOR_PASS_TOO_LONG;

    normilizeString(new_uname);
    normilizeString(new_passwd);

    LoginDatabase.escape_string(new_uname);
    LoginDatabase.escape_string(new_passwd);
    if(!LoginDatabase.PExecute("UPDATE account SET username='%s',sha_pass_hash=SHA1(CONCAT('%s',':','%s')) WHERE id='%d'", new_uname.c_str(), new_uname.c_str(), new_passwd.c_str(), accid))
        return AOR_DB_INTERNAL_ERROR;                       // unexpected error

    return AOR_OK;
}

AccountOpResult AccountMgr::ChangePassword(uint32 accid, std::string new_passwd)
{
    QueryResult *result = LoginDatabase.PQuery("SELECT 1 FROM account WHERE id='%d'", accid);
    if(!result)
        return AOR_NAME_NOT_EXIST;                          // account doesn't exist
    delete result;

    if (utf8length(new_passwd) > MAX_ACCOUNT_STR)
        return AOR_PASS_TOO_LONG;

    normilizeString(new_passwd);

    LoginDatabase.escape_string(new_passwd);
    if(!LoginDatabase.PExecute("UPDATE account SET sha_pass_hash=SHA1(CONCAT(username,':','%s')) WHERE id='%d'", new_passwd.c_str(), accid))
        return AOR_DB_INTERNAL_ERROR;                       // unexpected error

    return AOR_OK;
}

uint32 AccountMgr::GetId(std::string username)
{
    LoginDatabase.escape_string(username);
    QueryResult *result = LoginDatabase.PQuery("SELECT id FROM account WHERE username = '%s'", username.c_str());
    if(!result)
        return 0;
    else
    {
        uint32 id = (*result)[0].GetUInt32();
        delete result;
        return id;
    }
}

uint32 AccountMgr::GetSecurity(uint32 acc_id)
{
    QueryResult *result = LoginDatabase.PQuery("SELECT gmlevel FROM account WHERE id = '%u'", acc_id);
    if(result)
    {
        uint32 sec = (*result)[0].GetUInt32();
        delete result;
        return sec;
    }

    return 0;
}

bool AccountMgr::GetName(uint32 acc_id, std::string &name)
{
    QueryResult *result = LoginDatabase.PQuery("SELECT username FROM account WHERE id = '%u'", acc_id);
    if(result)
    {
        name = (*result)[0].GetCppString();
        delete result;
        return true;
    }

    return false;
}

bool AccountMgr::CheckPassword(uint32 accid, std::string passwd)
{
    normilizeString(passwd);
    LoginDatabase.escape_string(passwd);

    QueryResult *result = LoginDatabase.PQuery("SELECT 1 FROM account WHERE id='%d' AND sha_pass_hash=SHA1(CONCAT(username,':','%s'))", accid, passwd.c_str());
    if (result)
    {
        delete result;
        return true;
    }

    return false;
}

bool AccountMgr::normilizeString(std::string& utf8str)
{
    wchar_t wstr_buf[MAX_ACCOUNT_STR+1];

    size_t wstr_len = MAX_ACCOUNT_STR;
    if(!Utf8toWStr(utf8str,wstr_buf,wstr_len))
        return false;

    std::transform( &wstr_buf[0], wstr_buf+wstr_len, &wstr_buf[0], wcharToUpperOnlyLatin );

    return WStrToUtf8(wstr_buf,wstr_len,utf8str);
}

