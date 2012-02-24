#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "dns_message.h"
#include "md_array.h"

int
qr_aa_bits_indexer(const void *vp)
{
    const dns_message *m = vp;
    if (m->malformed)
	return -1;
    return m->qr + (m->aa << 1);
}

int
qr_aa_bits_iterator(char **label)
{
    static int next_iter = 0;
    if (NULL == label) {
	next_iter = 0;
	return 4;
    }
    switch (next_iter) {
    case 0:
	*label = "qr=0,aa=0";
	break;
    case 1:
	*label = "qr=1,aa=0";
	break;
    case 2:
	*label = "qr=0,aa=1";
	break;
    case 3:
	*label = "qr=1,aa=1";
	break;
    default:
	*label = "bug";
	return -1;
    }
    return next_iter++;
}
