#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <netdb.h>

#include "ip_message.h"
#include "md_array.h"

static int largest = 0;

int
ip_proto_indexer(const void * vp)
{
    const struct ip *ip = vp;
    int i = (int) ip->ip_p;
    if (i > largest)
	largest = i;
    return i;
}

static int next_iter = 0;

int
ip_proto_iterator(char **label)
{
    static char label_buf[20];
    struct protoent *p;
    if (NULL == label) {
	next_iter = 0;
	return largest;
    }
    if (next_iter > largest)
	return -1;
    p = getprotobynumber(next_iter);
    if (p)
	*label = p->p_name;
    else
        snprintf(*label = label_buf, 20, "p%d", next_iter);
    return next_iter++;
}
