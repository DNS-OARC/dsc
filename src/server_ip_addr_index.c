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

#include "server_ip_addr_index.h"
#include "xmalloc.h"
#include "hashtbl.h"
#include "inX_addr.h"

#define MAX_ARRAY_SZ 65536
static hashtbl* theHash  = NULL;
static int      next_idx = 0;

typedef struct
{
    inX_addr addr;
    int      index;
} ipaddrobj;

int sip_indexer(const dns_message* m)
{
    ipaddrobj* obj;
    inX_addr*  server_ip_addr = m->qr ? &m->tm->src_ip_addr : &m->tm->dst_ip_addr;

    if (m->malformed)
        return -1;
    if (NULL == theHash) {
        theHash = hash_create(MAX_ARRAY_SZ, (hashfunc*)inXaddr_hash, (hashkeycmp*)inXaddr_cmp, 1, NULL, afree);
        if (NULL == theHash)
            return -1;
    }
    if ((obj = hash_find(server_ip_addr, theHash)))
        return obj->index;
    obj = acalloc(1, sizeof(*obj));
    if (NULL == obj)
        return -1;
    obj->addr  = *server_ip_addr;
    obj->index = next_idx;
    if (0 != hash_add(&obj->addr, obj, theHash)) {
        afree(obj);
        return -1;
    }
    next_idx++;
    return obj->index;
}

int sip_iterator(const char** label)
{
    ipaddrobj*  obj;
    static char label_buf[128];
    if (0 == next_idx)
        return -1;
    if (NULL == label) {
        hash_iter_init(theHash);
        return next_idx;
    }
    if ((obj = hash_iterate(theHash)) == NULL)
        return -1;
    inXaddr_ntop(&obj->addr, label_buf, 128);
    *label = label_buf;
    return obj->index;
}

void sip_reset()
{
    theHash  = NULL;
    next_idx = 0;
}
