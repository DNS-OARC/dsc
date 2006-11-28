#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "dns_message.h"
#include "md_array.h"
#include "hashtbl.h"

static hashfunc ipv4addr_hashfunc;
static hashkeycmp ipv4addr_cmpfunc;

#define MAX_ARRAY_SZ 65536
static hashtbl *theHash = NULL;
static int next_idx = 0;

typedef struct {
	struct in_addr addr;
	int index;
} ipv4addrobj;

int
cip4_indexer(const void *vp)
{
    const dns_message *m = vp;
    ipv4addrobj *obj;
    if (m->malformed)
	return -1;
    if (NULL == theHash)
	theHash = hash_create(MAX_ARRAY_SZ, ipv4addr_hashfunc, ipv4addr_cmpfunc);
    if ((obj = hash_find(&m->client_ipv4_addr, theHash)))
	return obj->index;
    obj = calloc(1, sizeof(*obj));
    assert(obj);
    obj->addr = m->client_ipv4_addr;
    obj->index = next_idx++;
    hash_add(&obj->addr, obj, theHash);
    return obj->index;
}

int
cip4_iterator(char **label)
{
    ipv4addrobj *obj;
    static char label_buf[24];
    if (0 == next_idx)
	return -1;
    if (NULL == label) {
	hash_iter_init(theHash);
	return next_idx;
    }
    if ((obj = hash_iterate(theHash)) == NULL)
	return -1;
    strncpy(label_buf, inet_ntoa(obj->addr), 24);
    *label = label_buf;
    return obj->index;
}

static unsigned int
ipv4addr_hashfunc(const void *key)
{
	const struct in_addr *a = key;
	return ntohl(a->s_addr);
}

static int
ipv4addr_cmpfunc(const void *a, const void *b)
{
	const struct in_addr *a1 = a;
	const struct in_addr *a2 = b;
	if (ntohl(a1->s_addr) < ntohl(a2->s_addr))
		return -1;
	if (ntohl(a1->s_addr) > ntohl(a2->s_addr))
		return 1;
	return 0;
}

