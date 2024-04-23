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

#include "edns_nsid_index.h"
#include "xmalloc.h"
#include "hashtbl.h"

#include <string.h>
#include <ctype.h>

// edns_nsid

int edns_nsid_indexer(const dns_message* m)
{
    if (m->malformed)
        return -1;
    return m->edns.option.nsid;
}

static int next_iter;

int edns_nsid_iterator(const char** label)
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

// edns_nsid_len

static int len_largest = 0;

int edns_nsid_len_indexer(const dns_message* m)
{
    if (m->malformed)
        return -1;
    if (m->edns.nsid.len > len_largest)
        len_largest = m->edns.nsid.len;
    return m->edns.nsid.len;
}

static int len_next_iter;

int edns_nsid_len_iterator(const char** label)
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

void edns_nsid_len_reset()
{
    len_largest = 0;
}

// edns_nsid_data

#define MAX_ARRAY_SZ 65536

typedef struct
{
    char* nsid;
    int   index;
} nsidobj;

static unsigned int nsid_hashfunc(const void* key)
{
    return hashendian(key, strlen(key), 0);
}

static int nsid_cmpfunc(const void* a, const void* b)
{
    return strcasecmp(a, b);
}

static hashtbl* dataHash      = NULL;
static int      data_next_idx = 0;

int edns_nsid_data_indexer(const dns_message* m)
{
    nsidobj* obj;
    if (m->malformed || !m->edns.nsid.data)
        return -1;
    char nsid[m->edns.nsid.len * 2 + 1];
    strtohex(nsid, (char*)m->edns.nsid.data, m->edns.nsid.len);
    nsid[m->edns.nsid.len * 2] = 0;
    if (NULL == dataHash) {
        dataHash = hash_create(MAX_ARRAY_SZ, nsid_hashfunc, nsid_cmpfunc, 1, afree, afree);
        if (NULL == dataHash)
            return -1;
    }
    if ((obj = hash_find(nsid, dataHash)))
        return obj->index;
    obj = acalloc(1, sizeof(*obj));
    if (NULL == obj)
        return -1;
    obj->nsid = astrdup(nsid);
    if (NULL == obj->nsid) {
        afree(obj);
        return -1;
    }
    obj->index = data_next_idx;
    if (0 != hash_add(obj->nsid, obj, dataHash)) {
        afree(obj->nsid);
        afree(obj);
        return -1;
    }
    data_next_idx++;
    return obj->index;
}

int edns_nsid_data_iterator(const char** label)
{
    nsidobj* obj;
    if (0 == data_next_idx)
        return -1;
    if (NULL == label) {
        /* initialize and tell caller how big the array is */
        hash_iter_init(dataHash);
        return data_next_idx;
    }
    if ((obj = hash_iterate(dataHash)) == NULL)
        return -1;
    *label = obj->nsid;
    return obj->index;
}

void edns_nsid_data_reset()
{
    dataHash      = NULL;
    data_next_idx = 0;
}

// edns_nsid_text

static hashtbl* textHash      = NULL;
static int      text_next_idx = 0;

int edns_nsid_text_indexer(const dns_message* m)
{
    nsidobj* obj;
    if (m->malformed || !m->edns.nsid.data)
        return -1;
    char   nsid[m->edns.nsid.len + 1];
    size_t i;
    for (i = 0; i < m->edns.nsid.len; i++) {
        if (isprint(m->edns.nsid.data[i])) {
            nsid[i] = m->edns.nsid.data[i];
        } else {
            nsid[i] = '.';
        }
    }
    nsid[i] = 0;
    if (NULL == textHash) {
        textHash = hash_create(MAX_ARRAY_SZ, nsid_hashfunc, nsid_cmpfunc, 1, afree, afree);
        if (NULL == textHash)
            return -1;
    }
    if ((obj = hash_find(nsid, textHash)))
        return obj->index;
    obj = acalloc(1, sizeof(*obj));
    if (NULL == obj)
        return -1;
    obj->nsid = astrdup(nsid);
    if (NULL == obj->nsid) {
        afree(obj);
        return -1;
    }
    obj->index = text_next_idx;
    if (0 != hash_add(obj->nsid, obj, textHash)) {
        afree(obj->nsid);
        afree(obj);
        return -1;
    }
    text_next_idx++;
    return obj->index;
}

int edns_nsid_text_iterator(const char** label)
{
    nsidobj* obj;
    if (0 == text_next_idx)
        return -1;
    if (NULL == label) {
        /* initialize and tell caller how big the array is */
        hash_iter_init(textHash);
        return text_next_idx;
    }
    if ((obj = hash_iterate(textHash)) == NULL)
        return -1;
    *label = obj->nsid;
    return obj->index;
}

void edns_nsid_text_reset()
{
    textHash      = NULL;
    text_next_idx = 0;
}
