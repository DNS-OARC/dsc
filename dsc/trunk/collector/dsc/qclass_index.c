#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "dns_message.h"
#include "md_array.h"

static unsigned short idx_to_qclass[65536];
static int next_idx = 0;

int
qclass_indexer(const void * vp)
{
    const dns_message * h = vp;
    int i;
    for (i = 0; i < next_idx; i++) {
	if (h->qclass == idx_to_qclass[i]) {
	    return i;
	}
    }
    idx_to_qclass[next_idx] = h->qclass;
    return next_idx++;
}

static int next_iter;
struct _foo {
    unsigned short key;
    int idx;
};

static int
compare(const void *A, const void *B)
{
    const struct _foo *a = A;
    const struct _foo *b = B;
    if (a->key < b->key)
	return -1;
    if (a->key > b->key)
	return 1;
    return 0;
}


int
qclass_iterator(char **label)
{
    static char label_buf[32];
    static struct _foo *sortme = NULL;
    if (0 == next_idx)
	return -1;
    if (NULL == label) {
	int i;
	sortme = calloc(next_idx, sizeof(*sortme));
	for (i = 0; i < next_idx; i++) {
	    sortme[i].key = idx_to_qclass[i];
	    sortme[i].idx = i;
	}
	qsort(sortme, next_idx, sizeof(*sortme), compare);
	next_iter = 0;
	return next_idx;
    }
    assert(sortme);
    if (next_iter == next_idx) {
	free(sortme);
	sortme = NULL;
	return -1;
    }
    snprintf(label_buf, 32, "%d", sortme[next_iter].key);
    *label = label_buf;
    return sortme[next_iter++].idx;
}
