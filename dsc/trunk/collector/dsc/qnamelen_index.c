#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "dns_message.h"
#include "md_array.h"
static int largest = 0;

int
qnamelen_indexer(void * vp)
{
    dns_message *m = vp;
    int i = strlen(m->qname);;
    if (i >= MAX_QNAME_SZ)
	i = MAX_QNAME_SZ - 1;
    if (i > largest)
	largest = i;
    return i;
}

static int next_iter;

int
qnamelen_iterator(char **label)
{
    static char label_buf[10];
    if (NULL == label) {
	next_iter = 0;
	return largest;
    }
    if (next_iter > largest)
	return -1;
    snprintf(label_buf, 10, "%d", next_iter);
    *label = label_buf;
    return next_iter++;
}
