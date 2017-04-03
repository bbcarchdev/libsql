## Author: Mo McRoberts <mo.mcroberts@bbc.co.uk>
##
## Copyright 2017 BBC
##
## Copyright 2012-2013 Mo McRoberts.
##
##  Licensed under the Apache License, Version 2.0 (the "License");
##  you may not use this file except in compliance with the License.
##  You may obtain a copy of the License at
##
##      http://www.apache.org/licenses/LICENSE-2.0
##
##  Unless required by applicable law or agreed to in writing, software
##  distributed under the License is distributed on an "AS IS" BASIS,
##  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
##  See the License for the specific language governing permissions and
##  limitations under the License.

noinst_LTLIBRARIES = libsqlite-engine.la

libsqlite_engine_la_CPPFLAGS = @AM_CPPFLAGS@ -I${srcdir}/dist -O2 \
	-D_REENTRANT=1 -DHAVE_USLEEP=1 \
	-DSQLITE_THREADSAFE=1 -DSQLITE_ENABLE_FTS4=1 -DSQLITE_ENABLE_RTREE=1 \
	-DSQLITE_ENABLE_COLUMN_METADATA=1 -DSQLITE_ENABLE_CURSOR_HINTS=1 \
	-DSQLITE_ENABLE_EXPLAIN_COMMENTS=1 -DSQLITE_ENABLE_STMT_SCANSTATUS=1 \
	-DSQLITE_ENABLE_JSON1=1 -DSQLITE_ENABLE_RTREE=1 -DSQLITE_ENABLE_DBSTAT_VTAB=1 \
	-DSQLITE_ENABLE_UPDATE_DELETE_LIMIT=1

libsqlite_engine_la_SOURCES = p_sqlite.h \
	sqlite-engine.c sqlite-connect.c sqlite-query.c sqlite-statement.c sqlite-field.c sqlite-schema.c \
	dist/sqlite3.c

