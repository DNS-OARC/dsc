#include <stdlib.h>
#include <assert.h>

#include "dns_message.h"

int
null_indexer(dns_message * m)
{
    return 0;
}


int
null_iterator(char **label)
{
    if (label)
        *label = "ALL";
    return 0;
}
