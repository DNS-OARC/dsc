#include <stdlib.h>
#include <assert.h>

int
null_indexer(const void *vp)
{
    return 0;
}


int
null_iterator(char **label)
{
    static int state = 0;
    if (NULL == label) {
	state = 0;
	return 0;
    }
    *label = "ALL";
    state++;
    return state == 1 ? 0 : -1;
}
