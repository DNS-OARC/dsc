#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "dns_message.h"
#include "md_array.h"

#define MAX_ARRAY_SZ 65536
static struct in_addr idx_to_cip4[MAX_ARRAY_SZ];	/* XXX replace with hash */
static int next_idx = 0;

int
cip4_indexer(dns_message * m)
{
    int i;
    assert(next_idx < MAX_ARRAY_SZ);
    for (i = 0; i < next_idx; i++) {
	if (m->client_ipv4_addr.s_addr == idx_to_cip4[i].s_addr) {
	    return i;
	}
    }
    idx_to_cip4[next_idx] = m->client_ipv4_addr;
    return next_idx++;
}

static int next_iter;
struct _foo {
    struct in_addr key;
    int idx;
};

static int
compare(const void *A, const void *B)
{
    const struct _foo *a = A;
    const struct _foo *b = B;
    if (ntohs(a->key.s_addr) < ntohs(b->key.s_addr))
	return -1;
    if (ntohs(a->key.s_addr) > ntohs(b->key.s_addr))
	return 1;
    return 0;
}


int
cip4_iterator(char **label)
{
    static char label_buf[24];
    static struct _foo *sortme = NULL;
    if (0 == next_idx)
	return -1;
    if (NULL == label) {
	int i;
	sortme = calloc(next_idx, sizeof(*sortme));
	for (i = 0; i < next_idx; i++) {
	    sortme[i].key = idx_to_cip4[i];
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
    strncpy(label_buf, inet_ntoa(sortme[next_iter].key), 24);
    *label = label_buf;
    return sortme[next_iter++].idx;
}
