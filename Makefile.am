## Author: Mo McRoberts <mo.mcroberts@bbc.co.uk>
##
## Copyright 2015-2017 BBC

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

ACLOCAL_AMFLAGS = -I m4

DIST_SUBDIRS = m4 libedit mysql postgres sqlite

SUBDIRS = @subdirs@ @ENGINE_SUBDIRS@ .

DISTCLEANFILES = libsql.pc libsql-uninstalled.pc

CLEANFILES = libsql.la.orig

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = libsql.pc


lib_LTLIBRARIES = libsql.la

bin_PROGRAMS = isql

noinst_DATA = mysql-darwin-fixups-stamp

include_HEADERS = libsql.h

noinst_HEADERS = libsql-engine.h

libsql_la_SOURCES = p_libsql.h \
	engine.c connect.c error.c statement.c field.c defaults.c vasprintf.c schema.c

libsql_la_LIBADD = @ENGINE_LIBS@ @LOCAL_LIBS@
EXTRA_libsql_la_DEPENDENCIES = @ENGINE_LIBS@ @LOCAL_LIBS@ 
libsql_la_LDFLAGS = -avoid-version

isql_SOURCES = isql.c

isql_LDADD = libsql.la @LIBEDIT_LIBS@ @LIBEDIT_LOCAL_LIBS@

mysql-darwin-fixups-stamp: isql libsql.la
	if test x"${mysql_darwin_fixups}" = x"yes" ; then \
		install_name_tool -change libmysqlclient_r.18.dylib ${MYSQL_LIBDIR}/libmysqlclient_r.18.dylib -change libmysqlclient.18.dylib ${MYSQL_LIBDIR}/libmysqlclient.18.dylib .libs/libsql.dylib ; \
		install_name_tool -change libmysqlclient_r.18.dylib ${MYSQL_LIBDIR}/libmysqlclient_r.18.dylib -change libmysqlclient.18.dylib ${MYSQL_LIBDIR}/libmysqlclient.18.dylib .libs/isql ; \
		mv libsql.la libsql.la.orig && sed "s@^dependency_libs=.*\$$@dependency_libs=''@" < libsql.la.orig > libsql.la ; \
	fi
	touch $@

install-exec-hook:
	if test x"${mysql_darwin_fixups}" = x"yes" ; then \
		mv $(DESTDIR)$(libdir)/libsql.la $(DESTDIR)$(libdir)/libsql.la.orig && sed "s@^dependency_libs=.*\$$@dependency_libs=''@" < $(DESTDIR)$(libdir)/libsql.la.orig > $(DESTDIR)$(libdir)/libsql.la && rm $(DESTDIR)$(libdir)/libsql.la.orig ; \
	fi

BRANCH ?= develop

DEVELOP_SUBMODULES = m4

checkout:
	@for submod in $(DEVELOP_SUBMODULES) ; do \
		echo "+++ Checking out the latest version of $$submod on $(BRANCH)" >&2 ; \
		cd $$submod && \
		git checkout $(BRANCH) && \
		git pull && \
		$(MAKE) checkout BRANCH=$(BRANCH) || exit $? && \
		cd .. ; \
	done
