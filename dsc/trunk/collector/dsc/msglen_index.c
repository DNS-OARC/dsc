#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "dns_message.h"
#include "md_array.h"
static int largest = 0;

int
msglen_indexer(const void *vp)
{
    const dns_message *m = vp;
    if (m->msglen > largest)
	largest = m->msglen;
    return m->msglen;
}

static int next_iter;

int
msglen_iterator(char **label)
{
    static char label_buf[10];
    if (NULL == label) {
	next_iter = 0;
	return largest + 1;
    }
    if (next_iter > largest)
	return -1;
    snprintf(label_buf, 10, "%d", next_iter);
    *label = label_buf;
    return next_iter++;
}

void
msglen_reset()
{
    largest = 0;
}
