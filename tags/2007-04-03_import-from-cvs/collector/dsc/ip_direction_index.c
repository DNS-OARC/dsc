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
#include "ip_message.h"
#include "md_array.h"

#define LARGEST 2

struct _foo {
    inX_addr addr;
    struct _foo *next;
};

static struct _foo *local_addrs = NULL;

static int
ip_is_local(inX_addr *a)
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
    const struct ip *ip = vp;
    inX_addr a;
    if (4 == ip->ip_v) {
	inXaddr_assign_v4(&a, &ip->ip_src);
	if (ip_is_local(&a))
	    return 0;
	inXaddr_assign_v4(&a, &ip->ip_dst);
	if (ip_is_local(&a))
	    return 1;
#if USE_IPV6
    } else if (6 == ip->ip_v) {
	const struct ip6_hdr * ip6 = vp;
	inXaddr_assign_v6(&a, &ip6->ip6_src);
	if (ip_is_local(&a))
	    return 0;
	inXaddr_assign_v6(&a, &ip6->ip6_dst);
	if (ip_is_local(&a))
	    return 1;
#endif
    }
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
	free(n);
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
