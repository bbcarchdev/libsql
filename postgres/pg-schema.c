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

/* All methods below are invoked within a transaction by sql_migrate() */

#define SCHEMA_SQL \
	"CREATE TABLE IF NOT EXISTS \"_version\" (" \
	"\"ident\" VARCHAR(64) NOT NULL, " \
	"\"version\" INTEGER NOT NULL, " \
	"\"updated\" TIMESTAMP NOT NULL, " \
	"\"comment\" TEXT DEFAULT NULL, " \
	"PRIMARY KEY (\"ident\")" \
	")"

static int sql_pg_schema_select_version_(SQL *me, const char *identifier, size_t idlen, int defver);

int
sql_pg_schema_get_version_(SQL *me, const char *identifier)
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
	return sql_pg_schema_select_version_(me, identifier, idlen, 0);
}

int
sql_pg_schema_create_table_(SQL *me)
{
	PGresult *res;
	ExecStatusType status;
	const char *st;

	st = SCHEMA_SQL;
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
	return 0;
}

int
sql_pg_schema_set_version_(SQL *me, const char *identifier, int version)
{
	PGresult *res;
	ExecStatusType status;
	size_t idlen;
	char *p;
	
	if(version < 0)
	{
		return version;
	}
	idlen = strlen(identifier);
	if(idlen > 64)
	{
		idlen = 64;
	}
	if(sql_pg_schema_select_version_(me, identifier, idlen, -1) < 0)
	{
		strcpy(me->qbuf, "INSERT INTO \"_version\" (\"ident\", \"version\", \"updated\") VALUES ('");
		p = strchr(me->qbuf, 0);
		PQescapeStringConn(me->pg, p, identifier, idlen, NULL);
		p = strchr(me->qbuf, 0);
		sprintf(p, "', %d, NOW())", version);
	}
	else
	{
		snprintf(me->qbuf, me->qbuflen, "UPDATE \"_version\" SET \"version\" = %d, \"updated\" = NOW() WHERE \"ident\" = '", version);
		p = strchr(me->qbuf, 0);
		PQescapeStringConn(me->pg, p, identifier, idlen, NULL);
		p = strchr(me->qbuf, 0);
		*p = '\'';
		p++;
		*p = 0;
	}
	if(me->querylog)
	{
		me->querylog(me, me->qbuf);
	}
	res = PQexec(me->pg, me->qbuf);
	status = PQresultStatus(res);
	if(!PQSTATUS_SUCCESS(status))
	{
		sql_pg_copy_error_(me, res);
		return -1;
	}
	PQclear(res);
	return version;
}

static int
sql_pg_schema_select_version_(SQL *me, const char *identifier, size_t idlen, int defver)
{
	PGresult *res;
	ExecStatusType status;
	char *p, *value;  
	int r;
	
	strcpy(me->qbuf, "SELECT \"version\" FROM \"_version\" WHERE \"ident\" = '");
	p = strchr(me->qbuf, 0);
	PQescapeStringConn(me->pg, p, identifier, idlen, NULL);
	p = strchr(me->qbuf, 0);
	*p = '\'';
	p++;
	*p = 0;
	if(me->querylog)
	{
		me->querylog(me, me->qbuf);
	}
	res = PQexec(me->pg, me->qbuf);
	status = PQresultStatus(res);
	if(!PQSTATUS_SUCCESS(status))
	{
		
		sql_pg_copy_error_(me, res);
		return -1;
	}
	if(!PQntuples(res))
	{
		PQclear(res);
		return defver;
	}
	value = PQgetvalue(res, 0, 0);
	if(!value)
	{
		PQclear(res);
		return defver;
	}
	r = atoi(value);
	PQclear(res);
	return r;
}
