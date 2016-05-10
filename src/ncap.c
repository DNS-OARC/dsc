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

#if HAVE_LIBNCAP

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <ncap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <netinet/tcp.h>

#include <syslog.h>
#include <stdarg.h>
#include <errno.h>

#include "xmalloc.h"
#include "dns_message.h"
#include "ncap.h"
#include "byteorder.h"
#include "syslog_debug.h"

#define NCAP_SNAPLEN 70000

static unsigned short port53;

extern void handle_dns(const u_char * buf, uint16_t len, transport_message * tm, DMC * dns_message_callback);
extern int debug_flag;
extern char *bpf_program_str;        /* from pcap.c */
static DMC *dns_message_callback;
static struct timespec last_ts;
static struct timespec start_ts;
static struct timespec finish_ts;
#define MAX_VLAN_IDS 100
static int n_vlan_ids = 0;
static int vlan_ids[MAX_VLAN_IDS];
ncap_t NC = NULL;

static void
handle_ncap(ncap_t nc, void *udata, ncap_msg_ct msg, const char *wtf)
{
    transport_message tm;

    memset(&tm, '\0', sizeof(tm));
    last_ts = msg->ts;
    if (ncap_ip4 == msg->np) {
        tm.ip_version = 4;
        inXaddr_assign_v4(&tm.src_ip_addr, &msg->npu.ip4.src);
        inXaddr_assign_v4(&tm.dst_ip_addr, &msg->npu.ip4.dst);
    } else if (ncap_ip6 == msg->np) {
        tm.ip_version = 6;
        inXaddr_assign_v6(&tm.src_ip_addr, &msg->npu.ip6.src);
        inXaddr_assign_v6(&tm.dst_ip_addr, &msg->npu.ip6.dst);
    }

    if (ncap_udp == msg->tp) {
        tm.proto = i.proto = IPPROTO_UDP;
        tm.src_port = (u_short) nptohl(&msg->tpu.udp.sport);
        tm.dst_port = (u_short) nptohl(&msg->tpu.udp.dport);
    } else if (ncap_tcp == msg->tp) {
        tm.proto = i.proto = IPPROTO_TCP;
        tm.src_port = (u_short) nptohl(&msg->tpu.tcp.sport);
        tm.dst_port = (u_short) nptohl(&msg->tpu.tcp.dport);
    }

    if (i.proto)
        handle_dns(msg->payload, msg->paylen, &tm, dns_message_callback);
}



/* ========================================================================= */





void
Ncap_init(const char *device, int promisc)
{
    ncap_result_e r;
    struct stat sb;

    if (NULL == NC) {
        NC = ncap_create(NCAP_SNAPLEN);
        if (NULL == NC) {
            dsyslogf(LOG_ERR, "ncap_create*: %s", strerror(errno));
            exit(1);
        }
    }

    port53 = 53;
    last_ts.tv_sec = last_ts.tv_nsec = 0;
    finish_ts.tv_sec = finish_ts.tv_nsec = 0;

    if (0 == stat(device, &sb)) {
        FILE *fp = fopen(device, "r");
        if (NULL == fp) {
            dsyslogf(LOG_ERR, "fopen %s: %s", device, strerror(errno));
            exit(1);
        }
        r = NC->add_fp(NC, fp, ncap_ncap, device);
    } else {
        r = NC->add_if(NC, device, bpf_program_str, promisc, vlan_ids, n_vlan_ids, NULL);
    }
    if (ncap_failure == r) {
        dsyslogf(LOG_ERR, "NC->add_*() error: %s", "some error");
        exit(1);
    }
}

extern int sig_while_processing;

int
Ncap_run(DMC * dns_callback)
{
    int result = 1;
    extern uint64_t statistics_interval;

    dns_message_callback = dns_callback;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    TIMEVAL_TO_TIMESPEC(&tv, &start_ts);
    finish_ts.tv_sec = ((start_ts.tv_sec / statistics_interval) + 1) * statistics_interval;
    finish_ts.tv_nsec = 0;
    while (last_ts.tv_sec < finish_ts.tv_sec && !sig_while_processing) {
        NC->collect(NC, 1, handle_ncap, NULL);
    }

    if (sig_while_processing)
        finish_ts = last_ts;

    return result;
}

void
Ncap_close(void)
{
    NC->destroy(NC);
}

int
Ncap_start_time(void)
{
    return (int) start_ts.tv_sec;
}

int
Ncap_finish_time(void)
{
    return (int) finish_ts.tv_sec;
}

#endif /* HAVE_LIBNCAP */
