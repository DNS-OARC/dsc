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

#include <syslog.h>
#include <stdarg.h>

#include "dns_message.h"
#include "ip_message.h"
#include "pcap.h"

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
    char buf[PCAP_SNAPLEN];
    dns_message *m;
    if (port53 != udp->uh_dport && port53 != udp->uh_sport)
	return NULL;
    memcpy(buf, udp + 1, len - sizeof(*udp));
    m = handle_dns(buf, len - sizeof(*udp));
    if (NULL == m)
	return NULL;
    return m;
}

dns_message *
handle_ip(const struct ip * ip, int len)
{
    char buf[PCAP_SNAPLEN];
    dns_message *m;
    int offset = ip->ip_hl << 2;
    ip_message_callback(ip);
    if (IPPROTO_UDP != ip->ip_p)
	return NULL;
    memcpy(buf, (void *) ip + offset, len - offset);
    m = handle_udp((struct udphdr *) buf, len - offset);
    if (NULL == m)
	return NULL;
    if (0 == m->qr)		/* query */
	m->client_ipv4_addr = ip->ip_src;
    else			/* reply */
	m->client_ipv4_addr = ip->ip_dst;
    return m;
}

#if USE_PPP
dns_message *
handle_ppp(const u_char * pkt, int len)
{
    char buf[PCAP_SNAPLEN];
    unsigned short us;
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
	memcpy(&us, pkt, sizeof(us));
	proto = ntohs(us);
	pkt += 2;
	len -= 2;
    }
    if (ETHERTYPE_IP != proto && PPP_IP != proto)
	return NULL;
    memcpy(buf, pkt, len);
    return handle_ip((struct ip *) buf, len);
}

#endif

dns_message *
handle_null(const u_char * pkt, int len)
{
    unsigned int family;
    memcpy(&family, pkt, sizeof(family));
    if (AF_INET != family)
	return NULL;
    return handle_ip((struct ip *) (pkt + 4), len - 4);
}

#ifdef DLT_LOOP
dns_message *
handle_loop(const u_char * pkt, int len)
{
    unsigned int family;
    memcpy(&family, pkt, sizeof(family));
    if (AF_INET != ntohl(family))
	return NULL;
    return handle_ip((struct ip *) (pkt + 4), len - 4);
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
    for (i = 0; i < n_vlan_ids; i++)
	if (vlan_ids[i] == vlan)
	    return 1;
    return 0;
}

dns_message *
handle_ether(const u_char * pkt, int len)
{
    char buf[PCAP_SNAPLEN];
    struct ether_header *e = (void *) pkt;
    unsigned short etype = ntohs(e->ether_type);
    if (len < ETHER_HDR_LEN)
	return NULL;
    pkt += ETHER_HDR_LEN;
    len -= ETHER_HDR_LEN;
    if (ETHERTYPE_8021Q == etype) {
	if (!match_vlan(pkt))
	    return NULL;
	etype = ntohs(*(unsigned short *) (pkt + 2));
	pkt += 4;
	len -= 4;
    }
    if (len < 0)
	return NULL;
    if (ETHERTYPE_IP == etype) {
	memcpy(buf, pkt, len);
	return handle_ip((struct ip *) buf, len);
    }
    return NULL;
}

void
handle_pcap(u_char * udata, const struct pcap_pkthdr *hdr, const u_char * pkt)
{
    dns_message *m;
    last_ts = hdr->ts;
    if (hdr->caplen < ETHER_HDR_LEN)
	return;
    m = handle_datalink(pkt, hdr->caplen);
    if (NULL == m)
	return;
    m->ts = hdr->ts;
    dns_message_callback(m);
    free(m);
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
	pcap = calloc(MAX_N_PCAP, sizeof(*pcap));
	FD_ZERO(&pcap_fdset);
    }
    assert(pcap);
    assert(n_pcap < MAX_N_PCAP);

    port53 = htons(53);
    last_ts.tv_sec = last_ts.tv_usec = 0;

    if (0 == stat(device, &sb))
	readfile_state = 1;
    if (readfile_state) {
	new_pcap = pcap_open_offline(device, errbuf);
    } else {
	new_pcap = pcap_open_live((char *) device, PCAP_SNAPLEN, promisc, 1000, errbuf);
    }
    if (NULL == new_pcap) {
	syslog(LOG_ERR, "pcap_open_*: %s\n", errbuf);
	exit(1);
    }
    memset(&fp, '\0', sizeof(fp));
    x = pcap_compile(new_pcap, &fp, bpf_program_str, 1, 0);
    if (x < 0) {
	syslog(LOG_ERR, "pcap_compile failed\n");
	exit(1);
    }
    x = pcap_setfilter(new_pcap, &fp);
    if (x < 0) {
	syslog(LOG_ERR, "pcap_setfilter failed\n");
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
	syslog(LOG_ERR, "unsupported data link type %d\n",
	    pcap_datalink(new_pcap));
	exit(1);
	break;
    }
    FD_SET(pcap_fileno(new_pcap), &pcap_fdset);
    max_pcap_fds = pcap_fileno(new_pcap) + 1;	/* XXX FDs increment */
    pcap[n_pcap++] = new_pcap;
}

void
Pcap_run(DMC * dns_callback, IPC * ip_callback)
{
    dns_message_callback = dns_callback;
    ip_message_callback = ip_callback;
    gettimeofday(&start_ts, NULL);
    finish_ts.tv_sec = ((start_ts.tv_sec / 60) + 1) * 60;
    finish_ts.tv_usec = 0;
    while (last_ts.tv_sec < finish_ts.tv_sec) {
	fd_set *R = Pcap_select(&pcap_fdset, 0, 250000);
	if (NULL == R) {
	    gettimeofday(&last_ts, NULL);
	} else {
	    int i;
	    for (i = 0; i < n_pcap; i++)
		if (FD_ISSET(pcap_fileno(pcap[i]), &pcap_fdset))
		    pcap_dispatch(pcap[i], 50, handle_pcap, NULL);
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
