#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "dns_message.h"
#include "md_array.h"

#define MAX_RCODE_IDX 16
static unsigned short idx_to_rcode[MAX_RCODE_IDX];
static int next_idx = 0;

int
rcode_indexer(dns_message * m)
{
    int i;
    for (i = 0; i < next_idx; i++) {
	if (m->rcode == idx_to_rcode[i]) {
	    return i;
	}
    }
    idx_to_rcode[next_idx] = m->rcode;
    assert(next_idx < MAX_RCODE_IDX);
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
rcode_iterator(char **label)
{
    static char label_buf[32];
    static struct _foo *sortme = NULL;
    if (0 == next_idx)
	return -1;
    if (NULL == label) {
	int i;
	sortme = calloc(next_idx, sizeof(*sortme));
	for (i = 0; i < next_idx; i++) {
	    sortme[i].key = idx_to_rcode[i];
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
