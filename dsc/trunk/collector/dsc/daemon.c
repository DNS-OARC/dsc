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
    char *bpf_program_str = "port 53";
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

    dns_message_init();
    Pcap_init(device, promisc_flag);
    Pcap_run(dns_message_handle);
    Pcap_close();
    return 0;
}
