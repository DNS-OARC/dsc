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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "inX_addr.h"

static unsigned char v4_in_v6_prefix[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF };

static int
is_v4_in_v6(const struct in6_addr *addr)
{
    return (0 == memcmp(addr, v4_in_v6_prefix, 12));
}


const char *
inXaddr_ntop(const inX_addr * a, char *buf, socklen_t len)
{
    const char *p;
    if (!is_v4_in_v6(&a->in6))
        p = inet_ntop(AF_INET6, &a->in6, buf, len);
    else
        p = inet_ntop(AF_INET, &a->_.in4, buf, len);
    if (p)
        return p;
    return "[unprintable]";
}

int
inXaddr_pton(const char *buf, inX_addr * a)
{
    if (strchr(buf, ':'))
        return inet_pton(AF_INET6, buf, a);
    memcpy(a, v4_in_v6_prefix, 12);
    return inet_pton(AF_INET, buf, &a->_.in4);
}

unsigned int
inXaddr_hash(const inX_addr * a)
{
    /* just ignore the upper bits for v6? */
    return ntohl(a->_.in4.s_addr);
}

int
inXaddr_cmp(const inX_addr * a, const inX_addr * b)
{
    if (ntohl(a->_.in4.s_addr) < ntohl(b->_.in4.s_addr))
        return -1;
    if (ntohl(a->_.in4.s_addr) > ntohl(b->_.in4.s_addr))
        return 1;
    if (is_v4_in_v6(&a->in6))
        return 0;
    if (ntohl(a->_.pad2.s_addr) < ntohl(b->_.pad2.s_addr))
        return -1;
    if (ntohl(a->_.pad2.s_addr) > ntohl(b->_.pad2.s_addr))
        return 1;
    if (ntohl(a->_.pad1.s_addr) < ntohl(b->_.pad1.s_addr))
        return -1;
    if (ntohl(a->_.pad1.s_addr) > ntohl(b->_.pad1.s_addr))
        return 1;
    if (ntohl(a->_.pad0.s_addr) < ntohl(b->_.pad0.s_addr))
        return -1;
    if (ntohl(a->_.pad0.s_addr) > ntohl(b->_.pad0.s_addr))
        return 1;
    return 0;
}

inX_addr
inXaddr_mask(const inX_addr * a, const inX_addr * mask)
{
    inX_addr masked;
    masked._.in4.s_addr = a->_.in4.s_addr & mask->_.in4.s_addr;
    if (is_v4_in_v6(&a->in6)) {
        masked._.pad2.s_addr = a->_.pad2.s_addr;
        masked._.pad1.s_addr = a->_.pad1.s_addr;
        masked._.pad0.s_addr = a->_.pad0.s_addr;
    } else {
        masked._.pad2.s_addr = a->_.pad2.s_addr & mask->_.pad2.s_addr;
        masked._.pad1.s_addr = a->_.pad1.s_addr & mask->_.pad1.s_addr;
        masked._.pad0.s_addr = a->_.pad0.s_addr & mask->_.pad0.s_addr;
    }
    return masked;
}

int
inXaddr_version(const inX_addr * a)
{
    if (!is_v4_in_v6(&a->in6))
        return 6;
    return 4;
}

int
inXaddr_assign_v4(inX_addr * dst, const struct in_addr *src)
{
    memcpy(dst, v4_in_v6_prefix, 12);
    /* memcpy() instead of struct assignment in case src is not aligned */
    memcpy(&dst->_.in4, src, sizeof(*src));
    return 0;
}

int
inXaddr_assign_v6(inX_addr * dst, const struct in6_addr *src)
{
    /* memcpy() instead of struct assignment in case src is not aligned */
    memcpy(&dst->in6, src, sizeof(*src));
    return 0;
}
