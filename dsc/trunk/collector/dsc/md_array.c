
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <syslog.h>

#include "xmalloc.h"
#include "dataset_opt.h"
#include "md_array.h"
#include "dns_message.h"
#include "ip_message.h"
#include "pcap.h"
#include "syslog_debug.h"

void md_array_grow_d1(md_array * a);
void md_array_grow_d2(md_array * a);

static void
md_array_free(md_array *a)
{
    if (a->name)
	free((char *)a->name);
    if (a->d1.type)
	free((char *)a->d1.type);
    if (a->d2.type)
	free((char *)a->d2.type);
    if (a->array) {
	int i1;
        for (i1 = 0; i1 < a->d1.alloc_sz; i1++) {
	    if (a->array[i1])
		free(a->array[i1]);
	}
	free(a->array);
    }
    free(a);
}

md_array *
md_array_create(const char *name, filter_list * fl,
    const char *type1, IDXR * idx1, HITR * itr1,
    const char *type2, IDXR * idx2, HITR * itr2)
{
    int i1;
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
    a->d1.iterator = itr1;
    a->d1.alloc_sz = 2;
    a->d2.type = xstrdup(type2);
    if (a->d2.type == NULL) {
	md_array_free(a);
	return NULL;
    }
    a->d2.indexer = idx2;
    a->d2.iterator = itr2;
    a->d2.alloc_sz = 2;
    a->array = xcalloc(a->d1.alloc_sz, sizeof(int *));
    if (NULL == a->array) {
	md_array_free(a);
	return NULL;
    }
    for (i1 = 0; i1 < a->d1.alloc_sz; i1++) {
	a->array[i1] = xcalloc(a->d2.alloc_sz, sizeof(int));
	if (NULL == a->array[i1]) {
	    md_array_free(a);
	    return NULL;
	}
    }
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

    if ((i1 = a->d1.indexer(vp)) < 0)
	return -1;
    if ((i2 = a->d2.indexer(vp)) < 0)
	return -1;

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
    new = xcalloc(new_alloc_sz, sizeof(int *));
    if (NULL == new)
	return;

    /*
     * first, try allocating the new half
     */
    for (i1 = a->d1.alloc_sz; i1 < new_alloc_sz; i1++) {
	new[i1] = xcalloc(a->d2.alloc_sz, sizeof(int));
	if (NULL == new[i1]) {
	    /* oops, undo! */
	    for (--i1; i1 >= a->d1.alloc_sz; i1--)
		free(new[i1]);
	    free(new);
	    return;
	}
    }

    /*
     * now copy over the old half
     */
    memcpy(new, a->array, a->d1.alloc_sz * sizeof(int *));
    free(a->array);
    a->array = new;

    a->d1.alloc_sz = new_alloc_sz;
}

void
md_array_grow_d2(md_array * a)
{
    int new_alloc_sz;
    int **new;
    int i1;
    new = xcalloc(a->d1.alloc_sz, sizeof(int *));
    if (NULL == new)
	return;
    new_alloc_sz = a->d2.alloc_sz << 1;

    /*
     * first try to allocate all the new d2's
     */
    for (i1 = 0; i1 < a->d1.alloc_sz; i1++) {
	new[i1] = xcalloc(new_alloc_sz, sizeof(int));
	if (NULL == new[i1]) {
	    /* oops, undo! */
	    for (--i1; i1 >= 0; i1--)
		free(new[i1]);
	    free(new);
	    return;
	}
    }

    /*
     * now copy and swap
     */
    for (i1 = 0; i1 < a->d1.alloc_sz; i1++) {
	memcpy(new[i1], a->array[i1], a->d2.alloc_sz * sizeof(int));
	free(a->array[i1]);
    }
    free(a->array);
    a->array = new;

    a->d2.alloc_sz = new_alloc_sz;
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
md_array_print(md_array * a, md_array_printer * pr)
{
    int fd;
    FILE *fp;
    char fname[128];
    char tname[128];
    char *label1;
    char *label2;
    int i1;
    int i2;

    snprintf(fname, 128, "%d.%s.xml", Pcap_finish_time(), a->name);
    snprintf(tname, 128, "%s.XXXXXXXXX", fname);
    fd = mkstemp(tname);
    if (fd < 0)
	return -1;
    fp = fdopen(fd, "w");
    if (NULL == fp) {
	close(fd);
	return -1;
    }
    a->d1.iterator(NULL);
    pr->start_array(fp, a->name);
    pr->d1_type(fp, a->d1.type);
    pr->d2_type(fp, a->d2.type);
    pr->start_data(fp);
    while ((i1 = a->d1.iterator(&label1)) > -1) {
	int skipped = 0;
	int skipped_sum = 0;
	int nvals;
	int si = 0;
	struct _foo *sortme = NULL;
	if (i1 >= a->d1.alloc_sz)
	    continue;		/* see [1] */
	pr->d1_begin(fp, label1);
	a->d2.iterator(NULL);
	nvals = a->d2.alloc_sz;
	sortme = xcalloc(nvals, sizeof(*sortme));
	if (NULL == sortme) {
	    syslog(LOG_CRIT, "%s", "Cant output XML file chunk due to malloc failure!");
	    continue;		/* OUCH! */
	}
	while ((i2 = a->d2.iterator(&label2)) > -1) {
	    if (i2 >= a->d2.alloc_sz)
		continue;
	    if (0 == a->array[i1][i2])
		continue;
	    if (a->opts.min_count && (a->opts.min_count > a->array[i1][i2])) {
		skipped++;
		skipped_sum += a->array[i1][i2];
		continue;
	    }
	    sortme[si].val = a->array[i1][i2];
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
	    free(sortme[si].label);
	    sortme[si].label = NULL;
	}
	if (skipped) {
	    pr->print_element(fp, "-:SKIPPED:-", skipped);
	    pr->print_element(fp, "-:SKIPPED_SUM:-", skipped_sum);
	}
	pr->d1_end(fp, label1);
	free(sortme);
	sortme = NULL;
    }
    pr->finish_data(fp);
    pr->finish_array(fp);
    /*
     * XXX need chmod because files are written as root, but
     * may be processed by a non-priv user
     */
    fchmod(fd, 0664);
    fclose(fp);
    rename(tname, fname);
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
	free(f);
	return NULL;
    }
    f->func = func;
    f->context = context;
    return f;
}
