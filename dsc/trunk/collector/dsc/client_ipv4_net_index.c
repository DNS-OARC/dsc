#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "xmalloc.h"
#include "dns_message.h"
#include "md_array.h"
#include "hashtbl.h"

static hashfunc ipv4net_hashfunc;
static hashkeycmp ipv4net_cmpfunc;
static struct in_addr mask;

#define MAX_ARRAY_SZ 65536
static hashtbl *theHash = NULL;
static int next_idx = 0;

typedef struct {
	struct in_addr addr;
	int index;
} ipv4netobj;

int
cip4_net_indexer(const void *vp)
{
    const dns_message *m = vp;
    ipv4netobj *obj;
    struct in_addr masked_addr;
    if (m->malformed)
	return -1;
    if (NULL == theHash) {
	theHash = hash_create(MAX_ARRAY_SZ, ipv4net_hashfunc, ipv4net_cmpfunc);
	if (NULL == theHash)
	    return -1;
    }
    masked_addr.s_addr = m->client_ipv4_addr.s_addr & mask.s_addr;
    if ((obj = hash_find(&masked_addr, theHash)))
	return obj->index;
    obj = xcalloc(1, sizeof(*obj));
    if (NULL == obj)
	return -1;
    obj->addr = masked_addr;
    obj->index = next_idx;
    if (0 != hash_add(&obj->addr, obj, theHash)) {
	free(obj);
	return -1;
    }
    next_idx++;
    return obj->index;
}

int
cip4_net_iterator(char **label)
{
    ipv4netobj *obj;
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

void
cip4_net_indexer_init(void)
{
    mask.s_addr = inet_addr("255.255.255.0");
}

static unsigned int
ipv4net_hashfunc(const void *key)
{
	const struct in_addr *a = key;
	return ntohl(a->s_addr) >> 8;
}

static int
ipv4net_cmpfunc(const void *a, const void *b)
{
	const struct in_addr *a1 = a;
	const struct in_addr *a2 = b;
	if (ntohl(a1->s_addr) < ntohl(a2->s_addr))
		return -1;
	if (ntohl(a1->s_addr) > ntohl(a2->s_addr))
		return 1;
	return 0;
}
