#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "dns_message.h"
#include "md_array.h"

#define D0_BIT_CLR 0
#define D0_BIT_SET 1

int
d0_bit_indexer(const void *vp)
{
    const dns_message *m = vp;
    if (m->malformed)
	return -1;
    if (m->edns.found && m->edns.d0)
	return D0_BIT_SET;
    return D0_BIT_CLR;
}

int
d0_bit_iterator(char **label)
{
    static int next_iter = 0;
    if (NULL == label) {
        next_iter = D0_BIT_CLR;
        return D0_BIT_CLR;
    }
    if (D0_BIT_CLR == next_iter)
        *label = "clr";
    else if (D0_BIT_SET == next_iter)
        *label = "set";
    else
        return -1;
    return next_iter++;
}
