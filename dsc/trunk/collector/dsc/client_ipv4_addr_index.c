#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "dns_message.h"
#include "md_array.h"

#define MAX_ARRAY_SZ 65536
static struct in_addr idx_to_cip4[MAX_ARRAY_SZ];	/* XXX replace with hash */
static int next_idx = 0;

int
cip4_indexer(const void *vp)
{
    const dns_message *m = vp;
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

int
cip4_iterator(char **label)
{
    static char label_buf[24];
    if (0 == next_idx)
	return -1;
    if (NULL == label) {
	next_iter = 0;
	return next_idx;
    }
    if (next_iter == next_idx) {
	return -1;
    }
    strncpy(label_buf, inet_ntoa(idx_to_cip4[next_iter]), 24);
    *label = label_buf;
    return next_iter++;
}
