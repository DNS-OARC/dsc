#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "dns_message.h"
#include "md_array.h"

#define DO_BIT_CLR 0
#define DO_BIT_SET 1

int
do_bit_indexer(const void *vp)
{
    const dns_message *m = vp;
    if (m->malformed)
	return -1;
    if (m->edns.found && m->edns.DO)
	return DO_BIT_SET;
    return DO_BIT_CLR;
}

int
do_bit_iterator(char **label)
{
    static int next_iter = 0;
    if (NULL == label) {
	next_iter = DO_BIT_CLR;
	return DO_BIT_SET + 1;
    }
    if (DO_BIT_CLR == next_iter)
	*label = "clr";
    else if (DO_BIT_SET == next_iter)
	*label = "set";
    else
	return -1;
    return next_iter++;
}
