#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "dns_message.h"
#include "md_array.h"
#include "hashtbl.h"

static hashfunc tld_hashfunc;
static hashkeycmp tld_cmpfunc;

#define MAX_ARRAY_SZ 65536
static hashtbl *idx_to_tld = NULL;
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
    if (NULL == idx_to_tld)
	idx_to_tld = hash_create(MAX_ARRAY_SZ, tld_hashfunc, tld_cmpfunc);
    if ((obj = hash_find(tld, idx_to_tld)))
	return obj->index;
    obj = calloc(1, sizeof(*obj));
    assert(obj);
    obj->tld = strdup(tld);
    obj->index = next_idx++;
    hash_add(tld, obj, idx_to_tld);
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
	hash_iter_init(idx_to_tld);
	return next_idx;
    }
    if ((obj = hash_iterate(idx_to_tld)) == NULL)
	return -1;
    snprintf(label_buf, MAX_QNAME_SZ, "%s", obj->tld);
    *label = label_buf;
    return obj->index;
}

static unsigned int
tld_hashfunc(const void *key)
{
	return SuperFastHash(key, strlen(key));;
}

static int
tld_cmpfunc(const void *a, const void *b)
{
	return strcasecmp(a, b);
}
