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

#include "p_libsql.h"


/* Execute a statement not expected to return a result-set */
int
sql_execute(SQL *restrict sql, const char *restrict statement)
{
	int r;

	r = sql->api->execute(sql, statement, NULL);

	return r;
}

/* Execute a statement, interpolating parameters */
int
sql_vexecutef(SQL *restrict sql, const char *restrict format, va_list ap)
{
	char *qs;
	int r;
	
	r = sql_vasprintf_query_(sql, &qs, format, ap);
	if(r == -1)
	{
		return -1;
	}
	r = sql->api->execute(sql, qs, NULL);
	free(qs);
	return r;
}

int
sql_executef(SQL *restrict sql, const char *restrict format, ...)
{
	va_list ap;
	int r;

	va_start(ap, format);
	r = sql_vexecutef(sql, format, ap);
	va_end(ap);
	return r;
}

/* Execute a statement which is expected to return a result-set */
SQL_STATEMENT *
sql_query(SQL *restrict sql, const char *restrict statement)
{
	SQL_STATEMENT *rs;
	void *data;
	int r;

	data = NULL;
	rs = sql->api->statement(sql, NULL);
	if(!rs)
	{
		return NULL;
	}
	r = sql->api->execute(sql, statement, &data);
	if(r)
	{
		rs->api->release(rs);
		return NULL;
	}
	rs->api->set_results(rs, data);
	return rs;
}

/* Execute a statement returning a result-set, interpolating parameters */
SQL_STATEMENT *
sql_vqueryf(SQL *restrict sql, const char *restrict format, va_list ap)
{
	char *qs;
	int r;	
	SQL_STATEMENT *rs;
	void *data;

	data = NULL;
	rs = sql->api->statement(sql, NULL);
	if(!rs)
	{
		return NULL;
	}
	r = sql_vasprintf_query_(sql, &qs, format, ap);
	if(r == -1)
	{
		rs->api->release(rs);
		return NULL;
	}
	r = sql->api->execute(sql, qs, &data);
	free(qs);
	if(r)
	{
		rs->api->release(rs);
		return NULL;
	}
	rs->api->set_results(rs, data);
	return rs;
}

SQL_STATEMENT *
sql_queryf(SQL *restrict sql, const char *restrict format, ...)
{
	va_list ap;
	SQL_STATEMENT *r;

	va_start(ap, format);
	r = sql_vqueryf(sql, format, ap);
	va_end(ap);
	return r;
}

int
sql_stmt_destroy(SQL_STATEMENT *stmt)
{
	return stmt->api->release(stmt);
}

int
sql_stmt_rewind(SQL_STATEMENT *stmt)
{
	return stmt->api->rewind(stmt);
}

unsigned int
sql_stmt_columns(SQL_STATEMENT *stmt)
{
	return stmt->api->columns(stmt);
}

unsigned long long
sql_stmt_rows(SQL_STATEMENT *stmt)
{
	return stmt->api->rows(stmt);
}

unsigned long long
sql_stmt_affected(SQL_STATEMENT *stmt)
{
	return stmt->api->affected(stmt);
}

int
sql_stmt_eof(SQL_STATEMENT *stmt)
{
	return stmt->api->eof(stmt);
}

int
sql_stmt_next(SQL_STATEMENT *stmt)
{
	return stmt->api->next(stmt);
}

int
sql_stmt_null(SQL_STATEMENT *stmt, unsigned int col)
{
	return stmt->api->null(stmt, col);
}

size_t
sql_stmt_value(SQL_STATEMENT *restrict stmt, unsigned int col, char *restrict buf, size_t buflen)
{
	return stmt->api->value(stmt, col, buf, buflen);
}

const char *
sql_stmt_str(SQL_STATEMENT *stmt, unsigned int col)
{
	return (const char *) stmt->api->valueptr(stmt, col);
}

long
sql_stmt_long(SQL_STATEMENT *stmt, unsigned int col)
{
	const char *s;

	s = (const char *) stmt->api->valueptr(stmt, col);
	if(!s)
	{
		return 0;
	}
	return strtol(s, NULL, 10);
}

unsigned long
sql_stmt_ulong(SQL_STATEMENT *stmt, unsigned int col)
{
	const char *s;

	s = (const char *) stmt->api->valueptr(stmt, col);
	if(!s)
	{
		return 0;
	}
	return strtoul(s, NULL, 10);
}

int
sql_stmt_execf(SQL_STATEMENT *statement, ...)
{
	va_list ap;
	int r;
	
	va_start(ap, statement);
	r = sql_stmt_vexecf(statement, ap);
	va_end(ap);
	return 0;
}

int
sql_stmt_vexecf(SQL_STATEMENT *stmt, va_list ap)
{
	const char *format;
	char *qs;
	SQL *sql;
	int r;
	void *data;
	
	sql = stmt->api->connection(stmt);
	format = stmt->api->statement(stmt);
	stmt->api->set_results(stmt, NULL);	
	r = sql_vasprintf_query_(sql, &qs, format, ap);
	if(r == -1)
	{
		return -1;
	}
	r = sql->api->execute(sql, qs, &data);
	free(qs);
	if(r)
	{
		return -1;
	}
	return stmt->api->set_results(stmt, data);
}

int
sql_begin(SQL *sql, SQL_TXN_MODE mode)
{
	return sql->api->begin(sql, mode);
}

int
sql_commit(SQL *sql)
{
	return sql->api->commit(sql);
}

int
sql_rollback(SQL *sql)
{
	return sql->api->rollback(sql);
}

/* Repeatedly execute a callback function in the context of a transaction. If
 * maxretries is less than zero, the callback will be invoked until it
 * indicates that the loop should end or a SQL error occurs.
 *
 * Possible return values from the callback:
 *
 *    SQL_TXN_ROLLBACK      Roll back and return success
 *    SQL_TXN_RETRY         Roll back and retry
 *    SQL_TXN_ABORT         Roll back and abort
 *    SQL_TXN_FAIL          Roll back and retry if a deadlock is detected,
 *                          otherwise abort
 *    SQL_TXN_COMMIT        Commit, retrying if a deadlock is detected
 *
 * As pseudocode, sql_perform() is:
 *
 * count := 0
 * while (maxretries < 0 or count < maxretries)
 *   count := count + 1
 *   BEGIN
 *   r := invoke callback
 *   if r == SQL_TXN_ROLLBACK
 *     ROLLBACK
 *     return 0
 *   end if
 *   if r == SQL_TXN_ABORT
 *     ROLLBACK
 *     return -1
 *   end if
 *   if r == SQL_TXN_FAIL and a deadlock was not detected
 *     ROLLBACK
 *     return -1
 *   end if
 *   if r == SQL_TXN_COMMIT
 *     COMMIT
 *     if successfully committed
 *       return 0
 *     end if
 *     if a deadlock was not detected
 *       return -1
 *     end if
 *   end if
 *   ROLLBACK
 *   // try again until the retry count reaches maxretries
 * end while
 */
int
sql_perform(SQL *restrict sql, SQL_PERFORM_TXN fn, void *restrict userdata, int maxretries, SQL_TXN_MODE mode)
{
	int count, r;
	
	count = 0;
	while(maxretries < 0 || count < maxretries)
	{
		count++;
		if(sql_begin(sql, mode))
		{
			return -1;
		}
		r = fn(sql, userdata);
		if(r == SQL_TXN_ROLLBACK || r == SQL_TXN_ABORT)
		{
			/* Roll back with success or roll back and abort */
			sql_rollback(sql);
			return (r == SQL_TXN_ROLLBACK ? 0 : -1);
		}
		if(r == SQL_TXN_FAIL && !sql->api->deadlocked(sql))
		{
			/* Hard failure within the callback */
			sql_rollback(sql);
			return -1;
		}
		if(r == SQL_TXN_COMMIT)
		{
			if(!sql_commit(sql))
			{
				/* Successfully commited */
				return 0;
			}
			if(!sql->api->deadlocked(sql))
			{
				/* Hard failure */
				return -1;
			}
		}
		/* Try again */
		sql_rollback(sql);
	}
	/* Retry count exceeded */
	return -1;
}

int
sql_deadlocked(SQL *sql)
{
	return sql->api->deadlocked(sql);
}
