#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <netdb.h>

#include "dns_message.h"
#include "md_array.h"

/* Indexes the source port of DNS messages */

static int largest = 0;

int
dns_source_port_indexer(const void *vp)
{
    const dns_message *m = vp;
    int i = (int) m->tm->src_port;
    if (i > largest)
	largest = i;
    return i;
}

static int next_iter = 0;

int
dns_source_port_iterator(char **label)
{
    static char label_buf[20];
    if (NULL == label) {
	next_iter = 0;
	return largest + 1;
    }
    if (next_iter > largest)
	return -1;
    snprintf(*label = label_buf, 20, "%d", next_iter);
    return next_iter++;
}

void
dns_source_port_reset(void)
{
    largest = 0;
}

