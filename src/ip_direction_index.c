/*
 * Copyright (c) 2016, OARC, Inc.
 * Copyright (c) 2007, The Measurement Factory, Inc.
 * Copyright (c) 2007, Internet Systems Consortium, Inc.
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netinet/in.h>
#if USE_IPV6
#include <netinet/ip6.h>
#endif

#include "xmalloc.h"
#include "inX_addr.h"
#include "dns_message.h"
#include "md_array.h"
#include "syslog_debug.h"

#define LARGEST 2

struct _foo
{
    inX_addr addr;
    struct _foo *next;
};

static struct _foo *local_addrs = NULL;

#ifndef DROP_RECV_RESPONSE
static
#endif
    int
ip_is_local(const inX_addr * a)
{
    struct _foo *t;
    for (t = local_addrs; t; t = t->next)
        if (0 == inXaddr_cmp(&t->addr, a))
            return 1;
    return 0;
}

int
ip_direction_indexer(const void *vp)
{
    const dns_message *m = vp;
    const transport_message *tm = m->tm;
    if (ip_is_local(&tm->src_ip_addr))
        return 0;
    if (ip_is_local(&tm->dst_ip_addr))
        return 1;
    return LARGEST;
}

int
ip_local_address(const char *presentation)
{
    struct _foo *n = xcalloc(1, sizeof(*n));
    if (NULL == n)
        return 0;
    if (inXaddr_pton(presentation, &n->addr) != 1) {
        dfprintf(0, "yucky IP address %s", presentation);
        xfree(n);
        return 0;
    }
    n->next = local_addrs;
    local_addrs = n;
    return 1;
}

int
ip_direction_iterator(char **label)
{
    static int next_iter = 0;
    if (NULL == label) {
        next_iter = 0;
        return LARGEST + 1;
    }
    if (0 == next_iter)
        *label = "sent";
    else if (1 == next_iter)
        *label = "recv";
    else if (LARGEST == next_iter)
        *label = "else";
    else
        return -1;
    return next_iter++;
}
