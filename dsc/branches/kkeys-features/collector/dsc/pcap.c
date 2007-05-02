/*
 * $Id$
 *
 * http://dnstop.measurement-factory.com/
 *
 * Copyright (c) 2002, The Measurement Factory, Inc.  All rights
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

#if USE_PPP
#include <net/if_ppp.h>
#define PPP_ADDRESS_VAL       0xff	/* The address byte value */
#define PPP_CONTROL_VAL       0x03	/* The control byte value */
#endif

#ifdef __linux__
#define uh_dport dest
#define uh_sport source
#endif

#ifndef IP_OFFMASK
#define IP_OFFMASK 0x1fff
#endif

#define MAX_N_PCAP 10
static int n_pcap = 0;
static pcap_t **pcap = NULL;
static fd_set pcap_fdset;
static int max_pcap_fds = 0;
static unsigned short port53;

char *bpf_program_str = NULL;
dns_message *(*handle_datalink) (const u_char * pkt, int len) = NULL;
int vlan_tag_needs_byte_conversion = 1;

extern dns_message *handle_dns(const char *buf, int len);
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

dns_message *
handle_udp(const struct udphdr *udp, int len)
{
    dns_message *m;

    if (port53 != nptohs(&udp->uh_dport) && port53 != nptohs(&udp->uh_sport))
	return NULL;
    m = handle_dns((void *)(udp + 1), len - sizeof(*udp));
    if (NULL == m)
	return NULL;
    m->src_port = nptohs(&udp->uh_sport);
    return m;
}

#define MAX_DNS_LENGTH 0xFFFF

#define MAX_TCP_STATE 65536
#define MAX_TCP_HOLES 8 /* this should be enough for legitimate connections */

typedef struct {
    inX_addr src_ip_addr;
    inX_addr dst_ip_addr;
    uint16_t dport;
    uint16_t sport;
} tcpHashkey_t;

/* Description of hole in tcp reassembly buffer. */
typedef struct {
    uint16_t start; /* start of hole */
    uint16_t len; /* length of hole; 0 = infinity */
    int8_t used;
} tcphole_t;

typedef struct {
    uint32_t seq_start; /* sequence number of first byte of DNS header */
    uint16_t dnslen;
    int8_t fin; /* have we received a FIN? */
    tcphole_t hole[MAX_TCP_HOLES];
    int holes;
    u_char *buf;
} tcpstate_t;

static void
tcpstate_reset(tcpstate_t *tcpstate, uint32_t seq)
{
    tcpstate->seq_start = seq;
    tcpstate->dnslen = 0;
    /* DON'T reset tcpstate->fin */
    tcpstate->hole[0].used = 1;
    tcpstate->hole[0].start = 0;
    tcpstate->hole[0].len = 0; /* infinity */
    tcpstate->holes = 1;
    if (tcpstate->buf) {
	free(tcpstate->buf);
	tcpstate->buf = NULL;
    }
}

static void
tcpstate_free(void *p)
{
    tcpstate_t *tcpstate = (tcpstate_t *)p; 
    if (tcpstate->buf)
	free(tcpstate->buf);
    free(tcpstate);
}

static unsigned int
tcp_hashfunc(const void *key)
{
    return SuperFastHash(key, strlen(key));;
}

static int
tcp_cmpfunc(const void *a, const void *b)
{
    return memcmp(a, b, sizeof(tcpHashkey_t));
}

/* TCP Reassembly.
 *
 * We use SYN to establish the initial sequence number of the first dns
 * message (tcpstate->start_seq).  We assume that no other segment can arrive
 * before the SYN (if one does, it is discarded, and the message it belongs to
 * will never be completely reassembled).
 *
 * When we see the first segment, we allocate a reassembly buffer.  If the
 * segment contained the dns message length, we use that as the buffer length;
 * otherwise (i.e., segments arrived out of order) we allocate the maximum
 * buffer length.
 *
 * We maintain a list of holes in the current message, so we can reassemble
 * segments of a single dns message even if they arrive out of order,
 * duplicated, or overlapping.  Once all the holes have been filled, we can
 * parse the dns message, and prepare to reassemble another.
 *
 * We assume that dns message length is always on a segment boundary.
 *
 * We assume segments from different messages will not arrive out of order;
 * that is, when reassembling one message, we will not receive segments from
 * another message.  If this does happen, the segment from the second message
 * is discarded, so that message will never be completely reassembled.
 *
 * TODO:
 * - handle RST 
 * - deallocate state for connections that have been idle too long
 * - handle out-of-order segments from different dns messages
 * - handle dns message length that is not on segment boundary
 */
dns_message *
handle_tcp(const inX_addr *src_ip_addr, const inX_addr *dst_ip_addr,
    const struct tcphdr *tcp, int len)
{
    dns_message *m = NULL;
    int offset = tcp->th_off << 2;
    uint32_t seq, segstart;
    uint32_t buflen;
    tcpstate_t *tcpstate = NULL;
    tcpHashkey_t key, *newkey;
    int i;
    char label[384];

    key.src_ip_addr = *src_ip_addr;
    key.dst_ip_addr = *dst_ip_addr;
    key.sport = nptohs(&tcp->th_sport);
    key.dport = nptohs(&tcp->th_dport);

    if (debug_flag) {
	char *p = label;
	inXaddr_ntop(src_ip_addr, p, 128);
	p += strlen(p);
	p += sprintf(p, ":%d ", key.sport);
	inXaddr_ntop(dst_ip_addr, p, 128);
	p += strlen(p);
	p += sprintf(p, ":%d ", key.dport);
    }

    if (debug_flag)
	fprintf(stderr, "handle_tcp(): %s\n", label);

    if (port53 != key.dport && port53 != key.sport)
	return NULL;

    if (NULL == tcpHash) {
        tcpHash = hash_create(MAX_TCP_STATE, tcp_hashfunc, tcp_cmpfunc,
	    free, tcpstate_free);
	if (NULL == tcpHash)
	    return NULL;
    }

    seq = nptohl(&tcp->th_seq);
    len -= offset; /* len = length of TCP payload */
    if (debug_flag)
	fprintf(stderr, "handle_tcp: %s seq = %u, len = %d", label, seq, len);

    tcpstate = hash_find(&key, tcpHash);
    if (debug_flag) {
	if (tcpstate)
	    fprintf(stderr, ", start_seq = %u, dnslen = %d, holes = %d\n", tcpstate->seq_start, tcpstate->dnslen, tcpstate->holes);
	else
	    fprintf(stderr, "\n");
    }

    if (!tcpstate && !(tcp->th_flags & TH_SYN)) {
	/* There's no existing state, and this is not the start of a stream.
	 * We have no way to synchronize with the stream, so we give up.
	 * (This commonly happens for the final ACK in response to a FIN.) */
	if (debug_flag)
	    fprintf(stderr, "handle_tcp: %s no state\n", label);
	return NULL;
    }

    if (tcp->th_flags & TH_SYN) {
	if (debug_flag)
	    fprintf(stderr, "handle_tcp: %s SYN at %u\n", label, seq);
	seq++; /* skip the syn */
	if (tcpstate) {
	    tcpstate_reset(tcpstate, seq + 2);
	} else {
	    tcpstate = xcalloc(1, sizeof(*tcpstate));
	    if (!tcpstate)
		return NULL;
	    tcpstate_reset(tcpstate, seq + 2);
	    newkey = xmalloc(sizeof(*newkey));
	    if (!newkey) {
		free(tcpstate);
		return NULL;
	    }
	    *newkey = key;
	    if (0 != hash_add(newkey, tcpstate, tcpHash)) {
		free(newkey);
		tcpstate_free(tcpstate);
		return NULL;
	    }
	}
    }

    if (len <= 0) /* there is no payload */
	goto done_payload;

    if (seq + sizeof(uint16_t) == tcpstate->seq_start) {
	/* payload is 2-byte length field plus 0 or more bytes of DNS message */
	if (len < sizeof(uint16_t)) {
	    /* makes no sense */
	    if (debug_flag)
		fprintf(stderr, "handle_tcp: %s nonsense len in first segment\n", label);
	    hash_remove(&key, tcpHash);
	    return NULL;
	}
	tcpstate->dnslen = nptohs((void*)tcp + offset);
	len -= sizeof(uint16_t);
	offset += sizeof(uint16_t);
	seq += sizeof(uint16_t);
	if (debug_flag)
	    fprintf(stderr, "handle_tcp: %s first segment; dnslen = %d\n", label, tcpstate->dnslen);
	/* Now we know length, so we can set length of final hole */
	for (i = 0; i < MAX_TCP_HOLES; i++) {
	    if (!tcpstate->hole[i].used)
		continue;
	    if (tcpstate->hole[i].start >= tcpstate->dnslen) {
		/* shouldn't happen */
		tcpstate->hole[i].used = 0;
		if (debug_flag)
		    fprintf(stderr, "handle_tcp: %s deleting excess hole %d\n", label, i);
	    } else if (tcpstate->hole[i].len == 0 ||
		tcpstate->hole[i].len >
		    tcpstate->dnslen - tcpstate->hole[i].start)
	    {
		tcpstate->hole[i].len =
		    tcpstate->dnslen - tcpstate->hole[i].start;
		if (debug_flag)
		    fprintf(stderr, "handle_tcp: %s truncating hole %d to %d\n", label, i, tcpstate->hole[i].len);
	    }
	}
	/* buf may have been already (over)allocated if segment is misordered
	 * or duplicated */
	tcpstate->buf = xrealloc(tcpstate->buf, tcpstate->dnslen);

	if (len <= 0) /* there is no more payload */
	    goto done_payload;
    }

    segstart = seq - tcpstate->seq_start;
    if (tcpstate->dnslen > 0) {
	buflen = tcpstate->dnslen;
    } else {
	buflen = MAX_DNS_LENGTH;
	if (NULL == tcpstate->buf)
	    tcpstate->buf = xmalloc(MAX_DNS_LENGTH);
    }

    if (segstart >= buflen) { /* payload would be outside buf */
	goto done_payload;
    }
    if (segstart + len > buflen) { /* payload would overflow */
	len = tcpstate->dnslen - segstart;
	if (debug_flag)
	    fprintf(stderr, "handle_tcp: %s truncating payload to %d\n", label, len);
    }

    /* Reassembly algorithm adapted from RFC 815. */
    for (i = 0; i < MAX_TCP_HOLES; i++) {
	tcphole_t *newhole;
	uint16_t hole_start, hole_len;
	if (!tcpstate->hole[i].used)
	    continue; /* hole descriptor is not in use */
	hole_start = tcpstate->hole[i].start;
	hole_len = tcpstate->hole[i].len;
	if (hole_len > 0 && segstart >= hole_start + hole_len)
	    continue; /* segment is totally after hole */
	if (segstart + len <= hole_start)
	    continue; /* segment is totally before hole */
	/* The segment overlaps this hole.  Delete the hole. */
	if (debug_flag)
	    fprintf(stderr, "handle_tcp: %s overlaping hole %d: %d %d\n", label, i, hole_start, hole_len);
	tcpstate->hole[i].used = 0; tcpstate->holes--;
	if (hole_len == 0) {
	    /* create a new infinite hole after the segment */
	    newhole = &tcpstate->hole[i]; /* hole[i] is guaranteed free */
	    newhole->used = 1; tcpstate->holes++;
	    newhole->start = segstart + len;
	    newhole->len = 0; /* infinite */
	    if (debug_flag)
		fprintf(stderr, "handle_tcp: %s new post-hole %d: %d %d\n", label, i, newhole->start, newhole->len);
	} else if (segstart + len < hole_start + hole_len) {
	    /* create a new finite hole after the segment (common case) */
	    newhole = &tcpstate->hole[i]; /* hole[i] is guaranteed free */
	    newhole->used = 1; tcpstate->holes++;
	    newhole->start = segstart + len;
	    newhole->len = (hole_start + hole_len) - newhole->start;
	    if (debug_flag)
		fprintf(stderr, "handle_tcp: %s new post-hole %d: %d %d\n", label, i, newhole->start, newhole->len);
	}
	if (segstart > hole_start) {
	    /* create a new hole before the segment */
	    int j;
	    for (j=0; ; j++) {
		if (j == MAX_TCP_HOLES)
		    return NULL; /* XXX */
		if (!tcpstate->hole[j].used) {
		    newhole = &tcpstate->hole[j];
		    newhole->used = 1; tcpstate->holes++;
		    break;
		}
	    }
	    newhole->start = hole_start;
	    newhole->len = segstart - hole_start;
	    if (debug_flag)
		fprintf(stderr, "handle_tcp: %s new pre-hole %d: %d %d\n", label, j, newhole->start, newhole->len);
	}
	if (segstart >= hole_start &&
	    (hole_len == 0 || segstart + len < hole_start + hole_len))
	{
	    /* The segment does not extend past hole boundaries; there is
	     * no need to look for other matching holes. */
	    break;
	}
    }

    /* copy payload to appropriate location in reassembly buffer */
    memcpy(&tcpstate->buf[segstart], (void *)tcp + offset, len);

    if (debug_flag)
	fprintf(stderr, "handle_tcp: %s holes remaining: %d\n", label, tcpstate->holes);

    if (tcpstate->holes == 0) {
	/* We now have a completely reassembled dns message */
	m = handle_dns(tcpstate->buf, tcpstate->dnslen);
	if (NULL != m)
	    m->src_port = nptohs(&tcp->th_sport);
	if (!tcpstate->fin)
	    tcpstate_reset(tcpstate, seq + len + 2); /* prep for another msg */
    }

done_payload:

    if (tcp->th_flags & TH_FIN && !tcpstate->fin) {
	/* End of tcp stream */
	if (debug_flag)
	    fprintf(stderr, "handle_tcp: %s FIN at %u\n", label, seq);
	tcpstate->fin = 1;
    }

    if (tcpstate->fin) {
	/* If there are no holes left, or one (infinite) hole for a predicted
	 * message that never actually started... */
	if (tcpstate->holes == 0 ||
	    (tcpstate->holes == 1 && tcpstate->dnslen == 0))
		hash_remove(&key, tcpHash);
    }

    return m;
}

dns_message *
handle_ipv4(const struct ip * ip, int len)
{
    dns_message *m;
    int offset = ip->ip_hl << 2;
    int iplen = nptohs(&ip->ip_len);
    inX_addr src_ip_addr, dst_ip_addr;
    ip_message *i = xcalloc(1, sizeof(*i));

    inXaddr_assign_v4(&src_ip_addr, &ip->ip_src);
    inXaddr_assign_v4(&dst_ip_addr, &ip->ip_dst);

    i->version = 4;
    i->src = src_ip_addr;
    i->dst = dst_ip_addr;
    i->proto = ip->ip_p;
    ip_message_callback(i);
    free(i);

    /* sigh, punt on IP fragments */
    if (nptohs(&ip->ip_off) & IP_OFFMASK)
	return NULL;

    if (IPPROTO_UDP == ip->ip_p) {
	m = handle_udp((struct udphdr *) ((void *)ip + offset), iplen - offset);
    } else if (IPPROTO_TCP == ip->ip_p) {
	m = handle_tcp(&src_ip_addr, &dst_ip_addr,
	    (struct tcphdr *) ((void *)ip + offset), iplen - offset);
    } else {
	return NULL;
    }
    if (NULL == m)
	return NULL;
    if (0 == m->qr)		/* query */
	m->client_ip_addr = src_ip_addr;
    else			/* reply */
	m->client_ip_addr = dst_ip_addr;
    return m;
}

#if USE_IPV6
dns_message *
handle_ipv6(const struct ip6_hdr * ip6, int len)
{
    dns_message *m;
    ip_message *i;
    int offset = sizeof(struct ip6_hdr);
    int nexthdr = ip6->ip6_nxt;
    inX_addr src_ip_addr, dst_ip_addr;
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
            return NULL;

        /* Cannot handle fragments. */
        if (IPPROTO_FRAGMENT == nexthdr)
            return NULL;

        ext_hdr = (ext_hdr_t*)((char *)ip6 + offset);
        nexthdr = ext_hdr->nexthdr;
        ext_hdr_len = (8 * (ext_hdr->length + 1));

        /* This header is longer than the packets payload.. WTF? */
        if (ext_hdr_len > payload_len)
            return NULL;

        offset += ext_hdr_len;
        payload_len -= ext_hdr_len;
    }                           /* while */

    i = xcalloc(1, sizeof(*i));
    i->version = 6;
    inXaddr_assign_v6(&src_ip_addr, &ip6->ip6_src);
    inXaddr_assign_v6(&dst_ip_addr, &ip6->ip6_dst);
    i->src = src_ip_addr;
    i->dst = dst_ip_addr;
    i->proto = nexthdr;
    ip_message_callback(i);
    free(i);

    /* Catch broken and empty packets */
    if (((offset + payload_len) > len)
        || (payload_len == 0)
        || (payload_len > PCAP_SNAPLEN))
        return NULL;

    if (IPPROTO_UDP == nexthdr) {
	m = handle_udp((struct udphdr *) ((char *) ip6 + offset), payload_len);
    } else if (IPPROTO_TCP == nexthdr) {
	m = handle_tcp(&src_ip_addr, &dst_ip_addr,
	    (struct tcphdr *) ((char *) ip6 + offset), payload_len);
    } else {
	return NULL;
    }

    if (NULL == m)
	return NULL;

    if (0 == m->qr)		/* query */
	m->client_ip_addr = src_ip_addr;
    else			/* reply */
	m->client_ip_addr = dst_ip_addr;
    return m;
}
#endif /* USE_IPV6 */

dns_message *
handle_ip(const struct ip * ip, int len)
{
    /* note: ip->ip_v does not work if header is not int-aligned */
    switch((*(uint8_t*)ip) >> 4) {
    case 4:
        return handle_ipv4(ip, len);
	break;
#if USE_IPV6
    case 6:
        return handle_ipv6((struct ip6_hdr *)ip, len);
	break;
#endif
    default:
	return NULL;
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
dns_message *
handle_ppp(const u_char * pkt, int len)
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
    if (is_ethertype_ip(proto)) {
        return handle_ip((struct ip *) pkt, len);
    }
    return NULL;
}

#endif

dns_message *
handle_null(const u_char * pkt, int len)
{
    unsigned int family;
    memcpy(&family, pkt, sizeof(family));
    if (is_family_inet(family))
	return handle_ip((struct ip *) (pkt + 4), len - 4);
    return NULL;
}

#ifdef DLT_LOOP
dns_message *
handle_loop(const u_char * pkt, int len)
{
    unsigned int family;
    memcpy(&family, pkt, sizeof(family));
    if (is_family_inet(family))
	return handle_ip((struct ip *) (pkt + 4), len - 4);
    return NULL;
}

#endif

#ifdef DLT_RAW
dns_message *
handle_raw(const u_char * pkt, int len)
{
    return handle_ip((struct ip *) pkt, len);
}

#endif

int
match_vlan(const char *pkt)
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

dns_message *
handle_ether(const u_char * pkt, int len)
{
    struct ether_header *e = (void *) pkt;
    unsigned short etype = nptohs(&e->ether_type);
    if (len < ETHER_HDR_LEN)
	return NULL;
    pkt += ETHER_HDR_LEN;
    len -= ETHER_HDR_LEN;
    if (ETHERTYPE_8021Q == etype) {
	if (!match_vlan(pkt))
	    return NULL;
	etype = nptohs((unsigned short *) (pkt + 2));
	pkt += 4;
	len -= 4;
    }
    if (len < 0)
	return NULL;
    if (is_ethertype_ip(etype)) {
	return handle_ip((struct ip *) pkt, len);
    }
    return NULL;
}

void
handle_pcap(u_char * udata, const struct pcap_pkthdr *hdr, const u_char * pkt)
{
    dns_message *m;

#if 0 /* enable this to test code with unaligned headers */
    char buf[PCAP_SNAPLEN+1];
    memcpy(buf+1, pkt, hdr->caplen);
    pkt = buf+1;
#endif

    last_ts = hdr->ts;
#if 0
    if (debug_flag)
	fprintf(stderr, "handle_pcap()\n");
#endif
    if (hdr->caplen < ETHER_HDR_LEN)
	return;
    m = handle_datalink(pkt, hdr->caplen);
    if (NULL == m)
	return;
    m->ts = hdr->ts;
    dns_message_callback(m);
    free(m);
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
    if (!pcap_file(new_pcap)) {
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
    gettimeofday(&start_ts, NULL);
    finish_ts.tv_sec = ((start_ts.tv_sec / 60) + 1) * 60;
    finish_ts.tv_usec = 0;
    while (last_ts.tv_sec < finish_ts.tv_sec) {
	fd_set *R = Pcap_select(&pcap_fdset, 0, 250000);
	if (NULL == R) {
	    gettimeofday(&last_ts, NULL);
	}
	for (i = 0; i < n_pcap; i++) {
	    if (pcap_file(pcap[i]) || /* offline savefile */
		FD_ISSET(pcap_fileno(pcap[i]), &pcap_fdset)) /* ready device */
	    {
		pcap_dispatch(pcap[i], 50, handle_pcap, NULL);
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
