#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "dns_message.h"
#include "md_array.h"
#include "hashtbl.h"

static hashfunc qname_hashfunc;
static hashkeycmp qname_cmpfunc;

#define MAX_ARRAY_SZ 65536
static hashtbl *theHash = NULL;
static int next_idx = 0;

typedef struct {
        char *qname;
        int index;
} qnameobj;

int
qname_indexer(const void *vp)
{
    qnameobj *obj;
    const dns_message *m = vp;
    if (m->malformed)
	return -1;
    if (NULL == theHash)
        theHash = hash_create(MAX_ARRAY_SZ, qname_hashfunc, qname_cmpfunc);
    if ((obj = hash_find(m->qname, theHash)))
        return obj->index;
    obj = calloc(1, sizeof(*obj));
    assert(obj);
    obj->qname = strdup(m->qname);
    obj->index = next_idx++;
    hash_add(obj->qname, obj, theHash);
    return obj->index;

}

int
qname_iterator(char **label)
{
    qnameobj *obj;
    static char label_buf[MAX_QNAME_SZ];
    if (0 == next_idx)
	return -1;
    if (NULL == label) {
	hash_iter_init(theHash);
	return next_idx;
    }
    if ((obj = hash_iterate(theHash)) == NULL)
	return -1;
    snprintf(label_buf, MAX_QNAME_SZ, "%s", obj->qname);
    *label = label_buf;
    return obj->index;
}

static unsigned int
qname_hashfunc(const void *key)
{
        return SuperFastHash(key, strlen(key));;
}

static int
qname_cmpfunc(const void *a, const void *b)
{
        return strcasecmp(a, b);
}
