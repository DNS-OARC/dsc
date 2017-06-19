/*
 * Copyright (c) 2016-2017, OARC, Inc.
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "inX_addr.h"
#include "dataset_opt.h"
#include "md_array.h"

#ifndef __dsc_dns_message_h
#define __dsc_dns_message_h

#define MAX_QNAME_SZ 512

typedef struct
{
    struct timeval ts;
    inX_addr src_ip_addr;
    inX_addr dst_ip_addr;
    unsigned short src_port;
    unsigned short dst_port;
    unsigned char ip_version;
    unsigned char proto;
} transport_message;

typedef struct _dns_message dns_message;
struct _dns_message
{
    transport_message *tm;
    inX_addr client_ip_addr;
    inX_addr server_ip_addr;
    unsigned short qtype;
    unsigned short qclass;
    unsigned short msglen;
    char qname[MAX_QNAME_SZ];
    const char *tld;
    unsigned char opcode;
    unsigned char rcode;
    unsigned int malformed:1;
    unsigned int qr:1;
    unsigned int rd:1;                /* set if RECUSION DESIRED bit is set */
    unsigned int aa:1;                /* set if AUTHORITATIVE ANSWER bit is set */
    unsigned int tc:1;                /* set if TRUNCATED RESPONSE bit is set */
    unsigned int ad:1;
    struct
    {
        unsigned int found:1;        /* set if we found an OPT RR */
        unsigned int DO:1;        /* set if DNSSEC DO bit is set */
        unsigned char version;        /* version field from OPT RR */
        unsigned short bufsiz;        /* class field from OPT RR */
    } edns;
    /* ... */
};

void dns_message_report(FILE *, md_array_printer *);
int dns_message_add_array(const char *, const char *, const char *, const char *, const char *, const char *,
    dataset_opt);
const char *dns_message_QnameToNld(const char *, int);
const char *dns_message_tld(dns_message * m);
void dns_message_init(void);
void dns_message_clear_arrays(void);
void dns_message_handle(dns_message *);

int add_qname_filter(const char *name, const char *re);

#ifndef T_OPT
#define T_OPT 41                /* OPT pseudo-RR, RFC2761 */
#endif

#ifndef T_AAAA
#define T_AAAA 28
#endif

#ifndef C_CHAOS
#define C_CHAOS 3
#endif

#endif /* __dsc_dns_message_h */
