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

#include "config.h"

#include "dns_protocol.h"
#include "dns_message.h"
#include "pcap_layers/byteorder.h"
#include "xmalloc.h"

#include <string.h>
#include <assert.h>
#include <ctype.h>

#define DNS_MSG_HDR_SZ 12
#define RFC1035_MAXLABELSZ 63

static int rfc1035NameUnpack(const u_char* buf, size_t sz, off_t* off, char* name, int ns)
{
    off_t         no = 0;
    unsigned char c;
    size_t        len;
    /*
     * loop_detect[] tracks which position in the DNS message it has
     * jumped to so it can't jump to the same twice, aka loop
     */
    static unsigned char loop_detect[0x3FFF] = { 0 };
    if (ns <= 0)
        return 4; /* probably compression loop */
    do {
        if ((*off) >= sz)
            break;
        c = *(buf + (*off));
        if (c > 191) {
            /* blasted compression */
            int            rc;
            unsigned short s;
            off_t          ptr, loop_ptr;
            s = nptohs(buf + (*off));
            (*off) += sizeof(s);
            /* Sanity check */
            if ((*off) >= sz)
                return 1; /* message too short */
            ptr = s & 0x3FFF;
            /* Make sure the pointer is inside this message */
            if (ptr >= sz)
                return 2; /* bad compression ptr */
            if (ptr < DNS_MSG_HDR_SZ)
                return 2; /* bad compression ptr */
            if (loop_detect[ptr])
                return 4; /* compression loop */
            loop_detect[(loop_ptr = ptr)] = 1;

            rc = rfc1035NameUnpack(buf, sz, &ptr, name + no, ns - no);

            loop_detect[loop_ptr] = 0;
            return rc;
        } else if (c > RFC1035_MAXLABELSZ) {
            /*
             * "(The 10 and 01 combinations are reserved for future use.)"
             */
            return 3; /* reserved label/compression flags */
        } else {
            (*off)++;
            len = (size_t)c;
            if (len == 0)
                break;
            if (len > (ns - 1))
                len = ns - 1;
            if ((*off) + len > sz)
                return 4; /* message is too short */
            if (no + len + 1 > ns)
                return 5; /* qname would overflow name buffer */
            memcpy(name + no, buf + (*off), len);
            (*off) += len;
            no += len;
            *(name + (no++)) = '.';
        }
    } while (c > 0);
    if (no > 0)
        *(name + no - 1) = '\0';
    /* make sure we didn't allow someone to overflow the name buffer */
    assert(no <= ns);
    return 0;
}

static int rfc1035NameSkip(const u_char* buf, size_t sz, off_t* off)
{
    unsigned char c;
    size_t        len;
    /*
     * loop_detect[] tracks which position in the DNS message it has
     * jumped to so it can't jump to the same twice, aka loop
     */
    static unsigned char loop_detect[0x3FFF] = { 0 };
    do {
        if ((*off) >= sz)
            break;
        c = *(buf + (*off));
        if (c > 191) {
            /* blasted compression */
            int            rc;
            unsigned short s;
            off_t          ptr, loop_ptr;
            s = nptohs(buf + (*off));
            (*off) += sizeof(s);
            /* Sanity check */
            if ((*off) >= sz)
                return 1; /* message too short */
            ptr = s & 0x3FFF;
            /* Make sure the pointer is inside this message */
            if (ptr >= sz)
                return 2; /* bad compression ptr */
            if (ptr < DNS_MSG_HDR_SZ)
                return 2; /* bad compression ptr */
            if (loop_detect[ptr])
                return 4; /* compression loop */
            loop_detect[(loop_ptr = ptr)] = 1;

            rc = rfc1035NameSkip(buf, sz, &ptr);

            loop_detect[loop_ptr] = 0;
            return rc;
        } else if (c > RFC1035_MAXLABELSZ) {
            /*
             * "(The 10 and 01 combinations are reserved for future use.)"
             */
            return 3; /* reserved label/compression flags */
        } else {
            (*off)++;
            len = (size_t)c;
            if (len == 0)
                break;
            if ((*off) + len > sz)
                return 4; /* message is too short */
            (*off) += len;
        }
    } while (c > 0);
    return 0;
}

static off_t grok_question(const u_char* buf, int len, off_t offset, char* qname, unsigned short* qtype, unsigned short* qclass)
{
    char* t;
    int   x;
    x = rfc1035NameUnpack(buf, len, &offset, qname, MAX_QNAME_SZ);
    if (0 != x)
        return 0;
    if ('\0' == *qname) {
        *qname       = '.';
        *(qname + 1) = 0;
    }
    /* XXX remove special characters from QNAME */
    while ((t = strchr(qname, '\n')))
        *t = ' ';
    while ((t = strchr(qname, '\r')))
        *t = ' ';
    for (t = qname; *t; t++)
        *t = tolower(*t);
    if (offset + 4 > len)
        return 0;
    *qtype  = nptohs(buf + offset);
    *qclass = nptohs(buf + offset + 2);
    offset += 4;
    return offset;
}

static off_t skip_question(const u_char* buf, int len, off_t offset)
{
    if (rfc1035NameSkip(buf, len, &offset))
        return 0;
    if (offset + 4 > len)
        return 0;
    offset += 4;
    return offset;
}

#define EDNS0_TYPE_NSID 3
#define EDNS0_TYPE_ECS 8
#define EDNS0_TYPE_COOKIE 10
#define EDNS0_TYPE_EXTENDED_ERROR 15

static void process_edns0_options(const u_char* buf, int len, struct dns_message* m)
{
    unsigned short edns0_type;
    unsigned short edns0_len;
    off_t          offset = 0;

    while (len >= 4) {
        edns0_type = nptohs(buf + offset);
        edns0_len  = nptohs(buf + offset + 2);
        if (len < 4 + edns0_len)
            break;
        switch (edns0_type) {
        case EDNS0_TYPE_COOKIE:
            if (m->edns.option.cookie)
                break;
            if (edns0_len == 8) {
                m->edns.option.cookie = 1;
                m->edns.cookie.client = buf + offset + 4;
            } else if (edns0_len >= 16 && edns0_len <= 40) {
                m->edns.option.cookie     = 1;
                m->edns.cookie.client     = buf + offset + 4;
                m->edns.cookie.server     = m->edns.cookie.client + 8;
                m->edns.cookie.server_len = edns0_len - 8;
            }
            break;
        case EDNS0_TYPE_NSID:
            if (m->edns.option.nsid)
                break;
            m->edns.option.nsid = 1;
            if (edns0_len) {
                m->edns.nsid.data = buf + offset + 4;
                m->edns.nsid.len  = edns0_len;
            }
            break;
        case EDNS0_TYPE_ECS:
            if (m->edns.option.ecs || edns0_len < 4)
                break;
            m->edns.option.ecs        = 1;
            m->edns.ecs.family        = nptohs(buf + offset + 4);
            m->edns.ecs.source_prefix = *(buf + offset + 6);
            m->edns.ecs.scope_prefix  = *(buf + offset + 7);
            if (edns0_len > 4) {
                m->edns.ecs.address = buf + offset + 8;
                m->edns.ecs.len     = edns0_len - 4;
            }
            break;
        case EDNS0_TYPE_EXTENDED_ERROR:
            if (m->edns.option.ede || edns0_len < 2)
                break;
            m->edns.option.ede = 1;
            m->edns.ede.code   = nptohs(buf + offset + 4);
            if (edns0_len > 2) {
                m->edns.ede.text = buf + offset + 6;
                m->edns.ede.len  = edns0_len - 2;
            }
            break;
        }
        offset += 4 + edns0_len;
        len -= 4 + edns0_len;
    }
}

int dns_protocol_parse_edns_options = 0;

static off_t grok_additional_for_opt_rr(const u_char* buf, int len, off_t offset, dns_message* m)
{
    unsigned short us;
    /*
     * OPT RR for EDNS0 MUST be 0 (root domain), so if the first byte of
     * the name is anything it can't be a valid EDNS0 record.
     */
    if (*(buf + offset)) {
        if (rfc1035NameSkip(buf, len, &offset))
            return 0;
        if (offset + 10 > len)
            return 0;
    } else {
        offset++;
        if (offset + 10 > len)
            return 0;
        if (nptohs(buf + offset) == T_OPT && !m->edns.found) {
            m->edns.found   = 1;
            m->edns.bufsiz  = nptohs(buf + offset + 2);
            m->edns.version = *(buf + offset + 5);
            us              = nptohs(buf + offset + 6);
            m->edns.DO      = (us >> 15) & 0x01; /* RFC 3225 */

            us = nptohs(buf + offset + 8); // rd len
            offset += 10;
            if (offset + us > len)
                return 0;
            if (dns_protocol_parse_edns_options && !m->edns.version && us > 0)
                process_edns0_options(buf + offset, us, m);
            offset += us;
            return offset;
        }
    }
    /* get rdlength */
    us = nptohs(buf + offset + 8);
    offset += 10;
    if (offset + us > len)
        return 0;
    offset += us;
    return offset;
}

static off_t skip_rr(const u_char* buf, int len, off_t offset)
{
    if (rfc1035NameSkip(buf, len, &offset))
        return 0;
    if (offset + 10 > len)
        return 0;
    unsigned short us = nptohs(buf + offset + 8);
    offset += 10;
    if (offset + us > len)
        return 0;
    offset += us;
    return offset;
}

int dns_protocol_parse_edns = 0;

int dns_protocol_handler(const u_char* buf, int len, void* udata)
{
    transport_message* tm = udata;
    unsigned short     us;
    off_t              offset, new_offset;
    int                qdcount, ancount, nscount, arcount;

    dns_message m;

    memset(&m, 0, sizeof(dns_message));
    m.tm     = tm;
    m.msglen = len;

    if (len < DNS_MSG_HDR_SZ) {
        m.malformed = 1;
        return 0;
    }
    m.id     = nptohs(buf);
    us       = nptohs(buf + 2);
    m.qr     = (us >> 15) & 0x01;
    m.opcode = (us >> 11) & 0x0F;
    m.aa     = (us >> 10) & 0x01;
    m.tc     = (us >> 9) & 0x01;
    m.rd     = (us >> 8) & 0x01;
    /* m.ra = (us >> 7) & 0x01; */
    /* m.z  = (us >> 6) & 0x01; */
    m.ad = (us >> 5) & 0x01;
    /* m.cd = (us >> 4) & 0x01; */

    m.rcode = us & 0x0F;

    qdcount = nptohs(buf + 4);
    ancount = nptohs(buf + 6);
    nscount = nptohs(buf + 8);
    arcount = nptohs(buf + 10);

    offset = DNS_MSG_HDR_SZ;

    /*
     * Grab the first question
     */
    if (qdcount > 0 && offset < len) {
        if (!(new_offset = grok_question(buf, len, offset, m.qname, &m.qtype, &m.qclass))) {
            m.malformed = 1;
            return 0;
        }
        offset = new_offset;
        qdcount--;
    }
    if (!dns_protocol_parse_edns)
        goto handle_m;
    assert(offset <= len);

    /*
     * Gobble up subsequent questions, if any
     */
    while (qdcount > 0 && offset < len) {
        if (!(new_offset = skip_question(buf, len, offset))) {
            goto handle_m;
        }
        offset = new_offset;
        qdcount--;
    }
    assert(offset <= len);

    /*
     * Gobble up answers, if any
     */
    while (ancount > 0 && offset < len) {
        if (!(new_offset = skip_rr(buf, len, offset))) {
            goto handle_m;
        }
        offset = new_offset;
        ancount--;
    }
    assert(offset <= len);

    /*
     * Gobble up authorities, if any
     */
    while (nscount > 0 && offset < len) {
        if (!(new_offset = skip_rr(buf, len, offset))) {
            goto handle_m;
        }
        offset = new_offset;
        nscount--;
    }
    assert(offset <= len);

    /*
     * Process additional
     */
    while (arcount > 0 && offset < len) {
        if (!(new_offset = grok_additional_for_opt_rr(buf, len, offset, &m))) {
            goto handle_m;
        }
        offset = new_offset;
        arcount--;
    }

handle_m:
    assert(offset <= len);
    dns_message_handle(&m);
    return 0;
}
