/*
 * $Id$
 *
 * http://dnstop.measurement-factory.com/
 *
 * Copyright (c) 2002, The Measurement Factory, Inc.  All rights
 * reserved.  See the LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "dns_message.h"
#include "pcap.h"

char *progname = NULL;
int promisc_flag = 1;

void
usage(void)
{
    fprintf(stderr, "usage: %s [opts] netdevice|savefile\n",
	progname);
    fprintf(stderr, "\t-a\tAnonymize IP Addrs\n");
    fprintf(stderr, "\t-b expr\tBPF program code\n");
    fprintf(stderr, "\t-i addr\tIgnore this source IP address\n");
    fprintf(stderr, "\t-p\tDon't put interface in promiscuous mode\n");
    fprintf(stderr, "\t-s\tEnable 2nd level domain stats collection\n");
    exit(1);
}

int
main(int argc, char *argv[])
{
    int x;
    char *device = NULL;
    extern char *bpf_program_str;
    extern DMC dns_message_handle;

    progname = strdup(strrchr(argv[0], '/') ? strchr(argv[0], '/') + 1 : argv[0]);
    srandom(time(NULL));

    while ((x = getopt(argc, argv, "b:p")) != -1) {
	switch (x) {
	case 'p':
	    promisc_flag = 0;
	    break;
	case 'b':
	    bpf_program_str = strdup(optarg);
	    break;
	default:
	    usage();
	    break;
	}
    }
    argc -= optind;
    argv += optind;

    if (argc < 1)
	usage();
    device = strdup(argv[0]);

    /*
     * I'm using fork() in this loop, (a) out of laziness, and (b)
     * because I'm worried we might drop packets.  Making sure each
     * child collector runs for a small amount of time (60 secodns)
     * means I can be lazy about memory management (leaks).  To
     * minimize the chance for dropped packets, I'd like to spawn
     * a new collector as soon as (or even before) the current
     * collector exits.
     */

    Pcap_init(device, promisc_flag);
    for (;;) {
	pid_t cpid = fork();
	if (0 == cpid) {
	    dns_message_init();
	    Pcap_run(dns_message_handle);
	    dns_message_report();
	    _exit(0);
	} else {
	    int cstatus = 0;
	    fprintf(stderr, "waiting for child pid %d\n", (int) cpid);
	    while (waitpid(cpid, &cstatus, 0) < 0)
		(void) 0;
	}
    }
    Pcap_close();
    return 0;
}
