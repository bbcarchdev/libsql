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

#ifndef P_POSTGRES_H_
# define P_POSTGRES_H_                 1

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <pthread.h>
# include <libsql.h>

# include <libpq-fe.h>

# define SQL_STRUCT_DEFINED             1

# include <libsql-engine.h>

# define PQSTATUS_SUCCESS(r) \
	(r == PGRES_EMPTY_QUERY || r == PGRES_COMMAND_OK || r == PGRES_TUPLES_OK || r == PGRES_COPY_OUT || r == PGRES_COPY_IN || r == PGRES_SINGLE_TUPLE)

struct sql_engine_struct
{
	SQL_ENGINE_COMMON_MEMBERS
};

struct sql_struct
{
	SQL_COMMON_MEMBERS
	PGconn *pg;
	char sqlstate[6];
	char error[512];
	int depth;
	int deadlocked;
	char *qbuf;
	size_t qbuflen;
	SQL_LOG_QUERY querylog;
	SQL_LOG_ERROR errorlog;
};

struct sql_statement_struct
{
	SQL_STATEMENT_COMMON_MEMBERS
	SQL *sql;
	char *statement;
	PGresult *result;
	unsigned long *lengths;
	unsigned int columns;
	unsigned long long affected;
	unsigned long long rows;
	unsigned long long cur;
	size_t *widths;
};

struct sql_field_struct
{
	SQL_FIELD_COMMON_MEMBERS
	SQL_STATEMENT *st;
	unsigned int col;
};

SQL *sql_engine_pg_create_(SQL_ENGINE *me);

void sql_pg_set_error_(SQL *restrict me, const char *restrict sqlstate, const char *restrict message);
void sql_pg_copy_error_(SQL *restrict me, PGresult *restrict result);

unsigned long sql_pg_free_(SQL *me);
size_t sql_pg_escape_(SQL *restrict me, const unsigned char *restrict from, size_t length, char *restrict buf, size_t buflen);
const char *sql_pg_sqlstate_(SQL *me);
const char *sql_pg_error_(SQL *me);
int sql_pg_connect_(SQL *restrict me, URI *restrict uri);
int sql_pg_execute_(SQL *restrict me, const char *restrict statement, void *restrict *restrict resultdata);
SQL_STATEMENT *sql_pg_statement_(SQL *restrict me, const char *restrict statement);

unsigned long sql_statement_pg_free_(SQL_STATEMENT *me);
SQL *sql_statement_pg_connection_(SQL_STATEMENT *me);
const char *sql_statement_pg_statement_(SQL_STATEMENT *me);
int sql_statement_pg_set_results_(SQL_STATEMENT *restrict me, void *data);
unsigned int sql_statement_pg_columns_(SQL_STATEMENT *restrict me);
unsigned long long sql_statement_pg_rows_(SQL_STATEMENT *me);
unsigned long long sql_statement_pg_affected_(SQL_STATEMENT *me);
int sql_statement_pg_eof_(SQL_STATEMENT *me);
int sql_statement_pg_next_(SQL_STATEMENT *me);
SQL_FIELD *sql_statement_pg_field_(SQL_STATEMENT *me, unsigned int col);
int sql_statement_pg_null_(SQL_STATEMENT *me, unsigned int col);
size_t sql_statement_pg_value_(SQL_STATEMENT *restrict me, unsigned int col, char *restrict buf, size_t buflen);
const unsigned char *sql_statement_pg_valueptr_(SQL_STATEMENT *me, unsigned int col);
size_t sql_statement_pg_valuelen_(SQL_STATEMENT *me, unsigned int col);
unsigned long long sql_statement_pg_cur_(SQL_STATEMENT *me);
int sql_statement_pg_seek_(SQL_STATEMENT *me, unsigned long long row);
int sql_statement_pg_rewind_(SQL_STATEMENT *me);

unsigned long sql_field_pg_free_(SQL_FIELD *me);
const char *sql_field_pg_name_(SQL_FIELD *me);
size_t sql_field_pg_width_(SQL_FIELD *me);

int sql_pg_begin_(SQL *me, SQL_TXN_MODE mode);
int sql_pg_commit_(SQL *me);
int sql_pg_rollback_(SQL *me);
int sql_pg_deadlocked_(SQL *me);

int sql_pg_schema_get_version_(SQL *me, const char *identifier);
int sql_pg_schema_create_table_(SQL *me);
int sql_pg_schema_set_version_(SQL *me, const char *identifier, int version);

int sql_pg_set_querylog_(SQL *sql, SQL_LOG_QUERY fn);
int sql_pg_set_errorlog_(SQL *sql, SQL_LOG_ERROR fn);

#endif /*!P_POSTGRES_H_*/
