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

#include "client_subnet_index.h"
#include "xmalloc.h"
#include "hashtbl.h"
#include "syslog_debug.h"

static hashfunc   ipnet_hashfunc;
static hashkeycmp ipnet_cmpfunc;
static inX_addr   v4mask;
static inX_addr   v6mask;
static int        v4set = 0, v6set = 0;

#define MAX_ARRAY_SZ 65536
static hashtbl* theHash  = NULL;
static int      next_idx = 0;

typedef struct
{
    inX_addr addr;
    int      index;
} ipnetobj;

int client_subnet_indexer(const dns_message* m)
{
    ipnetobj* obj;
    inX_addr  masked_addr;
    inX_addr* client_ip_addr = m->qr ? &m->tm->dst_ip_addr : &m->tm->src_ip_addr;

    if (m->malformed)
        return -1;
    if (NULL == theHash) {
        theHash = hash_create(MAX_ARRAY_SZ, ipnet_hashfunc, ipnet_cmpfunc, 1, NULL, afree);
        if (NULL == theHash)
            return -1;
    }
    if (6 == inXaddr_version(client_ip_addr))
        masked_addr = inXaddr_mask(client_ip_addr, &v6mask);
    else
        masked_addr = inXaddr_mask(client_ip_addr, &v4mask);
    if ((obj = hash_find(&masked_addr, theHash)))
        return obj->index;
    obj = acalloc(1, sizeof(*obj));
    if (NULL == obj)
        return -1;
    obj->addr  = masked_addr;
    obj->index = next_idx;
    if (0 != hash_add(&obj->addr, obj, theHash)) {
        afree(obj);
        return -1;
    }
    next_idx++;
    return obj->index;
}

int client_subnet_iterator(const char** label)
{
    ipnetobj*   obj;
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

void client_subnet_reset()
{
    theHash  = NULL;
    next_idx = 0;
}

void client_subnet_init(void)
{
    if (!v4set) {
        inXaddr_pton("255.255.255.0", &v4mask);
    }
    if (!v6set) {
        inXaddr_pton("ffff:ffff:ffff:ffff:ffff:ffff:0000:0000", &v6mask);
    }
}

int client_subnet_v4_mask_set(const char* mask)
{
    v4set = 1;
    dsyslogf(LOG_INFO, "change v4 mask to %s", mask);
    return inXaddr_pton(mask, &v4mask);
}

int client_subnet_v6_mask_set(const char* mask)
{
    v6set = 1;
    dsyslogf(LOG_INFO, "change v6 mask to %s", mask);
    return inXaddr_pton(mask, &v6mask);
}

static unsigned int
ipnet_hashfunc(const void* key)
{
    const inX_addr* a = key;
    return inXaddr_hash(a);
}

static int
ipnet_cmpfunc(const void* a, const void* b)
{
    const inX_addr* a1 = a;
    const inX_addr* a2 = b;
    return inXaddr_cmp(a1, a2);
}
