#include <stdlib.h>
#include <assert.h>
#include <stdio.h>


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
#if USE_QCLASS_VS_QTYPE
static md_array *qclass_vs_qtype;
#endif
static md_array *qtype;
static md_array *rcode;
static md_array *client_subnet;
static md_array *qtype_vs_qnamelen;

void
dns_message_handle(dns_message * m)
{
#if USE_QCLASS_VS_QTYPE
    md_array_count(qclass_vs_qtype, m);
#endif
    md_array_count(qtype, m);
    md_array_count(rcode, m);
    md_array_count(client_subnet, m);
    md_array_count(qtype_vs_qnamelen, m);
}

static int
queries_only_filter(const void * vp)
{
    const dns_message * m = vp;
    return m->qr ? 0 : 1;
}

static int
replies_only_filter(const void * vp)
{
    const dns_message * m = vp;
    return m->qr ? 1 : 0;
}

void
dns_message_init(void)
{
#if USE_QCLASS_VS_QTYPE
    qclass_vs_qtype = md_array_create(
	queries_only_filter,
	"Qclass", qclass_indexer, qclass_iterator,
	"Qtype", qtype_indexer, qtype_iterator);
#endif

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

}

void
dns_message_report(void)
{
#if USE_QCLASS_VS_QTYPE
    md_array_print(qclass_vs_qtype, &xml_printer, "qclass_vs_qtype");
#endif
    md_array_print(qtype, &xml_printer, "qtype");
    md_array_print(rcode, &xml_printer, "rcode");
    md_array_print(client_subnet, &xml_printer, "client_subnet");
    md_array_print(qtype_vs_qnamelen, &xml_printer, "qtype_vs_qnamelen");
}
