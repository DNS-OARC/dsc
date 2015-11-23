#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "dns_message.h"
#include "md_array.h"

#define MAX_RCODE_IDX 16
static unsigned short idx_to_rcode[MAX_RCODE_IDX];
static int next_idx = 0;

int
rcode_indexer(const void *vp)
{
    const dns_message *m = vp;
    int i;
    if (m->malformed)
	return -1;
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

int
rcode_iterator(char **label)
{
    static char label_buf[32];
    if (0 == next_idx)
	return -1;
    if (NULL == label) {
	next_iter = 0;
	return next_idx;
    }
    if (next_iter == next_idx) {
	return -1;
    }
    snprintf(label_buf, 32, "%d", idx_to_rcode[next_iter]);
    *label = label_buf;
    return next_iter++;
}

void
rcode_reset()
{
    next_idx = 0;
}
