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
tld_indexer(const void *vp)
{
    const dns_message *m = vp;
    int i;
    const char *tld;
    if (m->malformed)
	return -1;
    tld = dns_message_tld((dns_message *) m);
    assert(next_idx < MAX_ARRAY_SZ);
    for (i = 0; i < next_idx; i++) {
	if (0 == strcmp(tld, idx_to_tld[i])) {
	    return i;
	}
    }
    idx_to_tld[next_idx] = strdup(tld);
    return next_idx++;
}

static int next_iter;

int
tld_iterator(char **label)
{
    static char label_buf[MAX_QNAME_SZ];
    if (0 == next_idx)
	return -1;
    if (NULL == label) {
	next_iter = 0;
	return next_idx;
    }
    if (next_iter == next_idx) {
	return -1;
    }
    snprintf(label_buf, MAX_QNAME_SZ, "%s", idx_to_tld[next_iter]);
    *label = label_buf;
    return next_iter++;
}
