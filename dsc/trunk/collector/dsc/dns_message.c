#include <stdlib.h>
#include <assert.h>
#include <stdio.h>


#include "dns_message.h"
#include "md_array.h"
#include "qtype_index.h"
#include "qclass_index.h"
#include "tld_index.h"

static md_array_printer stderr_printer;
extern md_array_printer xml_printer;
static md_array *qclass_vs_qtype;
static md_array *qtype_vs_tld;

void
dns_message_handle(dns_message * m)
{
    md_array_count(qclass_vs_qtype, m);
    md_array_count(qtype_vs_tld, m);
    md_array_print(qclass_vs_qtype, &xml_printer);
    md_array_print(qtype_vs_tld, &xml_printer);
}

void
x_start_array(void)
{
}

void
x_d1_begin(char *l)
{
	fprintf(stderr, "%s\n", l);
}

void
x_d1_end(char *l)
{
}

void
x_print_element(char *l, int val)
{
	fprintf(stderr, "\t%s=%d\n", l,val);
}

void
dns_message_init(void)
{
    qclass_vs_qtype = md_array_create(
	"Qclass", qclass_indexer, qclass_iterator,
	"Qtype", qtype_indexer, qtype_iterator);

    qtype_vs_tld = md_array_create(
	"Qtype", qtype_indexer, qtype_iterator,
	"TLD", tld_indexer, tld_iterator);

    stderr_printer.start_array = x_start_array;
    stderr_printer.d1_begin = x_d1_begin;
    stderr_printer.d1_end = x_d1_end;
    stderr_printer.print_element = x_print_element;
}
