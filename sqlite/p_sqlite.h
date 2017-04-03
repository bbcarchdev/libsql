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

#ifndef P_SQLITE_H_
# define P_SQLITE_H_                   1

# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <pthread.h>
# include <libsql.h>
# include "sqlite3.h"

# define SQL_STRUCT_DEFINED             1

# include <libsql-engine.h>

struct sql_engine_struct
{
	SQL_ENGINE_COMMON_MEMBERS
};

struct sql_struct
{
	SQL_COMMON_MEMBERS
	sqlite3 *sqlite;
	char sqlstate[6];
	char error[512];
	int depth;
	int deadlocked;
	char *qbuf;
	size_t qbuflen;
	SQL_LOG_QUERY querylog;
	SQL_LOG_ERROR errorlog;
	void *userdata;
};

struct sql_statement_struct
{
	SQL_STATEMENT_COMMON_MEMBERS
	SQL *sql;
	sqlite3_stmt *stmt;
	int eof;
	unsigned long long cur;
	unsigned long long rows;
	int affected;
	int columns;
	SQL_FIELD **fields;
};

struct sql_field_struct
{
	SQL_FIELD_COMMON_MEMBERS
	SQL_STATEMENT *stmt;
	int index;
	int width;
};

extern SQL_FIELD_API sql_sqlite_field_api_;

SQL *sql_engine_sqlite_create_(SQL_ENGINE *me);

void sql_sqlite_set_error_(SQL *restrict me, const char *restrict sqlstate, const char *restrict message);
void sql_sqlite_set_errcode_(SQL *me, int errcode);
void sql_sqlite_copy_error_(SQL *me);

unsigned long sql_sqlite_free_(SQL *me);
size_t sql_sqlite_escape_(SQL *restrict me, const unsigned char *restrict from, size_t length, char *restrict buf, size_t buflen);
const char *sql_sqlite_sqlstate_(SQL *me);
const char *sql_sqlite_error_(SQL *me);
int sql_sqlite_connect_(SQL *restrict me, URI *restrict uri);
int sql_sqlite_execute_(SQL *restrict me, const char *restrict statement, void *restrict *restrict resultdata);
SQL_STATEMENT *sql_sqlite_statement_(SQL *restrict me, const char *restrict statement);

unsigned long sql_statement_sqlite_free_(SQL_STATEMENT *me);
SQL *sql_statement_sqlite_connection_(SQL_STATEMENT *me);
const char *sql_statement_sqlite_statement_(SQL_STATEMENT *me);
int sql_statement_sqlite_set_results_(SQL_STATEMENT *restrict me, void *data);
unsigned int sql_statement_sqlite_columns_(SQL_STATEMENT *restrict me);
unsigned long long sql_statement_sqlite_rows_(SQL_STATEMENT *me);
unsigned long long sql_statement_sqlite_affected_(SQL_STATEMENT *me);
int sql_statement_sqlite_eof_(SQL_STATEMENT *me);
int sql_statement_sqlite_next_(SQL_STATEMENT *me);
SQL_FIELD *sql_statement_sqlite_field_(SQL_STATEMENT *me, unsigned int col);
int sql_statement_sqlite_null_(SQL_STATEMENT *me, unsigned int col);
size_t sql_statement_sqlite_value_(SQL_STATEMENT *restrict me, unsigned int col, char *restrict buf, size_t buflen);
const unsigned char *sql_statement_sqlite_valueptr_(SQL_STATEMENT *me, unsigned int col);
size_t sql_statement_sqlite_valuelen_(SQL_STATEMENT *me, unsigned int col);
unsigned long long sql_statement_sqlite_cur_(SQL_STATEMENT *me);
int sql_statement_sqlite_seek_(SQL_STATEMENT *me, unsigned long long row);
int sql_statement_sqlite_rewind_(SQL_STATEMENT *me);

unsigned long sql_field_sqlite_free_(SQL_FIELD *me);
const char *sql_field_sqlite_name_(SQL_FIELD *me);
size_t sql_field_sqlite_width_(SQL_FIELD *me);

int sql_sqlite_begin_(SQL *me, SQL_TXN_MODE mode);
int sql_sqlite_commit_(SQL *me);
int sql_sqlite_rollback_(SQL *me);
int sql_sqlite_deadlocked_(SQL *me);

int sql_sqlite_schema_get_version_(SQL *me, const char *identifier);
int sql_sqlite_schema_create_table_(SQL *me);
int sql_sqlite_schema_set_version_(SQL *me, const char *identifier, int version);

int sql_sqlite_set_querylog_(SQL *sql, SQL_LOG_QUERY fn);
int sql_sqlite_set_errorlog_(SQL *sql, SQL_LOG_ERROR fn);
int sql_sqlite_set_noticelog_(SQL *sql, SQL_LOG_NOTICE fn);

SQL_LANG sql_sqlite_lang_(SQL *sql);
SQL_VARIANT sql_sqlite_variant_(SQL *sql);

int sql_sqlite_set_userdata_(SQL *restrict sql, void *restrict userdata);
void *sql_sqlite_userdata_(SQL *sql);

#endif /*!P_SQLITE_H_*/
