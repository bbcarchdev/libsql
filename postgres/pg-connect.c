/* Author: Mo McRoberts <mo.mcroberts@bbc.co.uk>
 *
 * Copyright (c) 2015 BBC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "p_postgres.h"

/* TODO:
 *   percent-decode components
 *   connection options
 */
int
sql_pg_connect_(SQL *me, URI *uri)
{
	URI_INFO *info;
	ConnStatusType status;
	char *pw, *db;
	unsigned long flags;

	info = uri_info(uri);
	if(!info)
	{
		return -1;
	}
	pw = NULL;
	if(info->auth)
	{
		pw = strchr(info->auth, ':');
		if(pw)
		{
			*pw = 0;
			pw++;
		}
	}
	db = NULL;
	if(info->path)
	{
		db = info->path;
		while(*db == '/')
		{
			db++;
		}
		if(!*db)
		{
			db = NULL;
		}
	}
	flags = 0;	
	me->pg = PQsetdbLogin(info->host, NULL, NULL, NULL, db, info->auth, pw);
	uri_info_destroy(info);
	if(!me->pg)
	{
		strcpy(me->sqlstate, "58000");
		strcpy(me->error, "Memory allocation error");
		return -1;
	}
	status = PQstatus(me->pg);
	if(status != CONNECTION_OK)
	{
		sql_pg_set_error_(me, "57P03", PQerrorMessage(me->pg));
		return -1;
	}
	PQsetClientEncoding(me->pg, "UTF8");
	
	return 0;
}

const char *
sql_pg_sqlstate_(SQL *me)
{
	return me->sqlstate;
}

const char *
sql_pg_error_(SQL *me)
{
	return me->error;
}

void
sql_pg_set_error_(SQL *restrict me, const char *restrict sqlstate, const char *restrict message)
{
	strncpy(me->sqlstate, sqlstate, sizeof(me->sqlstate) - 1);
	if(!message)
	{
		message = sqlstate;
	}
	strncpy(me->error, message, sizeof(me->error) - 1);
	if(me->errorlog)
	{
		me->errorlog(me, me->sqlstate, me->error);
	}
}

void
sql_pg_copy_error_(SQL *restrict me, PGresult *restrict result)
{
	const char *sqlstate;
	const char *err;
	
	sqlstate = PQresultErrorField(result, PG_DIAG_SQLSTATE);
	if(!strcmp(sqlstate, "40001") || !strcmp(sqlstate, "40P01"))
	{
		me->deadlocked = 1;
		PQreset(me->pg);
	}
	err = PQerrorMessage(me->pg);
	sql_pg_set_error_(me, sqlstate, err);
}

size_t
sql_pg_escape_(SQL *restrict me, const unsigned char *restrict from, size_t length, char *restrict buf, size_t buflen)
{
	size_t needed;
	int err;   
	
	needed = (length * 2) + 1;
	if(buflen < needed || !buf)
	{
		if(buf)
		{
			*buf = 0;
		}
		return needed;
	}
	err = 0;
	return PQescapeStringConn(me->pg, (char *) buf, (const char *) from, length, NULL) + 1;
}

int
sql_pg_set_querylog_(SQL *sql, SQL_LOG_QUERY fn)
{
	sql->querylog = fn;
	return 0;
}

int
sql_pg_set_errorlog_(SQL *sql, SQL_LOG_ERROR fn)
{
	sql->errorlog = fn;
	return 0;
}
