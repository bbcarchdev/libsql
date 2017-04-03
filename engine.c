/* Author: Mo McRoberts <mo.mcroberts@bbc.co.uk>
 *
 * Copyright (c) 2015-2017 BBC
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

SQL_ENGINE *sql_mysql_engine(void);
SQL_ENGINE *sql_postgres_engine(void);
SQL_ENGINE *sql_sqlite_engine(void);

/* Note that until there's any sort of dynamic registration (or even a
 * static list), the matching here must be kept in sync with sql_engine_(),
 * below.
 */
int
sql_scheme_exists(const char *urischeme)
{
	if(!strcmp(urischeme, "sqlite") || !strcmp(urischeme, "sqlite3") || !strcmp(urischeme, "file")
	   || !strcmp(urischeme, "sqlite3+file"))
	{
		return 1;
	}
#ifdef WITH_MYSQL
	if(!strcmp(urischeme, "mysql") || !strcmp(urischeme, "mysqls"))
	{
		return 1;
	}
#endif
#ifdef WITH_LIBPQ
	if(!strcmp(urischeme, "pgsql") || !strcmp(urischeme, "postgresql"))
	{
		return 1;
	}
#endif
	return 0;
}

int
sql_scheme_foreach(int (*fn)(const char *scheme, void *userdata), void *userdata)
{
	if(fn("sqlite", userdata) ||
	   fn("sqlite3", userdata) ||
	   fn("file", userdata) ||
	   fn("sqlite3+file", userdata))
	{
		return -1;
	}
#ifdef WITH_MYSQL
	if(fn("mysql", userdata) ||
		fn("mysqls", userdata))
	{
		return -1;
	}
#endif
#ifdef WITH_LIBPQ
	if(fn("pgsql", userdata) ||
		fn("postgresql", userdata))
	{
		return -1;
	}
#endif
	return 0;
}

SQL_ENGINE *
sql_engine_(URI *uri)
{
	char scheme[64];
	size_t r;

	r = uri_scheme(uri, scheme, sizeof(scheme));
	if(r == (size_t) -1)
	{
		sql_set_error_("53000", "Failed to obtain scheme from parsed URI");
		return NULL;
	}
	if(r >= sizeof(scheme))
	{
		sql_set_error_("08000", "The specified URI scheme is not supported by any client engine");
		return NULL;
	}
	if(!strcmp(scheme, "sqlite") || !strcmp(scheme, "sqlite3") || !strcmp(scheme, "file") || !strcmp(scheme, "sqlite3+file"))
	{
		return sql_sqlite_engine();
	}
#ifdef WITH_MYSQL
	if(!strcmp(scheme, "mysql") || !strcmp(scheme, "mysqls"))
	{
		return sql_mysql_engine();
	}
#endif
#ifdef WITH_LIBPQ
	if(!strcmp(scheme, "pgsql") || !strcmp(scheme, "postgresql"))
	{
		return sql_postgres_engine();
	}
#endif
	sql_set_error_("08000", "The specified URI scheme is not supported by any client engine");
	return NULL;
}

int
sql_set_userdata(SQL *restrict sql, void *restrict data)
{
	return sql->api->set_userdata(sql, data);
}

void *
sql_userdata(SQL *sql)
{
	return sql->api->userdata(sql);
}
