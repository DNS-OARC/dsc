/*
 * $Id$
 *
 * Copyright (c) 2007, The Measurement Factory, Inc.  All rights
 * reserved.  See the LICENSE file for details.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <netinet/in.h>
#if USE_IPV6
#include <netinet/ip6.h>
#endif

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
#include "ip_message.h"
#include "pcap.h"
#include "byteorder.h"
#include "syslog_debug.h"
#include "hashtbl.h"

#define PCAP_SNAPLEN 1460
#ifndef ETHER_HDR_LEN
#define ETHER_ADDR_LEN 6
#define ETHER_TYPE_LEN 2
#define ETHER_HDR_LEN (ETHER_ADDR_LEN * 2 + ETHER_TYPE_LEN)
#endif
#ifndef ETHERTYPE_8021Q
#define ETHERTYPE_8021Q	0x8100
#endif

#if USE_IPV6
/* We might need to define ETHERTYPE_IPV6 */
#ifndef ETHERTYPE_IPV6
#define ETHERTYPE_IPV6 0x86dd
#endif
#endif

#if USE_PPP
#include <net/if_ppp.h>
#define PPP_ADDRESS_VAL       0xff	/* The address byte value */
#define PPP_CONTROL_VAL       0x03	/* The control byte value */
#endif

#ifdef __linux__
#define uh_dport dest
#define uh_sport source
#define th_off doff
#define th_dport dest
#define th_sport source
#define th_seq seq
#define TCPFLAGFIN(a) (a)->fin
#define TCPFLAGSYN(a) (a)->syn
#else
#define TCPFLAGSYN(a) ((a)->th_flags&TH_SYN)
#define TCPFLAGFIN(a) ((a)->th_flags&TH_FIN)
#endif

#ifndef IP_OFFMASK
#define IP_OFFMASK 0x1fff
#endif

#define MAX_N_PCAP 10
static int n_pcap = 0;
static int n_pcap_offline = 0;
static pcap_t **pcap = NULL;
static fd_set pcap_fdset;
static int max_pcap_fds = 0;
static unsigned short port53;

char *bpf_program_str = NULL;
void (*handle_datalink) (const u_char * pkt, int len, transport_message *tm) = NULL;
int vlan_tag_needs_byte_conversion = 1;

extern void handle_dns(const u_char *buf, uint16_t len, transport_message *tm,
    DMC *dns_message_callback);
extern int debug_flag;
#if 0
static int debug_count = 20;
#endif
static DMC *dns_message_callback;
static IPC *ip_message_callback;
static struct timeval last_ts;
static struct timeval start_ts;
static struct timeval finish_ts;
#define MAX_VLAN_IDS 100
static int n_vlan_ids = 0;
static int vlan_ids[MAX_VLAN_IDS];
static hashtbl *tcpHash;

void
handle_udp(const struct udphdr *udp, int len, transport_message *tm)
{
    tm->src_port = nptohs(&udp->uh_sport);
    tm->dst_port = nptohs(&udp->uh_dport);

    if (port53 != tm->dst_port && port53 != tm->src_port)
	return;
    handle_dns((void *)(udp + 1), len - sizeof(*udp), tm, dns_message_callback);
}

#define MAX_DNS_LENGTH 0xFFFF

#define MAX_TCP_WINDOW_SIZE (0xFFFF << 14)
#define MAX_TCP_STATE 65535

/* These numbers define the sizes of small arrays which are simpler to work
 * with than dynamically allocated lists. */
#define MAX_TCP_MSGS 8 /* messages being reassembled (per connection) */
#define MAX_TCP_SEGS 8 /* segments not assigned to a message (per connection) */
#define MAX_TCP_HOLES 8 /* holes in a msg buf (per message) */

typedef struct {
    inX_addr src_ip_addr;
    inX_addr dst_ip_addr;
    uint16_t dport;
    uint16_t sport;
} tcpHashkey_t;

/* Description of hole in tcp reassembly buffer. */
typedef struct {
    uint16_t start; /* start of hole, measured from beginning of msgbuf->buf */
    uint16_t len; /* length of hole (0 == unused) */
} tcphole_t;

/* TCP message reassembly buffer */
typedef struct {
    uint32_t seq; /* seq# of first byte of header of this DNS msg */
    uint16_t dnslen; /* length of dns message, and size of buf */
    tcphole_t hole[MAX_TCP_HOLES];
    int holes; /* number of holes remaining in message */
    u_char buf[]; /* reassembled message (C99 flexible array member) */
} tcp_msgbuf_t;

/* held TCP segment */
typedef struct {
    uint32_t seq; /* sequence number of first byte of segment */
    uint16_t len; /* length of segment, and size of buf */
    u_char buf[]; /* segment payload (C99 flexible array member) */
} tcp_segbuf_t;

/* TCP reassembly state */
typedef struct {
    uint32_t seq_start; /* seq# of length field of next DNS msg */
    short msgbufs; /* number of msgbufs in use */
    int8_t fin; /* have we seen a FIN? */
    tcp_msgbuf_t *msgbuf[MAX_TCP_MSGS];
    tcp_segbuf_t *segbuf[MAX_TCP_SEGS];
} tcpstate_t;

static void
tcpstate_reset(tcpstate_t *tcpstate, uint32_t seq)
{
    int i;
    tcpstate->seq_start = seq;
    tcpstate->fin = 0;
    if (tcpstate->msgbufs > 0) {
	tcpstate->msgbufs = 0;
	for (i = 0; i < MAX_TCP_MSGS; i++) {
	    if (tcpstate->msgbuf[i]) {
		free(tcpstate->msgbuf[i]);
		tcpstate->msgbuf[i] = NULL;
	    }
	}
    }
    for (i = 0; i < MAX_TCP_SEGS; i++) {
	if (tcpstate->segbuf[i]) {
	    free(tcpstate->segbuf[i]);
	    tcpstate->segbuf[i] = NULL;
	}
    }
}

static void
tcpstate_free(void *p)
{
    tcpstate_reset((tcpstate_t *)p, 0);
    free(p);
}

static unsigned int
tcp_hashfunc(const void *key)
{
    tcpHashkey_t *k = (tcpHashkey_t *) key;
    return (k->dport << 16) | k->sport |
	k->src_ip_addr._.in4.s_addr | k->dst_ip_addr._.in4.s_addr;
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
 * TODO:
 * - handle RST 
 * - deallocate state for connections that have been idle too long
 */
void
handle_tcp_segment(u_char *segment, int len, uint32_t seq, tcpstate_t *tcpstate,
    transport_message *tm)
{
    int i, m, s;
    uint16_t dnslen;
    int segoff, seglen;

    if (debug_flag)
	fprintf(stderr, "handle_tcp_segment(): seq=%u, len=%d\n", seq, len);

    if (len <= 0) /* there is no more payload */
	return;

    if (seq == tcpstate->seq_start) {
	/* payload is 2-byte length field plus 0 or more bytes of DNS message */
	if (len < sizeof(uint16_t)) {
	    /* makes no sense */
	    if (debug_flag)
		fprintf(stderr, "handle_tcp_segment: nonsense len in first segment\n");
	    return;
	}
	dnslen = nptohs(segment);
	tcpstate->seq_start += sizeof(uint16_t) + dnslen;
	len -= sizeof(uint16_t);
	segment += sizeof(uint16_t);
	seq += sizeof(uint16_t);
	if (debug_flag)
	    fprintf(stderr, "handle_tcp_segment: first segment; dnslen = %d\n", dnslen);
	if (len >= dnslen) {
	    /* this segment contains a complete message - avoid the reassembly
	     * buffer and just handle the message immediately */
	    handle_dns(segment, dnslen, tm, dns_message_callback);
	    /* handle the trailing part of the segment */
	    if (len > dnslen) {
		if (debug_flag)
		    fprintf(stderr, "handle_tcp_segment: segment tail\n");
		handle_tcp_segment(segment + dnslen, len - dnslen,
		    seq + dnslen, tcpstate, tm);
	    }
	    return;
	}
	/* allocate a msgbuf for reassembly */
	for (m = 0; tcpstate->msgbuf[m]; ) {
	    if (++m == MAX_TCP_MSGS) {
		if (debug_flag)
		    fprintf(stderr, "handle_tcp_segment: out of msgbufs\n");
		return;
	    }
	}
	tcpstate->msgbuf[m] = xcalloc(1, sizeof(tcp_msgbuf_t) + dnslen);
	if (NULL == tcpstate->msgbuf[m]) {
	    syslog(LOG_ERR, "out of memory for tcp_msgbuf (%d)", dnslen);
	    return;
	}
	tcpstate->msgbufs++;
	tcpstate->msgbuf[m]->seq = seq;
	tcpstate->msgbuf[m]->dnslen = dnslen;
	tcpstate->msgbuf[m]->holes = 1;
	tcpstate->msgbuf[m]->hole[0].start = len;
	tcpstate->msgbuf[m]->hole[0].len = dnslen - len;
	if (debug_flag) {
	    fprintf(stderr, "handle_tcp_segment: new msgbuf %d: seq = %u, dnslen = %d, hole start = %d, hole len = %d\n",
		m,
		tcpstate->msgbuf[m]->seq,
		tcpstate->msgbuf[m]->dnslen,
		tcpstate->msgbuf[m]->hole[0].start,
		tcpstate->msgbuf[m]->hole[0].len);
	}
	/* copy segment to appropriate location in reassembly buffer */
	memcpy(tcpstate->msgbuf[m]->buf, segment, len);

	/* Now that we know the length of this message, we must check any held
	 * segments to see if they belong to it. */
	for (s = 0; s < MAX_TCP_SEGS; s++) {
	    if (!tcpstate->segbuf[s]) continue;
	    if (tcpstate->segbuf[s]->seq - seq >= 0 &&
		tcpstate->segbuf[s]->seq - seq < dnslen)
	    {
		tcp_segbuf_t *segbuf = tcpstate->segbuf[s];
		tcpstate->segbuf[s] = NULL;
		handle_tcp_segment(segbuf->buf, segbuf->len, segbuf->seq,
		    tcpstate, tm);
		free(segbuf);
	    }
	}
	return;
    }

    /* find the message to which the first byte of this segment belongs */
    for (m = 0; ; m++) {
	if (m >= MAX_TCP_MSGS) {
	    /* seg does not match any msgbuf; just hold on to it. */
	    if (debug_flag)
		fprintf(stderr, "handle_tcp_segment: seg does not match any msgbuf\n");

	    if (seq - tcpstate->seq_start > MAX_TCP_WINDOW_SIZE) {
		if (debug_flag)
		    fprintf(stderr, "handle_tcp_segment: seg is outside window; discarding\n");
		return;
	    }
	    for (s = 0; ; s++) {
		if (s >= MAX_TCP_SEGS) {
		    if (debug_flag)
			fprintf(stderr, "handle_tcp_segment: out of segbufs\n");
		    return;
		}
		if (tcpstate->segbuf[s])
		    continue;
		tcpstate->segbuf[s] = xcalloc(1, sizeof(tcp_segbuf_t) + len);
		tcpstate->segbuf[s]->seq = seq;
		tcpstate->segbuf[s]->len = len;
		memcpy(tcpstate->segbuf[s]->buf, segment, len);
		if (debug_flag) {
		    fprintf(stderr, "handle_tcp_segment: new segbuf %d: seq = %u, len = %d\n",
			s,
			tcpstate->segbuf[s]->seq,
			tcpstate->segbuf[s]->len);
		}
		return;
	    }
	}
	if (!tcpstate->msgbuf[m])
	    continue;
	segoff = seq - tcpstate->msgbuf[m]->seq;
	if (segoff >= 0 && segoff < tcpstate->msgbuf[m]->dnslen) {
	    /* segment starts in this msgbuf */
	    fprintf(stderr, "handle_tcp_segment: seg matches msg %d: seq = %u, dnslen = %d\n",
		m,
		tcpstate->msgbuf[m]->seq, tcpstate->msgbuf[m]->dnslen);
	    if (segoff + len > tcpstate->msgbuf[m]->dnslen) {
		/* segment would overflow msgbuf */
		seglen = tcpstate->msgbuf[m]->dnslen - segoff;
		if (debug_flag)
		    fprintf(stderr, "handle_tcp_segment: using partial segment %d\n", seglen);
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
	    continue; /* hole descriptor is not in use */
	hole_start = tcpstate->msgbuf[m]->hole[i].start;
	hole_len = tcpstate->msgbuf[m]->hole[i].len;
	if (segoff >= hole_start + hole_len)
	    continue; /* segment is totally after hole */
	if (segoff + seglen <= hole_start)
	    continue; /* segment is totally before hole */
	/* The segment overlaps this hole.  Delete the hole. */
	if (debug_flag)
	    fprintf(stderr, "handle_tcp_segment: overlaping hole %d: %d %d\n", i, hole_start, hole_len);
	tcpstate->msgbuf[m]->hole[i].len = 0;
	tcpstate->msgbuf[m]->holes--;
	if (segoff + seglen < hole_start + hole_len) {
	    /* create a new hole after the segment (common case) */
	    newhole = &tcpstate->msgbuf[m]->hole[i]; /* hole[i] is guaranteed free */
	    newhole->start = segoff + seglen;
	    newhole->len = (hole_start + hole_len) - newhole->start;
	    tcpstate->msgbuf[m]->holes++;
	    if (debug_flag)
		fprintf(stderr, "handle_tcp_segment: new post-hole %d: %d %d\n", i, newhole->start, newhole->len);
	}
	if (segoff > hole_start) {
	    /* create a new hole before the segment */
	    int j;
	    for (j=0; ; j++) {
		if (j == MAX_TCP_HOLES) {
		    if (debug_flag)
			fprintf(stderr, "handle_tcp_segment: out of hole descriptors\n");
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
	    if (debug_flag)
		fprintf(stderr, "handle_tcp_segment: new pre-hole %d: %d %d\n", j, newhole->start, newhole->len);
	}
	if (segoff >= hole_start &&
	    (hole_len == 0 || segoff + seglen < hole_start + hole_len))
	{
	    /* The segment does not extend past hole boundaries; there is
	     * no need to look for other matching holes. */
	    break;
	}
    }

    /* copy payload to appropriate location in reassembly buffer */
    memcpy(&tcpstate->msgbuf[m]->buf[segoff], segment, seglen);

    if (debug_flag)
	fprintf(stderr, "handle_tcp_segment: holes remaining: %d\n", tcpstate->msgbuf[m]->holes);

    if (tcpstate->msgbuf[m]->holes == 0) {
	/* We now have a completely reassembled dns message */
	handle_dns(tcpstate->msgbuf[m]->buf, tcpstate->msgbuf[m]->dnslen, tm, dns_message_callback);
	free(tcpstate->msgbuf[m]);
	tcpstate->msgbuf[m] = NULL;
	tcpstate->msgbufs--;
    }

    if (seglen < len) {
	if (debug_flag)
	    fprintf(stderr, "handle_tcp_segment: segment tail\n");
	handle_tcp_segment(segment + seglen, len - seglen, seq + seglen,
	    tcpstate, tm);
    }
}

void
handle_tcp(const struct tcphdr *tcp, int len, transport_message *tm)
{
    int offset = tcp->th_off << 2;
    uint32_t seq;
    tcpstate_t *tcpstate = NULL;
    tcpHashkey_t key, *newkey;
    char label[384];

    tm->src_port = nptohs(&tcp->th_sport);
    tm->dst_port = nptohs(&tcp->th_dport);

    key.src_ip_addr = tm->src_ip_addr;
    key.dst_ip_addr = tm->dst_ip_addr;
    key.sport = tm->src_port;
    key.dport = tm->dst_port;

    if (debug_flag) {
	char *p = label;
	inXaddr_ntop(&key.src_ip_addr, p, 128);
	p += strlen(p);
	p += sprintf(p, ":%d ", key.sport);
	inXaddr_ntop(&key.dst_ip_addr, p, 128);
	p += strlen(p);
	p += sprintf(p, ":%d ", key.dport);
    }

    if (debug_flag)
	fprintf(stderr, "handle_tcp(): %s\n", label);

    if (port53 != key.dport && port53 != key.sport)
	return;

    if (NULL == tcpHash) {
        tcpHash = hash_create(MAX_TCP_STATE, tcp_hashfunc, tcp_cmpfunc,
	    free, tcpstate_free);
	if (NULL == tcpHash)
	    return;
    }

    seq = nptohl(&tcp->th_seq);
    len -= offset; /* len = length of TCP payload */
    if (debug_flag)
	fprintf(stderr, "handle_tcp: seq = %u, len = %d", seq, len);

    tcpstate = hash_find(&key, tcpHash);
    if (debug_flag) {
	if (tcpstate)
	    fprintf(stderr, ", seq_start = %u, msgs = %d\n", tcpstate->seq_start, tcpstate->msgbufs);
	else
	    fprintf(stderr, "\n");
    }

    if (!tcpstate && !(TCPFLAGSYN(tcp))) {
	/* There's no existing state, and this is not the start of a stream.
	 * We have no way to synchronize with the stream, so we give up.
	 * (This commonly happens for the final ACK in response to a FIN.) */
	if (debug_flag)
	    fprintf(stderr, "handle_tcp: no state\n");
	return;
    }

    if (TCPFLAGSYN(tcp)) {
	if (debug_flag)
	    fprintf(stderr, "handle_tcp: SYN at %u\n", seq);
	seq++; /* skip the syn */
	if (tcpstate) {
	    tcpstate_reset(tcpstate, seq);
	} else {
	    tcpstate = xcalloc(1, sizeof(*tcpstate));
	    if (!tcpstate)
		return;
	    tcpstate_reset(tcpstate, seq);
	    newkey = xmalloc(sizeof(*newkey));
	    if (!newkey) {
		free(tcpstate);
		return;
	    }
	    *newkey = key;
	    if (0 != hash_add(newkey, tcpstate, tcpHash)) {
		free(newkey);
		tcpstate_free(tcpstate);
		return;
	    }
	}
    }

    handle_tcp_segment((void*)tcp + offset, len, seq, tcpstate, tm);

    if (TCPFLAGFIN(tcp) && !tcpstate->fin) {
	/* End of tcp stream */
	if (debug_flag)
	    fprintf(stderr, "handle_tcp: FIN at %u\n", seq);
	tcpstate->fin = 1;
    }

    if (tcpstate->fin && tcpstate->msgbufs == 0) {
	/* FIN was seen, and there are no incomplete msgbufs left */
	if (debug_flag)
	    fprintf(stderr, "handle_tcp: connection done\n");
	hash_remove(&key, tcpHash);
    }
}

void
handle_ipv4(const struct ip * ip, int len, transport_message *tm)
{
    int offset = ip->ip_hl << 2;
    int iplen = nptohs(&ip->ip_len);
    ip_message i;

    inXaddr_assign_v4(&tm->src_ip_addr, &ip->ip_src);
    inXaddr_assign_v4(&tm->dst_ip_addr, &ip->ip_dst);

    i.version = 4;
    i.src = tm->src_ip_addr;
    i.dst = tm->dst_ip_addr;
    i.proto = ip->ip_p;
    ip_message_callback(&i);

    /* sigh, punt on IP fragments */
    if (nptohs(&ip->ip_off) & IP_OFFMASK)
	return;

    tm->proto = ip->ip_p;
    if (IPPROTO_UDP == ip->ip_p) {
	handle_udp((struct udphdr *) ((void *)ip + offset), iplen - offset, tm);
    } else if (IPPROTO_TCP == ip->ip_p) {
	handle_tcp((struct tcphdr *) ((void *)ip + offset), iplen - offset, tm);
    }
}

#if USE_IPV6
void
handle_ipv6(const struct ip6_hdr * ip6, int len, transport_message *tm)
{
    ip_message *i;
    int offset = sizeof(struct ip6_hdr);
    int nexthdr = ip6->ip6_nxt;
    uint16_t payload_len = nptohs(&ip6->ip6_plen);

    if (debug_flag)
	fprintf(stderr, "handle_ipv6()\n");

    /*
     * Parse extension headers. This only handles the standard headers, as
     * defined in RFC 2460, correctly. Fragments are discarded.
     */
    while ((IPPROTO_ROUTING == nexthdr) /* routing header */
        ||(IPPROTO_HOPOPTS == nexthdr)  /* Hop-by-Hop options. */
        ||(IPPROTO_FRAGMENT == nexthdr) /* fragmentation header. */
        ||(IPPROTO_DSTOPTS == nexthdr)  /* destination options. */
        ||(IPPROTO_DSTOPTS == nexthdr)  /* destination options. */
        ||(IPPROTO_AH == nexthdr)       /* destination options. */
        ||(IPPROTO_ESP == nexthdr)) {   /* encapsulating security payload. */
        typedef struct {
            uint8_t nexthdr;
            uint8_t length;
        } ext_hdr_t;
        ext_hdr_t *ext_hdr;
        uint16_t ext_hdr_len;

        /* Catch broken packets */
        if ((offset + sizeof(ext_hdr)) > len)
            return;

        /* Cannot handle fragments. */
        if (IPPROTO_FRAGMENT == nexthdr)
            return;

        ext_hdr = (ext_hdr_t*)((char *)ip6 + offset);
        nexthdr = ext_hdr->nexthdr;
        ext_hdr_len = (8 * (ext_hdr->length + 1));

        /* This header is longer than the packets payload.. WTF? */
        if (ext_hdr_len > payload_len)
            return;

        offset += ext_hdr_len;
        payload_len -= ext_hdr_len;
    }                           /* while */

    i = xcalloc(1, sizeof(*i));
    i->version = 6;
    inXaddr_assign_v6(&tm->src_ip_addr, &ip6->ip6_src);
    inXaddr_assign_v6(&tm->dst_ip_addr, &ip6->ip6_dst);
    i->src = tm->src_ip_addr;
    i->dst = tm->dst_ip_addr;
    i->proto = nexthdr;
    ip_message_callback(i);
    free(i);

    /* Catch broken and empty packets */
    if (((offset + payload_len) > len)
        || (payload_len == 0)
        || (payload_len > PCAP_SNAPLEN))
        return;

    tm->proto = nexthdr;
    if (IPPROTO_UDP == nexthdr) {
	handle_udp((struct udphdr *) ((char *) ip6 + offset), payload_len, tm);
    } else if (IPPROTO_TCP == nexthdr) {
	handle_tcp((struct tcphdr *) ((char *) ip6 + offset), payload_len, tm);
    }
}
#endif /* USE_IPV6 */

void
handle_ip(const struct ip * ip, int len, transport_message *tm)
{
    /* note: ip->ip_v does not work if header is not int-aligned */
    switch((*(uint8_t*)ip) >> 4) {
    case 4:
        handle_ipv4(ip, len, tm);
	break;
#if USE_IPV6
    case 6:
        handle_ipv6((struct ip6_hdr *)ip, len, tm);
	break;
#endif
    default:
	break;
    }
}

static int
is_ethertype_ip(unsigned short proto)
{
	if (ETHERTYPE_IP == proto)
		return 1;
#if USE_PPP
	if (PPP_IP == proto)
		return 1;
#endif
#if USE_IPV6 && defined(ETHERTYPE_IPV6)
	if (ETHERTYPE_IPV6 == proto)
		return 1;
#endif
	return 0;
}

static int
is_family_inet(unsigned int family)
{
	if (AF_INET == family)
		return 1;
#if USE_IPV6
	if (AF_INET6 == family)
		return 1;
#endif
	return 0;
}

#if USE_PPP
void
handle_ppp(const u_char * pkt, int len, transport_message *tm)
{
    char buf[PCAP_SNAPLEN];
    unsigned short proto;
    if (len < 2)
	return NULL;
    if (*pkt == PPP_ADDRESS_VAL && *(pkt + 1) == PPP_CONTROL_VAL) {
	pkt += 2;		/* ACFC not used */
	len -= 2;
    }
    if (len < 2)
	return NULL;
    if (*pkt % 2) {
	proto = *pkt;		/* PFC is used */
	pkt++;
	len--;
    } else {
	proto = nptohs(pkt);
	pkt += 2;
	len -= 2;
    }
    if (is_ethertype_ip(proto))
        handle_ip((struct ip *) pkt, len, tm);
}

#endif

void
handle_null(const u_char * pkt, int len, transport_message *tm)
{
    unsigned int family;
    memcpy(&family, pkt, sizeof(family));
    if (is_family_inet(family))
	handle_ip((struct ip *) (pkt + 4), len - 4, tm);
}

#ifdef DLT_LOOP
void
handle_loop(const u_char * pkt, int len, transport_message *tm)
{
    unsigned int family;
    memcpy(&family, pkt, sizeof(family));
    if (is_family_inet(family))
	handle_ip((struct ip *) (pkt + 4), len - 4, tm);
}

#endif

#ifdef DLT_RAW
void
handle_raw(const u_char * pkt, int len, transport_message *tm)
{
    handle_ip((struct ip *) pkt, len, tm);
}

#endif

int
match_vlan(const u_char *pkt)
{
    unsigned short vlan;
    int i;
    if (0 == n_vlan_ids)
	return 1;
    memcpy(&vlan, pkt, 2);
    if (vlan_tag_needs_byte_conversion)
	vlan = ntohs(vlan) & 0xfff;
    else
	vlan = vlan & 0xfff;
    if (debug_flag)
	fprintf(stderr, "vlan is %d\n", vlan);
    for (i = 0; i < n_vlan_ids; i++)
	if (vlan_ids[i] == vlan)
	    return 1;
    return 0;
}

void
handle_ether(const u_char * pkt, int len, transport_message *tm)
{
    struct ether_header *e = (void *) pkt;
    unsigned short etype = nptohs(&e->ether_type);
    if (len < ETHER_HDR_LEN)
	return;
    pkt += ETHER_HDR_LEN;
    len -= ETHER_HDR_LEN;
    if (ETHERTYPE_8021Q == etype) {
	if (!match_vlan(pkt))
	    return;
	etype = nptohs((unsigned short *) (pkt + 2));
	pkt += 4;
	len -= 4;
    }
    if (len < 0)
	return;
    if (is_ethertype_ip(etype)) {
	handle_ip((struct ip *) pkt, len, tm);
    }
}

void
handle_pcap(u_char * udata, const struct pcap_pkthdr *hdr, const u_char * pkt)
{
    transport_message tm;

#if 0 /* enable this to test code with unaligned headers */
    char buf[PCAP_SNAPLEN+1];
    memcpy(buf+1, pkt, hdr->caplen);
    pkt = buf+1;
#endif

    last_ts = hdr->ts;
    if (start_ts.tv_sec == 0)
	start_ts = last_ts;
#if 0
    if (debug_flag)
	fprintf(stderr, "handle_pcap()\n");
#endif
    if (hdr->caplen < ETHER_HDR_LEN)
	return;
    tm.ts = hdr->ts;
    handle_datalink(pkt, hdr->caplen, &tm);
#if 0
    if (debug_flag && --debug_count == 0)
	exit(0);
#endif
}



/* ========================================================================= */





fd_set *
Pcap_select(const fd_set * theFdSet, int sec, int usec)
{
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
    int readfile_state = 0;
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *new_pcap;
    int x;

    if (pcap == NULL) {
	pcap = xcalloc(MAX_N_PCAP, sizeof(*pcap));
	FD_ZERO(&pcap_fdset);
    }
    assert(pcap);
    assert(n_pcap < MAX_N_PCAP);

    port53 = 53;
    last_ts.tv_sec = last_ts.tv_usec = 0;

    if (0 == stat(device, &sb))
	readfile_state = 1;
    if (readfile_state) {
	new_pcap = pcap_open_offline(device, errbuf);
    } else {
	new_pcap = pcap_open_live((char *) device, PCAP_SNAPLEN, promisc, 1000, errbuf);
    }
    if (NULL == new_pcap) {
	syslog(LOG_ERR, "pcap_open_*: %s", errbuf);
	exit(1);
    }
    memset(&fp, '\0', sizeof(fp));
    x = pcap_compile(new_pcap, &fp, bpf_program_str, 1, 0);
    if (x < 0) {
	syslog(LOG_ERR, "pcap_compile failed: %s", pcap_geterr(new_pcap));
	exit(1);
    }
    x = pcap_setfilter(new_pcap, &fp);
    if (x < 0) {
	syslog(LOG_ERR, "pcap_setfilter failed: %s", pcap_geterr(new_pcap));
	exit(1);
    }
    switch (pcap_datalink(new_pcap)) {
    case DLT_EN10MB:
	handle_datalink = handle_ether;
	break;
#if USE_PPP
    case DLT_PPP:
	handle_datalink = handle_ppp;
	break;
#endif
#ifdef DLT_LOOP
    case DLT_LOOP:
	handle_datalink = handle_loop;
	break;
#endif
#ifdef DLT_RAW
    case DLT_RAW:
	handle_datalink = handle_raw;
	break;
#endif
    case DLT_NULL:
	handle_datalink = handle_null;
	break;
    default:
	syslog(LOG_ERR, "unsupported data link type %d",
	    pcap_datalink(new_pcap));
	exit(1);
	break;
    }
    if (pcap_file(new_pcap)) {
	n_pcap_offline++;
    } else {
	if (debug_flag)
	    fprintf(stderr, "Pcap_init: FD_SET %d\n", pcap_fileno(new_pcap));
	FD_SET(pcap_fileno(new_pcap), &pcap_fdset);
	if (pcap_fileno(new_pcap) >= max_pcap_fds)
	    max_pcap_fds = pcap_fileno(new_pcap) + 1;
    }
    pcap[n_pcap++] = new_pcap;
}

void
Pcap_run(DMC * dns_callback, IPC * ip_callback)
{
    int i;

    dns_message_callback = dns_callback;
    ip_message_callback = ip_callback;
    if (n_pcap_offline > 0) {
	start_ts.tv_sec = 0;
	start_ts.tv_usec = 0;
	for (i = 0; i < n_pcap; i++) {
	    pcap_dispatch(pcap[i], -1, handle_pcap, NULL);
	}
	finish_ts = last_ts;
    } else {
	gettimeofday(&start_ts, NULL);
	finish_ts.tv_sec = ((start_ts.tv_sec / 60) + 1) * 60;
	finish_ts.tv_usec = 0;
	while (last_ts.tv_sec < finish_ts.tv_sec) {
	    fd_set *R = Pcap_select(&pcap_fdset, 0, 250000);
	    if (NULL == R) {
		gettimeofday(&last_ts, NULL);
	    }
	    for (i = 0; i < n_pcap; i++) {
		if (FD_ISSET(pcap_fileno(pcap[i]), &pcap_fdset)) {
		    pcap_dispatch(pcap[i], 50, handle_pcap, NULL);
		}
	    }
	}
    }
}

void
Pcap_close(void)
{
    int i;
    for (i = 0; i < n_pcap; i++)
	pcap_close(pcap[i]);
    free(pcap);
    pcap = NULL;
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
