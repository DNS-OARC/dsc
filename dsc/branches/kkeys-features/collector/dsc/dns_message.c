#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/nameser.h>

#include <sys/types.h>
#include <regex.h>

#ifndef T_A6
#define T_A6 38
#endif


#include "xmalloc.h"
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
#include "qname_index.h"
#include "msglen_index.h"
#include "certain_qnames_index.h"
#include "idn_qname_index.h"
#include "query_classification_index.h"
#include "edns_version_index.h"
#include "do_bit_index.h"
#include "rd_bit_index.h"
#include "opcode_index.h"
#include "syslog_debug.h"

extern md_array_printer xml_printer;
extern int debug_flag;
static md_array_list *Arrays = NULL;
static filter_list *DNSFilters = NULL;

void
dns_message_print(dns_message * m)
{
	char buf[128];
	inXaddr_ntop(&m->client_ip_addr, buf, 128);
	fprintf(stderr, "%15s:%5d", buf, m->tm->src_port);
	fprintf(stderr, "\tQT=%d", m->qtype);
	fprintf(stderr, "\tQC=%d", m->qclass);
	fprintf(stderr, "\tlen=%d", m->msglen);
	fprintf(stderr, "\tqname=%s", m->qname);
	fprintf(stderr, "\ttld=%s", dns_message_tld(m));
	fprintf(stderr, "\topcode=%d", m->opcode);
	fprintf(stderr, "\trcode=%d", m->rcode);
	fprintf(stderr, "\tmalformed=%d", m->malformed);
	fprintf(stderr, "\tqr=%d", m->qr);
	fprintf(stderr, "\trd=%d", m->rd);
	fprintf(stderr, "\n");
}

void
dns_message_handle(dns_message * m)
{
    md_array_list *a;
    if (debug_flag)
	dns_message_print(m);
    for (a = Arrays; a; a = a->next)
	md_array_count(a->theArray, m);
}

static int
queries_only_filter(const void *vp, const void *ctx)
{
    const dns_message *m = vp;
    return m->qr ? 0 : 1;
}

static int
popular_qtypes_filter(const void *vp, const void *ctx)
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
aaaa_or_a6_filter(const void *vp, const void *ctx)
{
    const dns_message *m = vp;
    switch (m->qtype) {
    case T_AAAA:
    case T_A6:
	return 1;
    default:
	return 0;
    }
    return 0;
}

static int
idn_qname_filter(const void *vp, const void *ctx)
{
    const dns_message *m = vp;
    return (0 == strncmp(m->qname, "xn--", 4));
}

static int
root_servers_net_filter(const void *vp, const void *ctx)
{
    const dns_message *m = vp;
    return (0 == strcmp(m->qname + 1, ".root-servers.net"));
}

static int
chaos_class_filter(const void *vp, const void *ctx)
{
    const dns_message *m = vp;
    return m->qclass == C_CHAOS;
}

static int
replies_only_filter(const void *vp, const void *ctx)
{
    const dns_message *m = vp;
    return m->qr ? 1 : 0;
}

static int
qname_filter(const void *vp, const void *ctx)
{
    const regex_t *r = ctx;
    const dns_message *m = vp;
    return (0 == regexec(r, m->qname, 0, NULL, 0));
}


static int
dns_message_find_indexer(const char *in, IDXR ** ix, HITR ** it)
{
    if (0 == strcmp(in, "client")) {
	*ix = cip_indexer;
	*it = cip_iterator;
	return 1;
    }
    if (0 == strcmp(in, "cip4_addr")) {		/* compatibility */
	*ix = cip_indexer;
	*it = cip_iterator;
	return 1;
    }
    if (0 == strcmp(in, "client_subnet")) {
	*ix = cip_net_indexer;
	*it = cip_net_iterator;
	return 1;
    }
    if (0 == strcmp(in, "cip4_net")) {		/* compatibility */
	*ix = cip_net_indexer;
	*it = cip_net_iterator;
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
    if (0 == strcmp(in, "qname")) {
	*ix = qname_indexer;
	*it = qname_iterator;
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
    if (0 == strcmp(in, "do_bit")) {
	*ix = do_bit_indexer;
	*it = do_bit_iterator;
	return 1;
    }
    if (0 == strcmp(in, "d0_bit")) {	/* compat for bug */
	*ix = do_bit_indexer;
	*it = do_bit_iterator;
	return 1;
    }
    if (0 == strcmp(in, "rd_bit")) {
	*ix = rd_bit_indexer;
	*it = rd_bit_iterator;
	return 1;
    }
    if (0 == strcmp(in, "opcode")) {
	*ix = opcode_indexer;
	*it = opcode_iterator;
	return 1;
    }
    syslog(LOG_ERR, "unknown indexer '%s'", in);
    return 0;
}

static int
dns_message_find_filters(const char *fn, filter_list ** fl)
{
    char *t;
    char *copy = xstrdup(fn);
    filter_list *f;
    if (NULL == copy)
	return 0;
    for (t = strtok(copy, ","); t; t = strtok(NULL, ",")) {
	if (0 == strcmp(t, "any"))
	    continue;
	for (f = DNSFilters; f; f = f->next) {
	    if (0 == strcmp(t, f->filter->name))
		break;
	}
	if (f) {
	    fl = md_array_filter_list_append(fl, f->filter);
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
add_qname_filter(const char *name, const char *pat)
{
    filter_list **fl = &DNSFilters;
    regex_t *r;
    int x;
    while ((*fl))
	fl = &((*fl)->next);
    r = xcalloc(1, sizeof(*r));
    if (NULL == r) {
	syslog(LOG_ERR, "Cant allocate memory for '%s' qname filter", name);
	return 0;
    }
    if (0 != (x = regcomp(r, pat, REG_EXTENDED | REG_ICASE))) {
	char errbuf[512];
	regerror(x, r, errbuf, 512);
	syslog(LOG_ERR, "regcomp: %s", errbuf);
    }
    fl = md_array_filter_list_append(fl,
	md_array_create_filter(name, qname_filter, r));
    return 1;
}

int
dns_message_add_array(const char *name, const char *fn, const char *fi,
    const char *sn, const char *si, const char *f, int min_count,
    int max_cells)
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

    a = xcalloc(1, sizeof(*a));
    if (a == NULL) {
	syslog(LOG_ERR, "Cant allocate memory for '%s' DNS message array", name);
	return 0;
    }
    a->theArray = md_array_create(name, filters,
	fn, indexer1, iterator1,
	sn, indexer2, iterator2);
    if (NULL == a->theArray) {
	syslog(LOG_ERR, "Cant allocate memory for '%s' DNS message array", name);
	return 0;
    }
    a->theArray->opts.min_count = min_count;
    a->theArray->opts.max_cells = max_cells;
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

void
dns_message_init(void)
{
    filter_list **fl = &DNSFilters;
    fl = md_array_filter_list_append(fl,
	md_array_create_filter("queries-only", queries_only_filter, 0));
    fl = md_array_filter_list_append(fl,
	md_array_create_filter("replies-only", replies_only_filter, 0));
    fl = md_array_filter_list_append(fl,
	md_array_create_filter("popular-qtypes", popular_qtypes_filter, 0));
    fl = md_array_filter_list_append(fl,
	md_array_create_filter("idn-only", idn_qname_filter, 0));
    fl = md_array_filter_list_append(fl,
	md_array_create_filter("aaaa-or-a6-only", aaaa_or_a6_filter, 0));
    fl = md_array_filter_list_append(fl,
	md_array_create_filter("root-servers-net-only", root_servers_net_filter, 0));
    fl = md_array_filter_list_append(fl,
	md_array_create_filter("chaos-class", chaos_class_filter, 0));
}
