#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "ip_message.h"
#include "md_array.h"

#define LARGEST 2

struct _foo {
    struct in_addr addr;
    struct _foo *next;
};

static struct _foo *local_addrs = NULL;

static int
ip_is_local(struct in_addr a)
{
    struct _foo *t;
    for (t = local_addrs; t; t = t->next)
	if (t->addr.s_addr == a.s_addr)
	    return 1;
    return 0;
}

int
ip_direction_indexer(const void *vp)
{
    const struct ip *ip = vp;
    if (ip_is_local(ip->ip_src))
	return 0;
    if (ip_is_local(ip->ip_dst))
	return 1;
    return LARGEST;
}

int
ip_local_address(const char *dotted)
{
    struct _foo *n = calloc(1, sizeof(*n));
    n->next = local_addrs;
    if (inet_aton(dotted, &n->addr) != 1) {
	fprintf(stderr, "yucky IPv4 addr %s\n", dotted);
	return 0;
    }
    local_addrs = n;
    return 1;
}

int
ip_direction_iterator(char **label)
{
    static int next_iter = 0;
    if (NULL == label) {
	next_iter = 0;
	return LARGEST;
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
