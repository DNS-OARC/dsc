
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include "xmalloc.h"
#include "dataset_opt.h"
#include "md_array.h"
#include "dns_message.h"
#include "pcap.h"
#include "syslog_debug.h"

static void md_array_grow(md_array * a, int i1, int i2);

static void
md_array_free(md_array *a)
{
    if (a->name)
	xfree((char *)a->name);
    if (a->d1.type)
	xfree((char *)a->d1.type);
    if (a->d2.type)
	xfree((char *)a->d2.type);
    /* a->array contents were in an arena, so we don't need to free them. */
    xfree(a);
}

void
md_array_clear(md_array *a)
{
    /* a->array contents were in an arena, so we don't need to free them. */
    a->array = NULL;
    a->d1.alloc_sz = 0;
    if (a->d1.indexer->reset_fn)
	a->d1.indexer->reset_fn();
    a->d2.alloc_sz = 0;
    if (a->d2.indexer->reset_fn)
	a->d2.indexer->reset_fn();
}

md_array *
md_array_create(const char *name, filter_list * fl,
    const char *type1, indexer_t *idx1,
    const char *type2, indexer_t *idx2)
{
    md_array *a = xcalloc(1, sizeof(*a));
    if (NULL == a)
	return NULL;
    a->name = xstrdup(name);
    if (a->name == NULL) {
	md_array_free(a);
	return NULL;
    }
    a->filter_list = fl;
    a->d1.type = xstrdup(type1);
    if (a->d1.type == NULL) {
	md_array_free(a);
	return NULL;
    }
    a->d1.indexer = idx1;
    a->d1.alloc_sz = 0;
    a->d2.type = xstrdup(type2);
    if (a->d2.type == NULL) {
	md_array_free(a);
	return NULL;
    }
    a->d2.indexer = idx2;
    a->d2.alloc_sz = 0;
    a->array = NULL; /* will be allocated when needed, in an arena. */
    return a;
}

int
md_array_count(md_array * a, const void *vp)
{
    int i1;
    int i2;
    filter_list *fl;

    for (fl = a->filter_list; fl; fl = fl->next)
	if (0 == fl->filter->func(vp, fl->filter->context))
	    return -1;

    if ((i1 = a->d1.indexer->index_fn(vp)) < 0)
	return -1;
    if ((i2 = a->d2.indexer->index_fn(vp)) < 0)
	return -1;

    md_array_grow(a, i1, i2);

    assert(i1 < a->d1.alloc_sz);
    assert(i2 < a->d2.alloc_sz);
    return ++a->array[i1].array[i2];
}

static void
md_array_grow(md_array * a, int i1, int i2)
{
    int new_d1_sz, new_d2_sz;
    struct _md_array_node *d1 = NULL;
    int *d2 = NULL;

    if (i1 < a->d1.alloc_sz && i2 < a->array[i1].alloc_sz)
	return;

    /* dimension 1 */
    new_d1_sz = a->d1.alloc_sz;
    if (i1 >= a->d1.alloc_sz) {
	/* pick a new size */
	if (new_d1_sz == 0)
	    new_d1_sz = 2;
	while (i1 >= new_d1_sz)
	    new_d1_sz = new_d1_sz << 1;

	/* allocate new array */
	d1 = acalloc(new_d1_sz, sizeof(*d1));
	if (NULL == d1) {
	    /* oops, undo! */
	    return;
	}

	/* copy old contents to new array */
	memcpy(d1, a->array, a->d1.alloc_sz * sizeof(*d1));

    } else {
	d1 = a->array;
    }

    /* dimension 2 */
    new_d2_sz = d1[i1].alloc_sz;
    if (i2 >= d1[i1].alloc_sz) {
	/* pick a new size */
	if (new_d2_sz == 0)
	    new_d2_sz = 2;
	while (i2 >= new_d2_sz)
	    new_d2_sz = new_d2_sz << 1;

	/* allocate new array */
	d2 = acalloc(new_d2_sz, sizeof(*d2));
	if (NULL == d2) {
	    /* oops, undo! */
	    if (d1) afree(d1);
	    return;
	}

	/* copy old contents to new array */
	memcpy(d2, d1[i1].array, d1[i1].alloc_sz * sizeof(*d2));
    }

    if (d1 != a->array) {
	if (a->array) {
	    fprintf(stderr, "grew d1 of %s from %d to %d\n",
		a->name, a->d1.alloc_sz, new_d1_sz);
	    afree(a->array);
	}
	a->array = d1;
	a->d1.alloc_sz = new_d1_sz;
    }
    if (d2) {
	if (a->array[i1].array) {
	    fprintf(stderr, "grew d2[%d] of %s from %d to %d\n",
		i1, a->name, a->array[i1].alloc_sz, new_d2_sz);
	    afree(a->array[i1].array);
	}
	a->array[i1].array = d2;
	a->array[i1].alloc_sz = new_d2_sz;
    }

    if (new_d2_sz > a->d2.alloc_sz)
	a->d2.alloc_sz = new_d2_sz;
}

struct _foo {
    char *label;
    int val;
};

/*
 * descending sort order (larger to smaller)
 */
static int
compare(const void *A, const void *B)
{
    const struct _foo *a = A;
    const struct _foo *b = B;
    return b->val - a->val;
}

int
md_array_print(md_array * a, md_array_printer * pr, FILE *fp)
{
    char *label1;
    char *label2;
    int i1;
    int i2;

    a->d1.indexer->iter_fn(NULL);
    pr->start_array(fp, a->name);
    pr->d1_type(fp, a->d1.type);
    pr->d2_type(fp, a->d2.type);
    pr->start_data(fp);
    while ((i1 = a->d1.indexer->iter_fn(&label1)) > -1) {
	int skipped = 0;
	int skipped_sum = 0;
	int nvals;
	int si = 0;
	struct _foo *sortme = NULL;
	if (i1 >= a->d1.alloc_sz)
	    continue;		/* see [1] */
	pr->d1_begin(fp, label1);
	a->d2.indexer->iter_fn(NULL);
	nvals = a->d2.alloc_sz;
	sortme = xcalloc(nvals, sizeof(*sortme));
	if (NULL == sortme) {
	    syslog(LOG_CRIT, "%s", "Cant output XML file chunk due to malloc failure!");
	    continue;		/* OUCH! */
	}
	while ((i2 = a->d2.indexer->iter_fn(&label2)) > -1) {
	    int val;
	    if (i2 >= a->array[i1].alloc_sz)
		continue;
	    val = a->array[i1].array[i2];
	    if (0 == val)
		continue;
	    if (a->opts.min_count && (a->opts.min_count > val)) {
		skipped++;
		skipped_sum += val;
		continue;
	    }
	    sortme[si].val = val;
	    sortme[si].label = xstrdup(label2);
	    if (NULL == sortme[si].label)
		break;
	    si++;
	}
	assert(si <= nvals);
	nvals = si;
	qsort(sortme, nvals, sizeof(*sortme), compare);
	for (si = 0; si < nvals; si++) {
	    if (0 == a->opts.max_cells || si < a->opts.max_cells) {
		pr->print_element(fp, sortme[si].label, sortme[si].val);
	    } else {
		skipped++;
		skipped_sum += sortme[si].val;
	    }
	    xfree(sortme[si].label);
	    sortme[si].label = NULL;
	}
	if (skipped) {
	    pr->print_element(fp, "-:SKIPPED:-", skipped);
	    pr->print_element(fp, "-:SKIPPED_SUM:-", skipped_sum);
	}
	pr->d1_end(fp, label1);
	xfree(sortme);
	sortme = NULL;
    }
    pr->finish_data(fp);
    pr->finish_array(fp);
    return 0;
}


/* [1]
 * Its okay (not a bug) for the indexer's index to be larger
 * than the array size.  The indexer may have grown for use in a
 * different array, but the filter prevented it from growing this
 * particular array so far.
 */


filter_list **
md_array_filter_list_append(filter_list ** fl, FLTR * f)
{
    *fl = xcalloc(1, sizeof(**fl));
    if (NULL == (*fl))
	return NULL;
    (*fl)->filter = f;
    return (&(*fl)->next);
}

FLTR *
md_array_create_filter(const char *name, filter_func *func, const void *context)
{
    FLTR *f = xcalloc(1, sizeof(*f));
    if (NULL == f)
	return NULL;
    f->name = xstrdup(name);
    if (NULL == f->name) {
	xfree(f);
	return NULL;
    }
    f->func = func;
    f->context = context;
    return f;
}
