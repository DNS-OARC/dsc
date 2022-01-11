/*
 * Copyright (c) 2008-2022, OARC, Inc.
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

#include "rcode_index.h"

#include <assert.h>

#define MAX_RCODE_IDX 16
static unsigned short idx_to_rcode[MAX_RCODE_IDX];
static int            next_idx = 0;

int rcode_indexer(const dns_message* m)
{
    int i;
    if (m->malformed)
        return -1;
    for (i = 0; i < next_idx; i++) {
        if (m->rcode == idx_to_rcode[i]) {
            return i;
        }
    }
    idx_to_rcode[next_idx] = m->rcode;
    assert(next_idx < MAX_RCODE_IDX);
    return next_idx++;
}

static int next_iter;

int rcode_iterator(const char** label)
{
    static char label_buf[32];
    if (0 == next_idx)
        return -1;
    if (NULL == label) {
        next_iter = 0;
        return next_idx;
    }
    if (next_iter == next_idx) {
        return -1;
    }
    snprintf(label_buf, sizeof(label_buf), "%d", idx_to_rcode[next_iter]);
    *label = label_buf;
    return next_iter++;
}

void rcode_reset()
{
    next_idx = 0;
}
