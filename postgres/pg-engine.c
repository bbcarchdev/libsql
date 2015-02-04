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

static void engine_alloc(void);

static pthread_once_t engine_control = PTHREAD_ONCE_INIT;

static SQL_ENGINE *engine;

static SQL_ENGINE_API pg_engine_api = {
	sql_engine_def_queryinterface_,
	sql_engine_def_addref_,
	sql_engine_def_release_,
	sql_engine_pg_create_
};

static SQL_API pg_api = {
	sql_def_queryinterface_,
	sql_def_addref_,
	sql_pg_free_,
	sql_def_lock_,
	sql_def_unlock_,
	sql_def_trylock_,
	sql_pg_escape_,
	sql_pg_sqlstate_,
	sql_pg_error_,
	sql_pg_connect_,
	sql_pg_execute_,
	sql_pg_statement_,
	sql_pg_begin_,
	sql_pg_commit_,
	sql_pg_rollback_,
	sql_pg_deadlocked_,
	sql_pg_schema_get_version_,
	sql_pg_schema_set_version_,
	sql_pg_schema_create_table_,
	sql_pg_set_querylog_,
	sql_pg_set_errorlog_,
	sql_pg_set_noticelog_,
	sql_pg_lang_,
	sql_pg_variant_
};

SQL_ENGINE *
sql_postgres_engine(void)
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
	engine->api = &pg_engine_api;
	engine->refcount = 1;
}

SQL *
sql_engine_pg_create_(SQL_ENGINE *me)
{
	SQL *inst;

	(void) me;

	inst = (SQL *) calloc(1, sizeof(SQL));
	if(!inst)
	{
		return NULL;
	}
	inst->api = &pg_api;
	inst->refcount = 1;
	strcpy(inst->sqlstate, "00000");
	strcpy(inst->error, "No error");
	pthread_mutex_init(&(inst->lock), NULL);
	return inst;
}

unsigned long
sql_pg_free_(SQL *me)
{
	me->refcount--;
	if(me->refcount)
	{
		return me->refcount;
	}
	pthread_mutex_destroy(&(me->lock));
	if(me->pg)
	{
		PQfinish(me->pg);
	}
	free(me->qbuf);
	free(me);
	return 0;
}
