
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "md_array.h"
#include "dns_message.h"
#include "ip_message.h"
#include "pcap.h"

void md_array_grow_d1(md_array * a);
void md_array_grow_d2(md_array * a);


md_array *
md_array_create(FLTR * filter,
    char *type1, IDXR * idx1, HITR * itr1,
    char *type2, IDXR * idx2, HITR * itr2)
{
    int i1;
    md_array *a = calloc(1, sizeof(*a));
    a->filter = filter;
    a->d1.type = type1;
    a->d1.indexer = idx1;
    a->d1.iterator = itr1;
    a->d1.alloc_sz = 2;
    a->d2.type = type2;
    a->d2.indexer = idx2;
    a->d2.iterator = itr2;
    a->d2.alloc_sz = 2;
    a->array = calloc(a->d1.alloc_sz, sizeof(int *));
    for (i1 = 0; i1 < a->d1.alloc_sz; i1++) {
	a->array[i1] = calloc(a->d2.alloc_sz, sizeof(int));
    }
    return a;
}

int
md_array_count(md_array * a, void * m)
{
    int i1;
    int i2;

    if (a->filter)
	if (0 == a->filter(m))
	    return -1;

    i1 = a->d1.indexer(m);
    i2 = a->d2.indexer(m);

    while (i1 >= a->d1.alloc_sz)
	md_array_grow_d1(a);
    while (i2 >= a->d2.alloc_sz)
	md_array_grow_d2(a);

    assert(i1 < a->d1.alloc_sz);
    assert(i2 < a->d2.alloc_sz);
    a->array[i1][i2]++;
    return a->array[i1][i2];
}


void
md_array_grow_d1(md_array * a)
{
    int new_alloc_sz;
    int **new;
    int i1;
    new_alloc_sz = a->d1.alloc_sz << 1;
    new = calloc(new_alloc_sz, sizeof(int *));
    memcpy(new, a->array, a->d1.alloc_sz * sizeof(int *));
    free(a->array);
    a->array = new;

    for (i1 = a->d1.alloc_sz; i1 < new_alloc_sz; i1++) {
	a->array[i1] = calloc(a->d2.alloc_sz, sizeof(int));
    }

    a->d1.alloc_sz = new_alloc_sz;
}

void
md_array_grow_d2(md_array * a)
{
    int new_alloc_sz;
    int *new;
    int i1;
    new_alloc_sz = a->d2.alloc_sz << 1;
    for (i1 = 0; i1 < a->d1.alloc_sz; i1++) {
	new = calloc(new_alloc_sz, sizeof(int *));
	memcpy(new, a->array[i1], a->d2.alloc_sz * sizeof(int));
	free(a->array[i1]);
	a->array[i1] = new;
    }
    a->d2.alloc_sz = new_alloc_sz;
}

int
md_array_print(md_array * a, md_array_printer * pr, const char *name)
{
    FILE *fp;
    char fname[128];
    char *label1;
    char *label2;
    int i1;
    int i2;

    snprintf(fname, 128, "%d.%s.xml", Pcap_finish_time(), name);
    fp = fopen(fname, "w");
    if (NULL == fp)
	return -1;
    a->d1.iterator(NULL);
    pr->start_array(fp, name);
    pr->d1_type(fp, a->d1.type);
    pr->d2_type(fp, a->d2.type);
    pr->start_data(fp);
    while ((i1 = a->d1.iterator(&label1)) > -1) {
	if (i1 >= a->d1.alloc_sz)
	    continue;		/* see [1] */
	pr->d1_begin(fp, label1);
	a->d2.iterator(NULL);
	while ((i2 = a->d2.iterator(&label2)) > -1) {
	    if (i2 >= a->d2.alloc_sz)
		continue;
	    if (0 == a->array[i1][i2])
		continue;
	    pr->print_element(fp, label2, a->array[i1][i2]);
	}
	pr->d1_end(fp, label1);
    }
    pr->finish_data(fp);
    pr->finish_array(fp);
    fclose(fp);
    return 0;
}


/* [1]
 * Its okay (not a bug) for the indexer's index to be larger
 * than the array size.  The indexer may have grown for use in a
 * different array, but the filter prevented it from growing this
 * particular array so far.
 */
