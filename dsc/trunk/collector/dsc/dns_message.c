#include <stdlib.h>
#include <assert.h>
#include <stdio.h>


#include "dns_message.h"
#include "md_array.h"
#include "qtype_index.h"
#include "qclass_index.h"
#include "tld_index.h"
#include "rcode_index.h"

extern md_array_printer xml_printer;
static md_array *qclass_vs_qtype;
static md_array *qtype_vs_tld;
static md_array *rcode_vs_tld;

void
dns_message_handle(dns_message * m)
{
    md_array_count(qclass_vs_qtype, m);
    md_array_count(qtype_vs_tld, m);
    md_array_count(rcode_vs_tld, m);
}

static int
queries_only_filter(dns_message * m)
{
    return m->qr ? 0 : 1;
}

static int
replies_only_filter(dns_message * m)
{
    return m->qr ? 1 : 0;
}

void
dns_message_init(void)
{
    qclass_vs_qtype = md_array_create(
	queries_only_filter,
	"Qclass", qclass_indexer, qclass_iterator,
	"Qtype", qtype_indexer, qtype_iterator);

    qtype_vs_tld = md_array_create(
	queries_only_filter,
	"Qtype", qtype_indexer, qtype_iterator,
	"TLD", tld_indexer, tld_iterator);

    rcode_vs_tld = md_array_create(
	replies_only_filter,
	"Rcode", rcode_indexer, rcode_iterator,
	"TLD", tld_indexer, tld_iterator);

}

void
dns_message_report(void)
{
    md_array_print(qclass_vs_qtype, &xml_printer, "qclass_vs_qtype");
    md_array_print(qtype_vs_tld, &xml_printer, "qtype_vs_tld");
    md_array_print(rcode_vs_tld, &xml_printer, "rcode_vs_tld");
}
