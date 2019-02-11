/*
 * Copyright (c) 2008-2019, OARC, Inc.
 * Copyright (c) 2007-2008, Internet Systems Consortium, Inc.
 * Copyright (c) 2003-2007, The Measurement Factory, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "md_array.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "xmalloc.h"
#include "dataset_opt.h"
#include "dns_message.h"
#include "pcap.h"
#include "syslog_debug.h"

/*
 * Private
 */

struct d2sort {
    char* label;
    int   val;
};

static int d2cmp(const void* a, const void* b)
{
    /*
     * descending sort order (larger to smaller)
     */
    return ((struct d2sort*)b)->val - ((struct d2sort*)a)->val;
}

static void md_array_free(md_array* a)
{
    if (a->name)
        xfree((char*)a->name);
    if (a->d1.type)
        xfree((char*)a->d1.type);
    if (a->d2.type)
        xfree((char*)a->d2.type);
    /* a->array contents were in an arena, so we don't need to free them. */
    xfree(a);
}

static void md_array_grow(md_array* a, int i1, int i2)
{
    int            new_d1_sz, new_d2_sz;
    md_array_node* d1 = NULL;
    int*           d2 = NULL;

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
            afree(d1);
            return;
        }

        /* copy old contents to new array */
        memcpy(d2, d1[i1].array, d1[i1].alloc_sz * sizeof(*d2));
    }

    if (d1 != a->array) {
        if (a->array) {
            dfprintf(0, "grew d1 of %s from %d to %d", a->name, a->d1.alloc_sz, new_d1_sz);
            afree(a->array);
        }
        a->array       = d1;
        a->d1.alloc_sz = new_d1_sz;
    }
    if (d2) {
        if (a->array[i1].array) {
            dfprintf(0, "grew d2[%d] of %s from %d to %d", i1, a->name, a->array[i1].alloc_sz, new_d2_sz);
            afree(a->array[i1].array);
        }
        a->array[i1].array    = d2;
        a->array[i1].alloc_sz = new_d2_sz;
    }

    if (new_d2_sz > a->d2.alloc_sz)
        a->d2.alloc_sz = new_d2_sz;
}

/*
 * Public
 */

md_array* md_array_create(const char* name, filter_list* fl, const char* type1, indexer* idx1, const char* type2, indexer* idx2)
{
    md_array* a = xcalloc(1, sizeof(*a));
    if (NULL == a)
        return NULL;
    a->name = xstrdup(name);
    if (a->name == NULL) {
        md_array_free(a);
        return NULL;
    }
    a->filter_list = fl;
    a->d1.type     = xstrdup(type1);
    if (a->d1.type == NULL) {
        md_array_free(a);
        return NULL;
    }
    a->d1.indexer  = idx1;
    a->d1.alloc_sz = 0;
    a->d2.type     = xstrdup(type2);
    if (a->d2.type == NULL) {
        md_array_free(a);
        return NULL;
    }
    a->d2.indexer  = idx2;
    a->d2.alloc_sz = 0;
    a->array       = NULL; /* will be allocated when needed, in an arena. */
    return a;
}

void md_array_clear(md_array* a)
{
    /* a->array contents were in an arena, so we don't need to free them. */
    a->array       = NULL;
    a->d1.alloc_sz = 0;
    if (a->d1.indexer->reset_fn)
        a->d1.indexer->reset_fn();
    a->d2.alloc_sz = 0;
    if (a->d2.indexer->reset_fn)
        a->d2.indexer->reset_fn();
}

int md_array_count(md_array* a, const void* vp)
{
    int          i1;
    int          i2;
    filter_list* fl;

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

void md_array_flush(md_array* a)
{
    const void* vp;

    if (a->d1.indexer->flush_fn)
        a->d1.indexer->flush_fn(flush_on);
    if (a->d2.indexer->flush_fn)
        a->d2.indexer->flush_fn(flush_on);

    if (a->d1.indexer->flush_fn) {
        while ((vp = a->d1.indexer->flush_fn(flush_get))) {
            md_array_count(a, vp);
        }
    }
    if (a->d2.indexer->flush_fn) {
        while ((vp = a->d2.indexer->flush_fn(flush_get))) {
            md_array_count(a, vp);
        }
    }

    if (a->d1.indexer->flush_fn)
        a->d1.indexer->flush_fn(flush_off);
    if (a->d2.indexer->flush_fn)
        a->d2.indexer->flush_fn(flush_off);
}

int md_array_print(md_array* a, md_array_printer* pr, FILE* fp)
{
    const char* label1;
    const char* label2;
    int         i1;
    int         i2;

    a->d1.indexer->iter_fn(NULL);
    pr->start_array(fp, a->name);
    pr->d1_type(fp, a->d1.type);
    pr->d2_type(fp, a->d2.type);
    pr->start_data(fp);
    while ((i1 = a->d1.indexer->iter_fn(&label1)) > -1) {
        int            skipped     = 0;
        int            skipped_sum = 0;
        int            nvals;
        int            si = 0;
        struct d2sort* sortme;

        if (i1 >= a->d1.alloc_sz)
            /*
             * Its okay (not a bug) for the indexer's index to be larger
             * than the array size.  The indexer may have grown for use in a
             * different array, but the filter prevented it from growing this
             * particular array so far.
             */
            continue;

        pr->d1_begin(fp, label1);
        a->d2.indexer->iter_fn(NULL);
        nvals = a->d2.alloc_sz;

        sortme = xcalloc(nvals, sizeof(*sortme));
        if (NULL == sortme) {
            dsyslogf(LOG_CRIT, "Cant output %s file chunk due to malloc failure!", pr->format);
            continue;
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
            sortme[si].val   = val;
            sortme[si].label = xstrdup(label2);
            if (NULL == sortme[si].label)
                break;
            si++;
        }
        assert(si <= nvals);
        nvals = si;

        qsort(sortme, nvals, sizeof(*sortme), d2cmp);

        for (si = 0; si < nvals; si++) {
            if (0 == a->opts.max_cells || si < a->opts.max_cells) {
                pr->print_element(fp, sortme[si].label, sortme[si].val);
            } else {
                skipped++;
                skipped_sum += sortme[si].val;
            }
            xfree(sortme[si].label);
        }
        xfree(sortme);

        if (skipped) {
            pr->print_element(fp, "-:SKIPPED:-", skipped);
            pr->print_element(fp, "-:SKIPPED_SUM:-", skipped_sum);
        }
        pr->d1_end(fp, label1);
    }
    pr->finish_data(fp);
    pr->finish_array(fp);
    return 0;
}

filter_list** md_array_filter_list_append(filter_list** fl, filter_defn* f)
{
    *fl = xcalloc(1, sizeof(**fl));
    if (NULL == (*fl))
        return NULL;
    (*fl)->filter = f;
    return (&(*fl)->next);
}

filter_defn* md_array_create_filter(const char* name, filter_func func, const void* context)
{
    filter_defn* f = xcalloc(1, sizeof(*f));
    if (NULL == f)
        return NULL;
    f->name = xstrdup(name);
    if (NULL == f->name) {
        xfree(f);
        return NULL;
    }
    f->func    = func;
    f->context = context;
    return f;
}
