#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "xmalloc.h"
#include "dns_message.h"
#include "md_array.h"
#include "hashtbl.h"

static hashfunc tld_hashfunc;
static hashkeycmp tld_cmpfunc;

#define MAX_ARRAY_SZ 65536
static hashtbl *theHash = NULL;
static int next_idx = 0;

typedef struct {
	char *tld;
	int index;
} tldobj;

int
tld_indexer(const void *vp)
{
    const dns_message *m = vp;
    const char *tld;
    tldobj *obj;
    if (m->malformed)
	return -1;
    tld = dns_message_tld((dns_message *) m);
    if (NULL == theHash) {
	theHash = hash_create(MAX_ARRAY_SZ, tld_hashfunc, tld_cmpfunc,
	    1, afree, afree);
	if (NULL == theHash)
	    return -1;
    }
    if ((obj = hash_find(tld, theHash)))
	return obj->index;
    obj = acalloc(1, sizeof(*obj));
    if (NULL == obj)
	return -1;
    obj->tld = astrdup(tld);
    if (NULL == obj->tld) {
	afree(obj);
	return -1;
    }
    obj->index = next_idx;
    if (0 != hash_add(obj->tld, obj, theHash)) {
	afree(obj->tld);
	afree(obj);
	return -1;
    }
    next_idx++;
    return obj->index;
}

int
tld_iterator(char **label)
{
    tldobj *obj;
    static char label_buf[MAX_QNAME_SZ];
    if (0 == next_idx)
	return -1;
    if (NULL == label) {
	/* initialize and tell caller how big the array is */
	hash_iter_init(theHash);
	return next_idx;
    }
    if ((obj = hash_iterate(theHash)) == NULL)
	return -1;
    snprintf(label_buf, MAX_QNAME_SZ, "%s", obj->tld);
    *label = label_buf;
    return obj->index;
}

void
tld_reset()
{
    theHash = NULL;
    next_idx = 0;
}

static unsigned int
tld_hashfunc(const void *key)
{
	return hashendian(key, strlen(key), 0);
}

static int
tld_cmpfunc(const void *a, const void *b)
{
	return strcasecmp(a, b);
}
