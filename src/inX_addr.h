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

#ifndef __dsc_inX_addr_h
#define __dsc_inX_addr_h

#include <netinet/in.h>
#ifndef s6_addr32
#define s6_addr32 __u6_addr.__u6_addr32
#endif

typedef struct {
    int             family;
    struct in6_addr in6;
    struct in_addr  in4;
} inX_addr;

extern int          inXaddr_version(const inX_addr*);
extern const char*  inXaddr_ntop(const inX_addr*, char*, socklen_t len);
extern int          inXaddr_pton(const char*, inX_addr*);
extern unsigned int inXaddr_hash(const inX_addr*);
extern int          inXaddr_cmp(const inX_addr* a, const inX_addr* b);
extern inX_addr     inXaddr_mask(const inX_addr* a, const inX_addr* mask);

extern int inXaddr_assign_v4(inX_addr*, const struct in_addr*);
extern int inXaddr_assign_v6(inX_addr*, const struct in6_addr*);

#endif /* __dsc_inX_addr_h */
