#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <syslog.h>


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
replies_only_filter(const void *vp)
{
    const dns_message *m = vp;
    return m->qr ? 1 : 0;
}

void
dns_message_init(void)
{
    assert(Arrays == NULL);

#if 0
    qtype = md_array_create(
	queries_only_filter,
	"All", null_indexer, null_iterator,
	"Qtype", qtype_indexer, qtype_iterator);

    rcode = md_array_create(
	replies_only_filter,
	"All", null_indexer, null_iterator,
	"Rcode", rcode_indexer, rcode_iterator);

    client_subnet = md_array_create(
	queries_only_filter,
	"All", null_indexer, null_iterator,
	"ClientSubnet", cip4_net_indexer, cip4_net_iterator);

    qtype_vs_qnamelen = md_array_create(
	queries_only_filter,
	"Qtype", qtype_indexer, qtype_iterator,
	"QnameLen", qnamelen_indexer, qnamelen_iterator);
#endif

}


int
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
    if (0 == strcmp(in, "qtype")) {
	*ix = qtype_indexer;
	*it = qtype_iterator;
	return 1;
    }
    if (0 == strcmp(in, "qnamelen")) {
	*ix = qnamelen_indexer;
	*it = qnamelen_iterator;
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
    syslog(LOG_ERR, "unknown indexer '%s'", in);
    return 0;
}

int
dns_message_find_filter(const char *fn, FLTR ** f)
{
    if (0 == strcmp(fn, "any")) {
	*f = NULL;
	return 1;
    }
    if (0 == strcmp(fn, "queries-only")) {
	*f = queries_only_filter;
	return 1;
    }
    if (0 == strcmp(fn, "replies-only")) {
	*f = replies_only_filter;
	return 1;
    }
    syslog(LOG_ERR, "unknown filter '%s'", fn);
    return 0;
}

int
dns_message_add_array(const char *name, const char *fn, const char *fi,
    const char *sn, const char *si, const char *f)
{
    FLTR *filter;
    IDXR *indexer1;
    HITR *iterator1;
    IDXR *indexer2;
    HITR *iterator2;
    md_array_list *a;

syslog(LOG_DEBUG, "%s:%d, name=%s\n", __FILE__,__LINE__,name);
syslog(LOG_DEBUG, "%s:%d, fn=%s\n", __FILE__,__LINE__,fn);
syslog(LOG_DEBUG, "%s:%d, fi=%s\n", __FILE__,__LINE__,fi);
syslog(LOG_DEBUG, "%s:%d, sn=%s\n", __FILE__,__LINE__,sn);
syslog(LOG_DEBUG, "%s:%d, si=%s\n", __FILE__,__LINE__,si);
syslog(LOG_DEBUG, "%s:%d, f=%s\n", __FILE__,__LINE__,f);

    if (0 == dns_message_find_indexer(fi, &indexer1, &iterator1))
	return 0;
    if (0 == dns_message_find_indexer(si, &indexer2, &iterator2))
	return 0;
    if (0 == dns_message_find_filter(f, &filter))
	return 0;

    a = calloc(1, sizeof(*a));
    a->theArray = md_array_create(filter,
	fn, indexer1, iterator1,
	sn, indexer2, iterator2);
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
