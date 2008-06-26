#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <netdb.h>
#include <memory.h>

#include "dns_message.h"
#include "md_array.h"

/* Indexes the source port of DNS messages */

static unsigned int f_index[65536];
static unsigned short r_index[65536];
static unsigned int largest = 0;

int
dns_source_port_indexer(const void *vp)
{
    const dns_message *m = vp;
    unsigned short p = m->tm->src_port;
    if (0 == f_index[p]) {
	f_index[p] = ++largest;
	r_index[largest] = p;
    }
    return f_index[p];
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
    snprintf(*label = label_buf, 20, "%hu", r_index[next_iter++]);
    return next_iter;
}

void
dns_source_port_reset(void)
{
    memset(f_index, 0, sizeof f_index);
    memset(r_index, 0, sizeof r_index);
    largest = 0;
}
