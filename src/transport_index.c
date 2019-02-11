/*
 * Copyright (c) 2008-2019, OARC, Inc.
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

#include "transport_index.h"

/*
 * This is very similar to transport_index, but this
 * indexer is applied only for DNS messages, rather than
 * all IP packets.
 */

#include "md_array.h"

#define LARGEST 2

int transport_indexer(const dns_message* m)
{
    if (IPPROTO_UDP == m->tm->proto)
        return 0;
    if (IPPROTO_TCP == m->tm->proto)
        return 1;
    return LARGEST;
}

static int next_iter = 0;

int transport_iterator(const char** label)
{
    if (NULL == label) {
        next_iter = 0;
        return LARGEST + 1;
    }
    if (0 == next_iter)
        *label = "udp";
    else if (1 == next_iter)
        *label = "tcp";
    else if (LARGEST == next_iter)
        *label = "else";
    else
        return -1;
    return next_iter++;
}
