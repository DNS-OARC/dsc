#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>


#include "dns_message.h"
#include "md_array.h"
#include "null_index.h"
#include "qtype_index.h"
#include "qclass_index.h"
#include "tld_index.h"
#include "rcode_index.h"
#include "client_ipv4_addr_index.h"
#include "client_ipv4_net_index.h"
#include "qnamelen_index.h"
#include "msglen_index.h"
#include "certain_qnames_index.h"
#include "idn_qname_index.h"
#include "query_classification_index.h"
#include "edns_version_index.h"
#include "d0_bit_index.h"

extern md_array_printer xml_printer;
static md_array_list *Arrays = NULL;

void
dns_message_handle(dns_message * m)
{
    md_array_list *a;
    for (a = Arrays; a; a = a->next)
	md_array_count(a->theArray, m);
}

static int
queries_only_filter(const void *vp)
{
    const dns_message *m = vp;
    return m->qr ? 0 : 1;
}

static int
popular_qtypes_filter(const void *vp)
{
    const dns_message *m = vp;
    switch (m->qtype) {
    case 1:
    case 2:
    case 5:
    case 6:
    case 12:
    case 15:
    case 28:
    case 33:
    case 38:
    case 255:
	return 1;
    default:
	return 0;
    }
    return 0;
}

static int
replies_only_filter(const void *vp)
{
    const dns_message *m = vp;
    return m->qr ? 1 : 0;
}


static int
dns_message_find_indexer(const char *in, IDXR ** ix, HITR ** it)
{
    if (0 == strcmp(in, "client")) {
	*ix = cip4_indexer;
	*it = cip4_iterator;
	return 1;
    }
    if (0 == strcmp(in, "cip4_net")) {
	*ix = cip4_net_indexer;
	*it = cip4_net_iterator;
	return 1;
    }
    if (0 == strcmp(in, "null")) {
	*ix = null_indexer;
	*it = null_iterator;
	return 1;
    }
    if (0 == strcmp(in, "qclass")) {
	*ix = qclass_indexer;
	*it = qclass_iterator;
	return 1;
    }
    if (0 == strcmp(in, "qnamelen")) {
	*ix = qnamelen_indexer;
	*it = qnamelen_iterator;
	return 1;
    }
    if (0 == strcmp(in, "msglen")) {
	*ix = msglen_indexer;
	*it = msglen_iterator;
	return 1;
    }
    if (0 == strcmp(in, "qtype")) {
	*ix = qtype_indexer;
	*it = qtype_iterator;
	return 1;
    }
    if (0 == strcmp(in, "rcode")) {
	*ix = rcode_indexer;
	*it = rcode_iterator;
	return 1;
    }
    if (0 == strcmp(in, "tld")) {
	*ix = tld_indexer;
	*it = tld_iterator;
	return 1;
    }
    if (0 == strcmp(in, "certain_qnames")) {
	*ix = certain_qnames_indexer;
	*it = certain_qnames_iterator;
	return 1;
    }
    if (0 == strcmp(in, "query_classification")) {
	*ix = query_classification_indexer;
	*it = query_classification_iterator;
	return 1;
    }
    if (0 == strcmp(in, "idn_qname")) {
	*ix = idn_qname_indexer;
	*it = idn_qname_iterator;
	return 1;
    }
    if (0 == strcmp(in, "edns_version")) {
	*ix = edns_version_indexer;
	*it = edns_version_iterator;
	return 1;
    }
    if (0 == strcmp(in, "d0_bit")) {
	*ix = d0_bit_indexer;
	*it = d0_bit_iterator;
	return 1;
    }
    syslog(LOG_ERR, "unknown indexer '%s'", in);
    return 0;
}

static int
dns_message_find_filters(const char *fn, filter_list ** fl)
{
    char *t;
    char *copy = strdup(fn);
    for (t = strtok(copy, ","); t; t = strtok(NULL, ",")) {
	if (0 == strcmp(t, "any")) {
	    continue;
	}
	if (0 == strcmp(t, "queries-only")) {
	    fl = md_array_filter_list_append(fl, queries_only_filter);
	    continue;
	}
	if (0 == strcmp(t, "replies-only")) {
	    fl = md_array_filter_list_append(fl, replies_only_filter);
	    continue;
	}
	if (0 == strcmp(t, "popular-qtypes")) {
	    fl = md_array_filter_list_append(fl, popular_qtypes_filter);
	    continue;
	}
	syslog(LOG_ERR, "unknown filter '%s'", t);
	return 0;
    }
    return 1;
}

int
dns_message_add_array(const char *name, const char *fn, const char *fi,
    const char *sn, const char *si, const char *f, int min_count)
{
    filter_list *filters = NULL;
    IDXR *indexer1;
    HITR *iterator1;
    IDXR *indexer2;
    HITR *iterator2;
    md_array_list *a;

    if (0 == dns_message_find_indexer(fi, &indexer1, &iterator1))
	return 0;
    if (0 == dns_message_find_indexer(si, &indexer2, &iterator2))
	return 0;
    if (0 == dns_message_find_filters(f, &filters))
	return 0;

    a = calloc(1, sizeof(*a));
    a->theArray = md_array_create(name, filters,
	fn, indexer1, iterator1,
	sn, indexer2, iterator2);
    a->theArray->opts.min_count = min_count;
    assert(a->theArray);
    a->next = Arrays;
    Arrays = a;
    return 1;
}

void
dns_message_report(void)
{
    md_array_list *a;
    for (a = Arrays; a; a = a->next)
	md_array_print(a->theArray, &xml_printer);
}

const char *
dns_message_tld(dns_message * m)
{
    if (NULL == m->tld) {
	m->tld = m->qname + strlen(m->qname) - 2;
	while (m->tld >= m->qname && (*m->tld != '.'))
	    m->tld--;
	if (*m->tld == '.')
	    m->tld++;
	if (m->tld < m->qname)
	    m->tld = m->qname;
    }
    return m->tld;
}
