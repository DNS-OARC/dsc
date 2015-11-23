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
#include <syslog.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include "xmalloc.h"
#include "dns_message.h"
#include "ip_message.h"
#include "pcap.h"
#include "syslog_debug.h"

char *progname = NULL;
char *pid_file_name = NULL;
int promisc_flag = 1;
int debug_flag = 0;

extern void cip_net_indexer_init(void);
extern void ParseConfig(const char *);

void
daemonize(void)
{
    int fd;
    pid_t pid;
    if ((pid = fork()) < 0) {
	syslog(LOG_ERR, "fork failed: %s", strerror(errno));
	exit(1);
    }
    if (pid > 0)
	exit(0);
    if (setsid() < 0)
	syslog(LOG_ERR, "setsid failed: %s", strerror(errno));
    closelog();
#ifdef TIOCNOTTY
    if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
	ioctl(fd, TIOCNOTTY, NULL);
	close(fd);
    }
#endif
    fd = open("/dev/null", O_RDWR);
    if (fd < 0) {
	syslog(LOG_ERR, "/dev/null: %s\n", strerror(errno));
    } else {
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
	close(fd);
    }
    openlog(progname, LOG_PID | LOG_NDELAY, LOG_DAEMON);
}

void
write_pid_file(void)
{
    FILE *fp;
    if (NULL == pid_file_name)
	return;
    syslog(LOG_INFO, "writing PID to %s", pid_file_name);
    fp = fopen(pid_file_name, "w");
    if (NULL == fp) {
        perror(pid_file_name);
        syslog(LOG_ERR, "fopen: %s: %s", pid_file_name, strerror(errno));
	return;
    }
    fprintf(fp, "%d\n", getpid());
    fclose(fp);
}

void
usage(void)
{
    fprintf(stderr, "usage: %s [opts] dsc.conf\n",
	progname);
    fprintf(stderr, "\t-p\tDon't put interface in promiscuous mode\n");
    fprintf(stderr, "\t-d\tDebug mode.  Exits after first write\n");
    exit(1);
}

int
main(int argc, char *argv[])
{
    int x;
    extern DMC dns_message_handle;

    progname = xstrdup(strrchr(argv[0], '/') ? strchr(argv[0], '/') + 1 : argv[0]);
    if (NULL == progname)
	return 1;
    srandom(time(NULL));
    openlog(progname, LOG_PID | LOG_NDELAY, LOG_DAEMON);

    while ((x = getopt(argc, argv, "pd")) != -1) {
	switch (x) {
	case 'p':
	    promisc_flag = 0;
	    break;
	case 'd':
	    debug_flag = 1;
	    break;
	default:
	    usage();
	    break;
	}
    }
    argc -= optind;
    argv += optind;

    if (argc != 1)
	usage();
    dns_message_init();
    ParseConfig(argv[0]);
    cip_net_indexer_init();

    if (!debug_flag)
    	daemonize();
    write_pid_file();

    /*
     * I'm using fork() in this loop, (a) out of laziness, and (b)
     * because I'm worried we might drop packets.  Making sure each
     * child collector runs for a small amount of time (60 secodns)
     * means I can be lazy about memory management (leaks).  To
     * minimize the chance for dropped packets, I'd like to spawn
     * a new collector as soon as (or even before) the current
     * collector exits.
     */

    if (!debug_flag) {
        syslog(LOG_INFO, "Sleeping for %d seconds", 60 - (int) (time(NULL) % 60));
        sleep(60 - (time(NULL) % 60));
    }
    syslog(LOG_INFO, "%s", "Running");

    if (debug_flag) {
	Pcap_run(dns_message_handle, ip_message_handle);
	dns_message_report();
	ip_message_report();

    } else {
	for (;;) {
	    pid_t cpid = fork();
	    if (0 == cpid) {
		Pcap_run(dns_message_handle, ip_message_handle);
		if (0 == fork()) {
		    dns_message_report();
		    ip_message_report();
		}
		_exit(0);
	    } else {
		int cstatus = 0;
		syslog(LOG_DEBUG, "waiting for child pid %d", (int) cpid);
		while (waitpid(cpid, &cstatus, 0) < 0)
		    (void) 0;
		if (WIFSIGNALED(cstatus))
		    syslog(LOG_NOTICE, "child exited with signal %d, status %d",
			    WTERMSIG(cstatus), WEXITSTATUS(cstatus));
	    }
	}
    }

    Pcap_close();
    return 0;
}
