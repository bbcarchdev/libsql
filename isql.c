/* Author: Mo McRoberts <mo.mcroberts@bbc.co.uk>
 *
 * Copyright 2015-2016 BBC
 *
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#include "histedit.h"
#include "libsql.h"

#ifndef EXIT_SUCCESS
# define EXIT_SUCCESS                  0
#endif

#ifndef EXIT_FAILURE
# define EXIT_FAILURE                  1
#endif
	 
#define QUERY_BLOCK                    128
#define PQUERY_BLOCK                   4
	 
struct query_struct
{
	char *query;
	int output_mode;
};

static const char *short_program_name;
static URI *connect_uri;
static int query_state = 0;
static char *query_buf;
static size_t query_len, query_alloc;
static struct query_struct *pqueries;
static size_t pquery_count, pquery_alloc;
static SQL *sql_conn;

static void
usage(void)
{
	fprintf(stderr, "Usage: %s [URI]\n", short_program_name);
}

static int
check_args(int argc, char **argv)
{
	char *t;
	int c;
	URI *here;

	connect_uri = NULL;
	t = strrchr(argv[0], '/');
	if(t)
	{
		short_program_name = t + 1;
	}
	else
	{
		short_program_name = t;
	}
	while((c = getopt(argc, argv, "h")) != -1)
	{
		switch(c)
		{
			case 'h':
				usage();
				exit(EXIT_SUCCESS);
			default:
				return -1;
		}
	}
	argc -= optind;
	argv += optind;
	if(argc > 1)
	{
		return -1;
	}
	if(argc == 1)
	{
		here = uri_create_cwd();
		if(!here)
		{
			fprintf(stderr, "%s: failed to obtain URI for current working directory\n", short_program_name);
		}
		connect_uri = uri_create_str(argv[0], here);
		if(!connect_uri)
		{
			uri_destroy(here);
			fprintf(stderr, "%s: failed to parse URI <%s>\n", short_program_name, argv[0]);
			return -1;
		}
		uri_destroy(here);
	}
	return 0;
}

static char *
prompt(EditLine *el)
{
	(void) el;

	switch(query_state)
	{
	case 0:
		return "SQL> ";
	case '\'':
		return "  '> ";
	case '"':
		return "  \"> ";
	}
	return "  -> ";
}

static int
show_results(SQL_STATEMENT *rs)
{
	size_t d, max, len;
	size_t *widths;
	unsigned int cols, c;
	SQL_FIELD **fields;
	const char *n;
	char *fbuf;
	
	cols = sql_stmt_columns(rs);
	if(!cols)
	{
		return 0;
	}
	fbuf = NULL;
	max = 0;
	fields = (SQL_FIELD **) calloc(cols, sizeof(SQL_FIELD *));
	widths = (size_t *) calloc(cols, sizeof(size_t));
	/* Determine the widths of the columns and print top border */	
	putchar('+');	
	for(c = 0; c < cols; c++)
	{
		fields[c] = sql_stmt_field(rs, c);
		widths[c] = sql_field_width(fields[c]);
		n = sql_field_name(fields[c]);
		if(n && strlen(n) > widths[c])
		{
			widths[c] = strlen(n);
		}
		if(widths[c] > max)
		{
			max = widths[c];
		}
		for(d = 0; d < widths[c] + 2; d++)
		{
			putchar('-');
		}
		putchar('+');
	}
	if(max < 4)
	{
		max = 4;
	}
	fbuf = (char *) malloc(max + 1);
	putchar('\n');
	/* Print the header row */
	putchar('|');
	for(c = 0; c < cols; c++)
	{
		putchar(' ');
		n = sql_field_name(fields[c]);
		d = widths[c] - strlen(n);
		fputs(n, stdout);
		for(; d; d--)
		{
			putchar(' ');
		}
		putchar(' ');
		putchar('|');
	}
	putchar('\n');
	/* Print the header border */
	putchar('+');
	for(c = 0; c < cols; c++)
	{
		for(d = 0; d < widths[c] + 2; d++)
		{
			putchar('-');
		}
		putchar('+');
	}
	putchar('\n');
	/* Print each row of the results */
	while(!sql_stmt_eof(rs))
	{
		putchar('|');
		for(c = 0; c < cols; c++)
		{
			putchar(' ');
			if(sql_stmt_null(rs, c))
			{
				strcpy(fbuf, "NULL");
				len = 4;
			}
			else
			{
				len = sql_stmt_value(rs, c, fbuf, max + 1);
				/* if len is ((size_t) -1), an error occurred; if len = 0, there
				 * is no value to retrieve; otherwise, the length includes the
				 * terminating NULL byte.
				 */
				if(len == (size_t) - 1 || len == 0)
				{
					len = 0;
				}
				else
				{
					len--;
				}
			}			
			for(d = 0; d < widths[c]; d++)
			{
				if(d < len)
				{
					putchar(fbuf[d]);
				}
				else
				{
					putchar(' ');
				}
			}
			putchar(' ');
			putchar('|');
		}
		putchar('\n');
		sql_stmt_next(rs);
	}
	putchar('+');
	/* Print the bottom border */
	for(c = 0; c < cols; c++)
	{
		for(d = 0; d < widths[c] + 2; d++)
		{
			putchar('-');
		}
		putchar('+');
	}
	putchar('\n');
	/* Free resources */
	for(c = 0; c < cols; c++)
	{
		sql_field_destroy(fields[c]);
	}
	free(fbuf);
	free(fields);
	free(widths);
	return 0;
}

static int
show_results_long(SQL_STATEMENT *rs)
{
	unsigned int cols, c;
	unsigned long long row;
	size_t w, maxname;
	SQL_FIELD **fields;
	const char *n;
	char fmt[64];
	
	cols = sql_stmt_columns(rs);
	if(!cols)
	{
		return 0;
	}
	fields = (SQL_FIELD **) calloc(cols, sizeof(SQL_FIELD *));
	maxname = 10;
	for(c = 0; c < cols; c++)
	{
		fields[c] = sql_stmt_field(rs, c);
		n = sql_field_name(fields[c]);
		w = strlen(n);
		if(w > maxname)
		{
			maxname = w;
		}
	}
	if(maxname)
	{
		sprintf(fmt, "%%%us: ", (unsigned) maxname);
	}
	else
	{
		strcpy(fmt, "%s: ");
	}
	row = 0;
	while(!sql_stmt_eof(rs))
	{
		row++;
		printf("*************************** %qu. row ***************************\n", row);
		for(c = 0; c < cols; c++)
		{
			printf(fmt, sql_field_name(fields[c]));
			if(sql_stmt_null(rs, c))
			{
				puts("NULL");
			}
			else
			{
				puts(sql_stmt_str(rs, c));
			}
		}
		sql_stmt_next(rs);
	}
	for(c = 0; c < cols; c++)
	{
		sql_field_destroy(fields[c]);
	}
	free(fields);
	return 0;
}

static int
add_query(char *query, int output_mode)
{
	struct query_struct *p;
	
	if(pquery_count + 1 > pquery_alloc)
	{
		pquery_alloc += PQUERY_BLOCK;
		p = (struct query_struct *) realloc(pqueries, sizeof(struct query_struct) * pquery_alloc);
		if(!p)
		{
			fprintf(stderr, "%s: failed to allocate %u bytes for queries\n", short_program_name, (unsigned) (sizeof(struct query_struct) * pquery_alloc));
			exit(EXIT_FAILURE);
		}
		pqueries = p;
	}
	pqueries[pquery_count].query = query;
	pqueries[pquery_count].output_mode = output_mode;
	pquery_count++;
	return 0;
}

static int
parse_query(const char *buf)
{
	size_t l;
	char *p, *qs;
	
	pquery_count = 0;
	if(buf)
	{
		l = strlen(buf);
		if(l + query_len + 1 > query_alloc)
		{
			query_alloc = (((query_len + l + 1) / QUERY_BLOCK) + 1) * QUERY_BLOCK;
			p = (char *) realloc(query_buf, query_alloc);
			if(!p)
			{
				fprintf(stderr, "%s: failed to allocate %u bytes for query buffer\n", short_program_name, (unsigned) query_alloc);
				exit(EXIT_FAILURE);
			}
			query_buf = p;		
		}
		p = &(query_buf[query_len]);
		strcpy(p, buf);
		query_len += l;
	}
	p = query_buf;
	query_state = 0;

	/* The state-keeping algorithm is fairly simplistic; although it only
	 * needs to care about quoting, it should (and doesn't yet) handle
	 * nesting properly, which is why it doesn't attempt to deal with
	 * parentheses at all.
	 */
	while(*p)
	{
		switch(query_state)
		{
		case 0:
			if(isspace(*p) || *p == '\r' || *p == '\n')
			{
				p++;
				break;
			}
			qs = p;
			switch(*p)
			{
				case '\'':
				case '"':
					query_state = *p;
					break;
				case '\\':
					if(p[1] == 'g' || p[1] == 'G')
					{
						p++;
						break;
					}
					qs = p;
					query_state = 2;
					break;
				case ';':
					break;
				default:
					query_state = 1;
					break;
			}
			p++;
			break;
		case 1:
			if(*p == ';')
			{
				add_query(qs, *p);
				*p = 0;
				query_state = 0;
				p++;
				break;
			}
			if(p[0] == '\\' && p[1] == 'g')
			{
				add_query(qs, p[1]);
				*p = 0;
				query_state = 0;
				p += 2;
				break;
			}
			if(p[0] == '\\' && p[1] == 'G')
			{
				add_query(qs, p[1]);
				*p = 0;
				query_state = 0;
				p += 2;
				break;
			}
			switch(*p)
			{
				case '\'':
				case '"':
				case '`':
				case '{':
				case '[':
					query_state = *p;
					break;
			}
			p++;
			break;
		case 2:
			/* Built-in commands */
			p++;
			break;
		case '\'':
		case '"':
		case '`':
			if(*p == query_state)
			{
				query_state = 1;
			}
			p++;
			break;
		case '{':
			if(*p == '}')
			{
				query_state = 1;
			}
			p++;
			break;
		case '[':
			if(*p == ']')
			{
				query_state = 1;
			}
			p++;
			break;
		}
	}
	if(query_state == 2)
	{
		/* Trim any trailing whitespace before adding to the query list */
		p--;
		while(p > qs)
		{
			if(!isspace(*p) && *p != '\r' && *p != '\n')
			{
				break;
			}
			*p = 0;
			p--;
		}
		add_query(qs, 0);
		query_state = 0;
	}
	if(!query_state)
	{
		query_len = 0;
	}
	return query_state;
}

static int
exec_builtin(SQL *conn, History *hist, char *query)
{
	URI *here, *uri;

	(void) conn;
	(void) hist;
	
	if(!strncmp(query, "\\q", 2))
	{
		exit(EXIT_SUCCESS);
	}
	if(!strncmp(query, "\\c", 2))
	{
		query += 2;
		while(isspace(*query)) query++;
		if(!*query)
		{
			printf("[08000] Specify \"\\c URI\" to establish a new connection\n");
			return -1;
		}
		here = uri_create_cwd();
		if(!here)
		{
			printf("[08800] failed to obtain URI for current working directory\n");
			return -1;
		}
		uri = uri_create_str(query, here);
		if(!uri)
		{
			uri_destroy(here);
			printf("[08801] failed to parse URI <%s>\n", query);
			return -1;
		}
		uri_destroy(here);
		conn = sql_connect_uri(uri);
		uri_destroy(uri);
		if(!conn)
		{
			printf("[%s] %s\n", sql_sqlstate(NULL), sql_error(NULL));
			return -1;
		}
		if(sql_conn)
		{
			sql_disconnect(sql_conn);
		}
		sql_conn = conn;
		return 0;
	}
	printf("[42000] Unknown command '%s'\n", query);
	return -1;
}

static int
exec_queries(History *hist)
{
	HistEvent ev;
	size_t c;
	SQL_STATEMENT *rs;
	unsigned int cols;
	unsigned long long rows;
	
	for(c = 0; c < pquery_count; c++)
	{
		history(hist, &ev, H_ENTER, pqueries[c].query);
		if(pqueries[c].query[0] == '\\')
		{
			exec_builtin(sql_conn, hist, pqueries[c].query);
			continue;
		}
		if(pqueries[c].output_mode == 'G')
		{
			history(hist, &ev, H_ADD, "\\G");
		}
		else
		{
			history(hist, &ev, H_ADD, ";");
		}
		if(!sql_conn)
		{
			printf("[08003] Not connected to database server\n");
			continue;
		}
		rs = sql_query(sql_conn, pqueries[c].query);
		if(!rs)
		{
			printf("[%s] %s\n", sql_sqlstate(sql_conn), sql_error(sql_conn));
			return -1;
		}
		cols = sql_stmt_columns(rs);
		if(!cols)
		{
			printf("%qu rows affected.\n", sql_stmt_affected(rs));
			sql_stmt_destroy(rs);
			return 0;
		}
		if(pqueries[c].output_mode == 'G')
		{
			show_results_long(rs);
		}
		else
		{
			show_results(rs);
		}
		rows = sql_stmt_rows(rs);
		if(rows == 1)
		{
			printf("1 row in set.\n");
		}
		else
		{
			printf("%qu rows in set.\n", rows);
		}		
		sql_stmt_destroy(rs);
	}
	return 0;
}

int
main(int argc, char **argv)
{
	EditLine *el;
	History *hist;
	HistEvent ev;
	const char *buf;
	int num, state;
	
	check_args(argc, argv);
	if(connect_uri)
	{		
		sql_conn = sql_connect_uri(connect_uri);
		uri_destroy(connect_uri);
		connect_uri = NULL;
		if(!sql_conn)
		{
			fprintf(stderr, "%s: [%s] %s\n", short_program_name, sql_sqlstate(NULL), sql_error(NULL));
			exit(EXIT_FAILURE);
		}
	}
	fprintf(stderr, "%s interactive SQL shell (%s)\n\n", PACKAGE, VERSION);
	fprintf(stderr, 
		"Type:  \\c URI to establish a new connection\n"
		"       \\g or ; to execute query\n"
		"       \\G to execute the query showing results in long format\n"
		"       \\q to end the SQL session\n"
		"\n"
		);
	hist = history_init();
	history(hist, &ev, H_SETSIZE, 100);
	el = el_init(argv[0], stdin, stdout, stderr);
	el_set(el, EL_EDITOR, "emacs");
	el_set(el, EL_SIGNAL, 1);
	el_set(el, EL_PROMPT, prompt);
	el_set(el, EL_HIST, history, hist);
	el_source(el, NULL);
	while((buf = el_gets(el, &num)) != NULL && num != 0)
	{
		state = parse_query(buf);
		if(state == 0)
		{
			exec_queries(hist);
		}
	}
	return 0;
}
