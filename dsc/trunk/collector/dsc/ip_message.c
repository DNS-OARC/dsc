#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "ip_message.h"
#include "md_array.h"

#include "ip_direction_index.h"
#include "ip_proto_index.h"

extern md_array_printer xml_printer;

static md_array *direction_vs_ipproto;

void
ip_message_handle(const struct ip *ip)
{
    md_array_count(direction_vs_ipproto, ip);
}

void
ip_message_init(void)
{
    direction_vs_ipproto = md_array_create(
	NULL,
	"Direction", ip_direction_indexer, ip_direction_iterator,
	"IPProto", ip_proto_indexer, ip_proto_iterator);
}

void
ip_message_report(void)
{
    md_array_print(direction_vs_ipproto, &xml_printer, "direction_vs_ipproto");
}
