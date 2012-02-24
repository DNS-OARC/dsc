#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netinet/in.h>
#if USE_IPV6
#include <netinet/ip6.h>
#endif

#include "xmalloc.h"
#include "inX_addr.h"
#include "dns_message.h"
#include "md_array.h"

#define LARGEST 2

struct _foo {
    inX_addr addr;
    struct _foo *next;
};

static struct _foo *local_addrs = NULL;

static int
ip_is_local(const inX_addr *a)
{
    struct _foo *t;
    for (t = local_addrs; t; t = t->next)
	if (0 == inXaddr_cmp(&t->addr, a))
	    return 1;
    return 0;
}

int
ip_direction_indexer(const void *vp)
{
    const dns_message *m = vp;
    const transport_message *tm = m->tm;
    if (ip_is_local(&tm->src_ip_addr))
	return 0;
    if (ip_is_local(&tm->dst_ip_addr))
	return 1;
    return LARGEST;
}

int
ip_local_address(const char *presentation)
{
    struct _foo *n = xcalloc(1, sizeof(*n));
    if (NULL == n)
	return 0;
    if (inXaddr_pton(presentation, &n->addr) != 1) {
	fprintf(stderr, "yucky IP address %s\n", presentation);
	xfree(n);
	return 0;
    }
    n->next = local_addrs;
    local_addrs = n;
    return 1;
}

int
ip_direction_iterator(char **label)
{
    static int next_iter = 0;
    if (NULL == label) {
	next_iter = 0;
	return LARGEST + 1;
    }
    if (0 == next_iter)
	*label = "sent";
    else if (1 == next_iter)
	*label = "recv";
    else if (LARGEST == next_iter)
	*label = "else";
    else
	return -1;
    return next_iter++;
}
