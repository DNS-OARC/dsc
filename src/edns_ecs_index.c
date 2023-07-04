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

#include "edns_ecs_index.h"
#include "xmalloc.h"
#include "hashtbl.h"

#include <string.h>

// edns_ecs

int edns_ecs_indexer(const dns_message* m)
{
    if (m->malformed)
        return -1;
    return m->edns.option.ecs;
}

static int next_iter;

int edns_ecs_iterator(const char** label)
{
    if (NULL == label) {
        next_iter = 0;
        return 2;
    }
    if (next_iter > 1)
        return -1;
    if (next_iter)
        *label = "yes";
    else
        *label = "no";
    return next_iter++;
}

// edns_ecs_family

static int family_largest = 0;

int edns_ecs_family_indexer(const dns_message* m)
{
    if (m->malformed)
        return -1;
    // family + 1 because ECS can have family 0 and idx 0 is none
    if (!m->edns.option.ecs)
        return 0;
    if (m->edns.ecs.family + 1 > family_largest)
        family_largest = m->edns.ecs.family + 1;
    return m->edns.ecs.family + 1;
}

static int family_next_iter;

int edns_ecs_family_iterator(const char** label)
{
    static char label_buf[20];
    if (NULL == label) {
        family_next_iter = 0;
        return family_largest + 1;
    }
    if (family_next_iter > family_largest)
        return -1;
    if (family_next_iter == 0)
        snprintf(label_buf, sizeof(label_buf), "none");
    else
        snprintf(label_buf, sizeof(label_buf), "%d", family_next_iter - 1);
    *label = label_buf;
    return family_next_iter++;
}

void edns_ecs_family_reset()
{
    family_largest = 0;
}

// edns_ecs_source_prefix

static int source_prefix_largest = 0;

int edns_ecs_source_prefix_indexer(const dns_message* m)
{
    if (m->malformed)
        return -1;
    // source_prefix + 1 because ECS can have source_prefix 0 and idx 0 is none
    if (!m->edns.option.ecs)
        return 0;
    if (m->edns.ecs.source_prefix + 1 > source_prefix_largest)
        source_prefix_largest = m->edns.ecs.source_prefix + 1;
    return m->edns.ecs.source_prefix + 1;
}

static int source_prefix_next_iter;

int edns_ecs_source_prefix_iterator(const char** label)
{
    static char label_buf[20];
    if (NULL == label) {
        source_prefix_next_iter = 0;
        return source_prefix_largest + 1;
    }
    if (source_prefix_next_iter > source_prefix_largest)
        return -1;
    if (source_prefix_next_iter == 0)
        snprintf(label_buf, sizeof(label_buf), "none");
    else
        snprintf(label_buf, sizeof(label_buf), "%d", source_prefix_next_iter - 1);
    *label = label_buf;
    return source_prefix_next_iter++;
}

void edns_ecs_source_prefix_reset()
{
    source_prefix_largest = 0;
}

// edns_ecs_scope_prefix

static int scope_prefix_largest = 0;

int edns_ecs_scope_prefix_indexer(const dns_message* m)
{
    if (m->malformed)
        return -1;
    // scope_prefix + 1 because ECS can have scope_prefix 0 and idx 0 is none
    if (!m->edns.option.ecs)
        return 0;
    if (m->edns.ecs.scope_prefix + 1 > scope_prefix_largest)
        scope_prefix_largest = m->edns.ecs.scope_prefix + 1;
    return m->edns.ecs.scope_prefix + 1;
}

static int scope_prefix_next_iter;

int edns_ecs_scope_prefix_iterator(const char** label)
{
    static char label_buf[20];
    if (NULL == label) {
        scope_prefix_next_iter = 0;
        return scope_prefix_largest + 1;
    }
    if (scope_prefix_next_iter > scope_prefix_largest)
        return -1;
    if (scope_prefix_next_iter == 0)
        snprintf(label_buf, sizeof(label_buf), "none");
    else
        snprintf(label_buf, sizeof(label_buf), "%d", scope_prefix_next_iter - 1);
    *label = label_buf;
    return scope_prefix_next_iter++;
}

void edns_ecs_scope_prefix_reset()
{
    scope_prefix_largest = 0;
}

// edns_ecs_address

#define MAX_ARRAY_SZ 65536

typedef struct
{
    const void* address;
    size_t      len;
} addresskey;

typedef struct
{
    addresskey key;
    void*      address;
    int        index;
} addressobj;

static unsigned int address_hashfunc(const void* key)
{
    return hashendian(((addresskey*)key)->address, ((addresskey*)key)->len, 0);
}

static int address_cmpfunc(const void* a, const void* b)
{
    if (((addresskey*)a)->len == ((addresskey*)b)->len) {
        return memcmp(((addresskey*)a)->address, ((addresskey*)b)->address, ((addresskey*)a)->len);
    }
    return ((addresskey*)a)->len < ((addresskey*)b)->len ? -1 : 1;
}

static void address_freefunc(void* obj)
{
    if (obj)
        afree(((addressobj*)obj)->address);
    afree(obj);
}

static hashtbl* addressHash      = NULL;
static int      address_next_idx = 0;

int edns_ecs_address_indexer(const dns_message* m)
{
    addressobj* obj;
    if (m->malformed || !m->edns.ecs.address)
        return -1;
    addresskey key = { m->edns.ecs.address, m->edns.ecs.len };
    if (NULL == addressHash) {
        addressHash = hash_create(MAX_ARRAY_SZ, address_hashfunc, address_cmpfunc, 1, 0, address_freefunc);
        if (NULL == addressHash)
            return -1;
    }
    if ((obj = hash_find(&key, addressHash)))
        return obj->index;
    obj = acalloc(1, sizeof(*obj));
    if (NULL == obj)
        return -1;
    obj->address = amalloc(m->edns.ecs.len);
    if (NULL == obj->address) {
        afree(obj);
        return -1;
    }
    obj->key.len     = m->edns.ecs.len;
    obj->key.address = obj->address;
    memcpy(obj->address, m->edns.ecs.address, obj->key.len);
    obj->index = address_next_idx;
    if (0 != hash_add(&obj->key, obj, addressHash)) {
        afree(obj->address);
        afree(obj);
        return -1;
    }
    address_next_idx++;
    return obj->index;
}

int edns_ecs_address_iterator(const char** label)
{
    static char label_buf[1024];
    addressobj* obj;
    if (0 == address_next_idx)
        return -1;
    if (NULL == label) {
        /* initialize and tell caller how big the array is */
        hash_iter_init(addressHash);
        return address_next_idx;
    }
    if ((obj = hash_iterate(addressHash)) == NULL)
        return -1;
    size_t len = obj->key.len;
    if (len > 128)
        len = 128;
    strtohex(label_buf, obj->key.address, len);
    label_buf[len * 2] = 0;
    *label             = label_buf;
    return obj->index;
}

void edns_ecs_address_reset()
{
    addressHash      = NULL;
    address_next_idx = 0;
}
