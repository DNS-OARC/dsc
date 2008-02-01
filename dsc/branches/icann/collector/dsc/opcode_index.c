#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <netdb.h>

#include "dns_message.h"
#include "md_array.h"

static int largest = 0;

int
opcode_indexer(const void *vp)
{
    const dns_message *m = vp;
    int i = (int) m->opcode;
    if (m->malformed)
	return -1;
    if (i > largest)
	largest = i;
    return i;
}

static int next_iter = 0;

int
opcode_iterator(char **label)
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
opcode_reset()
{
    largest = 0;
}
