/* Author: Mo McRoberts <mo.mcroberts@bbc.co.uk>
 *
 * Copyright 2015 BBC.
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

#include "p_postgres.h"

int
sql_pg_execute_(SQL *restrict me, const char *restrict statement, void *restrict *restrict resultdata)
{
	PGresult *res;
	ExecStatusType status;

	if(me->depth && me->deadlocked)
	{
		/* If we're already deadlocked mid-transaction, there's no point in
		 * doing anything
		 */
		return -1;
	}
	if(me->deadlocked)
	{
		PQreset(me->pg);
		me->deadlocked = 0;
	}
	if(me->querylog)
	{
		me->querylog(me, statement);
	}
	res = PQexec(me->pg, statement);
	status = PQresultStatus(res);
	if(!PQSTATUS_SUCCESS(status))
	{
		sql_pg_copy_error_(me, res);
		return -1;
	}
	if(resultdata)
	{
		*resultdata = NULL;
		if(status == PGRES_TUPLES_OK)
		{
			*resultdata = res;
		}
		else
		{
			PQclear(res);
		}
	}
	else
	{
		PQclear(res);
	}
	return 0;
}

int
sql_pg_begin_(SQL *me, SQL_TXN_MODE mode)
{
	const char *st;
	PGresult *res;
	ExecStatusType status;
	
	if(me->depth)	
	{
		/* Can't nest transactions */
		sql_pg_set_error_(me, "25000", "You are not allowed to execute this command in a transaction");
		return -1;
	}
	if(me->deadlocked)
	{
		PQreset(me->pg);
		me->deadlocked = 0;
	}
	switch(mode)
	{
	case SQL_TXN_CONSISTENT:
		st = "START TRANSACTION ISOLATION LEVEL REPEATABLE READ";
		break;
	default:
		st = "START TRANSACTION";
		break;
	}
	if(me->querylog)
	{
		me->querylog(me, st);
	}
	res = PQexec(me->pg, st);
	status = PQresultStatus(res);
	if(!PQSTATUS_SUCCESS(status))
	{
		sql_pg_copy_error_(me, res);
		return -1;
	}
	PQclear(res);
	me->depth++;
	return 0;
}

int
sql_pg_commit_(SQL *me)
{
	const char *st = "COMMIT";
	PGresult *res;
	ExecStatusType status;
	
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
	res = PQexec(me->pg, st);
	status = PQresultStatus(res);
	if(!PQSTATUS_SUCCESS(status))
	{
		sql_pg_copy_error_(me, res);
		return -1;
	}
	PQclear(res);
	me->depth--;
	return 0;
}

int
sql_pg_rollback_(SQL *me)	
{
	const char *st = "ROLLBACK";
	PGresult *res;
	ExecStatusType status;
	
	if(!me->depth)
	{
		return 0;
	}
	if(me->querylog)
	{
		me->querylog(me, st);
	}
	res = PQexec(me->pg, st);
	status = PQresultStatus(res);
	if(!PQSTATUS_SUCCESS(status))
	{
		sql_pg_copy_error_(me, res);
		return -1;
	}
	PQclear(res);
	me->depth--;
	return 0;
}

int
sql_pg_deadlocked_(SQL *me)
{
	return me->deadlocked;
}
