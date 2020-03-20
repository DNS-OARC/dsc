/*
 * Copyright (c) 2008-2020, OARC, Inc.
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

#include "qname_index.h"
#include "hashtbl.h"
#include "xmalloc.h"

#include <string.h>

typedef struct
{
    int      next_idx;
    hashtbl* hash;
} levelobj;

static hashfunc   name_hashfunc;
static hashkeycmp name_cmpfunc;
static int        name_indexer(const char*, levelobj*);
static int        name_iterator(const char**, levelobj*);
static void       name_reset(levelobj*);

#define MAX_ARRAY_SZ 65536

static levelobj Full   = { 0, NULL };
static levelobj Second = { 0, NULL };
static levelobj Third  = { 0, NULL };

typedef struct
{
    char* name;
    int   index;
} nameobj;

/* ==== QNAME ============================================================= */

int qname_indexer(const dns_message* m)
{
    if (m->malformed)
        return -1;
    return name_indexer(m->qname, &Full);
}

int qname_iterator(const char** label)
{
    return name_iterator(label, &Full);
}

void qname_reset()
{
    name_reset(&Full);
}

/* ==== SECOND LEVEL DOMAIN =============================================== */

int second_ld_indexer(const dns_message* m)
{
    if (m->malformed)
        return -1;
    return name_indexer(dns_message_QnameToNld(m->qname, 2), &Second);
}

int second_ld_iterator(const char** label)
{
    return name_iterator(label, &Second);
}

void second_ld_reset()
{
    name_reset(&Second);
}

/* ==== QNAME ============================================================= */

int third_ld_indexer(const dns_message* m)
{
    if (m->malformed)
        return -1;
    return name_indexer(dns_message_QnameToNld(m->qname, 3), &Third);
}

int third_ld_iterator(const char** label)
{
    return name_iterator(label, &Third);
}

void third_ld_reset()
{
    name_reset(&Third);
}

/* ======================================================================== */

static int
name_indexer(const char* theName, levelobj* theLevel)
{
    nameobj* obj;
    if (NULL == theLevel->hash) {
        theLevel->hash = hash_create(MAX_ARRAY_SZ, name_hashfunc, name_cmpfunc, 1, afree, afree);
        if (NULL == theLevel->hash)
            return -1;
    }
    if ((obj = hash_find(theName, theLevel->hash)))
        return obj->index;
    obj = acalloc(1, sizeof(*obj));
    if (NULL == obj)
        return -1;
    obj->name = astrdup(theName);
    if (NULL == obj->name) {
        afree(obj);
        return -1;
    }
    obj->index = theLevel->next_idx;
    if (0 != hash_add(obj->name, obj, theLevel->hash)) {
        afree(obj->name);
        afree(obj);
        return -1;
    }
    theLevel->next_idx++;
    return obj->index;
}

static int
name_iterator(const char** label, levelobj* theLevel)
{
    nameobj*    obj;
    static char label_buf[MAX_QNAME_SZ];
    if (0 == theLevel->next_idx)
        return -1;
    if (NULL == label) {
        hash_iter_init(theLevel->hash);
        return theLevel->next_idx;
    }
    if ((obj = hash_iterate(theLevel->hash)) == NULL)
        return -1;
    snprintf(label_buf, sizeof(label_buf), "%s", obj->name);
    *label = label_buf;
    return obj->index;
}

static void
name_reset(levelobj* theLevel)
{
    theLevel->hash     = NULL;
    theLevel->next_idx = 0;
}

static unsigned int
name_hashfunc(const void* key)
{
    return hashendian(key, strlen(key), 0);
}

static int
name_cmpfunc(const void* a, const void* b)
{
    return strcasecmp(a, b);
}
