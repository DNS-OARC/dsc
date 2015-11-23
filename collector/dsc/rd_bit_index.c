#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "dns_message.h"
#include "md_array.h"

#define RD_BIT_CLR 0
#define RD_BIT_SET 1

int
rd_bit_indexer(const void *vp)
{
    const dns_message *m = vp;
    if (m->malformed)
	return -1;
    if (m->rd)
	return RD_BIT_SET;
    return RD_BIT_CLR;
}

int
rd_bit_iterator(char **label)
{
    static int next_iter = 0;
    if (NULL == label) {
	next_iter = RD_BIT_CLR;
	return RD_BIT_SET + 1;
    }
    if (RD_BIT_CLR == next_iter)
	*label = "clr";
    else if (RD_BIT_SET == next_iter)
	*label = "set";
    else
	return -1;
    return next_iter++;
}
