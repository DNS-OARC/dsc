/*
 * Copyright (c) 2008-2023, OARC, Inc.
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

#include "hashtbl.h"
#include "xmalloc.h"

hashtbl* hash_create(int N, hashfunc* hasher, hashkeycmp* cmp, int use_arena, hashfree* keyfree, hashfree* datafree)
{
    hashtbl* new = (*(use_arena ? acalloc : xcalloc))(1, sizeof(*new));
    if (NULL == new)
        return NULL;
    new->modulus   = N;
    new->hasher    = hasher;
    new->keycmp    = cmp;
    new->use_arena = use_arena;
    new->keyfree   = keyfree;
    new->datafree  = datafree;
    new->items     = (*(use_arena ? acalloc : xcalloc))(N, sizeof(hashitem*));
    if (NULL == new->items) {
        if (!use_arena)
            xfree(new);
        return NULL;
    }
    return new;
}

void hash_destroy(hashtbl* tbl)
{
    hashitem *i, *next;
    int       slot;
    for (slot = 0; slot < tbl->modulus; slot++) {
        for (i = tbl->items[slot]; i;) {
            next = i->next;
            if (tbl->keyfree)
                tbl->keyfree((void*)i->key);
            if (tbl->datafree)
                tbl->datafree(i->data);
            if (!tbl->use_arena)
                xfree(i);
            i = next;
        }
    }
    if (!tbl->use_arena)
        xfree(tbl);
}

int hash_add(const void* key, void* data, hashtbl* tbl)
{
    hashitem* new = (*(tbl->use_arena ? acalloc : xcalloc))(1, sizeof(*new));
    hashitem** I;
    int        slot;
    if (NULL == new)
        return 1;
    new->key  = key;
    new->data = data;
    slot      = tbl->hasher(key) % tbl->modulus;
    for (I = &tbl->items[slot]; *I; I = &(*I)->next)
        ;
    *I = new;
    return 0;
}

void hash_remove(const void* key, hashtbl* tbl)
{
    hashitem **I, *i;
    int        slot;
    slot = tbl->hasher(key) % tbl->modulus;
    for (I = &tbl->items[slot]; *I; I = &(*I)->next) {
        if (0 == tbl->keycmp(key, (*I)->key)) {
            i  = *I;
            *I = (*I)->next;
            if (tbl->keyfree)
                tbl->keyfree((void*)i->key);
            if (tbl->datafree)
                tbl->datafree(i->data);
            if (!tbl->use_arena)
                xfree(i);
            break;
        }
    }
}

void* hash_find(const void* key, hashtbl* tbl)
{
    int       slot = tbl->hasher(key) % tbl->modulus;
    hashitem* i;
    for (i = tbl->items[slot]; i; i = i->next) {
        if (0 == tbl->keycmp(key, i->key))
            return i->data;
    }
    return NULL;
}

static void
hash_iter_next_slot(hashtbl* tbl)
{
    while (tbl->iter.next == NULL) {
        tbl->iter.slot++;
        if (tbl->iter.slot == tbl->modulus)
            break;
        tbl->iter.next = tbl->items[tbl->iter.slot];
    }
}

void hash_iter_init(hashtbl* tbl)
{
    tbl->iter.slot = 0;
    tbl->iter.next = tbl->items[tbl->iter.slot];
    if (NULL == tbl->iter.next)
        hash_iter_next_slot(tbl);
}

void* hash_iterate(hashtbl* tbl)
{
    hashitem* this = tbl->iter.next;
    if (this) {
        tbl->iter.next = this->next;
        if (NULL == tbl->iter.next)
            hash_iter_next_slot(tbl);
    }
    return this ? this->data : NULL;
}

// dst needs to be at least len * 2 in size
void strtohex(char* dst, const char* src, size_t len)
{
    const char xx[] = "0123456789ABCDEF";
    size_t     i;
    for (i = 0; i < len; i++) {
        dst[i * 2]     = xx[(unsigned char)src[i] >> 4];
        dst[i * 2 + 1] = xx[(unsigned char)src[i] & 0xf];
    }
}
