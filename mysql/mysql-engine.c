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

#include "p_mysql.h"

static void engine_alloc(void);

static pthread_once_t engine_control = PTHREAD_ONCE_INIT;

static SQL_ENGINE *engine;

static SQL_ENGINE_API mysql_engine_api = {
	sql_engine_def_queryinterface_,
	sql_engine_def_addref_,
	sql_engine_def_release_,
	sql_engine_mysql_create_
};

static SQL_API mysql_api = {
	sql_def_queryinterface_,
	sql_def_addref_,
	sql_mysql_free_,
	sql_def_lock_,
	sql_def_unlock_,
	sql_def_trylock_,
	sql_mysql_escape_,
	sql_mysql_sqlstate_,
	sql_mysql_error_,
	sql_mysql_connect_,
	sql_mysql_execute_,
	sql_mysql_statement_,
	sql_mysql_begin_,
	sql_mysql_commit_,
	sql_mysql_rollback_,
	sql_mysql_deadlocked_,
	sql_mysql_schema_get_version_,
	sql_mysql_schema_set_version_,
	sql_mysql_schema_create_table_,
	sql_mysql_set_querylog_,
	sql_mysql_set_errorlog_,
	sql_mysql_set_noticelog_,
	sql_mysql_lang_,
	sql_mysql_variant_,
	sql_mysql_set_userdata_,
	sql_mysql_userdata_,
};

SQL_ENGINE *
sql_mysql_engine(void)
{
	/* Create the engine instance as a singleton */
	pthread_once(&engine_control, engine_alloc);
	return engine;
}

static void
engine_alloc(void)
{
	engine = (SQL_ENGINE *) calloc(1, sizeof(SQL_ENGINE));
	if(!engine)
	{
		return;
	}
	engine->api = &mysql_engine_api;
	engine->refcount = 1;
}

SQL *
sql_engine_mysql_create_(SQL_ENGINE *me)
{
	SQL *inst;
	MYSQL *res;

	(void) me;

	inst = (SQL *) calloc(1, sizeof(SQL));
	if(!inst)
	{
		return NULL;
	}
	inst->api = &mysql_api;
	inst->refcount = 1;
	strcpy(inst->sqlstate, "0000");
	strcpy(inst->error, "No error");
	res = mysql_init(&(inst->mysql));
	if(!res)
	{
		free(inst);
		return NULL;
	}
	pthread_mutex_init(&(inst->lock), NULL);
	return inst;
}

unsigned long
sql_mysql_free_(SQL *me)
{
	me->refcount--;
	if(me->refcount)
	{
		return me->refcount;
	}
	pthread_mutex_destroy(&(me->lock));
	mysql_close(&(me->mysql));
	free(me->qbuf);
	free(me);
	return 0;
}

int
sql_mysql_set_userdata_(SQL *restrict me, void *restrict userdata)
{
	me->userdata = userdata;
	return 0;
}

void *
sql_mysql_userdata_(SQL *me)
{
	return me->userdata;
}

