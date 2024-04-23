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

#include "inX_addr.h"
#include "hashtbl.h"

#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h> // For AF_ on BSDs

const char*
inXaddr_ntop(const inX_addr* a, char* buf, socklen_t len)
{
    const char* p;
    if (a->family == AF_INET6)
        p = inet_ntop(AF_INET6, &a->in6, buf, len);
    else
        p = inet_ntop(AF_INET, &a->in4, buf, len);
    if (p)
        return p;
    return "[unprintable]";
}

int inXaddr_pton(const char* buf, inX_addr* a)
{
    if (strchr(buf, ':')) {
        a->family = AF_INET6;
        return inet_pton(AF_INET6, buf, &a->in6);
    }
    a->family = AF_INET;
    return inet_pton(AF_INET, buf, &a->in4);
}

unsigned int
inXaddr_hash(const inX_addr* a)
{
    if (a->family == AF_INET6) {
        return hashword(a->in6.s6_addr32, 4, 0);
    }
    return hashword(&a->in4.s_addr, 1, 0);
}

int inXaddr_cmp(const inX_addr* a, const inX_addr* b)
{
    if (a->family == AF_INET6) {
        if (ntohl(a->in6.s6_addr32[3]) < ntohl(b->in6.s6_addr32[3]))
            return -1;
        if (ntohl(a->in6.s6_addr32[3]) > ntohl(b->in6.s6_addr32[3]))
            return 1;
        if (ntohl(a->in6.s6_addr32[2]) < ntohl(b->in6.s6_addr32[2]))
            return -1;
        if (ntohl(a->in6.s6_addr32[2]) > ntohl(b->in6.s6_addr32[2]))
            return 1;
        if (ntohl(a->in6.s6_addr32[1]) < ntohl(b->in6.s6_addr32[1]))
            return -1;
        if (ntohl(a->in6.s6_addr32[1]) > ntohl(b->in6.s6_addr32[1]))
            return 1;
        if (ntohl(a->in6.s6_addr32[0]) < ntohl(b->in6.s6_addr32[0]))
            return -1;
        if (ntohl(a->in6.s6_addr32[0]) > ntohl(b->in6.s6_addr32[0]))
            return 1;
        return 0;
    }
    if (ntohl(a->in4.s_addr) < ntohl(b->in4.s_addr))
        return -1;
    if (ntohl(a->in4.s_addr) > ntohl(b->in4.s_addr))
        return 1;
    return 0;
}

inX_addr
inXaddr_mask(const inX_addr* a, const inX_addr* mask)
{
    inX_addr masked;
    if (a->family == AF_INET6) {
        masked.family           = AF_INET6;
        masked.in6.s6_addr32[0] = a->in6.s6_addr32[0] & mask->in6.s6_addr32[0];
        masked.in6.s6_addr32[1] = a->in6.s6_addr32[1] & mask->in6.s6_addr32[1];
        masked.in6.s6_addr32[2] = a->in6.s6_addr32[2] & mask->in6.s6_addr32[2];
        masked.in6.s6_addr32[3] = a->in6.s6_addr32[3] & mask->in6.s6_addr32[3];
    } else {
        masked.family     = AF_INET;
        masked.in4.s_addr = a->in4.s_addr & mask->in4.s_addr;
    }
    return masked;
}

int inXaddr_version(const inX_addr* a)
{
    if (a->family == AF_INET6)
        return 6;
    return 4;
}

int inXaddr_assign_v4(inX_addr* dst, const struct in_addr* src)
{
    dst->family = AF_INET;
    dst->in4    = *src;
    return 0;
}

int inXaddr_assign_v6(inX_addr* dst, const struct in6_addr* src)
{
    dst->family = AF_INET6;
    dst->in6    = *src;
    return 0;
}
