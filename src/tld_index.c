/*
 * Copyright (c) 2008-2018, OARC, Inc.
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

#include "tld_index.h"
#include "xmalloc.h"
#include "hashtbl.h"

#include <string.h>

static hashfunc   tld_hashfunc;
static hashkeycmp tld_cmpfunc;

#define MAX_ARRAY_SZ 65536
static hashtbl* theHash  = NULL;
static int      next_idx = 0;

typedef struct
{
    char* tld;
    int   index;
} tldobj;

int tld_indexer(const dns_message* m)
{
    const char* tld;
    tldobj*     obj;
    if (m->malformed)
        return -1;
    tld = dns_message_tld((dns_message*)m);
    if (NULL == theHash) {
        theHash = hash_create(MAX_ARRAY_SZ, tld_hashfunc, tld_cmpfunc, 1, afree, afree);
        if (NULL == theHash)
            return -1;
    }
    if ((obj = hash_find(tld, theHash)))
        return obj->index;
    obj = acalloc(1, sizeof(*obj));
    if (NULL == obj)
        return -1;
    obj->tld = astrdup(tld);
    if (NULL == obj->tld) {
        afree(obj);
        return -1;
    }
    obj->index = next_idx;
    if (0 != hash_add(obj->tld, obj, theHash)) {
        afree(obj->tld);
        afree(obj);
        return -1;
    }
    next_idx++;
    return obj->index;
}

int tld_iterator(const char** label)
{
    tldobj*     obj;
    static char label_buf[MAX_QNAME_SZ];
    if (0 == next_idx)
        return -1;
    if (NULL == label) {
        /* initialize and tell caller how big the array is */
        hash_iter_init(theHash);
        return next_idx;
    }
    if ((obj = hash_iterate(theHash)) == NULL)
        return -1;
    snprintf(label_buf, sizeof(label_buf), "%s", obj->tld);
    *label = label_buf;
    return obj->index;
}

void tld_reset()
{
    theHash  = NULL;
    next_idx = 0;
}

static unsigned int
tld_hashfunc(const void* key)
{
    return hashendian(key, strlen(key), 0);
}

static int
tld_cmpfunc(const void* a, const void* b)
{
    return strcasecmp(a, b);
}
