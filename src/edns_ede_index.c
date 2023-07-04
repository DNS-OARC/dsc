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

#include "edns_ede_index.h"
#include "xmalloc.h"
#include "hashtbl.h"

#include <string.h>

// edns_ede

int edns_ede_indexer(const dns_message* m)
{
    if (m->malformed)
        return -1;
    return m->edns.option.ede;
}

static int next_iter;

int edns_ede_iterator(const char** label)
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

// edns_ede_code

static int code_largest = 0;

int edns_ede_code_indexer(const dns_message* m)
{
    if (m->malformed)
        return -1;
    // code + 1 because EDE can have code 0 and idx 0 is none
    if (!m->edns.option.ede)
        return 0;
    if (m->edns.ede.code + 1 > code_largest)
        code_largest = m->edns.ede.code + 1;
    return m->edns.ede.code + 1;
}

static int code_next_iter;

int edns_ede_code_iterator(const char** label)
{
    static char label_buf[20];
    if (NULL == label) {
        code_next_iter = 0;
        return code_largest + 1;
    }
    if (code_next_iter > code_largest)
        return -1;
    if (code_next_iter == 0)
        snprintf(label_buf, sizeof(label_buf), "none");
    else
        snprintf(label_buf, sizeof(label_buf), "%d", code_next_iter - 1);
    *label = label_buf;
    return code_next_iter++;
}

void edns_ede_code_reset()
{
    code_largest = 0;
}

// edns_ede_textlen

static int textlen_largest = 0;

int edns_ede_textlen_indexer(const dns_message* m)
{
    if (m->malformed)
        return -1;
    if (m->edns.ede.len > textlen_largest)
        textlen_largest = m->edns.ede.len;
    return m->edns.ede.len;
}

static int textlen_next_iter;

int edns_ede_textlen_iterator(const char** label)
{
    static char label_buf[20];
    if (NULL == label) {
        textlen_next_iter = 0;
        return textlen_largest + 1;
    }
    if (textlen_next_iter > textlen_largest)
        return -1;
    if (textlen_next_iter == 0)
        snprintf(label_buf, sizeof(label_buf), "none");
    else
        snprintf(label_buf, sizeof(label_buf), "%d", textlen_next_iter);
    *label = label_buf;
    return textlen_next_iter++;
}

void edns_ede_textlen_reset()
{
    textlen_largest = 0;
}

// edns_ede_text

#define MAX_ARRAY_SZ 65536

typedef struct
{
    char* text;
    int   index;
} textobj;

static unsigned int text_hashfunc(const void* key)
{
    return hashendian(key, strlen(key), 0);
}

static int text_cmpfunc(const void* a, const void* b)
{
    return strcasecmp(a, b);
}

static hashtbl* textHash      = NULL;
static int      text_next_idx = 0;

int edns_ede_text_indexer(const dns_message* m)
{
    textobj* obj;
    if (m->malformed || !m->edns.ede.text)
        return -1;
    char text[m->edns.ede.len + 1];
    memcpy(text, m->edns.ede.text, m->edns.ede.len);
    text[m->edns.ede.len] = 0;
    if (NULL == textHash) {
        textHash = hash_create(MAX_ARRAY_SZ, text_hashfunc, text_cmpfunc, 1, afree, afree);
        if (NULL == textHash)
            return -1;
    }
    if ((obj = hash_find(text, textHash)))
        return obj->index;
    obj = acalloc(1, sizeof(*obj));
    if (NULL == obj)
        return -1;
    obj->text = astrdup(text);
    if (NULL == obj->text) {
        afree(obj);
        return -1;
    }
    obj->index = text_next_idx;
    if (0 != hash_add(obj->text, obj, textHash)) {
        afree(obj->text);
        afree(obj);
        return -1;
    }
    text_next_idx++;
    return obj->index;
}

int edns_ede_text_iterator(const char** label)
{
    textobj* obj;
    if (0 == text_next_idx)
        return -1;
    if (NULL == label) {
        /* initialize and tell caller how big the array is */
        hash_iter_init(textHash);
        return text_next_idx;
    }
    if ((obj = hash_iterate(textHash)) == NULL)
        return -1;
    *label = obj->text;
    return obj->index;
}

void edns_ede_text_reset()
{
    textHash      = NULL;
    text_next_idx = 0;
}
