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
#include <sys/time.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <netinet/ip6.h>

#include <pcap.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <assert.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>

#include <sys/socket.h>
#include <net/if_arp.h>
#include <net/if.h>
#include <netinet/if_ether.h>

#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>

#include <syslog.h>
#include <stdarg.h>

#include "xmalloc.h"
#include "dns_message.h"
#include "pcap.h"
#include "byteorder.h"
#include "syslog_debug.h"
#include "hashtbl.h"
#include "pcap_layers.h"

#define PCAP_SNAPLEN 65536
#ifndef ETHER_HDR_LEN
#define ETHER_ADDR_LEN 6
#define ETHER_TYPE_LEN 2
#define ETHER_HDR_LEN (ETHER_ADDR_LEN * 2 + ETHER_TYPE_LEN)
#endif
#ifndef ETHERTYPE_8021Q
#define ETHERTYPE_8021Q        0x8100
#endif

#ifdef __OpenBSD__
#define assign_timeval(A,B) A.tv_sec=B.tv_sec; A.tv_usec=B.tv_usec
#else
#define assign_timeval(A,B) A=B
#endif


/* We might need to define ETHERTYPE_IPV6 */
#ifndef ETHERTYPE_IPV6
#define ETHERTYPE_IPV6 0x86dd
#endif

#ifdef __GLIBC__
#define uh_dport dest
#define uh_sport source
#define th_off doff
#define th_dport dest
#define th_sport source
#define th_seq seq
#define TCPFLAGFIN(a) (a)->fin
#define TCPFLAGSYN(a) (a)->syn
#define TCPFLAGRST(a) (a)->rst
#else
#define TCPFLAGSYN(a) ((a)->th_flags&TH_SYN)
#define TCPFLAGFIN(a) ((a)->th_flags&TH_FIN)
#define TCPFLAGRST(a) ((a)->th_flags&TH_RST)
#endif

#ifndef IP_OFFMASK
#define IP_OFFMASK 0x1fff
#endif

struct _interface
{
    char *device;
    pcap_t *pcap;
    int fd;
    struct pcap_stat ps0, ps1;
    unsigned int pkts_captured;
};

#define MAX_N_INTERFACES 10
static int n_interfaces = 0;
static struct _interface *interfaces = NULL;
static fd_set pcap_fdset;
static int max_pcap_fds = 0;
static unsigned short port53;

int n_pcap_offline = 0;                /* global so daemon.c can use it */
char *bpf_program_str = NULL;
int vlan_tag_needs_byte_conversion = 1;

extern int dns_protocol_handler(const u_char * buf, int len, void *udata);
#if 0
static int debug_count = 20;
#endif
static struct timeval last_ts;
static struct timeval start_ts;
static struct timeval finish_ts;
#define MAX_VLAN_IDS 100
static int n_vlan_ids = 0;
static int vlan_ids[MAX_VLAN_IDS];
static hashtbl *tcpHash;

static int
pcap_udp_handler(const struct udphdr *udp, int len, void *udata)
{
    transport_message *tm = udata;
    tm->src_port = nptohs(&udp->uh_sport);
    tm->dst_port = nptohs(&udp->uh_dport);
    tm->proto = IPPROTO_UDP;
    if (port53 != tm->dst_port && port53 != tm->src_port)
        return 1;
    return 0;
}

#define MAX_DNS_LENGTH 0xFFFF

#define MAX_TCP_WINDOW_SIZE (0xFFFF << 14)
#define MAX_TCP_STATE 65535
#define MAX_TCP_IDLE 60                /* tcpstate is tossed if idle for this many seconds */

/* These numbers define the sizes of small arrays which are simpler to work
 * with than dynamically allocated lists. */
#define MAX_TCP_MSGS 8                /* messages being reassembled (per connection) */
#define MAX_TCP_SEGS 8                /* segments not assigned to a message (per connection) */
#define MAX_TCP_HOLES 8                /* holes in a msg buf (per message) */

typedef struct
{
    inX_addr src_ip_addr;
    inX_addr dst_ip_addr;
    uint16_t dport;
    uint16_t sport;
} tcpHashkey_t;

/* Description of hole in tcp reassembly buffer. */
typedef struct
{
    uint16_t start;                /* start of hole, measured from beginning of msgbuf->buf */
    uint16_t len;                /* length of hole (0 == unused) */
} tcphole_t;

/* TCP message reassembly buffer */
typedef struct
{
    uint32_t seq;                /* seq# of first byte of header of this DNS msg */
    uint16_t dnslen;                /* length of dns message, and size of buf */
    tcphole_t hole[MAX_TCP_HOLES];
    int holes;                        /* number of holes remaining in message */
    u_char buf[];                /* reassembled message (C99 flexible array member) */
} tcp_msgbuf_t;

/* held TCP segment */
typedef struct
{
    uint32_t seq;                /* sequence number of first byte of segment */
    uint16_t len;                /* length of segment, and size of buf */
    u_char buf[];                /* segment payload (C99 flexible array member) */
} tcp_segbuf_t;

/* TCP reassembly state */
typedef struct tcpstate
{
    tcpHashkey_t key;
    struct tcpstate *newer, *older;
    long last_use;
    uint32_t seq_start;                /* seq# of length field of next DNS msg */
    short msgbufs;                /* number of msgbufs in use */
    u_char dnslen_buf[2];        /* full dnslen field might not arrive in first segment */
    u_char dnslen_bytes_seen_mask;        /* bitmask, when == 3 we have full dnslen */
    int8_t fin;                        /* have we seen a FIN? */
    tcp_msgbuf_t *msgbuf[MAX_TCP_MSGS];
    tcp_segbuf_t *segbuf[MAX_TCP_SEGS];
} tcpstate_t;

/* List of tcpstates ordered by time of last use, so we can quickly identify
 * and discard stale entries. */
struct
{
    tcpstate_t *oldest;
    tcpstate_t *newest;
} tcpList;

static void
tcpstate_reset(tcpstate_t * tcpstate, uint32_t seq)
{
    int i;
    tcpstate->seq_start = seq;
    tcpstate->fin = 0;
    if (tcpstate->msgbufs > 0) {
        tcpstate->msgbufs = 0;
        for (i = 0; i < MAX_TCP_MSGS; i++) {
            if (tcpstate->msgbuf[i]) {
                xfree(tcpstate->msgbuf[i]);
                tcpstate->msgbuf[i] = NULL;
            }
        }
    }
    for (i = 0; i < MAX_TCP_SEGS; i++) {
        if (tcpstate->segbuf[i]) {
            xfree(tcpstate->segbuf[i]);
            tcpstate->segbuf[i] = NULL;
        }
    }
}

static void
tcpstate_free(void *p)
{
    tcpstate_reset((tcpstate_t *) p, 0);
    xfree(p);
}

static unsigned int
tcp_hashfunc(const void *key)
{
    tcpHashkey_t *k = (tcpHashkey_t *) key;
    return (k->dport << 16) | k->sport | k->src_ip_addr._.in4.s_addr | k->dst_ip_addr._.in4.s_addr;
    /* 32 low bits of ipv6 address are good enough for a hash */
}

static int
tcp_cmpfunc(const void *a, const void *b)
{
    return memcmp(a, b, sizeof(tcpHashkey_t));
}

/* TCP Reassembly.
 *
 * When we see a SYN, we allocate a new tcpstate for the connection, and
 * establish the initial sequence number of the first dns message (seq_start)
 * on the connection.  We assume that no other segment can arrive before the
 * SYN (if one does, it is discarded, and if is not repeated the message it
 * belongs to can never be completely reassembled).
 *
 * Then, for each segment that arrives on the connection:
 * - If it's the first segment of a message (containing the 2-byte message
 *   length), we allocate a msgbuf, and check for any held segments that might
 *   belong to it.
 * - If the first byte of the segment belongs to any msgbuf, we fill
 *   in the holes of that message.  If the message has no more holes, we
 *   handle the complete dns message.  If the tail of the segment was longer
 *   than the hole, we recurse on the tail.
 * - Otherwise, if the segment could be within the tcp window, we hold onto it
 *   pending the creation of a matching msgbuf.
 *
 * This algorithm handles segments that arrive out of order, duplicated or
 * overlapping (including segments from different dns messages arriving out of
 * order), and dns messages that do not necessarily start on segment
 * boundaries.
 *
 */
static void
pcap_handle_tcp_segment(u_char * segment, int len, uint32_t seq, tcpstate_t * tcpstate, transport_message * tm)
{
    int i, m, s;
    uint16_t dnslen;
    int segoff, seglen;

    dfprintf(1, "pcap_handle_tcp_segment: seq=%u, len=%d", seq, len);

    if (len <= 0)                /* there is no more payload */
        return;

    if (seq - tcpstate->seq_start < 2) {
        /* this segment contains all or part of the 2-byte DNS length field */
        uint32_t o = seq - tcpstate->seq_start;
        int l = (len > 1 && o == 0) ? 2 : 1;
        dfprintf(1, "pcap_handle_tcp_segment: copying %d bytes to dnslen_buf[%d]", l, o);
        memcpy(&tcpstate->dnslen_buf[o], segment, l);
        if (l == 2)
            tcpstate->dnslen_bytes_seen_mask = 3;
        else
            tcpstate->dnslen_bytes_seen_mask |= (1 << o);
        len -= l;
        segment += l;
        seq += l;
    }

    if (3 == tcpstate->dnslen_bytes_seen_mask) {
        /* We have the dnslen stored now */
        dnslen = nptohs(tcpstate->dnslen_buf);
        /*
         * Next we poison the mask to indicate we are in to the message body.
         * If one doesn't remember we're past the then,
         * one loops forever getting more msgbufs rather than filling
         * in the contents of THIS message.
         *
         * We need to later reset that mask when we process the message
         * (method: tcpstate->dnslen_bytes_seen_mask = 0).
         */
        tcpstate->dnslen_bytes_seen_mask = 7;
        tcpstate->seq_start += sizeof(uint16_t) + dnslen;
        dfprintf(1, "pcap_handle_tcp_segment: first segment; dnslen = %d", dnslen);
        if (len >= dnslen) {
            /* this segment contains a complete message - avoid the reassembly
             * buffer and just handle the message immediately */
            dns_protocol_handler(segment, dnslen, tm);
            tcpstate->dnslen_bytes_seen_mask = 0;        /* go back for another message in this tcp connection */
            /* handle the trailing part of the segment? */
            if (len > dnslen) {
                dfprintf(1, "pcap_handle_tcp_segment: %s", "segment tail");
                pcap_handle_tcp_segment(segment + dnslen, len - dnslen, seq + dnslen, tcpstate, tm);
            }
            return;
        }
        /*
         * At this point we KNOW we have an incomplete message and need to do reassembly.
         * i.e.:  assert(len < dnslen);
         */
        dfprintf(2, "pcap_handle_tcp_segment: %s", "buffering segment");
        /* allocate a msgbuf for reassembly */
        for (m = 0; tcpstate->msgbuf[m];) {
            if (++m >= MAX_TCP_MSGS) {
                dfprintf(1, "pcap_handle_tcp_segment: %s", "out of msgbufs");
                return;
            }
        }
        tcpstate->msgbuf[m] = xcalloc(1, sizeof(tcp_msgbuf_t) + dnslen);
        if (NULL == tcpstate->msgbuf[m]) {
            dsyslogf(LOG_ERR, "out of memory for tcp_msgbuf (%d)", dnslen);
            return;
        }
        tcpstate->msgbufs++;
        tcpstate->msgbuf[m]->seq = seq;
        tcpstate->msgbuf[m]->dnslen = dnslen;
        tcpstate->msgbuf[m]->holes = 1;
        tcpstate->msgbuf[m]->hole[0].start = len;
        tcpstate->msgbuf[m]->hole[0].len = dnslen - len;
        dfprintf(1,
            "pcap_handle_tcp_segment: new msgbuf %d: seq = %u, dnslen = %d, hole start = %d, hole len = %d", m,
            tcpstate->msgbuf[m]->seq, tcpstate->msgbuf[m]->dnslen, tcpstate->msgbuf[m]->hole[0].start,
            tcpstate->msgbuf[m]->hole[0].len);
        /* copy segment to appropriate location in reassembly buffer */
        memcpy(tcpstate->msgbuf[m]->buf, segment, len);

        /* Now that we know the length of this message, we must check any held
         * segments to see if they belong to it. */
        for (s = 0; s < MAX_TCP_SEGS; s++) {
            if (!tcpstate->segbuf[s])
                continue;
            if (tcpstate->segbuf[s]->seq - seq >= 0 && tcpstate->segbuf[s]->seq - seq < dnslen) {
                tcp_segbuf_t *segbuf = tcpstate->segbuf[s];
                tcpstate->segbuf[s] = NULL;
                dfprintf(1, "pcap_handle_tcp_segment: %s", "message reassembled");
                pcap_handle_tcp_segment(segbuf->buf, segbuf->len, segbuf->seq, tcpstate, tm);
                /*
                 * Note that our recursion will also cover any tail messages (I hope).
                 * Thus we do not need to do so here and can return.
                 */
                xfree(segbuf);
            }
        }
        return;
    }

    /*
     * Welcome to reassembly-land.
     */
    /* find the message to which the first byte of this segment belongs */
    for (m = 0;; m++) {
        if (m >= MAX_TCP_MSGS) {
            /* seg does not match any msgbuf; just hold on to it. */
            dfprintf(1, "pcap_handle_tcp_segment: %s", "seg does not match any msgbuf");

            if (seq - tcpstate->seq_start > MAX_TCP_WINDOW_SIZE) {
                dfprintf(1, "pcap_handle_tcp_segment: %s", "seg is outside window; discarding");
                return;
            }
            for (s = 0;; s++) {
                if (s >= MAX_TCP_SEGS) {
                    dfprintf(1, "pcap_handle_tcp_segment: %s", "out of segbufs");
                    return;
                }
                if (tcpstate->segbuf[s])
                    continue;
                tcpstate->segbuf[s] = xcalloc(1, sizeof(tcp_segbuf_t) + len);
                tcpstate->segbuf[s]->seq = seq;
                tcpstate->segbuf[s]->len = len;
                memcpy(tcpstate->segbuf[s]->buf, segment, len);
                dfprintf(1, "pcap_handle_tcp_segment: new segbuf %d: seq = %u, len = %d",
                    s, tcpstate->segbuf[s]->seq, tcpstate->segbuf[s]->len);
                return;
            }
        }
        if (!tcpstate->msgbuf[m])
            continue;
        segoff = seq - tcpstate->msgbuf[m]->seq;
        if (segoff >= 0 && segoff < tcpstate->msgbuf[m]->dnslen) {
            /* segment starts in this msgbuf */
            dfprintf(1, "pcap_handle_tcp_segment: seg matches msg %d: seq = %u, dnslen = %d",
                m, tcpstate->msgbuf[m]->seq, tcpstate->msgbuf[m]->dnslen);
            if (segoff + len > tcpstate->msgbuf[m]->dnslen) {
                /* segment would overflow msgbuf */
                seglen = tcpstate->msgbuf[m]->dnslen - segoff;
                dfprintf(1, "pcap_handle_tcp_segment: using partial segment %d", seglen);
            } else {
                seglen = len;
            }
            break;
        }
    }

    /* Reassembly algorithm adapted from RFC 815. */
    for (i = 0; i < MAX_TCP_HOLES; i++) {
        tcphole_t *newhole;
        uint16_t hole_start, hole_len;
        if (tcpstate->msgbuf[m]->hole[i].len == 0)
            continue;                /* hole descriptor is not in use */
        hole_start = tcpstate->msgbuf[m]->hole[i].start;
        hole_len = tcpstate->msgbuf[m]->hole[i].len;
        if (segoff >= hole_start + hole_len)
            continue;                /* segment is totally after hole */
        if (segoff + seglen <= hole_start)
            continue;                /* segment is totally before hole */
        /* The segment overlaps this hole.  Delete the hole. */
        dfprintf(1, "pcap_handle_tcp_segment: overlaping hole %d: %d %d", i, hole_start, hole_len);
        tcpstate->msgbuf[m]->hole[i].len = 0;
        tcpstate->msgbuf[m]->holes--;
        if (segoff + seglen < hole_start + hole_len) {
            /* create a new hole after the segment (common case) */
            newhole = &tcpstate->msgbuf[m]->hole[i];        /* hole[i] is guaranteed free */
            newhole->start = segoff + seglen;
            newhole->len = (hole_start + hole_len) - newhole->start;
            tcpstate->msgbuf[m]->holes++;
            dfprintf(1, "pcap_handle_tcp_segment: new post-hole %d: %d %d", i, newhole->start, newhole->len);
        }
        if (segoff > hole_start) {
            /* create a new hole before the segment */
            int j;
            for (j = 0;; j++) {
                if (j == MAX_TCP_HOLES) {
                    dfprintf(1, "pcap_handle_tcp_segment: %s", "out of hole descriptors");
                    return;
                }
                if (tcpstate->msgbuf[m]->hole[j].len == 0) {
                    newhole = &tcpstate->msgbuf[m]->hole[j];
                    break;
                }
            }
            tcpstate->msgbuf[m]->holes++;
            newhole->start = hole_start;
            newhole->len = segoff - hole_start;
            dfprintf(1, "pcap_handle_tcp_segment: new pre-hole %d: %d %d", j, newhole->start, newhole->len);
        }
        if (segoff >= hole_start && (hole_len == 0 || segoff + seglen < hole_start + hole_len)) {
            /* The segment does not extend past hole boundaries; there is
             * no need to look for other matching holes. */
            break;
        }
    }

    /* copy payload to appropriate location in reassembly buffer */
    memcpy(&tcpstate->msgbuf[m]->buf[segoff], segment, seglen);

    dfprintf(1, "pcap_handle_tcp_segment: holes remaining: %d", tcpstate->msgbuf[m]->holes);

    if (tcpstate->msgbuf[m]->holes == 0) {
        /* We now have a completely reassembled dns message */
        dfprintf(2, "pcap_handle_tcp_segment: %s", "reassembly to dns_protocol_handler");
        dns_protocol_handler(tcpstate->msgbuf[m]->buf, tcpstate->msgbuf[m]->dnslen, tm);
        tcpstate->dnslen_bytes_seen_mask = 0;        /* go back for another message in this tcp connection */
        xfree(tcpstate->msgbuf[m]);
        tcpstate->msgbuf[m] = NULL;
        tcpstate->msgbufs--;
    }

    if (seglen < len) {
        dfprintf(1, "pcap_handle_tcp_segment: %s", "segment tail after reassembly");
        pcap_handle_tcp_segment(segment + seglen, len - seglen, seq + seglen, tcpstate, tm);
    } else {
        dfprintf(1, "pcap_handle_tcp_segment: %s", "nothing more after reassembly");
    };
}

static void
tcpList_add_newest(tcpstate_t * tcpstate)
{
    tcpstate->older = tcpList.newest;
    tcpstate->newer = NULL;
    *(tcpList.newest ? &tcpList.newest->newer : &tcpList.oldest) = tcpstate;
    tcpList.newest = tcpstate;
}

static void
tcpList_remove(tcpstate_t * tcpstate)
{
    *(tcpstate->older ? &tcpstate->older->newer : &tcpList.oldest) = tcpstate->newer;
    *(tcpstate->newer ? &tcpstate->newer->older : &tcpList.newest) = tcpstate->older;
}

static void
tcpList_remove_older_than(long t)
{
    int n = 0;
    tcpstate_t *tcpstate;
    while (tcpList.oldest && tcpList.oldest->last_use < t) {
        tcpstate = tcpList.oldest;
        tcpList_remove(tcpstate);
        hash_remove(&tcpstate->key, tcpHash);
        n++;
    }
    dfprintf(1, "discarded %d old tcpstates", n);
}

/*
 * This function always returns 1 because we do our own assembly and
 * we don't want pcap_layers to do any further processing of this
 * packet.
 */
static int
pcap_tcp_handler(const struct tcphdr *tcp, int len, void *udata)
{
    transport_message *tm = udata;
    int offset = tcp->th_off << 2;
    uint32_t seq;
    tcpstate_t *tcpstate = NULL;
    tcpHashkey_t key;
    char label[384];

    tm->src_port = nptohs(&tcp->th_sport);
    tm->dst_port = nptohs(&tcp->th_dport);
    tm->proto = IPPROTO_TCP;

    key.src_ip_addr = tm->src_ip_addr;
    key.dst_ip_addr = tm->dst_ip_addr;
    key.sport = tm->src_port;
    key.dport = tm->dst_port;

    if (debug_flag > 1) {
        char *p = label;
        inXaddr_ntop(&key.src_ip_addr, p, 128);
        p += strlen(p);
        p += sprintf(p, ":%d ", key.sport);
        inXaddr_ntop(&key.dst_ip_addr, p, 128);
        p += strlen(p);
        p += sprintf(p, ":%d ", key.dport);
        dfprintf(1, "handle_tcp: %s", label);
    }

    if (port53 != key.dport && port53 != key.sport)
        return 1;

    if (NULL == tcpHash) {
        dfprintf(2, "pcap_tcp_handler: %s", "hash_create");
        tcpHash = hash_create(MAX_TCP_STATE, tcp_hashfunc, tcp_cmpfunc, 0, NULL, tcpstate_free);
        if (NULL == tcpHash)
            return 1;
    }

    seq = nptohl(&tcp->th_seq);
    len -= offset;                /* len = length of TCP payload */
    dfprintf(1, "handle_tcp: seq = %u, len = %d", seq, len);

    tcpstate = hash_find(&key, tcpHash);
    if (tcpstate)
        dfprintf(1, "handle_tcp: tcpstate->seq_start = %u, ->msgs = %d", tcpstate->seq_start, tcpstate->msgbufs);

    if (!tcpstate && !(TCPFLAGSYN(tcp))) {
        /* There's no existing state, and this is not the start of a stream.
         * We have no way to synchronize with the stream, so we give up.
         * (This commonly happens for the final ACK in response to a FIN.) */
        dfprintf(1, "handle_tcp: %s", "no state");
        return 1;
    }

    if (tcpstate)
        tcpList_remove(tcpstate);        /* remove from its current position */

    if (TCPFLAGRST(tcp)) {
        dfprintf(1, "handle_tcp: RST at %u", seq);

        /* remove the state for this direction */
        if (tcpstate)
            hash_remove(&key, tcpHash);        /* this also frees tcpstate */

        /* remove the state for the opposite direction */
        key.src_ip_addr = tm->dst_ip_addr;
        key.dst_ip_addr = tm->src_ip_addr;
        key.sport = tm->dst_port;
        key.dport = tm->src_port;
        tcpstate = hash_find(&key, tcpHash);
        if (tcpstate) {
            tcpList_remove(tcpstate);
            hash_remove(&key, tcpHash);        /* this also frees tcpstate */
        }
        return 1;
    }

    if (TCPFLAGSYN(tcp)) {
        dfprintf(1, "handle_tcp: SYN at %u", seq);
        seq++;                        /* skip the syn */
        if (tcpstate) {
            dfprintf(2, "handle_tcp: %s", "...resetting existing tcpstate");
            tcpstate_reset(tcpstate, seq);
        } else {
            dfprintf(2, "handle_tcp: %s", "...creating new tcpstate");
            tcpstate = xcalloc(1, sizeof(*tcpstate));
            if (!tcpstate)
                return 1;
            tcpstate_reset(tcpstate, seq);
            tcpstate->key = key;
            if (0 != hash_add(&tcpstate->key, tcpstate, tcpHash)) {
                tcpstate_free(tcpstate);
                return 1;
            }
        }
    }

    pcap_handle_tcp_segment((uint8_t *) tcp + offset, len, seq, tcpstate, tm);

    if (TCPFLAGFIN(tcp) && !tcpstate->fin) {
        /* End of tcp stream */
        dfprintf(1, "handle_tcp: FIN at %u", seq);
        tcpstate->fin = 1;
    }

    if (tcpstate->fin && tcpstate->msgbufs == 0) {
        /* FIN was seen, and there are no incomplete msgbufs left */
        dfprintf(1, "handle_tcp: %s", "connection done");
        hash_remove(&key, tcpHash);        /* this also frees tcpstate */

    } else {
        /* We're keeping this tcpstate.  Store it in tcpList by age. */
        tcpstate->last_use = tm->ts.tv_sec;
        tcpList_add_newest(tcpstate);
    }
    return 1;
}

static int
pcap_ipv4_handler(const struct ip *ip4, int len, void *udata)
{
    transport_message *tm = udata;
    inXaddr_assign_v4(&tm->src_ip_addr, &ip4->ip_src);
    inXaddr_assign_v4(&tm->dst_ip_addr, &ip4->ip_dst);
    tm->ip_version = 4;
    return 0;
}

static int
pcap_ipv6_handler(const struct ip6_hdr *ip6, int len, void *udata)
{
    transport_message *tm = udata;
    dfprintf(1, "pcap_ipv6_handler: %s", "called");
    inXaddr_assign_v6(&tm->src_ip_addr, &ip6->ip6_src);
    inXaddr_assign_v6(&tm->dst_ip_addr, &ip6->ip6_dst);
    tm->ip_version = 6;
    return 0;
}

static int
pcap_match_vlan(unsigned short vlan, void *udata)
{
    int i;
    if (vlan_tag_needs_byte_conversion)
        vlan = ntohs(vlan);
    dfprintf(1, "vlan is %d", vlan);
    for (i = 0; i < n_vlan_ids; i++)
        if (vlan_ids[i] == vlan)
            return 0;
    return 1;
}

static void
pcap_handle_packet(u_char * udata, const struct pcap_pkthdr *hdr, const u_char * pkt)
{
    transport_message tm;

#if 0                                /* enable this to test code with unaligned headers */
    char buf[PCAP_SNAPLEN + 1];
    memcpy(buf + 1, pkt, hdr->caplen);
    pkt = buf + 1;
#endif

    assign_timeval(last_ts, hdr->ts);
    if (hdr->caplen < ETHER_HDR_LEN)
        return;
    memset(&tm, 0, sizeof(tm));
    assign_timeval(tm.ts, hdr->ts);
    handle_pcap((u_char *) & tm, hdr, pkt);
}



/* ========================================================================= */





static fd_set *
Pcap_select(const fd_set * theFdSet, int sec, int usec)
{
    /* XXX BUG: libpcap may have already buffered a packet that we have not
     * processed yet, but this select will not wake up until new data arrives
     * on the socket.  This problem is serious only if there are long gaps
     * between packets. */
    static fd_set R;
    struct timeval to;
    to.tv_sec = sec;
    to.tv_usec = usec;
    R = *theFdSet;
    if (select(max_pcap_fds, &R, NULL, NULL, &to) > 0)
        return &R;
    return NULL;
}

void
Pcap_init(const char *device, int promisc)
{
    struct stat sb;
    struct bpf_program fp;
    char errbuf[PCAP_ERRBUF_SIZE];
    int x;
    struct _interface *i;

    if (interfaces == NULL) {
        interfaces = xcalloc(MAX_N_INTERFACES, sizeof(*interfaces));
        FD_ZERO(&pcap_fdset);
    }
    assert(interfaces);
    assert(n_interfaces < MAX_N_INTERFACES);
    i = &interfaces[n_interfaces];
    i->device = strdup(device);

    port53 = 53;
    last_ts.tv_sec = last_ts.tv_usec = 0;
    finish_ts.tv_sec = finish_ts.tv_usec = 0;

    if (0 == stat(device, &sb)) {
        i->pcap = pcap_open_offline(device, errbuf);
    } else {
        /*
         * NOTE: the to_ms argument here used to be 50, which seems to make
         * good sense, but actually causes problems when taking packets from
         * multiple interfaces (and one of those interfaces is very quiet).
         * Even though we select() on the pcap FDs, we ignore what it tells
         * us and always try to read from all interfaces, so the timeout
         * here is important.
         */
        i->pcap = pcap_open_live((char *) device, PCAP_SNAPLEN, promisc, 1, errbuf);
    }
    if (NULL == i->pcap) {
        dsyslogf(LOG_ERR, "pcap_open_*: %s", errbuf);
        exit(1);
    }
    if (!pcap_file(i->pcap) && pcap_setnonblock(i->pcap, 1, errbuf) < 0) {
        dsyslogf(LOG_ERR, "pcap_setnonblock(%s): %s", device, errbuf);
        exit(1);
    }
    memset(&fp, '\0', sizeof(fp));
    x = pcap_compile(i->pcap, &fp, bpf_program_str, 1, 0);
    if (x < 0) {
        dsyslogf(LOG_ERR, "pcap_compile failed: %s", pcap_geterr(i->pcap));
        exit(1);
    }
    x = pcap_setfilter(i->pcap, &fp);
    if (x < 0) {
        dsyslogf(LOG_ERR, "pcap_setfilter failed: %s", pcap_geterr(i->pcap));
        exit(1);
    }
    if (pcap_file(i->pcap)) {
        n_pcap_offline++;
    } else {
        i->fd = pcap_get_selectable_fd(i->pcap);
        dfprintf(0, "Pcap_init: FD_SET %d", i->fd);
        FD_SET(i->fd, &pcap_fdset);
        if (i->fd >= max_pcap_fds)
            max_pcap_fds = i->fd + 1;
    }
    if (0 == n_interfaces) {
        /*
         * Initialize pcap_layers library and specifiy IP fragment reassembly
         */
        pcap_layers_init(pcap_datalink(i->pcap), 1);
        if (n_vlan_ids)
            callback_vlan = pcap_match_vlan;
        callback_ipv4 = pcap_ipv4_handler;
        callback_ipv6 = pcap_ipv6_handler;
        callback_udp = pcap_udp_handler;
        callback_tcp = pcap_tcp_handler;
        callback_l7 = dns_protocol_handler;
    } else {
        if (pcap_datalink(i->pcap) != pcap_datalink(interfaces[0].pcap)) {
            dsyslogf(LOG_ERR, "%s", "All interfaces must have same datalink type");
            dsyslogf(LOG_ERR, "First interface has datalink type %d", pcap_datalink(interfaces[0].pcap));
            dsyslogf(LOG_ERR, "Interface '%s' has datalink type %d", device, pcap_datalink(i->pcap));
            exit(1);
        }
    }
    n_interfaces++;
    if (n_pcap_offline > 1 || (n_pcap_offline > 0 && n_interfaces > n_pcap_offline)) {
        dsyslogf(LOG_ERR, "%s", "offline interface must be only interface");
        exit(1);
    }
}

extern int sig_while_processing;

int
Pcap_run(void)
{
    int i;
    int result = 1;
    extern uint64_t statistics_interval;

    for (i = 0; i < n_interfaces; i++)
        interfaces[i].pkts_captured = 0;
    if (n_pcap_offline > 0) {
        result = 0;
        if (finish_ts.tv_sec > 0) {
            start_ts.tv_sec = finish_ts.tv_sec;
            finish_ts.tv_sec += statistics_interval;
        }
        do {
            result = pcap_dispatch(interfaces[0].pcap, 1, pcap_handle_packet, 0);
            if (result <= 0)        /* error or EOF */
                break;
            interfaces[0].pkts_captured += result;
            if (start_ts.tv_sec == 0) {
                start_ts = last_ts;
                finish_ts.tv_sec = ((start_ts.tv_sec / statistics_interval) + 1) * statistics_interval;
                finish_ts.tv_usec = 0;
            }
        } while (last_ts.tv_sec < finish_ts.tv_sec && !sig_while_processing);
        if (result <= 0 || sig_while_processing)
            finish_ts = last_ts;        /* finish was cut short */
    } else {
        gettimeofday(&start_ts, NULL);
        finish_ts.tv_sec = ((start_ts.tv_sec / statistics_interval) + 1) * statistics_interval;
        finish_ts.tv_usec = 0;
        while (last_ts.tv_sec < finish_ts.tv_sec && !sig_while_processing) {
            fd_set *R = Pcap_select(&pcap_fdset, 0, 250000);
            if (NULL == R) {
                gettimeofday(&last_ts, NULL);
            }
            /*
             * Here we intentionally ignore the return value from
             * select() and always try to read from all pcaps. See
             * http://www.tcpdump.org/lists/workers/2002/09/msg00033.html
             */
            for (i = 0; i < n_interfaces; i++) {
                struct _interface *I = &interfaces[i];
                if (FD_ISSET(interfaces[i].fd, &pcap_fdset)) {
                    int x = pcap_dispatch(I->pcap, -1, pcap_handle_packet, 0);
                    if (x > 0)
                        I->pkts_captured += x;
                }
            }
        }
        /*
         * get pcap stats
         */
        for (i = 0; i < n_interfaces; i++) {
            struct _interface *I = &interfaces[i];
            I->ps0 = I->ps1;
            pcap_stats(I->pcap, &I->ps1);
        }

        if (sig_while_processing)
            finish_ts = last_ts;
    }
    tcpList_remove_older_than(last_ts.tv_sec - MAX_TCP_IDLE);
    return result;
}

void
Pcap_close(void)
{
    int i;
    for (i = 0; i < n_interfaces; i++)
        pcap_close(interfaces[i].pcap);
    xfree(interfaces);
    interfaces = NULL;
}

int
Pcap_start_time(void)
{
    return (int) start_ts.tv_sec;
}

int
Pcap_finish_time(void)
{
    return (int) finish_ts.tv_sec;
}

void
pcap_set_match_vlan(int vlan)
{
    assert(n_vlan_ids < MAX_VLAN_IDS);
    vlan_ids[n_vlan_ids++] = vlan;
}

/* ========== PCAP_STAT INDEXER ========== */

int pcap_ifname_iterator(char **);
int pcap_stat_iterator(char **);

static indexer_t indexers[] = {
    {"ifname", NULL, pcap_ifname_iterator, NULL},
    {"pcap_stat", NULL, pcap_stat_iterator, NULL},
    {NULL, NULL, NULL, NULL},
};

int
pcap_ifname_iterator(char **label)
{
    static int next_iter = 0;
    if (NULL == label) {
        next_iter = 0;
        return n_interfaces;
    }
    if (next_iter >= 0 && next_iter < n_interfaces) {
        *label = interfaces[next_iter].device;
        return next_iter++;
    }
    return -1;
}

int
pcap_stat_iterator(char **label)
{
    static int next_iter = 0;
    if (NULL == label) {
        next_iter = 0;
        return 3;
    }
    if (0 == next_iter)
        *label = "pkts_captured";
    else if (1 == next_iter)
        *label = "filter_received";
    else if (2 == next_iter)
        *label = "kernel_dropped";
    else
        return -1;
    return next_iter++;
}

void
pcap_report(FILE * fp, md_array_printer * printer)
{
    int i;
    md_array *theArray = acalloc(1, sizeof(*theArray));
    if ( !theArray ) {
        dsyslog(LOG_ERR, "unable to write report, out of memory");
        return;
    }
    theArray->name = "pcap_stats";
    theArray->d1.indexer = &indexers[0];
    theArray->d1.type = "ifname";
    theArray->d1.alloc_sz = n_interfaces;
    theArray->d2.indexer = &indexers[1];
    theArray->d2.type = "pcap_stat";
    theArray->d2.alloc_sz = 3;
    theArray->array = acalloc(n_interfaces, sizeof(*theArray->array));
    if ( !theArray->array ) {
        dsyslog(LOG_ERR, "unable to write report, out of memory");
        return;
    }
    for (i = 0; i < n_interfaces; i++) {
        struct _interface *I = &interfaces[i];
        theArray->array[i].alloc_sz = 3;
        theArray->array[i].array = acalloc(3, sizeof(int));
        theArray->array[i].array[0] = I->pkts_captured;
        theArray->array[i].array[1] = I->ps1.ps_recv - I->ps0.ps_recv;
        theArray->array[i].array[2] = I->ps1.ps_drop - I->ps0.ps_drop;
    }
    md_array_print(theArray, printer, fp);
}
