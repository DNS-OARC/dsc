#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "dns_message.h"
#include "md_array.h"

#define QNAME_LOCALHOST 0
#define QNAME_RSN 1
#define QNAME_OTHER 2
#define LARGEST 3

int
certain_qnames_indexer(const void *vp)
{
    const dns_message *m = vp;
    if (0 == strcmp(m->qname, "localhost"))
	return QNAME_LOCALHOST;
    if (0 == strcmp(m->qname + 1, ".root-servers.net"))
	return QNAME_RSN;
    return QNAME_OTHER;
}

int
certain_qnames_iterator(char **label)
{
    static int next_iter = 0;
    if (NULL == label) {
        next_iter = 0;
        return LARGEST;
    }
    if (0 == next_iter)
        *label = "localhost";
    else if (1 == next_iter)
        *label = "?.root-servers.net";
    else if (LARGEST == next_iter)
        *label = "else";
    else
        return -1;
    return next_iter++;
}
