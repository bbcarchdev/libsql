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

#include "p_mysql.h"

/* TODO:
 *   percent-decode components
 *   connection options
 */
int
sql_mysql_connect_(SQL *me, URI *uri)
{
	URI_INFO *info;
	MYSQL *res;
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
	res = mysql_real_connect(&(me->mysql), info->host, info->auth, pw, db, info->port, NULL, flags);
	uri_info_destroy(info);
	if(!res)
	{
		sql_mysql_copy_error_(me);
		return -1;
	}
	mysql_set_character_set(&(me->mysql), "utf");
	sql_mysql_execute_(me, "SET sql_mode='ANSI_QUOTES,IGNORE_SPACE,PIPES_AS_CONCAT'", NULL);
	sql_mysql_execute_(me, "SET storage_engine='InnoDB'", NULL);
	sql_mysql_execute_(me, "SET time_zone='+00:00'", NULL);
	return 0;
}

const char *
sql_mysql_sqlstate_(SQL *me)
{
	return me->sqlstate;
}

const char *
sql_mysql_error_(SQL *me)
{
	return me->error;
}

void
sql_mysql_set_error_(SQL *restrict me, const char *restrict sqlstate, const char *restrict message)
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
sql_mysql_copy_error_(SQL *me)
{
	const char *sqlstate;
	const char *err;
	int e;
	
	e = mysql_errno(&(me->mysql));
	if(e == 1205 || e == 1213 || e == 1478 || e == 1479)
	{
		me->deadlocked = 1;
	}
	sqlstate = mysql_sqlstate(&(me->mysql));
	err = mysql_error(&(me->mysql));	
	sql_mysql_set_error_(me, sqlstate, err);
}

size_t
sql_mysql_escape_(SQL *restrict me, const unsigned char *restrict from, size_t length, char *restrict buf, size_t buflen)
{
	size_t needed;
	
	needed = (length * 2) + 1;
	if(buflen < needed || !buf)
	{
		if(buf)
		{
			*buf = 0;
		}
		return needed;
	}
	return mysql_real_escape_string(&(me->mysql), buf, (const char *) from, length) + 1;
}

int
sql_mysql_set_querylog_(SQL *sql, SQL_LOG_QUERY fn)
{
	sql->querylog = fn;
	return 0;
}

int
sql_mysql_set_errorlog_(SQL *sql, SQL_LOG_ERROR fn)
{
	sql->errorlog = fn;
	return 0;
}

int
sql_mysql_set_noticelog_(SQL *sql, SQL_LOG_NOTICE fn)
{
	(void) sql;
	(void) fn;

	return 0;
}

SQL_LANG
sql_mysql_lang_(SQL *sql)
{
	(void) sql;

	return SQL_LANG_SQL;
}

SQL_VARIANT
sql_mysql_variant_(SQL *sql)
{
	(void) sql;

	return SQL_VARIANT_MYSQL;
}
