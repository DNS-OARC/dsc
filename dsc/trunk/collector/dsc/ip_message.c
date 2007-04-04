#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>

#include "xmalloc.h"
#include "ip_message.h"
#include "md_array.h"

#include "ip_direction_index.h"
#include "ip_proto_index.h"
#include "ip_version_index.h"

extern md_array_printer xml_printer;
static md_array_list *Arrays = NULL;

void
ip_message_handle(const ip_message *ip)
{
    md_array_list *a;
    for (a = Arrays; a; a = a->next)
	md_array_count(a->theArray, ip);
}

static int
ip_message_find_indexer(const char *in, IDXR ** ix, HITR ** it)
{
    if (0 == strcmp(in, "ip_direction")) {
	*ix = ip_direction_indexer;
	*it = ip_direction_iterator;
	return 1;
    }
    if (0 == strcmp(in, "ip_proto")) {
	*ix = ip_proto_indexer;
	*it = ip_proto_iterator;
	return 1;
    }
    if (0 == strcmp(in, "ip_version")) {
	*ix = ip_version_indexer;
	*it = ip_version_iterator;
	return 1;
    }
    syslog(LOG_ERR, "unknown indexer '%s'", in);
    return 0;
}

static int
ip_message_find_filters(const char *fn, filter_list ** fl)
{
    char *t;
    char *copy = xstrdup(fn);
    if (NULL == copy)
	return 0;
    for (t = strtok(copy, ","); t; t = strtok(NULL, ",")) {
	if (0 == strcmp(t, "any")) {
	    continue;
	}
	syslog(LOG_ERR, "unknown filter '%s'", t);
	free(copy);
	return 0;
    }
    free(copy);
    return 1;
}

int
ip_message_add_array(const char *name, const char *fn, const char *fi,
    const char *sn, const char *si, const char *f, int min_count,
    int max_cells)
{
    filter_list *filters = NULL;
    IDXR *indexer1;
    HITR *iterator1;
    IDXR *indexer2;
    HITR *iterator2;
    md_array_list *a;

    if (0 == ip_message_find_indexer(fi, &indexer1, &iterator1))
	return 0;
    if (0 == ip_message_find_indexer(si, &indexer2, &iterator2))
	return 0;
    if (0 == ip_message_find_filters(f, &filters))
	return 0;

    a = xcalloc(1, sizeof(*a));
    if (NULL == a)
	return 0;
    a->theArray = md_array_create(name, filters,
	fn, indexer1, iterator1,
	sn, indexer2, iterator2);
    a->theArray->opts.min_count = min_count;
    a->theArray->opts.max_cells = max_cells;
    assert(a->theArray);
    a->next = Arrays;
    Arrays = a;
    return 1;
}

void
ip_message_report(void)
{
    md_array_list *a;
    for (a = Arrays; a; a = a->next)
	md_array_print(a->theArray, &xml_printer);
}
