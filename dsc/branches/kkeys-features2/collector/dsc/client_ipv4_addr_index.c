#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "xmalloc.h"
#include "dns_message.h"
#include "md_array.h"
#include "hashtbl.h"

static hashfunc ipaddr_hashfunc;
static hashkeycmp ipaddr_cmpfunc;

#define MAX_ARRAY_SZ 65536
static hashtbl *theHash = NULL;
static int next_idx = 0;

typedef struct {
	inX_addr addr;
	int index;
} ipaddrobj;

int
cip_indexer(const void *vp)
{
    const dns_message *m = vp;
    ipaddrobj *obj;
    if (m->malformed)
	return -1;
    if (NULL == theHash) {
	theHash = hash_create(MAX_ARRAY_SZ, ipaddr_hashfunc, ipaddr_cmpfunc,
	    1, NULL, afree);
	if (NULL == theHash)
	    return -1;
    }
    if ((obj = hash_find(&m->client_ip_addr, theHash)))
	return obj->index;
    obj = acalloc(1, sizeof(*obj));
    if (NULL == obj)
	return -1;
    obj->addr = m->client_ip_addr;
    obj->index = next_idx;
    if (0 != hash_add(&obj->addr, obj, theHash)) {
	afree(obj);
	return -1;
    }
    next_idx++;
    return obj->index;
}

int
cip_iterator(char **label)
{
    ipaddrobj *obj;
    static char label_buf[128];
    if (0 == next_idx)
	return -1;
    if (NULL == label) {
	hash_iter_init(theHash);
	return next_idx;
    }
    if ((obj = hash_iterate(theHash)) == NULL)
	return -1;
    inXaddr_ntop(&obj->addr, label_buf, 128);
    *label = label_buf;
    return obj->index;
}

void
cip_reset()
{
    theHash = NULL;
    next_idx = 0;
}

static unsigned int
ipaddr_hashfunc(const void *key)
{
	const inX_addr *a = key;
	return inXaddr_hash(a);
}

static int
ipaddr_cmpfunc(const void *a, const void *b)
{
	const inX_addr *a1 = a;
	const inX_addr *a2 = b;
	return inXaddr_cmp(a1, a2);
}

