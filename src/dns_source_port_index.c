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

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <netdb.h>
#include <memory.h>

#include "dns_message.h"
#include "md_array.h"

/* dns_source_port_indexer */
/* Indexes the source port of DNS messages */

static unsigned int   f_index[65536];
static unsigned short r_index[65536];
static unsigned int   largest = 0;

int dns_source_port_indexer(const void* vp)
{
    const dns_message* m = vp;
    unsigned short     p = m->tm->src_port;
    if (0 == f_index[p]) {
        f_index[p]       = ++largest;
        r_index[largest] = p;
    }
    return f_index[p];
}

static int next_iter = 0;

int dns_source_port_iterator(char** label)
{
    static char label_buf[20];
    if (NULL == label) {
        next_iter = 0;
        return largest + 1;
    }
    if (next_iter > largest)
        return -1;
    snprintf(*label = label_buf, 20, "%hu", r_index[next_iter++]);
    return next_iter;
}

void dns_source_port_reset(void)
{
    memset(f_index, 0, sizeof f_index);
    memset(r_index, 0, sizeof r_index);
    largest = 0;
}

/* dns_sport_range_indexer */
/* Indexes the "range" of a TCP/UDP source port of DNS messages */
/* "Range" is defined as port/1024. */

static int range_largest   = 0;
static int range_next_iter = 0;

int dns_sport_range_indexer(const void* vp)
{
    const dns_message* m = vp;
    int                r = (int)m->tm->src_port >> 10;
    if (r > range_largest)
        range_largest = r;
    return r;
}

int dns_sport_range_iterator(char** label)
{
    static char label_buf[20];
    if (NULL == label) {
        range_next_iter = 0;
        return range_largest + 1;
    }
    if (range_next_iter > range_largest)
        return -1;
    snprintf(*label = label_buf, 20, "%d-%d", (range_next_iter << 10), ((range_next_iter + 1) << 10) - 1);
    return ++range_next_iter;
}

void dns_sport_range_reset(void)
{
    range_largest = 0;
}
