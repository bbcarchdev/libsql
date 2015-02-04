## Author: Mo McRoberts <mo.mcroberts@bbc.co.uk>
##
## Copyright (c) 2015 BBC

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

AC_INIT([libsql],[m4_esyscmd(/bin/sh m4/get-version.sh)],[https://github.com/bbcarchdev/libsql/issues],,[https://github.com/bbcarchdev/])
PACKAGE_DESC="SQL client library"
AC_SUBST([PACKAGE_DESC])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([foreign])
AM_MAINTAINER_MODE

AC_CONFIG_HEADER([config.h])

BT_PROG_XCODE
AC_PROG_CC([clang gcc c99 cc])
AC_PROG_CC_C99
AC_HEADER_STDC
BT_PROG_CC_DEBUG
LT_INIT

AC_CHECK_HEADERS([limits.h stddef.h])

BT_PROG_CC_WARN
BT_DEFINE_PREFIX
BT_REQUIRE_LIBURI_INCLUDED
BT_REQUIRE_LIBUUID

dnl libedit is used only by isql, don't include it in the ordinary libs
BT_REQUIRE_LIBEDIT_INCLUDED([
AM_CPPFLAGS="$AM_CPPFLAGS $LIBEDIT_CPPFLAGS"
AM_LDFLAGS="$AM_LDFLAGS $LIBEDIT_LDFLAGS"
])

BT_CHECK_MYSQL
if test x"$have_mysql" = x"yes" ; then
   engine_mysql="yes"
   ENGINE_SUBDIRS="$ENGINE_SUBDIRS mysql"
   ENGINE_LIBS="$ENGINE_LIBS mysql/libmysql-engine.la"
   ENGINE_DEPLIBS="$ENGINE_DEPLIBS $MYSQL_LIBS"
else
	engine_mysql="no"
fi

BT_CHECK_LIBPQ
if test x"$have_libpq" = x"yes" ; then
   engine_postgres="yes"
   ENGINE_SUBDIRS="$ENGINE_SUBDIRS postgres"
   ENGINE_LIBS="$ENGINE_LIBS postgres/libpostgres-engine.la"
   ENGINE_DEPLIBS="$ENGINE_DEPLIBS $LIBPQ_LIBS"
else
	engine_pgsql="no"
fi

AC_SUBST([ENGINE_LIBS])
AC_SUBST([ENGINE_SUBDIRS])
AC_SUBST([ENGINE_DEPLIBS])

AC_CONFIG_FILES([Makefile
m4/Makefile
mysql/Makefile
postgres/Makefile
libsql.pc
libsql-uninstalled.pc
])

AC_OUTPUT

AC_MSG_RESULT([

enabling engines:

           mysql ... $engine_mysql
        postgres ... $engine_postgres

])
