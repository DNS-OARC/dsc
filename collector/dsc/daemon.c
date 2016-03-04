/*
 * $Id$
 *
 * Copyright (c) 2007, The Measurement Factory, Inc.  All rights
 * reserved.  See the LICENSE file for details.
 */

#include "config.h"

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
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#if HAVE_STATVFS
#if HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#endif
#if HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
#if HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif

#include "xmalloc.h"
#include "dns_message.h"
#include "pcap.h"
#if HAVE_LIBNCAP
#include "ncap.h"
#endif
#include "syslog_debug.h"

char *progname = NULL;
char *pid_file_name = NULL;
int promisc_flag = 1;
int debug_flag = 0;
int nodaemon_flag = 0;

extern void cip_net_indexer_init(void);
#if HAVE_LIBGEOIP
extern void country_indexer_init(void);
#endif
extern void ParseConfig(const char *);
extern uint64_t minfree_bytes;
extern char output_json;
extern char output_ext_json;
extern int n_pcap_offline;
extern md_array_printer xml_printer;
extern md_array_printer json_printer;
extern md_array_printer ext_json_printer;

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

int
disk_is_full(void)
{
    uint64_t avail_bytes;
#if HAVE_STATVFS
    struct statvfs s;
    if (statvfs(".", &s) < 0)
	return 0;		/* assume not */
    avail_bytes = (uint64_t) s.f_frsize * (uint64_t) s.f_bavail;
#else
    struct statfs s;
    if (statfs(".", &s) < 0)
	return 0;		/* assume not */
    avail_bytes = (uint64_t) s.f_bsize * (uint64_t) s.f_bavail;
#endif
    if (avail_bytes < minfree_bytes)
	return 1;
    return 0;
}

void
usage(void)
{
    fprintf(stderr, "usage: %s [opts] dsc.conf\n", progname);
    fprintf(stderr, "\t-d\tDebug mode.  Exits after first write.\n");
    fprintf(stderr, "\t-f\tForeground mode.  Don't become a daemon.\n");
    fprintf(stderr, "\t-p\tDon't put interface in promiscuous mode.\n");
    exit(1);
}

static int
dump_report(char *extension, char *start_file, char *end_file, md_array_printer * printer)
{
    int fd;
    FILE *fp;
    char fname[128];
    char tname[128];

    if (disk_is_full()) {
	syslog(LOG_NOTICE, "Not enough free disk space to write '%s' report files", extension);
	return 1;
    }
#if HAVE_LIBNCAP
    snprintf(fname, 128, "%d.dscdata.%s", Ncap_finish_time(), extension);
#else
    snprintf(fname, 128, "%d.dscdata.%s", Pcap_finish_time(), extension);
#endif
    snprintf(tname, 128, "%s.XXXXXXXXX", fname);
    fd = mkstemp(tname);
    if (fd < 0) {
	syslog(LOG_ERR, "%s: %s", tname, strerror(errno));
	return 1;
    }
    fp = fdopen(fd, "w");
    if (NULL == fp) {
	syslog(LOG_ERR, "%s: %s", tname, strerror(errno));
	close(fd);
	return 1;
    }
    if (debug_flag)
	fprintf(stderr, "writing to %s\n", tname);

    fprintf(fp, start_file);

    /* amalloc_report(); */
    pcap_report(fp, printer);
    dns_message_report(fp, printer);

    fprintf(fp, end_file);

    /*
     * XXX need chmod because files are written as root, but may be processed
     * by a non-priv user
     */
    fchmod(fd, 0664);
    fclose(fp);
    if (debug_flag)
	fprintf(stderr, "renaming to %s\n", fname);

    rename(tname, fname);
    return 0;
}

static int
dump_reports(void)
{
    int err = dump_report("xml", "<dscdata>\n", "</dscdata>\n", &xml_printer);

    if (output_json)
	err = err & dump_report("json", "{\n", "\n}\n", &json_printer);
    else if (output_ext_json)
	err = err & dump_report("json", "{\n", "\n}\n", &ext_json_printer);

    return err;
}

int
main(int argc, char *argv[])
{
    int x;
    int result;
    struct timeval break_start = { 0, 0 };

    progname = xstrdup(strrchr(argv[0], '/') ? strchr(argv[0], '/') + 1 : argv[0]);
    if (NULL == progname)
	return 1;
    srandom(time(NULL));
    openlog(progname, LOG_PID | LOG_NDELAY, LOG_DAEMON);

    while ((x = getopt(argc, argv, "fpd")) != -1) {
	switch (x) {
	case 'f':
	    nodaemon_flag = 1;
	    break;
	case 'p':
	    promisc_flag = 0;
	    break;
	case 'd':
	    debug_flag++;
	    nodaemon_flag = 1;
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
#if HAVE_LIBGEOIP
    country_indexer_init();
#endif
    cip_net_indexer_init();

    if (!nodaemon_flag)
	daemonize();
    write_pid_file();

    if (!debug_flag && 0 == n_pcap_offline) {
	syslog(LOG_INFO, "Sleeping for %d seconds", 60 - (int) (time(NULL) % 60));
	sleep(60 - (time(NULL) % 60));
    }
    syslog(LOG_INFO, "%s", "Running");

    do {
	useArena();		/* Initialize a memory arena for data collection. */
	if (debug_flag && break_start.tv_sec > 0) {
	    struct timeval now;
	    gettimeofday(&now, NULL);
	    syslog(LOG_INFO, "inter-run processing delay: %ld ms",
		(now.tv_usec - break_start.tv_usec) / 1000 + 1000 * (now.tv_sec - break_start.tv_sec));
	}
#if HAVE_LIBNCAP
	result = Ncap_run();
#else
	result = Pcap_run();
#endif
	if (debug_flag)
	    gettimeofday(&break_start, NULL);
	if (0 == fork()) {
	    dump_reports();
	    _exit(0);
	}
	/* Parent quickly frees and clears its copy of the data so it can
	 * resume processing packets. */
	freeArena();
	dns_message_clear_arrays();

	{
	    /* Reap children. (Most recent probably has not exited yet, but
	     * older ones should have.) */
	    int cstatus = 0;
	    pid_t pid;
	    while ((pid = waitpid(0, &cstatus, WNOHANG)) > 0) {
		if (WIFSIGNALED(cstatus))
		    syslog(LOG_NOTICE, "child %d exited with signal %d", pid, WTERMSIG(cstatus));
		if (WIFEXITED(cstatus) && WEXITSTATUS(cstatus) != 0)
		    syslog(LOG_NOTICE, "child %d exited with status %d", pid, WEXITSTATUS(cstatus));
	    }
	}

    } while (result > 0 && debug_flag == 0);

#if HAVE_LIBNCAP
    Ncap_close();
#else
    Pcap_close();
#endif
    return 0;
}
