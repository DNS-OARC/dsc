/*
 * Copyright (c) 2016-2017, OARC, Inc.
 * Copyright (c) 2007, The Measurement Factory, Inc.
 * Copyright (c) 2007, Internet Systems Consortium, Inc.
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

#ifndef __dsc_hashtbl_h
#define __dsc_hashtbl_h


typedef struct _hashitem
{
    const void *key;
    void *data;
    struct _hashitem *next;
} hashitem;

typedef unsigned int hashfunc(const void *key);
typedef int hashkeycmp(const void *a, const void *b);
typedef void hashfree(void *p);

typedef struct
{
    unsigned int modulus;
    hashitem **items;
    hashfunc *hasher;
    hashkeycmp *keycmp;
    int use_arena;
    hashfree *keyfree;
    hashfree *datafree;
    struct
    {
        hashitem *next;
        unsigned int slot;
    } iter;
} hashtbl;


hashtbl *hash_create(int N, hashfunc *, hashkeycmp *, int use_arena, hashfree *, hashfree *);
void hash_destroy(hashtbl *);
int hash_add(const void *key, void *data, hashtbl *);
void hash_remove(const void *key, hashtbl * tbl);
void *hash_find(const void *key, hashtbl *);
void hash_iter_init(hashtbl *);
void *hash_iterate(hashtbl *);


/*
 * found in lookup3.c
 */
extern uint32_t hashlittle(const void *key, size_t length, uint32_t initval);
extern uint32_t hashbig(const void *key, size_t length, uint32_t initval);
extern uint32_t hashword(const uint32_t * k, size_t length, uint32_t initval);

#ifndef BYTE_ORDER
#define hashendian hashlittle
#elif BYTE_ORDER == BIG_ENDIAN
#define hashendian hashbig
#else
#define hashendian hashlittle
#endif

#endif /* __dsc_hashtbl_h */
