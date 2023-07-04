/*
 * Copyright (c) 2008-2023, OARC, Inc.
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

#ifndef __dsc_dns_message_h
#define __dsc_dns_message_h

typedef struct transport_message transport_message;
typedef struct dns_message       dns_message;

#include "inX_addr.h"
#include "dataset_opt.h"
#include "md_array.h"

#include <stdio.h>
#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#define MAX_QNAME_SZ 512

enum transport_encryption {
    TRANSPORT_ENCRYPTION_UNENCRYPTED = 0,
    TRANSPORT_ENCRYPTION_DOT         = 1,
    TRANSPORT_ENCRYPTION_DOH         = 2,
    TRANSPORT_ENCRYPTION_DNSCrypt    = 3,
    TRANSPORT_ENCRYPTION_DOQ         = 4,
};

struct transport_message {
    struct timeval            ts;
    inX_addr                  src_ip_addr;
    inX_addr                  dst_ip_addr;
    unsigned short            src_port;
    unsigned short            dst_port;
    unsigned char             ip_version;
    unsigned char             proto;
    enum transport_encryption encryption;
};

struct dns_message {
    transport_message* tm;
    unsigned short     id;
    unsigned short     qtype;
    unsigned short     qclass;
    unsigned short     msglen;
    char               qname[MAX_QNAME_SZ];
    const char*        tld;
    unsigned char      opcode;
    unsigned char      rcode;
    unsigned int       malformed : 1;
    unsigned int       qr : 1;
    unsigned int       rd : 1; /* set if RECUSION DESIRED bit is set */
    unsigned int       aa : 1; /* set if AUTHORITATIVE ANSWER bit is set */
    unsigned int       tc : 1; /* set if TRUNCATED RESPONSE bit is set */
    unsigned int       ad : 1; /* set if AUTHENTIC DATA bit is set */
    struct
    {
        unsigned int   found : 1; /* set if we found an OPT RR */
        unsigned int   DO : 1; /* set if DNSSEC DO bit is set */
        unsigned char  version; /* version field from OPT RR */
        unsigned short bufsiz; /* class field from OPT RR */

        // bitmap of found EDNS(0) options
        struct {
            unsigned int cookie : 1;
            unsigned int nsid : 1;
            unsigned int ede : 1;
            unsigned int ecs : 1;
        } option;

        // cookie rfc 7873
        struct {
            const u_char*  client; // pointer to 8 byte client part
            const u_char*  server; // pointer to server part, may be null
            unsigned short server_len; // length of server part, if any
        } cookie;

        // nsid rfc 5001
        struct {
            const u_char*  data; // pointer to nsid payload, may be null
            unsigned short len; // length of nsid, if any
        } nsid;

        // extended error codes rfc 8914
        struct {
            unsigned short code;
            const u_char*  text; // pointer to EXTRA-TEXT, may be null
            unsigned short len; // length of text, if any
        } ede;

        // client subnet rfc 7871
        struct {
            unsigned short family;
            unsigned char  source_prefix;
            unsigned char  scope_prefix;
            const u_char*  address; // pointer to address, may be null
            unsigned short len; // length of address, if any
        } ecs;
    } edns;
};

void        dns_message_handle(dns_message* m);
int         dns_message_add_array(const char* name, const char* fn, const char* fi, const char* sn, const char* si, const char* f, dataset_opt opts);
void        dns_message_flush_arrays(void);
void        dns_message_report(FILE* fp, md_array_printer* printer);
void        dns_message_clear_arrays(void);
const char* dns_message_QnameToNld(const char* qname, int nld);
const char* dns_message_tld(dns_message* m);
void        dns_message_filters_init(void);
void        dns_message_indexers_init(void);
int         add_qname_filter(const char* name, const char* pat);

void indexer_want_edns(void);
void indexer_want_edns_options(void);

#include <arpa/nameser.h>
#ifdef HAVE_ARPA_NAMESER_COMPAT_H
#include <arpa/nameser_compat.h>
#endif

/* DNS types that may be missing */

#ifndef T_AAAA
#define T_AAAA 28
#endif
#ifndef T_A6
#define T_A6 38
#endif
#ifndef T_OPT
#define T_OPT 41 /* OPT pseudo-RR, RFC2761 */
#endif

/* DNS classes that may be missing */

#ifndef C_CHAOS
#define C_CHAOS 3
#endif
#ifndef C_NONE
#define C_NONE 254
#endif

#endif /* __dsc_dns_message_h */
