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

static SQL_FIELD_API pg_field_api = {
	sql_field_def_queryinterface_,
	sql_field_def_addref_,
	sql_field_pg_free_,
	sql_field_pg_name_,
	sql_field_pg_width_
};

/* Return a SQL_FIELD instance for a particular column (0..cols-1) */
SQL_FIELD *
sql_statement_pg_field_(SQL_STATEMENT *me, unsigned int col)
{
	SQL_FIELD *p;
	
	if(col >= me->columns)
	{
		return NULL;
	}
	p = (SQL_FIELD *) calloc(1, sizeof(SQL_FIELD));
	p->refcount = 1;
	p->api = &pg_field_api;
	p->st = me;
	p->col = col;
	return p;
}

/* Free a SQL_FIELD structure */
unsigned long
sql_field_pg_free_(SQL_FIELD *me)
{
	me->refcount--;
	if(me->refcount)
	{
		return me->refcount;
	}
	free(me);
	return 0;
}

/* Return a pointer to the name of the column */
const char *
sql_field_pg_name_(SQL_FIELD *me)
{
	return PQfname(me->st->result, me->col);
}

/* Return the maximum width (i.e., number of characters) for this column */
size_t
sql_field_pg_width_(SQL_FIELD *me)
{
	unsigned long long c;
	size_t l;

	if(!me->st->widths)
	{
		me->st->widths = (size_t *) calloc(me->st->columns, sizeof(size_t));
		if(!me->st->widths)
		{
			return 0;
		}
	}
	if(!me->st->widths[me->col])
	{
		for(c = 0; c < me->st->rows; c++)
		{
			l = PQgetlength(me->st->result, c, me->col);
			if(l > me->st->widths[me->col])
			{
				me->st->widths[me->col] = l;
			}
		}
	}
	return me->st->widths[me->col];
}
