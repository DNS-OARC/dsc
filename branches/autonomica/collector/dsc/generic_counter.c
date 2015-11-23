#include "generic_counter.h"

int
gen_cnt_cmp(const void *A, const void *B)
{
    const gen_cnt *a = A;
    const gen_cnt *b = B;
    if (a->cnt < b->cnt)
	return 1;
    if (a->cnt > b->cnt)
	return -1;
    if (a->ptr < b->ptr)
	return 1;
    if (a->ptr > b->ptr)
	return -1;
    return 0;
}
