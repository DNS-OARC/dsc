#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "dns_message.h"
#include "md_array.h"

#define MAX_ARRAY_SZ 65536
static char *idx_to_tld[MAX_ARRAY_SZ];	/* XXX replace with hash */
static int next_idx = 0;

int
tld_indexer(const void * vp)
{
    const dns_message * m = vp;
    int i;
    const char *tld;
    assert(next_idx < MAX_ARRAY_SZ);
    tld = m->qname + strlen(m->qname) - 2;
    while (tld >= m->qname && (*tld != '.'))
	tld--;
    if (*tld == '.')
	tld++;
    if (tld < m->qname)
	tld = m->qname;
    for (i = 0; i < next_idx; i++) {
	if (0 == strcmp(tld, idx_to_tld[i])) {
	    return i;
	}
    }
    idx_to_tld[next_idx] = strdup(tld);
    return next_idx++;
}

static int next_iter;
struct _foo {
    char *key;
    int idx;
};

static int
compare(const void *A, const void *B)
{
    const struct _foo *a = A;
    const struct _foo *b = B;
    return strcmp(a->key, b->key);
}


int
tld_iterator(char **label)
{
    static char label_buf[MAX_QNAME_SZ];
    static struct _foo *sortme = NULL;
    if (0 == next_idx)
	return -1;
    if (NULL == label) {
	int i;
	sortme = calloc(next_idx, sizeof(*sortme));
	for (i = 0; i < next_idx; i++) {
	    sortme[i].key = idx_to_tld[i];
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
    snprintf(label_buf, MAX_QNAME_SZ, "%s", sortme[next_iter].key);
    *label = label_buf;
    return sortme[next_iter++].idx;
}
