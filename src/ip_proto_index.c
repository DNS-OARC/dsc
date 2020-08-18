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

#include "ip_proto_index.h"

#include <netdb.h>
#include <string.h>

static int largest = 0;

int ip_proto_indexer(const dns_message* m)
{
    const transport_message* tm = m->tm;
    int                      i  = (int)tm->proto;
    if (i > largest)
        largest = i;
    return i;
}

static int next_iter = 0;

int ip_proto_iterator(const char** label)
{
    static char label_buf[20];
#if __OpenBSD__
    struct protoent_data pdata;
#else
    char buf[1024];
#endif
    struct protoent  proto;
    struct protoent* p = 0;
    if (NULL == label) {
        next_iter = 0;
        return largest + 1;
    }
    if (next_iter > largest)
        return -1;
#if __OpenBSD__
    memset(&pdata, 0, sizeof(struct protoent_data));
    getprotobynumber_r(next_iter, &proto, &pdata);
    p = &proto;
#else
    getprotobynumber_r(next_iter, &proto, buf, sizeof(buf), &p);
#endif
    if (p)
        *label = p->p_name;
    else {
        snprintf(label_buf, sizeof(label_buf), "p%d", next_iter);
        *label = label_buf;
    }
    return next_iter++;
}

void ip_proto_reset()
{
    largest = 0;
}
