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
#include "syslog_debug.h"

extern md_array_printer xml_printer;
static md_array_list *Arrays = NULL;

void
ip_message_handle(const ip_message *ip)
{
    md_array_list *a;
    for (a = Arrays; a; a = a->next)
	md_array_count(a->theArray, ip);
}

static indexer_t indexers[] = {
    { "ip_direction", ip_direction_indexer, ip_direction_iterator, NULL },
    { "ip_proto",     ip_proto_indexer,     ip_proto_iterator,     ip_proto_reset },
    { "ip_version",   ip_version_indexer,   ip_version_iterator,   ip_version_reset },
    { NULL,           NULL,                 NULL,                  NULL }
};

static indexer_t *
ip_message_find_indexer(const char *in)
{
    indexer_t *indexer;
    for (indexer = indexers; indexer->name; indexer++) {
        if (0 == strcmp(in, indexer->name))
            return indexer;
    }
    syslog(LOG_ERR, "unknown indexer '%s'", in);
    return NULL;
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
	xfree(copy);
	return 0;
    }
    xfree(copy);
    return 1;
}

int
ip_message_add_array(const char *name, const char *fn, const char *fi,
    const char *sn, const char *si, const char *f, dataset_opt opts)
{
    filter_list *filters = NULL;
    indexer_t *indexer1, *indexer2;
    md_array_list *a;

    if (NULL == (indexer1 = ip_message_find_indexer(fi)))
	return 0;
    if (NULL == (indexer2 = ip_message_find_indexer(si)))
	return 0;
    if (0 == ip_message_find_filters(f, &filters))
	return 0;

    a = xcalloc(1, sizeof(*a));
    if (NULL == a)
	return 0;
    a->theArray = md_array_create(name, filters,
	fn, indexer1, sn, indexer2);
    a->theArray->opts = opts;
    assert(a->theArray);
    a->next = Arrays;
    Arrays = a;
    return 1;
}

void
ip_message_report(FILE *fp)
{
    md_array_list *a;
    for (a = Arrays; a; a = a->next)
	md_array_print(a->theArray, &xml_printer, fp);
}

void
ip_message_clear_arrays(void)
{
    md_array_list *a;
    for (a = Arrays; a; a = a->next)
	md_array_clear(a->theArray);
}
