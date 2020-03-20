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

#include "ip_direction_index.h"
#include "xmalloc.h"
#include "inX_addr.h"
#include "syslog_debug.h"

#include <stdlib.h>
#include <string.h>

#define LARGEST 2

struct _foo {
    inX_addr     addr;
    inX_addr     mask;
    struct _foo* next;
};

static struct _foo* local_addrs = NULL;

#ifndef DROP_RECV_RESPONSE
static
#endif
    int
    ip_is_local(const inX_addr* a)
{
    struct _foo* t;
    for (t = local_addrs; t; t = t->next) {
        inX_addr m = inXaddr_mask(a, &(t->mask));
        if (!inXaddr_cmp(&(t->addr), &m)) {
            return 1;
        }
    }
    return 0;
}

int ip_direction_indexer(const dns_message* m)
{
    const transport_message* tm = m->tm;
    if (ip_is_local(&tm->src_ip_addr))
        return 0;
    if (ip_is_local(&tm->dst_ip_addr))
        return 1;
    return LARGEST;
}

int ip_local_address(const char* presentation, const char* mask)
{
    struct _foo* n = xcalloc(1, sizeof(*n));
    if (NULL == n)
        return 0;
    if (inXaddr_pton(presentation, &n->addr) != 1) {
        dfprintf(0, "yucky IP address %s", presentation);
        xfree(n);
        return 0;
    }
    memset(&(n->mask), 255, sizeof(n->mask));
    if (mask) {
        if (!strchr(mask, '.') && !strchr(mask, ':')) {
            in_addr_t bit_mask = -1;
            int       bits     = atoi(mask);

            if (strchr(presentation, ':')) {
                if (bits < 0 || bits > 128) {
                    dfprintf(0, "yucky IP mask bits %s", mask);
                    xfree(n);
                    return 0;
                }

                if (bits > 96) {
                    bit_mask <<= 128 - bits;
                    n->mask._.in4.s_addr = htonl(bit_mask);
                } else {
                    n->mask._.in4.s_addr = 0;
                    if (bits > 64) {
                        bit_mask <<= 96 - bits;
                        n->mask._.pad2.s_addr = htonl(bit_mask);
                    } else {
                        n->mask._.pad2.s_addr = 0;
                        if (bits > 32) {
                            bit_mask <<= 64 - bits;
                            n->mask._.pad1.s_addr = htonl(bit_mask);
                        } else {
                            n->mask._.pad1.s_addr = 0;
                            if (bits) {
                                bit_mask <<= 32 - bits;
                                n->mask._.pad0.s_addr = htonl(bit_mask);
                            } else {
                                n->mask._.pad0.s_addr = 0;
                            }
                        }
                    }
                }
            } else {
                if (bits < 0 || bits > 32) {
                    dfprintf(0, "yucky IP mask bits %s", mask);
                    xfree(n);
                    return 0;
                }

                if (bits) {
                    bit_mask <<= 32 - bits;
                    n->mask._.in4.s_addr = htonl(bit_mask);
                } else {
                    n->mask._.in4.s_addr = 0;
                }
            }
        } else if (inXaddr_pton(mask, &n->mask) != 1) {
            dfprintf(0, "yucky IP mask %s", mask);
            xfree(n);
            return 0;
        }
    }
    n->next     = local_addrs;
    local_addrs = n;
    return 1;
}

int ip_direction_iterator(const char** label)
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
