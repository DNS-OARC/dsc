/*
 * $Id$
 *
 * http://dnstop.measurement-factory.com/
 *
 * Copyright (c) 2002, The Measurement Factory, Inc.  All rights
 * reserved.  See the LICENSE file for details.
 */

#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
#include <string.h>

#include "string_counter.h"
#include "generic_counter.h"

StringCounter *
StringCounter_lookup_or_add(StringCounter ** headP, const char *s)
{
    StringCounter **T;
    for (T = headP; (*T); T = &(*T)->next)
	if (0 == strcmp((*T)->s, s))
	    return (*T);
    (*T) = calloc(1, sizeof(**T));
    (*T)->s = strdup(s);
    return (*T);
}

void
StringCounter_sort(StringCounter ** headP)
{
    gen_cnt *sortme;
    int n_things = 0;
    int i;
    StringCounter *sc;
    for (sc = *headP; sc; sc = sc->next)
	n_things++;
    sortme = calloc(n_things, sizeof(gen_cnt));
    n_things = 0;
    for (sc = *headP; sc; sc = sc->next) {
	sortme[n_things].cnt = sc->count;
	sortme[n_things].ptr = sc;
	n_things++;
    }
    qsort(sortme, n_things, sizeof(gen_cnt), gen_cnt_cmp);
    for (i = 0; i < n_things; i++) {
	*headP = sortme[i].ptr;
	headP = &(*headP)->next;
    }
    free(sortme);
    *headP = NULL;
}
