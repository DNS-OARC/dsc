/*
 * $Id: pcap_layers.c,v 1.9 2009/11/09 21:01:23 wessels Exp $
 * 
 * Copyright (c) 2007, The Measurement Factory, Inc.  All rights reserved.  See
 * the LICENSE file for details.
 */

#define _BSD_SOURCE 1

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

#define USE_IPV6 1

#include "byteorder.h"
#include "pcap_layers.h"

#define PCAP_SNAPLEN 1460
#ifndef ETHER_HDR_LEN
#define ETHER_ADDR_LEN 6
#define ETHER_TYPE_LEN 2
#define ETHER_HDR_LEN (ETHER_ADDR_LEN * 2 + ETHER_TYPE_LEN)
#endif
#ifndef ETHERTYPE_8021Q
#define ETHERTYPE_8021Q 0x8100
#endif

#if USE_PPP
#include <net/if_ppp.h>
#define PPP_ADDRESS_VAL       0xff	/* The address byte value */
#define PPP_CONTROL_VAL       0x03	/* The control byte value */
#endif

#ifndef IP_OFFMASK
#define IP_OFFMASK 0x1fff
#endif

#define XMIN(a,b) ((a)<(b)?(a):(b))

typedef struct _ipV4Frag {
    uint32_t offset;
    uint32_t len;
    char *buf;
    struct _ipV4Frag *next;
    u_char more;
} ipV4Frag;

typedef struct _ipV4Flow {
    uint16_t ip_id;
    uint8_t ip_p;
    struct in_addr src;
    struct in_addr dst;
    struct _ipV4Flow *next;
    ipV4Frag *frags;
} ipV4Flow;

static ipV4Flow *ipV4Flows = NULL;
static int _reassemble_fragments = 0;

static void (*handle_datalink) (const u_char * pkt, int len, void *userdata)= NULL;

int (*callback_ether) (const u_char * pkt, int len, void *userdata)= NULL;
int (*callback_vlan) (unsigned short vlan, void *userdata)= NULL;
int (*callback_ipv4) (const struct ip *ipv4, int len, void *userdata)= NULL;
int (*callback_ipv6) (const struct ip6_hdr *ipv6, int len, void *userdata)= NULL;
int (*callback_gre) (const u_char *pkt, int len, void *userdata)= NULL;
int (*callback_tcp) (const struct tcphdr *tcp, int len, void *userdata)= NULL;
int (*callback_udp) (const struct udphdr *udp, int len, void *userdata)= NULL;
int (*callback_tcp_sess) (const struct tcphdr *tcp, int len, void *userdata, l7_callback *)= NULL;
int (*callback_l7) (const u_char * l7, int len, void *userdata)= NULL;

/* need prototypes for GRE recursion */
void handle_ip(const struct ip *ip, int len, void *userdata);
static int is_ethertype_ip(unsigned short proto);

void
handle_l7(const u_char * pkt, int len, void *userdata)
{
    if (callback_l7)
	callback_l7(pkt, len, userdata);
}

void
handle_tcp_session(const struct tcphdr *tcp, int len, void *userdata)
{
    if (callback_tcp_sess)
	callback_tcp_sess(tcp, len, userdata, callback_l7);
    else if (callback_l7)
	callback_l7((u_char *) tcp + (tcp->th_off<<2), len - (tcp->th_off<<2), userdata);
}

void
handle_udp(const struct udphdr *udp, int len, void *userdata)
{
    if (len < sizeof(*udp))
	return;
    if (callback_udp)
	if (0 != callback_udp(udp, len, userdata))
	    return;
    handle_l7((u_char *) (udp + 1), len - sizeof(*udp), userdata);
}

void
handle_tcp(const struct tcphdr *tcp, int len, void *userdata)
{
    if (len < sizeof(*tcp))
	return;
    if (callback_tcp)
	if (0 != callback_tcp(tcp, len, userdata))
	    return;
    handle_tcp_session(tcp, len, userdata);
}

void
handle_ipv4_fragment(const struct ip *ip, int len, void *userdata)
{
    ipV4Flow *l = NULL;
    ipV4Flow **L = NULL;
    ipV4Frag *f = NULL;
    ipV4Frag *nf = NULL;
    ipV4Frag **F = NULL;
    uint16_t ip_off = ntohs(ip->ip_off);
    uint32_t s = 0;
    char *newbuf = NULL;
    if (ip_off & IP_OFFMASK) {
	for (l = ipV4Flows; l; l = l->next) {
	    if (l->ip_id != ntohs(ip->ip_id))
		continue;
	    if (l->src.s_addr != ip->ip_src.s_addr)
		continue;
	    if (l->dst.s_addr != ip->ip_dst.s_addr)
		continue;
	    if (l->ip_p != ip->ip_p)
		continue;
	    break;
	}
#if DEBUG
	if (l)
	    fprintf(stderr, "found saved flow for i=%hx s=%x d=%x p=%d\n", l->ip_id, l->src.s_addr, l->dst.s_addr, l->ip_p);
#endif
    } else {
	l = calloc(1, sizeof(*l));
	assert(l);
	l->ip_id = ntohs(ip->ip_id);
	l->ip_p = ip->ip_p;
	l->src = ip->ip_src;
	l->dst = ip->ip_dst;
	l->next = ipV4Flows;
	ipV4Flows = l;
#if DEBUG
	fprintf(stderr, "created saved flow for i=%hx s=%x d=%x p=%d\n", l->ip_id, l->src.s_addr, l->dst.s_addr, l->ip_p);
#endif
    }
    if (NULL == l)		/* didn't find or couldn't create state */
	return;
    /*
     * Store new frag
     */
    f = calloc(1, sizeof(*f));
    assert(f);
    f->offset = (ip_off & IP_OFFMASK) << 3;
    f->len = ntohs(ip->ip_len);
    f->buf = malloc(f->len);
    f->more = (ip_off & IP_MF) ? 1 : 0;
    assert(f->buf);
    memcpy(f->buf, (char *)ip + (ip->ip_hl << 2), f->len);
    /*
     * Insert frag into list ordered by offset
     */
    for (F = &l->frags; *F && ((*F)->offset < f->offset); F = &(*F)->next);
    f->next = *F;
    *F = f;
#if DEBUG
    fprintf(stderr, "saved frag o=%u l=%u\n", f->offset, f->len);
#endif
    /*
     * Do we have the whole packet?
     */
    for (f = l->frags; f; f = f->next) {
#if DEBUG
	fprintf(stderr, " frag %u:%u mf=%d\n", f->offset, f->len, f->more);
#endif
	if (f->offset > s)	/* gap */
	    return;
	s = f->offset + f->len;
	if (!f->more)
	    break;
    }
    if (NULL == f)		/* didn't find last frag */
	return;
#if DEBUG
    fprintf(stderr, "have whole packet s=%u, mf=%u\n", s, f->more);
#endif
    /*
     * Reassemble, free, deliver
     */
    newbuf = malloc(s);
    nf = l->frags;
    while ((f = nf)) {
	nf = f->next;
	memcpy(newbuf + f->offset, f->buf, f->len);
	free(f->buf);
	free(f);
    }
    for (L = &ipV4Flows; *L; L = &(*L)->next) {
	if (*L == l) {
	    *L = (*L)->next;
	    free(l);
	    break;
	}
    }
#if DEBUG
    fprintf(stderr, "delivering reassmebled packet\n");
#endif
    if (IPPROTO_UDP == ip->ip_p) {
	handle_udp((struct udphdr *)newbuf, s, userdata);
    } else if (IPPROTO_TCP == ip->ip_p) {
	handle_tcp((struct tcphdr *)newbuf, s, userdata);
    }
#if DEBUG
    fprintf(stderr, "freeing newbuf\n");
#endif
    free(newbuf);
}

void
handle_gre(const u_char * gre, int len, void *userdata)
{
    int grelen = 4;
    unsigned short flags = nptohs(gre);
    unsigned short etype = nptohs(gre + 2);
    if (len < grelen)
	return;
    if (callback_gre)
	callback_gre(gre, len, userdata);
    if (flags & 0x0001)		/* checksum present? */
	grelen += 4;
    if (flags & 0x0004)		/* key present? */
	grelen += 4;
    if (flags & 0x0008)		/* sequence number present? */
	grelen += 4;
    if (is_ethertype_ip(etype))
	handle_ip((struct ip *) (gre + grelen), len - grelen, userdata);
}

/*
 * When passing on to the next layers, use the ip_len
 * value for the length, unless the given len happens to
 * to be less for some reason.  Note that ip_len might
 * be less than len due to Ethernet padding.
 */
void
handle_ipv4(const struct ip *ip, int len, void *userdata)
{
    int offset;
    int iplen;
    uint16_t ip_off;
    if (len < sizeof(*ip))
	return;
    offset = ip->ip_hl << 2;
    iplen = XMIN(nptohs(&ip->ip_len), len);
    if (callback_ipv4)
	if (0 != callback_ipv4(ip, iplen, userdata))
	    return;
    ip_off = ntohs(ip->ip_off);
    if ((ip_off & (IP_OFFMASK | IP_MF)) && _reassemble_fragments) {
	handle_ipv4_fragment(ip, iplen, userdata);
    } else if (IPPROTO_UDP == ip->ip_p) {
	handle_udp((struct udphdr *)((char *)ip + offset), iplen - offset, userdata);
    } else if (IPPROTO_TCP == ip->ip_p) {
	handle_tcp((struct tcphdr *)((char *)ip + offset), iplen - offset, userdata);
    } else if (IPPROTO_GRE == ip->ip_p) {
	handle_gre((u_char *)ip + offset, iplen - offset, userdata);
    }
}

void
handle_ipv6(const struct ip6_hdr *ip6, int len, void *userdata)
{
    int offset;
    int nexthdr;
    uint16_t payload_len;

    if (len < sizeof(*ip6))
	return;
    if (callback_ipv6)
	if (0 != callback_ipv6(ip6, len, userdata))
	    return;

    offset = sizeof(struct ip6_hdr);
    nexthdr = ip6->ip6_nxt;
    payload_len = nptohs(&ip6->ip6_plen);

    /*
     * Parse extension headers. This only handles the standard headers, as
     * defined in RFC 2460, correctly. Fragments are discarded.
     */
    while ((IPPROTO_ROUTING == nexthdr)	/* routing header */
	||(IPPROTO_HOPOPTS == nexthdr)	/* Hop-by-Hop options. */
	||(IPPROTO_FRAGMENT == nexthdr)	/* fragmentation header. */
	||(IPPROTO_DSTOPTS == nexthdr)	/* destination options. */
	||(IPPROTO_DSTOPTS == nexthdr)	/* destination options. */
	||(IPPROTO_AH == nexthdr)	/* destination options. */
	||(IPPROTO_ESP == nexthdr)) {	/* encapsulating security payload. */
	typedef struct {
	    uint8_t nexthdr;
	    uint8_t length;
	}      ext_hdr_t;
	ext_hdr_t *ext_hdr;
	uint16_t ext_hdr_len;

	/* Catch broken packets */
	if ((offset + sizeof(ext_hdr)) > len)
	    return;

	/* Cannot handle fragments. */
	if (IPPROTO_FRAGMENT == nexthdr)
	    return;

	ext_hdr = (ext_hdr_t *) ((char *)ip6 + offset);
	nexthdr = ext_hdr->nexthdr;
	ext_hdr_len = (8 * (ext_hdr->length + 1));

	/* This header is longer than the packets payload.. WTF? */
	if (ext_hdr_len > payload_len)
	    return;

	offset += ext_hdr_len;
	payload_len -= ext_hdr_len;
    }				/* while */

    /* Catch broken and empty packets */
    if (((offset + payload_len) > len)
	|| (payload_len == 0)
	|| (payload_len > PCAP_SNAPLEN))
	return;

    if (IPPROTO_UDP == nexthdr) {
	handle_udp((struct udphdr *)((char *)ip6 + offset), payload_len, userdata);
    } else if (IPPROTO_TCP == nexthdr) {
	handle_tcp((struct tcphdr *)((char *)ip6 + offset), payload_len, userdata);
    } else if (IPPROTO_GRE == nexthdr) {
	handle_gre((u_char *)ip6 + offset, payload_len, userdata);
    }
}

void
handle_ip(const struct ip *ip, int len, void *userdata)
{
    /* note: ip->ip_v does not work if header is not int-aligned */
    /* fprintf(stderr, "IPv %d\n", (*(uint8_t *) ip) >> 4); */
    switch ((*(uint8_t *) ip) >> 4) {
	case 4:
	handle_ipv4(ip, len, userdata);
	break;
    case 6:
	handle_ipv6((struct ip6_hdr *)ip, len, userdata);
	break;
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
handle_ppp(const u_char * pkt, int len, void *userdata)
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
	handle_ip((struct ip *)pkt, len, userdata);
}

#endif

void
handle_null(const u_char * pkt, int len, void *userdata)
{
    unsigned int family = nptohl(pkt);
    if (is_family_inet(family))
	handle_ip((struct ip *)(pkt + 4), len - 4, userdata);
}

#ifdef DLT_LOOP
void
handle_loop(const u_char * pkt, int len, void *userdata)
{
    unsigned int family = nptohl(pkt);
    if (is_family_inet(family))
	handle_ip((struct ip *)(pkt + 4), len - 4, userdata);
}

#endif

#ifdef DLT_RAW
void
handle_raw(const u_char * pkt, int len, void *userdata)
{
    handle_ip((struct ip *)pkt, len, userdata);
}

#endif

void
handle_ether(const u_char * pkt, int len, void *userdata)
{
    struct ether_header *e = (struct ether_header *)pkt;
    unsigned short etype;
    if (len < ETHER_HDR_LEN)
	return;
    etype = nptohs(&e->ether_type);
    if (callback_ether)
	if (0 != callback_ether(pkt, len, userdata))
	    return;
    pkt += ETHER_HDR_LEN;
    len -= ETHER_HDR_LEN;
    if (ETHERTYPE_8021Q == etype) {
	unsigned short vlan = nptohs((unsigned short *) pkt);
	if (callback_vlan)
	    if (0 != callback_vlan(vlan, userdata))
		return;
	etype = nptohs((unsigned short *)(pkt + 2));
	pkt += 4;
	len -= 4;
    }
    if (len < 0)
	return;
    /* fprintf(stderr, "Ethernet packet of len %d ethertype %#04x\n", len, etype); */
    if (is_ethertype_ip(etype)) {
	handle_ip((struct ip *)pkt, len, userdata);
    }
}

void
handle_pcap(u_char * userdata, const struct pcap_pkthdr *hdr, const u_char * pkt)
{
    if (hdr->caplen < ETHER_HDR_LEN)
	return;
    handle_datalink(pkt, hdr->caplen, userdata);
}


int
pcap_layers_init(int dlt, int reassemble)
{
    switch (dlt) {
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
	fprintf(stderr, "unsupported data link type %d", dlt);
	exit(1);
	break;
    }
    _reassemble_fragments = reassemble;
    return 0;
}
