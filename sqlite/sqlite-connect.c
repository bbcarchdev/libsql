/* Author: Mo McRoberts <mo.mcroberts@bbc.co.uk>
 *
 * Copyright 2017 BBC
 */


/*
 * Copyright 2012-2013 Mo McRoberts.
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

#include "p_sqlite.h"

/* TODO:
 *   percent-decode components
 *   connection options
 */
int
sql_sqlite_connect_(SQL *me, URI *uri)
{
	URI_INFO *info;
	int r;

	info = uri_info(uri);
	if(!info)
	{
		return -1;
	}
	if(!info->path)
	{
		uri_info_destroy(info);
		sql_sqlite_set_error_(me, "X000", "No database path provided in connection URI");
		return -1;
	}
	r = sqlite3_open_v2(info->path, &(me->sqlite), SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE, NULL);
	uri_info_destroy(info);
	if(r != SQLITE_OK)
	{
		if(!me->sqlite)
		{
			sql_sqlite_set_errcode_(me, r);
		}
		else
		{
			sql_sqlite_copy_error_(me);
		}
		return -1;
	}
	return 0;
}

const char *
sql_sqlite_sqlstate_(SQL *me)
{
	return me->sqlstate;
}

const char *
sql_sqlite_error_(SQL *me)
{
	return me->error;
}

void
sql_sqlite_set_error_(SQL *restrict me, const char *restrict sqlstate, const char *restrict message)
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
sql_sqlite_copy_error_(SQL *me)
{
	sql_sqlite_set_errcode_(me, sqlite3_extended_errcode(me->sqlite));
}

void
sql_sqlite_set_errcode_(SQL *me, int errcode)
{
	char sqlstate[32];
	
	snprintf(sqlstate, 31, "Z%03d", errcode);
	sql_sqlite_set_error_(me, sqlstate, sqlite3_errstr(errcode));
}

size_t
sql_sqlite_escape_(SQL *restrict me, const unsigned char *restrict from, size_t length, char *restrict buf, size_t buflen)
{
	size_t needed;
	char *bp;

	(void) me;

	needed = (length * 2) + 1;
	if(buflen < needed || !buf)
	{
		if(buf)
		{
			*buf = 0;
		}
		return needed;
	}
	memset(buf, 0, buflen);
	for(bp = buf; *from && length && buflen > 1;)
	{
		if(*from == '\'')
		{
			if(buflen > 2)
			{
				*bp = *from;
				bp++;
				buflen--;
			}
			else
			{
				break;
			}
		}
		*bp = *from;
		bp++;
		buflen--;
		from++;
		length--;
	}
	*bp = 0;
	return (bp - buf) + 1;
}

int
sql_sqlite_set_querylog_(SQL *sql, SQL_LOG_QUERY fn)
{
	sql->querylog = fn;
	return 0;
}

int
sql_sqlite_set_errorlog_(SQL *sql, SQL_LOG_ERROR fn)
{
	sql->errorlog = fn;
	return 0;
}

int
sql_sqlite_set_noticelog_(SQL *sql, SQL_LOG_NOTICE fn)
{
	(void) sql;
	(void) fn;

	return 0;
}

SQL_LANG
sql_sqlite_lang_(SQL *sql)
{
	(void) sql;

	return SQL_LANG_SQL;
}

SQL_VARIANT
sql_sqlite_variant_(SQL *sql)
{
	(void) sql;

	return SQL_VARIANT_SQLITE;
}
