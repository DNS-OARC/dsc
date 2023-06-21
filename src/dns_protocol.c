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
    static int    loop_detect = 0;
    if (loop_detect > 5)
        return 4; /* compression loop */
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
            off_t          ptr;
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
            loop_detect++;
            rc = rfc1035NameUnpack(buf, sz, &ptr, name + no, ns - no);
            loop_detect--;
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

static off_t grok_additional_for_opt_rr(const u_char* buf, int len, off_t offset, dns_message* m, int arcount)
{
    int            i, x, have_edns = 0;
    unsigned short sometype;
    unsigned short someclass;
    unsigned short us;
    char           somename[MAX_QNAME_SZ];

    while (arcount > 0) {
        somename[0] = (char)0;
        x = rfc1035NameUnpack(buf, len, &offset, somename, MAX_QNAME_SZ);
        if (0 != x)
            return 0;
        if (offset + 10 > len)
            return 0;
        sometype  = nptohs(buf + offset);
        if ((sometype == T_OPT) && (somename[0] == 0) && (have_edns == 0)) {
            someclass = nptohs(buf + offset + 2);
            m->edns.found  = 1;
            m->edns.bufsiz = someclass;
            memcpy(&m->edns.version, buf + offset + 5, 1);
            us         = nptohs(buf + offset + 6);
            m->edns.DO = (us >> 15) & 0x01; /* RFC 3225 */
            have_edns = 1;
            us = nptohs(buf + offset + 8);
            offset += 10;
            if (offset + us > len)
                return 0;
            offset += us;
        } else {
            /* get rdlength */
            us = nptohs(buf + offset + 8);
            offset += 10;
            if (offset + us > len)
                return 0;
            offset += us;
        }
        arcount--;
        continue;
    }
    return (offset);
}

static off_t skip_rrs (const unsigned char *buf, int len, off_t offset, int rr_count) {
    unsigned short rdlen;
    char t_qname[MAX_QNAME_SZ] = { 0 };

    while(rr_count > 0) {
        int x;
        if (offset + 1 > len)   /* minimum name length */
            return 0;
        x = rfc1035NameUnpack(buf, len, &offset, t_qname, MAX_QNAME_SZ);
        if (offset + 10 > len)  /* minimum record length */
            return 0;
        rdlen = nptohs(buf + offset + 8);
        offset += 10 + rdlen;
        if (offset > len)
            return 0;
        rr_count--;
    }
    return(offset);
}

int dns_protocol_handler(const u_char* buf, int len, void* udata)
{
    transport_message* tm = udata;
    unsigned short     us;
    off_t              offset;
    int                qdcount;
    int                ancount;
    int                nscount;
    int                arcount;

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
        off_t new_offset;
        new_offset = grok_question(buf, len, offset, m.qname, &m.qtype, &m.qclass);
        if (0 == new_offset) {
            m.malformed = 1;
            return 0;
        }
        offset = new_offset;
        qdcount--;
    }
    assert(offset <= len);
    /*
     * Gobble up subsequent questions, if any
     */
    while (qdcount > 0 && offset < len) {
        off_t          new_offset;
        char           t_qname[MAX_QNAME_SZ] = { 0 };
        unsigned short t_qtype;
        unsigned short t_qclass;
        new_offset = grok_question(buf, len, offset, t_qname, &t_qtype, &t_qclass);
        if (0 == new_offset) {
            /*
             * point offset to the end of the buffer to avoid any subsequent processing
             */
            offset = len;
            break;
        }
        offset = new_offset;
        qdcount--;
    }
    assert(offset <= len);

    /* Gobble up AN RRs */
    if (ancount > 0) {
        off_t new_offset;
        new_offset = skip_rrs (buf, len, offset, ancount);
        if (0 == new_offset) {
            m.malformed = 1;
            return 0;
        }
        offset = new_offset;
    }
    assert(offset <= len);

    /* Gobble up NS RRs */
    if (nscount > 0) {
        off_t new_offset;
        new_offset = skip_rrs (buf, len, offset, nscount);
        if (0 == new_offset) {
            m.malformed = 1;
            return 0;
        }
        offset = new_offset;
    }
    assert(offset <= len);

    if (arcount > 0 && offset < len) {
        off_t new_offset;
        new_offset = grok_additional_for_opt_rr(buf, len, offset, &m, arcount);
        if (0 == new_offset) {
            offset = len;
        } else {
            offset = new_offset;
        }
    }
    assert(offset <= len);
    dns_message_handle(&m);
    return 0;
}
