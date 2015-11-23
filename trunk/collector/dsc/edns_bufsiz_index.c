#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "dns_message.h"
#include "md_array.h"

int edns_bufsiz_max = 0;

int
edns_bufsiz_indexer(const void *vp)
{
    const dns_message *m = vp;
    int index;
    if (m->malformed)
	return -1;
    if (0 == m->edns.found)
	return 0;
    index = (int) (m->edns.bufsiz>>9) + 1;
    if (index > edns_bufsiz_max)
	edns_bufsiz_max = index;
    return index;
}

int
edns_bufsiz_iterator(char **label)
{
    static int next_iter = 0;
    static char buf[20];
    if (NULL == label) {
	next_iter = 0;
	return 0;
    }
    if (next_iter > edns_bufsiz_max) {
	return -1;
    } else if (0 == next_iter) {
	*label = "None";
    } else {
	snprintf(buf, 20, "%d-%d",
		(next_iter - 1)<<9,
		(next_iter<<9) - 1);
	*label = buf;
    }
    return next_iter++;
}
