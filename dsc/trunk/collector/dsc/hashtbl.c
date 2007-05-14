
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#ifdef __linux__
#include <stdint.h>
#endif
#include "xmalloc.h"
#include "hashtbl.h"

hashtbl
*hash_create(int N, hashfunc *hasher, hashkeycmp *cmp)
{
	hashtbl *new = xcalloc(1, sizeof(*new));
	if (NULL == new)
	    return NULL;
	new->modulus = N;
	new->hasher = hasher;
	new->keycmp = cmp;
	new->items = xcalloc(N, sizeof(hashitem*));
	if (NULL == new->items) {
		free(new);
		return NULL;
	}
	return new;
}

int
hash_add(const void *key, void *data, hashtbl *tbl)
{
	hashitem *new = xcalloc(1, sizeof(*new));
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
