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

static SQL_STATEMENT_API sqlite_statement_api = {
	sql_statement_def_queryinterface_,
	sql_statement_def_addref_,
	sql_statement_sqlite_free_,
	sql_statement_sqlite_connection_,
	sql_statement_sqlite_statement_,
	sql_statement_sqlite_set_results_,
	sql_statement_sqlite_columns_,
	sql_statement_sqlite_rows_,
	sql_statement_sqlite_affected_,
	sql_statement_sqlite_field_,
	sql_statement_sqlite_null_,
	sql_statement_sqlite_value_,
	sql_statement_sqlite_valueptr_,
	sql_statement_sqlite_valuelen_,
	sql_statement_sqlite_eof_,
	sql_statement_sqlite_next_,
	sql_statement_sqlite_cur_,
	sql_statement_sqlite_rewind_,
	sql_statement_sqlite_seek_
};

/* Create a new statement or result-set */
SQL_STATEMENT *
sql_sqlite_statement_(SQL *restrict me, const char *restrict statement)
{
	SQL_STATEMENT *p;

	p = (SQL_STATEMENT *) calloc(1, sizeof(SQL_STATEMENT));
	if(!p)
	{
		return NULL;
	}
	p->api = &sqlite_statement_api;
	p->refcount = 1;
	p->sql = me;
	p->cur = (unsigned long long) -1;
	if(statement)
	{
		if(sqlite3_prepare_v2(me->sqlite, statement, strlen(statement), &(p->stmt), NULL) != SQLITE_OK)
		{
			free(p);
			return NULL;
		}
	}
	return p;
}

/* Free the resources used by a statement */
unsigned long
sql_statement_sqlite_free_(SQL_STATEMENT *me)
{
	size_t c;

	me->refcount--;
	if(me->refcount)
	{
		return me->refcount;
	}
	if(me->stmt)
	{
		sqlite3_finalize(me->stmt);
	}
	if(me->fields)
	{
		for(c = 0; me->fields[c]; c++)
		{
			if(me->fields[c])
			{
				me->fields[c]->stmt = NULL;
				me->fields[c]->api->release(me->fields[c]);
			}
		}
		free(me->fields);
	}
	free(me);
	return 0;
}

/* Return the connection associated with this statement */
SQL *
sql_statement_sqlite_connection_(SQL_STATEMENT *me)
{
	return me->sql;
}

/* Return the (parameterised) SQL statement */
const char *
sql_statement_sqlite_statement_(SQL_STATEMENT *me)
{
	return sqlite3_sql(me->stmt);
}

/* Update the result-set following query execution */
int
sql_statement_sqlite_set_results_(SQL_STATEMENT *restrict me, void *data)
{
	int r, i;
	size_t c;

	if(me->stmt && me->stmt != (sqlite3_stmt *) data)
	{
		sqlite3_finalize(me->stmt);
	}
	if(me->fields)
	{
		for(c = 0; me->fields[c]; c++)
		{
			me->fields[c]->stmt = NULL;
			me->fields[c]->api->release(me->fields[c]);
		}
		me->fields = NULL;
	}
	me->stmt = (sqlite3_stmt *) data;
	me->cur = (unsigned long long) -1;
	me->eof = 0;
	me->rows = 0;
	me->affected = 0;
	if(me->stmt)
	{
		me->columns = sqlite3_column_count(me->stmt);
		if(me->columns < 0)
		{
			/* XXX can this happen? */
			return -1;
		}
		me->fields = (SQL_FIELD **) calloc(me->columns + 1, sizeof(SQL_FIELD *));
		if(!me->fields)
		{
			/* XXX memory allocation failure */
			return -1;
		}
		for(i = 0; i < me->columns; i++)
		{
			me->fields[i] = (SQL_FIELD *) calloc(1, sizeof(SQL_FIELD));
			if(!me->fields[i])
			{
				/* XXX memory allocation failure */
				return -1;
			}
			me->fields[i]->api = &sql_sqlite_field_api_;
			me->fields[i]->refcount = 1;
			me->fields[i]->index = i;
			me->fields[i]->stmt = me;
		}
		r = sqlite3_step(me->stmt);
		if(r == SQLITE_DONE)
		{
			me->affected = sqlite3_changes(me->sql->sqlite);
			me->eof = 1;
		}
		else if(r != SQLITE_ROW)
		{
			sql_sqlite_set_errcode_(me->sql, r);
			return -1;
		}
		else
		{
			me->affected = sqlite3_changes(me->sql->sqlite);
			me->rows++;
		}
	}
	else
	{
		me->columns = 0;
		me->eof = 1;
	}
	return 0;
}

/* Return the number of columns in the result-set */
unsigned int
sql_statement_sqlite_columns_(SQL_STATEMENT *me)
{
	return me->columns;
}

/* Return the number of rows in the result-set */
unsigned long long
sql_statement_sqlite_rows_(SQL_STATEMENT *me)
{
	return me->rows;
}

/* Return the number of rows affected by the query */
unsigned long long
sql_statement_sqlite_affected_(SQL_STATEMENT *me)
{
	return me->affected;
}

/* Return 1 if we have reached the end of the result-set, or 0 otherwise */
int
sql_statement_sqlite_eof_(SQL_STATEMENT *me)
{
	return (me->eof) ? 1 : 0;
}

/* Move to the next row; returns 1 if there was a row, 0 if not or -1 if an error occurred */
int
sql_statement_sqlite_next_(SQL_STATEMENT *me)
{
	int r;

	if(!me->stmt)
	{
		return 0;
	}
	r = sqlite3_step(me->stmt);
	if(r == SQLITE_DONE)
	{
		me->eof = 1;
		return 0;
	}
	if(r == SQLITE_ROW)
	{
		me->cur++;
		if(me->cur >= me->rows)
		{
			me->rows = me->cur + 1;
		}
		return 1;
	}
	sql_sqlite_set_errcode_(me->sql, r);
	return -1;
}

/* Retrieve the contents of a column in the current row */
size_t
sql_statement_sqlite_value_(SQL_STATEMENT *restrict me, unsigned int col, char *restrict buf, size_t buflen)
{
	const unsigned char *t;

	if(buf)
	{
		*buf = 0;
	}
	if(me->eof)
	{
		return 0;
	}
	t = sqlite3_column_text(me->stmt, col);
	if(!t)
	{
		return 1;
	}
	if(buf)
	{
		/* buflen includes the NULL terminator */
		strncpy(buf, (const char *) t, buflen);
		buf[buflen - 1] = 0;
	}
	return strlen((const char *) t) + 1;
}

/* Retrieve a pointer to the first byte in a field, or NULL if the value is NULL */
const unsigned char *
sql_statement_sqlite_valueptr_(SQL_STATEMENT *me, unsigned int col)
{
	if(me->eof)
	{
		return NULL;
	}
	return sqlite3_column_text(me->stmt, col);
}


/* Return 1 if the specified column in the current row is NULL */
int
sql_statement_sqlite_null_(SQL_STATEMENT *me, unsigned int col)
{
	if(me->eof)
	{
		return 1;
	}
	if(sqlite3_column_type(me->stmt, col) == SQLITE_NULL)
	{
		return 1;
	}
	return 0;
}

/* Retrieve the length of the data in a column in the current row, including terminating NULL */
size_t
sql_statement_sqlite_valuelen_(SQL_STATEMENT *me, unsigned int col)
{
	if(me->eof)
	{
		return 0;
	}
	return sqlite3_column_bytes(me->stmt, col) + 1;
}

/* Return the row index of the current row */
unsigned long long
sql_statement_sqlite_cur_(SQL_STATEMENT *me)
{
	return me->cur;
}

/* Seek to a particular row in the result-set */
int
sql_statement_sqlite_seek_(SQL_STATEMENT *me, unsigned long long row)
{
	(void) row;

	sql_sqlite_set_error_(me->sql, "X0001", "cannot seek a SQLite cursor");
	return -1;
}

/* Rewind the current pointer into the result-set back to the start */
int
sql_statement_sqlite_rewind_(SQL_STATEMENT *me)
{
	sql_sqlite_set_error_(me->sql, "X0001", "cannot rewind a SQLite cursor");
	return -1;
}

/* Return a SQL_FIELD instance for a particular column (0..cols-1) */
SQL_FIELD *
sql_statement_sqlite_field_(SQL_STATEMENT *me, unsigned int col)
{
	if(!me->stmt)
	{
		return NULL;
	}
	if(col >= (unsigned int) me->columns || !me->fields[col])
	{
		return NULL;
	}
	me->fields[col]->api->addref(me->fields[col]);
	return me->fields[col];
}
