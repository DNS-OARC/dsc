#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "dns_message.h"
#include "md_array.h"

#define QNAME_NORMAL 0
#define QNAME_IDN 1

int
idn_qname_indexer(const void *vp)
{
    const dns_message *m = vp;
    if (m->malformed)
	return -1;
    if (0 == strncmp(m->qname, "xn--", 4))
	return QNAME_IDN;
    return QNAME_NORMAL;
}

int
idn_qname_iterator(char **label)
{
    static int next_iter = 0;
    if (NULL == label) {
	next_iter = QNAME_NORMAL;
	return QNAME_IDN + 1;
    }
    if (QNAME_NORMAL == next_iter)
	*label = "normal";
    else if (QNAME_IDN == next_iter)
	*label = "idn";
    else
	return -1;
    return next_iter++;
}
