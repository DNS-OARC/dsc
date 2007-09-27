#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <netdb.h>

/*
 * This is very similar to transport_index, but this
 * indexer is applied only for DNS messages, rather than
 * all IP packets.
 */

#include "dns_message.h"
#include "md_array.h"

#define LARGEST 2

int
transport_indexer(const void *vp)
{
    const dns_message *dns = vp;
    if (IPPROTO_UDP == dns->tm->proto)
	return 0;
    if (IPPROTO_TCP == dns->tm->proto)
	return 1;
    return LARGEST;
}

static int next_iter = 0;

int
transport_iterator(char **label)
{
    if (NULL == label) {
	next_iter = 0;
	return LARGEST + 1;
    }
    if (0 == next_iter)
	*label = "udp";
    else if (1 == next_iter)
	*label = "tcp";
    else if (LARGEST == next_iter)
	*label = "else";
    else
	return -1;
    return next_iter++;
}
