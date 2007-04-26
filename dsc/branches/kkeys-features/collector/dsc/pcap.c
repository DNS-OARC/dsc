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

dns_message *
handle_tcp(const struct tcphdr *tcp, int len)
{
    dns_message *m;
    int offset = tcp->th_off << 2;
    uint16_t dnslen;

    if (port53 != nptohs(&tcp->th_dport) && port53 != nptohs(&tcp->th_sport))
	return NULL;
    len -= offset + sizeof(dnslen);
    if (len <= sizeof(dnslen))
	return NULL;
    dnslen = nptohs((void*)tcp + offset);
    if (dnslen != len) /* not a single-packet message */
	return NULL;
    m = handle_dns((void *)tcp + offset + sizeof(dnslen), dnslen);
    if (NULL == m)
	return NULL;
    m->src_port = nptohs(&tcp->th_sport);
    return m;
}

dns_message *
handle_ipv4(const struct ip * ip, int len)
{
    dns_message *m;
    int offset = ip->ip_hl << 2;
    ip_message *i = xcalloc(1, sizeof(*i));

    i->version = 4;
    inXaddr_assign_v4(&i->src, &ip->ip_src);
    inXaddr_assign_v4(&i->dst, &ip->ip_dst);
    i->proto = ip->ip_p;
    ip_message_callback(i);
    free(i);

    /* sigh, punt on IP fragments */
    if (nptohs(&ip->ip_off) & IP_OFFMASK)
	return NULL;

    if (IPPROTO_UDP == ip->ip_p) {
	m = handle_udp((struct udphdr *) ((void *)ip + offset), len - offset);
    } else if (IPPROTO_TCP == ip->ip_p) {
	m = handle_tcp((struct tcphdr *) ((void *)ip + offset), len - offset);
    } else {
	return NULL;
    }
    if (NULL == m)
	return NULL;
    if (0 == m->qr)		/* query */
	inXaddr_assign_v4(&m->client_ip_addr, &ip->ip_src);
    else			/* reply */
	inXaddr_assign_v4(&m->client_ip_addr, &ip->ip_dst);
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
    inXaddr_assign_v6(&i->src, &ip6->ip6_src);
    inXaddr_assign_v6(&i->dst, &ip6->ip6_dst);
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
	m = handle_tcp((struct tcphdr *) ((char *) ip6 + offset), payload_len);
    } else {
	return NULL;
    }

    if (NULL == m)
	return NULL;

    if (0 == m->qr)		/* query */
	inXaddr_assign_v6(&m->client_ip_addr, &ip6->ip6_src);
    else			/* reply */
	inXaddr_assign_v6(&m->client_ip_addr, &ip6->ip6_dst);
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
