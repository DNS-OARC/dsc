#include <stdlib.h>
#include <assert.h>
#include <stdio.h>


#include "dns_message.h"
#include "md_array.h"
#include "qtype_index.h"
#include "qclass_index.h"

static md_array *qclass_vs_qtype;
static md_array_printer stderr_printer;

void
dns_message_handle(dns_message * m)
{
    fprintf(stderr, "handling a message\n");
    md_array_count(qclass_vs_qtype, m);
    md_array_print(qclass_vs_qtype, &stderr_printer);
}

void
x_start_array(void)
{
}

void
x_d1_begin(char *t, char *v)
{
	fprintf(stderr, "%s=%s\n", t, v);
}

void
x_d1_end(char *t, char *v)
{
}

void
x_print_element(char *t, char *v, int val)
{
	fprintf(stderr, "\t%s=%s=%d\n", t,v,val);
}

void
dns_message_init(void)
{
    qclass_vs_qtype = md_array_create(
	"Qclass", qclass_indexer, qclass_iterator,
	"Qtype", qtype_indexer, qtype_iterator);
    stderr_printer.start_array = x_start_array;
    stderr_printer.d1_begin = x_d1_begin;
    stderr_printer.d1_end = x_d1_end;
    stderr_printer.print_element = x_print_element;
}
