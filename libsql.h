/* Author: Mo McRoberts <mo.mcroberts@bbc.co.uk>
 *
 * Copyright 2014-2016 BBC.
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

#ifndef LIBSQL_H_
# define LIBSQL_H_                      1

# include <stdarg.h>
# include <liburi.h>

typedef struct sql_struct SQL;
typedef struct sql_statement_struct SQL_STATEMENT;
typedef struct sql_field_struct SQL_FIELD; 
typedef int (*SQL_PERFORM_TXN)(SQL *restrict, void *restrict userdata);
typedef int (*SQL_PERFORM_MIGRATE)(SQL *restrict sql, const char *identifier, int newversion, void *restrict userdata);
typedef int (*SQL_LOG_QUERY)(SQL *restrict sql, const char *query);
typedef int (*SQL_LOG_ERROR)(SQL *restrict sql, const char *sqlstate, const char *message);
typedef int (*SQL_LOG_NOTICE)(SQL *restrict sql, const char *notice);

/* Return values for SQL_PERFORM_TXN */
# define SQL_TXN_COMMIT                 1
# define SQL_TXN_ROLLBACK               0
# define SQL_TXN_RETRY                  -1
# define SQL_TXN_ABORT                  -2

/* sql_perform() transaction modes */
typedef enum
{
	SQL_TXN_DEFAULT,
	SQL_TXN_CONSISTENT
} SQL_TXN_MODE;

/* Known query languages */
typedef enum
{
	SQL_LANG_SQL
} SQL_LANG;

/* Known SQL variants */
typedef enum
{
	SQL_VARIANT_MYSQL,
	SQL_VARIANT_POSTGRES,
	SQL_VARIANT_SQLITE
} SQL_VARIANT;

# if (!defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L) && !defined(restrict)
#  define restrict
# endif

# if defined(__cplusplus)
extern "C" {
# endif
  
	SQL *sql_connect(const char *uri);
	SQL *sql_connect_uri(URI *uri);
	int sql_disconnect(SQL *sql);
	int sql_scheme_exists(const char *urischeme);
	int sql_scheme_foreach(int (*fn)(const char *scheme, void *userdata), void *userdata);

	int sql_set_userdata(SQL *restrict sql, void *restrict data);
	void *sql_userdata(SQL *sql);

	SQL_LANG sql_lang(SQL *sql);
	SQL_VARIANT sql_variant(SQL *sql);

	int sql_lock(SQL *sql);
	int sql_unlock(SQL *sql);
	int sql_trylock(SQL *sql);

	int sql_set_querylog(SQL *sql, SQL_LOG_QUERY fn);
	int sql_set_errorlog(SQL *sql, SQL_LOG_ERROR fn);
	int sql_set_noticelog(SQL *sql, SQL_LOG_NOTICE fn);

	const char *sql_sqlstate(SQL *connection);
	const char *sql_error(SQL *connection);

	/* Escape a string */
	size_t sql_escape(SQL *restrict sql, const unsigned char *restrict from, size_t length, char *restrict buf, size_t buflen);
	
	/* Execute a statement not expected to return a result-set */
	int sql_execute(SQL *restrict sql, const char *restrict statement);
	int sql_executef(SQL *restrict sql, const char *restrict statement, ...);
	int sql_vexecutef(SQL *restrict sql, const char *restrict statement, va_list ap);

	/* Execute a statement which is expected to return a result-set */
	SQL_STATEMENT *sql_query(SQL *restrict sql, const char *restrict statement);
	SQL_STATEMENT *sql_queryf(SQL *restrict sql, const char *restrict statement, ...);
	SQL_STATEMENT *sql_vqueryf(SQL *restrict sql, const char *restrict statement, va_list ap);

	/* Create a parameterised statement */
	SQL_STATEMENT *sql_stmt_create(const char *statement);
	
	/* Execute a parameterised statement */
	int sql_stmt_execf(SQL_STATEMENT *statement, ...);
	int sql_stmt_vexecf(SQL_STATEMENT *statement, va_list ap);
	
	/* Destroy a statement or result-set */
	int sql_stmt_destroy(SQL_STATEMENT *statement);
	
	int sql_stmt_next(SQL_STATEMENT *statement);
	int sql_stmt_eof(SQL_STATEMENT *statement);
	int sql_stmt_rewind(SQL_STATEMENT *statement);
	int sql_stmt_seek(SQL_STATEMENT *statement, unsigned long long row);
	unsigned int sql_stmt_columns(SQL_STATEMENT *statement);
	unsigned long long sql_stmt_rows(SQL_STATEMENT *statement);
	unsigned long long sql_stmt_affected(SQL_STATEMENT *statement);
	unsigned long long sql_stmt_cur(SQL_STATEMENT *statement);
	SQL_FIELD *sql_stmt_field(SQL_STATEMENT *statement, unsigned int col);	
	int sql_stmt_null(SQL_STATEMENT *statement, unsigned int col);
	size_t sql_stmt_value(SQL_STATEMENT *restrict statement, unsigned int col, char *restrict buf, size_t buflen);
	const char *sql_stmt_str(SQL_STATEMENT *statement, unsigned int col);
	long sql_stmt_long(SQL_STATEMENT *statement, unsigned int col);
	unsigned long sql_stmt_ulong(SQL_STATEMENT *statement, unsigned int col);

	/* Return the name of a column */
	int sql_field_destroy(SQL_FIELD *field);
	const char *sql_field_name(SQL_FIELD *field);
	size_t sql_field_width(SQL_FIELD *field);
	
	/* Transaction handling */
	int sql_begin(SQL *sql, SQL_TXN_MODE mode);
	int sql_commit(SQL *sql);
	int sql_rollback(SQL *sql);
	int sql_deadlocked(SQL *sql);
	int sql_perform(SQL *restrict sql, SQL_PERFORM_TXN fn, void *restrict userdata, int maxretries, SQL_TXN_MODE mode);
	
	/* Schema migration */
	int sql_migrate(SQL *restrict sql, const char *restrict identifier, SQL_PERFORM_MIGRATE fn, void *userdata);

# if defined(__cplusplus)
}
# endif

#endif /*!LIBSQL_H_*/
