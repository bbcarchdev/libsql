/* Author: Mo McRoberts <mo.mcroberts@bbc.co.uk>
 *
 * Copyright 2015 BBC
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

static SQL_STATEMENT_API pg_statement_api = {
	sql_statement_def_queryinterface_,
	sql_statement_def_addref_,
	sql_statement_pg_free_,
	sql_statement_pg_connection_,
	sql_statement_pg_statement_,
	sql_statement_pg_set_results_,
	sql_statement_pg_columns_,
	sql_statement_pg_rows_,
	sql_statement_pg_affected_,
	sql_statement_pg_field_,
	sql_statement_pg_null_,
	sql_statement_pg_value_,
	sql_statement_pg_valueptr_,
	sql_statement_pg_valuelen_,
	sql_statement_pg_eof_,
	sql_statement_pg_next_,
	sql_statement_pg_cur_,
	sql_statement_pg_rewind_,
	sql_statement_pg_seek_
};

/* Create a new statement or result-set */
SQL_STATEMENT *
sql_pg_statement_(SQL *restrict me, const char *restrict statement)
{
	SQL_STATEMENT *p;

	p = (SQL_STATEMENT *) calloc(1, sizeof(SQL_STATEMENT));
	if(!p)
	{
		return NULL;
	}
	p->api = &pg_statement_api;
	p->refcount = 1;
	p->sql = me;
	if(statement)
	{
		p->statement = strdup(statement);
		if(!p->statement)
		{
			free(p);
			return NULL;
		}
	}
	return p;
}

/* Free the resources used by a statement */
unsigned long
sql_statement_pg_free_(SQL_STATEMENT *me)
{
	me->refcount--;
	if(me->refcount)
	{
		return me->refcount;
	}
	if(me->result)
	{
		PQclear(me->result);
	}
	free(me->widths);
	free(me->statement);
	free(me);
	return 0;
}

/* Return the connection associated with this statement */
SQL *
sql_statement_pg_connection_(SQL_STATEMENT *me)
{
	return me->sql;
}

/* Return the (parameterised) SQL statement */
const char *
sql_statement_pg_statement_(SQL_STATEMENT *me)
{
	return me->statement;
}

/* Update the result-set with a new PGresult pointer */
int
sql_statement_pg_set_results_(SQL_STATEMENT *restrict me, void *data)
{
	if(me->result)
	{
		PQclear(me->result);
	}
	me->result = (PGresult *) data;
	me->lengths = NULL;
	me->cur = 0;
	if(data)
	{
		me->affected = atoll(PQcmdTuples(me->result));
		me->columns = PQnfields(me->result);
		me->rows = PQntuples(me->result);
	}
	else
	{
		me->affected = 0;
		me->columns = 0;
		me->rows = 0;
	}
	return 0;
}

/* Return the number of columns in the result-set */
unsigned int
sql_statement_pg_columns_(SQL_STATEMENT *me)
{
	return me->columns;
}

/* Return the number of rows in the result-set */
unsigned long long
sql_statement_pg_rows_(SQL_STATEMENT *me)
{
	return me->rows;
}

/* Return the number of rows affected by the query */
unsigned long long
sql_statement_pg_affected_(SQL_STATEMENT *me)
{
	return me->affected;
}

/* Return 1 if we have reached the end of the result-set, or 0 otherwise */
int
sql_statement_pg_eof_(SQL_STATEMENT *me)
{
	if(me->cur >= me->rows)
	{
		return 1;
	}
	return 0;
}

/* Move to the next row; returns 1 if there was a row, 0 if not or -1 if an error occurred */
int
sql_statement_pg_next_(SQL_STATEMENT *me)
{
	if(!me->result)
	{
		return 0;
	}
	me->cur++;
	if(me->cur >= me->rows)
	{
		return 0;
	}
	return 1;
}

/* Retrieve the contents of a column in the current row */
size_t
sql_statement_pg_value_(SQL_STATEMENT *restrict me, unsigned int col, char *restrict buf, size_t buflen)
{
	size_t l;
	char *value;

	if(buf)
	{
		*buf = 0;
	}
	if(me->cur >= me->rows || col >= me->columns)
	{
		return 0;
	}
	l = PQgetlength(me->result, me->cur, col);
	if(buf)
	{
		if(l >= buflen)
		{
			l = buflen - 1;
		}
		value = PQgetvalue(me->result, me->cur, col);
		if(!value)
		{
			return 0;
		}
		memcpy(buf, value, l);
		buf[l] = 0;
	}
	return l + 1;
}

/* Retrieve a pointer to the first byte in a field, or NULL if the value is NULL */
const unsigned char *
sql_statement_pg_valueptr_(SQL_STATEMENT *me, unsigned int col)
{
	return (const unsigned char *) PQgetvalue(me->result, me->cur, col);	
}


/* Return 1 if the specified column in the current row is NULL */
int
sql_statement_pg_null_(SQL_STATEMENT *me, unsigned int col)
{
	return PQgetisnull(me->result, me->cur, col);
}

/* Retrieve the length of the data in a column in the current row, including terminating NULL */
size_t
sql_statement_pg_valuelen_(SQL_STATEMENT *me, unsigned int col)
{
	return PQgetlength(me->result, me->cur, col);
}

/* Return the row index of the current row */
unsigned long long
sql_statement_pg_cur_(SQL_STATEMENT *me)
{
	return me->cur;
}

/* Seek to a particular row in the result-set */
int
sql_statement_pg_seek_(SQL_STATEMENT *me, unsigned long long row)
{
	if(!me->result || row >= me->rows)
	{
		return -1;
	}
	me->cur = row;
	return 0;
}

/* Rewind the current pointer into the result-set back to the start */
int
sql_statement_pg_rewind_(SQL_STATEMENT *me)
{	
	return sql_statement_pg_seek_(me, 0);
}
