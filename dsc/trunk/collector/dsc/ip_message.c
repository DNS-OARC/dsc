#include <stdlib.h>
#include <assert.h>
#include <stdio.h>


#include "ip_message.h"
#include "md_array.h"

extern md_array_printer xml_printer;

void
ip_message_handle(const void * m)
{
    const struct ip *ip = m;
#if 0
    md_array_count(qtype, m);
    md_array_count(rcode, m);
    md_array_count(client_subnet, m);
    md_array_count(qtype_vs_qnamelen, m);
#endif
}

void
ip_message_init(void)
{
}

void
ip_message_report(void)
{
}
