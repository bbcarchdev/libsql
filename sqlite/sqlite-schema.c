/* Author: Mo McRoberts <mo.mcroberts@bbc.co.uk>
 *
 * Copyright 2014-2017 BBC.
 */

/*
 * Copyright 2013 Mo McRoberts.
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

/* All methods below are invoked within a transaction by sql_migrate() */

#define SCHEMA_SQL \
	"CREATE TABLE IF NOT EXISTS \"_version\" (" \
	"\"ident\" VARCHAR(64) NOT NULL COMMENT 'Module identifier', " \
	"\"version\" BIGINT UNSIGNED NOT NULL COMMENT 'Current schema version'," \
	"\"updated\" DATETIME NOT NULL COMMENT 'Timestamp of the last schema update'," \
	"\"comment\" TEXT DEFAULT NULL COMMENT 'Description of the last update'," \
	"PRIMARY KEY (\"ident\")" \
	") ENGINE=InnoDB DEFAULT CHARSET=utf8 DEFAULT COLLATE=utf8_unicode_ci"

static int sql_sqlite_schema_select_version_(SQL *me, const char *identifier, size_t idlen, int defver);

int
sql_sqlite_schema_get_version_(SQL *me, const char *identifier)
{
	size_t idlen, qbuflen;
	char *p;
	
	idlen = strlen(identifier);
	if(idlen > 64)
	{
		idlen = 64;
	}
	qbuflen = (idlen * 2) + 128;
	if(qbuflen > me->qbuflen)
	{
		p = (char *) realloc(me->qbuf, qbuflen);
		if(!p)
		{
			return -1;
		}
		me->qbuf = p;
		me->qbuflen = qbuflen;
	}
	return sql_sqlite_schema_select_version_(me, identifier, idlen, 0);
}

int
sql_sqlite_schema_create_table_(SQL *me)
{
	int r;
	
	if(me->querylog)
	{
		me->querylog(me, SCHEMA_SQL);
	}
	r = sqlite3_exec(me->sqlite, SCHEMA_SQL, NULL, NULL, NULL);
	if(r != SQLITE_OK)
	{
		sql_sqlite_copy_error_(me);
		return -1;
	}
	return 0;
}

int
sql_sqlite_schema_set_version_(SQL *me, const char *identifier, int version)
{
	size_t idlen;
	char *p;
	int r;

	if(version < 0)
	{
		return version;
	}
	idlen = strlen(identifier);
	if(idlen > 64)
	{
		idlen = 64;
	}
	if(sql_sqlite_schema_select_version_(me, identifier, idlen, -1) < 0)
	{
		strcpy(me->qbuf, "INSERT INTO \"_version\" (\"ident\", \"version\", \"updated\") VALUES ('");
		p = strchr(me->qbuf, 0);
		/* sql_sqlite_escape_(SQL *restrict me, const unsigned char *restrict from, size_t length, char *restrict buf, size_t buflen) */
		sql_sqlite_escape_(me, (const unsigned char *) identifier, idlen, p, 128);
		p = strchr(me->qbuf, 0);
		sprintf(p, "', %d, NOW())", version);
	}
	else
	{
		snprintf(me->qbuf, me->qbuflen, "UPDATE \"_version\" SET \"version\" = %d, \"updated\" = NOW() WHERE \"ident\" = '", version);
		p = strchr(me->qbuf, 0);
		sql_sqlite_escape_(me, (const unsigned char *) identifier, idlen, p, 128);
		p = strchr(me->qbuf, 0);
		*p = '\'';
		p++;
		*p = 0;
	}
	if(me->querylog)
	{
		me->querylog(me, me->qbuf);	
	}
	r = sqlite3_exec(me->sqlite, me->qbuf, NULL, NULL, NULL);
	if(r != SQLITE_OK)
	{
		sql_sqlite_copy_error_(me);
		return -1;
	}
	return version;
}

static int
sql_sqlite_schema_select_version_(SQL *me, const char *identifier, size_t idlen, int defver)
{
	char *p;
	int r;
	sqlite3_stmt *stmt;

	strcpy(me->qbuf, "SELECT \"version\" FROM \"_version\" WHERE \"ident\" = '");
	p = strchr(me->qbuf, 0);
	sql_sqlite_escape_(me, (const unsigned char *) identifier, idlen, p, 128);
	p = strchr(me->qbuf, 0);
	*p = '\'';
	p++;
	*p = 0;
	if(me->querylog)
	{
		me->querylog(me, me->qbuf);
	}
	r = sqlite3_prepare_v2(me->sqlite, me->qbuf, strlen(me->qbuf), &stmt, NULL);
	if(r != SQLITE_OK)
	{
		sql_sqlite_copy_error_(me);
		return -1;		
	}
	if(sqlite3_step(stmt) != SQLITE_ROW || !sqlite3_column_bytes(stmt, 0))
	{
		fprintf(stderr, "no row or empty row\n");
		sqlite3_finalize(stmt);
		return defver;
	}
	r = sqlite3_column_int(stmt, 0);
	sqlite3_finalize(stmt);
	return r;	
}
