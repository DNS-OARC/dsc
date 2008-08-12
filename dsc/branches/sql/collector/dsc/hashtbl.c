#include "config.h"
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#if HAVE_STDINT_H
#include <stdint.h>
#endif
#include "xmalloc.h"
#include "hashtbl.h"

hashtbl
*hash_create(int N, hashfunc *hasher, hashkeycmp *cmp, int use_arena,
    hashfree *keyfree, hashfree *datafree)
{
	hashtbl *new = (*(use_arena ? acalloc : xcalloc))(1, sizeof(*new));
	if (NULL == new)
	    return NULL;
	new->modulus = N;
	new->hasher = hasher;
	new->keycmp = cmp;
	new->use_arena = use_arena;
	new->keyfree = keyfree;
	new->datafree = datafree;
	new->items = (*(use_arena ? acalloc : xcalloc))(N, sizeof(hashitem*));
	if (NULL == new->items) {
		if (!use_arena)
		    xfree(new);
		return NULL;
	}
	return new;
}

void
hash_destroy(hashtbl *tbl)
{
    hashitem *i, *next;
    int slot;
    for (slot = 0; slot < tbl->modulus; slot++) {
	for (i = tbl->items[slot]; i; i = next) {
	    next = i->next;
	    if (tbl->keyfree)
		tbl->keyfree((void *)i->key);
	    if (tbl->datafree)
		tbl->datafree(i->data);
	    if (!tbl->use_arena)
		xfree(i);
	}
    }
    if (!tbl->use_arena)
	xfree(tbl);
}

int
hash_add(const void *key, void *data, hashtbl *tbl)
{
	hashitem *new = (*(tbl->use_arena ? acalloc : xcalloc))(1,sizeof(*new));
	hashitem **I;
	int slot;
	if (NULL == new)
	    return 1;
	new->key = key;
	new->data = data;
	slot = tbl->hasher(key) % tbl->modulus;
	for (I = &tbl->items[slot]; *I; I = &(*I)->next);
	*I = new;
	return 0;
}

void
hash_remove(const void *key, hashtbl *tbl)
{
	hashitem **I, *i;
	int slot;
	slot = tbl->hasher(key) % tbl->modulus;
	for (I = &tbl->items[slot]; *I; I = &(*I)->next) {
	    if (0 == tbl->keycmp(key, (*I)->key)) {
		i = *I;
		*I = (*I)->next;
		if (tbl->keyfree)
		    tbl->keyfree((void *)i->key);
		if (tbl->datafree)
		    tbl->datafree(i->data);
		if (!tbl->use_arena)
		    xfree(i);
		break;
	    }
	}
}

void *
hash_find(const void *key, hashtbl *tbl)
{
	int slot = tbl->hasher(key) % tbl->modulus;
	hashitem *i;
	for (i = tbl->items[slot]; i; i = i->next) {
		if (0 == tbl->keycmp(key, i->key))
		    return i->data;
	}
	return NULL;
}

static void
hash_iter_next_slot(hashtbl *tbl)
{
	while (tbl->iter.next == NULL) {
		tbl->iter.slot++;
		if (tbl->iter.slot == tbl->modulus)
			break;
		tbl->iter.next = tbl->items[tbl->iter.slot];
	}
}

void
hash_iter_init(hashtbl *tbl)
{
	tbl->iter.slot = 0;
	tbl->iter.next = tbl->items[tbl->iter.slot];
	if (NULL == tbl->iter.next)
		hash_iter_next_slot(tbl);
}

void *
hash_iterate(hashtbl *tbl)
{
	hashitem *this = tbl->iter.next;
	if (this) {
		tbl->iter.next = this->next;
		if (NULL == tbl->iter.next)
			hash_iter_next_slot(tbl);
	}
	return this ? this->data : NULL;
}
