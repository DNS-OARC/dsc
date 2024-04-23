/*
 * Copyright (c) 2008-2024 OARC, Inc.
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

#include "edns_cookie_index.h"
#include "xmalloc.h"
#include "hashtbl.h"

#include <string.h>

// edns_cookie

int edns_cookie_indexer(const dns_message* m)
{
    if (m->malformed)
        return -1;
    return m->edns.option.cookie;
}

static int next_iter;

int edns_cookie_iterator(const char** label)
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

// edns_cookie_len

static int len_largest = 0;

int edns_cookie_len_indexer(const dns_message* m)
{
    if (m->malformed)
        return -1;
    size_t len = m->edns.cookie.server_len;
    if (m->edns.cookie.client)
        len += 8;
    if (len > len_largest)
        len_largest = len;
    return len;
}

static int len_next_iter;

int edns_cookie_len_iterator(const char** label)
{
    static char label_buf[20];
    if (NULL == label) {
        len_next_iter = 0;
        return len_largest + 1;
    }
    if (len_next_iter > len_largest)
        return -1;
    if (len_next_iter == 0)
        snprintf(label_buf, sizeof(label_buf), "none");
    else
        snprintf(label_buf, sizeof(label_buf), "%d", len_next_iter);
    *label = label_buf;
    return len_next_iter++;
}

void edns_cookie_len_reset()
{
    len_largest = 0;
}

// cookie hash

#define MAX_ARRAY_SZ 65536

typedef struct
{
    char* cookie;
    int   index;
} cookieobj;

static unsigned int cookie_hashfunc(const void* key)
{
    return hashendian(key, strlen(key), 0);
}

static int cookie_cmpfunc(const void* a, const void* b)
{
    return strcasecmp(a, b);
}

// edns_cookie_client

static hashtbl* clientHash      = NULL;
static int      client_next_idx = 0;

int edns_cookie_client_indexer(const dns_message* m)
{
    cookieobj* obj;
    if (m->malformed || !m->edns.cookie.client)
        return -1;
    char cookie[64];
    strtohex(cookie, (char*)m->edns.cookie.client, 8);
    cookie[16] = 0;
    if (NULL == clientHash) {
        clientHash = hash_create(MAX_ARRAY_SZ, cookie_hashfunc, cookie_cmpfunc, 1, afree, afree);
        if (NULL == clientHash)
            return -1;
    }
    if ((obj = hash_find(cookie, clientHash)))
        return obj->index;
    obj = acalloc(1, sizeof(*obj));
    if (NULL == obj)
        return -1;
    obj->cookie = astrdup(cookie);
    if (NULL == obj->cookie) {
        afree(obj);
        return -1;
    }
    obj->index = client_next_idx;
    if (0 != hash_add(obj->cookie, obj, clientHash)) {
        afree(obj->cookie);
        afree(obj);
        return -1;
    }
    client_next_idx++;
    return obj->index;
}

int edns_cookie_client_iterator(const char** label)
{
    cookieobj* obj;
    if (0 == client_next_idx)
        return -1;
    if (NULL == label) {
        /* initialize and tell caller how big the array is */
        hash_iter_init(clientHash);
        return client_next_idx;
    }
    if ((obj = hash_iterate(clientHash)) == NULL)
        return -1;
    *label = obj->cookie;
    return obj->index;
}

void edns_cookie_client_reset()
{
    clientHash      = NULL;
    client_next_idx = 0;
}

// edns_cookie_server

static hashtbl* serverHash      = NULL;
static int      server_next_idx = 0;

int edns_cookie_server_indexer(const dns_message* m)
{
    cookieobj* obj;
    if (m->malformed || !m->edns.cookie.server)
        return -1;
    char cookie[128];
    strtohex(cookie, (char*)m->edns.cookie.server, m->edns.cookie.server_len);
    cookie[m->edns.cookie.server_len * 2] = 0;
    if (NULL == serverHash) {
        serverHash = hash_create(MAX_ARRAY_SZ, cookie_hashfunc, cookie_cmpfunc, 1, afree, afree);
        if (NULL == serverHash)
            return -1;
    }
    if ((obj = hash_find(cookie, serverHash)))
        return obj->index;
    obj = acalloc(1, sizeof(*obj));
    if (NULL == obj)
        return -1;
    obj->cookie = astrdup(cookie);
    if (NULL == obj->cookie) {
        afree(obj);
        return -1;
    }
    obj->index = server_next_idx;
    if (0 != hash_add(obj->cookie, obj, serverHash)) {
        afree(obj->cookie);
        afree(obj);
        return -1;
    }
    server_next_idx++;
    return obj->index;
}

int edns_cookie_server_iterator(const char** label)
{
    cookieobj* obj;
    if (0 == server_next_idx)
        return -1;
    if (NULL == label) {
        /* initialize and tell caller how big the array is */
        hash_iter_init(serverHash);
        return server_next_idx;
    }
    if ((obj = hash_iterate(serverHash)) == NULL)
        return -1;
    *label = obj->cookie;
    return obj->index;
}

void edns_cookie_server_reset()
{
    serverHash      = NULL;
    server_next_idx = 0;
}
