#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "dns_message.h"
#include "md_array.h"

#define TC_BIT_CLR 0
#define TC_BIT_SET 1

int
tc_bit_indexer(const void *vp)
{
    const dns_message *m = vp;
    if (m->malformed)
	return -1;
    if (m->tc)
	return TC_BIT_SET;
    return TC_BIT_CLR;
}

int
tc_bit_iterator(char **label)
{
    static int next_iter = 0;
    if (NULL == label) {
	next_iter = TC_BIT_CLR;
	return TC_BIT_SET + 1;
    }
    if (TC_BIT_CLR == next_iter)
	*label = "clr";
    else if (TC_BIT_SET == next_iter)
	*label = "set";
    else
	return -1;
    return next_iter++;
}
