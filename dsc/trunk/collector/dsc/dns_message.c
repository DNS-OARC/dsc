#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <syslog.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/nameser.h>
#include <arpa/nameser_compat.h>

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
#if HAVE_LIBGEOIP
#include "country_index.h"
#endif
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
#include "edns_bufsiz_index.h"
#include "do_bit_index.h"
#include "rd_bit_index.h"
#include "tc_bit_index.h"
#include "qr_aa_bits_index.h"
#include "opcode_index.h"
#include "transport_index.h"
#include "dns_ip_version_index.h"
#include "dns_source_port_index.h"

#include "ip_direction_index.h"
#include "ip_proto_index.h"
#include "ip_version_index.h"

#include "syslog_debug.h"

extern md_array_printer xml_printer;
extern int debug_flag;
static md_array_list *Arrays = NULL;
static filter_list *DNSFilters = NULL;

static const char *printable_dnsname(const char *name)
{
    static char buf[MAX_QNAME_SZ];
    int i;

    for (i = 0; i < sizeof(buf) - 1; name++) {
	if (!*name)
	    break;
	if (isgraph(*name)) {
	    buf[i] = *name;
	    i++;
	} else {
	    if (i + 3 > MAX_QNAME_SZ - 1)
		break; /* expanded character would overflow buffer */
	    sprintf(buf+i, "%%%02x", (unsigned char)*name);
	    i += 3;
	}
    }
    buf[i] = '\0';
    return buf;
}

void
dns_message_print(dns_message * m)
{
	char buf[128];
	inXaddr_ntop(&m->client_ip_addr, buf, 128);
	fprintf(stderr, "%15s:%5d", buf, m->tm->src_port);
	fprintf(stderr, "\t%s", (m->tm->proto == IPPROTO_UDP) ? "UDP" :
	    (m->tm->proto == IPPROTO_TCP) ? "TCP" : "???");
	fprintf(stderr, "\tQT=%d", m->qtype);
	fprintf(stderr, "\tQC=%d", m->qclass);
	fprintf(stderr, "\tlen=%d", m->msglen);
	fprintf(stderr, "\tqname=%s", printable_dnsname(m->qname));
	fprintf(stderr, "\ttld=%s", printable_dnsname(dns_message_tld(m)));
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
    if (debug_flag > 1)
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
priming_query_filter(const void *vp, const void *ctx)
{
    const dns_message *m = vp;
    if (m->qtype != T_NS)
	return 0;
    if (0 != strcmp(m->qname, "."))
	return 0;
    return 1;
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

static indexer_t indexers[] = {
    { "client",               cip_indexer,                  cip_iterator,                  cip_reset },
    { "cip4_addr",            cip_indexer,                  cip_iterator,                  cip_reset },     /* compatibility */
#if HAVE_LIBGEOIP
    { "country",                  country_indexer,                  country_iterator,                  country_reset },
#endif
    { "client_subnet",        cip_net_indexer,              cip_net_iterator,              cip_net_reset },
    { "cip4_net",             cip_net_indexer,              cip_net_iterator,              cip_net_reset }, /* compatibility */
    { "null",                 null_indexer,                 null_iterator,                 NULL },
    { "qclass",               qclass_indexer,               qclass_iterator,               qclass_reset },
    { "qnamelen",             qnamelen_indexer,             qnamelen_iterator,             qnamelen_reset },
    { "qname",                qname_indexer,                qname_iterator,                qname_reset },
    { "second_ld",            second_ld_indexer,            second_ld_iterator,            second_ld_reset },
    { "third_ld",             third_ld_indexer,             third_ld_iterator,             third_ld_reset },
    { "msglen",               msglen_indexer,               msglen_iterator,               msglen_reset },
    { "qtype",                qtype_indexer,                qtype_iterator,                qtype_reset },
    { "rcode",                rcode_indexer,                rcode_iterator,                rcode_reset },
    { "tld",                  tld_indexer,                  tld_iterator,                  tld_reset },
    { "certain_qnames",       certain_qnames_indexer,       certain_qnames_iterator,       NULL },
    { "query_classification", query_classification_indexer, query_classification_iterator, NULL },
    { "idn_qname",            idn_qname_indexer,            idn_qname_iterator,            NULL },
    { "edns_version",         edns_version_indexer,         edns_version_iterator,         NULL },
    { "edns_bufsiz",          edns_bufsiz_indexer,          edns_bufsiz_iterator,         NULL },
    { "do_bit",               do_bit_indexer,               do_bit_iterator,               NULL },
    { "d0_bit",               do_bit_indexer,               do_bit_iterator,               NULL },          /* compat for bug */
    { "rd_bit",               rd_bit_indexer,               rd_bit_iterator,               NULL },
    { "tc_bit",               tc_bit_indexer,               tc_bit_iterator,               NULL },
    { "opcode",               opcode_indexer,               opcode_iterator,               opcode_reset },
    { "transport",            transport_indexer,            transport_iterator,            NULL },
    { "dns_ip_version",       dns_ip_version_indexer,       dns_ip_version_iterator,       dns_ip_version_reset },
    { "dns_source_port",      dns_source_port_indexer,      dns_source_port_iterator,      dns_source_port_reset },
    { "dns_sport_range",      dns_sport_range_indexer,      dns_sport_range_iterator,      dns_sport_range_reset },
    { "qr_aa_bits",           qr_aa_bits_indexer,           qr_aa_bits_iterator,           NULL, },
    /* these used to be "IP" indexers */
    { "ip_direction",         ip_direction_indexer,         ip_direction_iterator,         NULL },
    { "ip_proto",             ip_proto_indexer,             ip_proto_iterator,             ip_proto_reset },
    { "ip_version",           ip_version_indexer,           ip_version_iterator,           ip_version_reset },
    { NULL,                   NULL,                         NULL,                          NULL }
};

static indexer_t *
dns_message_find_indexer(const char *in)
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
	xfree(copy);
	return 0;
    }
    xfree(copy);
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
    const char *sn, const char *si, const char *f, dataset_opt opts)
{
    filter_list *filters = NULL;
    indexer_t *indexer1, *indexer2;
    md_array_list *a;

    if (NULL == (indexer1 = dns_message_find_indexer(fi)))
	return 0;
    if (NULL == (indexer2 = dns_message_find_indexer(si)))
	return 0;
    if (0 == dns_message_find_filters(f, &filters))
	return 0;

    a = xcalloc(1, sizeof(*a));
    if (a == NULL) {
	syslog(LOG_ERR, "Cant allocate memory for '%s' DNS message array", name);
	return 0;
    }
    a->theArray = md_array_create(name, filters,
	fn, indexer1, sn, indexer2);
    if (NULL == a->theArray) {
	syslog(LOG_ERR, "Cant allocate memory for '%s' DNS message array", name);
	return 0;
    }
    a->theArray->opts = opts;
    assert(a->theArray);
    a->next = Arrays;
    Arrays = a;
    return 1;
}

void
dns_message_report(FILE *fp)
{
    md_array_list *a;
    for (a = Arrays; a; a = a->next)
	md_array_print(a->theArray, &xml_printer, fp);
}

void
dns_message_clear_arrays(void)
{
    md_array_list *a;
    for (a = Arrays; a; a = a->next)
	md_array_clear(a->theArray);
}

/*
 * QnameToNld
 *
 * qname is a 0-terminated string containing a DNS name
 * nld is the domain level to find
 *
 * return value is a pointer into the qname string.
 *
 * Handles the following cases:
 *    qname is empty ("")
 *    qname ends with one or more dots
 *    qname begins with one or more dots
 *    multiple consequtive dots in qname
 *
 * TESTS
 *	assert(0 == strcmp(QnameToNld("a.b.c.d", 1), "d"));
 *	assert(0 == strcmp(QnameToNld("a.b.c.d", 2), "c.d"));
 *	assert(0 == strcmp(QnameToNld("a.b.c.d.", 2), "c.d."));
 *	assert(0 == strcmp(QnameToNld("a.b.c.d....", 2), "c.d...."));
 *	assert(0 == strcmp(QnameToNld("c.d", 5), "c.d"));
 *	assert(0 == strcmp(QnameToNld(".c.d", 5), "c.d"));
 *	assert(0 == strcmp(QnameToNld(".......c.d", 5), "c.d"));
 *	assert(0 == strcmp(QnameToNld("", 1), ""));
 *	assert(0 == strcmp(QnameToNld(".", 1), "."));
 *	assert(0 == strcmp(QnameToNld("a.b..c..d", 2), "c..d"));
 *	assert(0 == strcmp(QnameToNld("a.b................c..d", 3), "b................c..d"));
 */
const char *
dns_message_QnameToNld(const char *qname, int nld)
{
    const char *e = qname + strlen(qname) - 1;
    const char *t;
    int dotcount = 0;
    int state = 0;	/* 0 = not in dots, 1 = in dots */
    while (*e == '.' && e > qname)
	e--;
    t = e;
    if (0 == strcmp(t, ".arpa"))
        dotcount--;
    while (t > qname && dotcount < nld) {
        t--;
        if ('.' == *t) {
	    if (0 == state)
		dotcount++;
	    state = 1;
	} else {
	    state = 0;
	}
    }
    while (*t == '.' && t < e)
        t++;
    return t;
}

const char *
dns_message_tld(dns_message * m)
{
    if (NULL == m->tld)
	m->tld = dns_message_QnameToNld(m->qname, 1);
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
    fl = md_array_filter_list_append(fl,
	md_array_create_filter("priming-query", priming_query_filter, 0));
}
