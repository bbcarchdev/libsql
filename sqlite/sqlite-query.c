/* Author: Mo McRoberts <mo.mcroberts@bbc.co.uk>
 *
 * Copyright 2014-2017 BBC.
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

int
sql_sqlite_execute_(SQL *restrict me, const char *restrict statement, void *restrict *restrict resultdata)
{
	int r;
	sqlite3_stmt *stmt;

	if(me->depth && me->deadlocked)
	{
		/* If we're already deadlocked mid-transaction, there's no point in
		 * doing anything
		 */
		return -1;
	}
	me->deadlocked = 0;
	if(me->querylog)
	{
		me->querylog(me, statement);
	}
	stmt = NULL;
	r = sqlite3_prepare_v2(me->sqlite, statement, strlen(statement), &stmt, NULL);
	if(r != SQLITE_OK)
	{		
		sql_sqlite_copy_error_(me);
		return -1;
	}	
	if(resultdata)
	{
		*resultdata = stmt;
	}
	else
	{
		r = sqlite3_step(stmt);
		sqlite3_finalize(stmt);
		if(r != SQLITE_DONE && r != SQLITE_ROW)
		{
			return -1;
		}
	}
	return 0;
}

int
sql_sqlite_begin_(SQL *me, SQL_TXN_MODE mode)
{
	const char *st;
	
	(void) mode;

	if(me->depth)	
	{
		/* Can't nest transactions */
		sql_sqlite_set_error_(me, "25000", "You are not allowed to execute this command in a transaction");
		return -1;
	}
	st = "BEGIN TRANSACTION";
	if(me->querylog)
	{
		me->querylog(me, st);
	}
	if(sqlite3_exec(me->sqlite, st, NULL, NULL, NULL) != SQLITE_OK)
	{
		sql_sqlite_copy_error_(me);
		return -1;
	}
	me->depth++;
	return 0;
}

int
sql_sqlite_commit_(SQL *me)
{
	const char *st = "COMMIT";
	
	if(!me->depth)
	{
		return 0;
	}
	if(me->deadlocked)
	{
		return -1;
	}
	if(me->querylog)
	{
		me->querylog(me, st);
	}
	if(sqlite3_exec(me->sqlite, st, NULL, NULL, NULL) != SQLITE_OK)
	{
		sql_sqlite_copy_error_(me);
		return -1;
	}
	me->depth--;
	return 0;
}

int
sql_sqlite_rollback_(SQL *me)	
{
	const char *st = "ROLLBACK";
	
	if(!me->depth)
	{
		return 0;
	}
	if(me->querylog)
	{
		me->querylog(me, st);
	}
	if(sqlite3_exec(me->sqlite, st, NULL, NULL, NULL) != SQLITE_OK)
	{
		sql_sqlite_copy_error_(me);
		if(me->deadlocked)
		{
			/* It doesn't matter if the rollback failed */
			me->depth--;
			me->deadlocked = 0;
			return 0;
		}
		return -1;
	}
	me->depth--;
	return 0;
}

int
sql_sqlite_deadlocked_(SQL *me)
{
	return me->deadlocked;
}
