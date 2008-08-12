
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "dns_message.h"
#include "md_array.h"

static unsigned short idx_to_qclass[65536];
static int next_idx = 0;

int
qclass_indexer(const void *vp)
{
    const dns_message *m = vp;
    int i;
    if (m->malformed)
	return -1;
    for (i = 0; i < next_idx; i++) {
	if (m->qclass == idx_to_qclass[i]) {
	    return i;
	}
    }
    idx_to_qclass[next_idx] = m->qclass;
    return next_idx++;
}

static int next_iter;

int
qclass_iterator(char **label)
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
    snprintf(label_buf, 32, "%d", idx_to_qclass[next_iter]);
    *label = label_buf;
    return next_iter++;
}

void
qclass_reset()
{
    next_idx = 0;
}
