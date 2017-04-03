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

SQL_FIELD_API sql_sqlite_field_api_ = {
	sql_field_def_queryinterface_,
	sql_field_def_addref_,
	sql_field_sqlite_free_,
	sql_field_sqlite_name_,
	sql_field_sqlite_width_
};

/* Free a SQL_FIELD structure */
unsigned long
sql_field_sqlite_free_(SQL_FIELD *me)
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
sql_field_sqlite_name_(SQL_FIELD *me)
{
	return sqlite3_column_name(me->stmt->stmt, me->index);
}

/* Return the maximum width (i.e., number of characters) for this column */
size_t
sql_field_sqlite_width_(SQL_FIELD *me)
{
	return 1;
}
