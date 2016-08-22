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
#include <signal.h>

#include "xmalloc.h"
#include "dns_message.h"
#include "pcap.h"
#include "syslog_debug.h"

char *progname = NULL;
char *pid_file_name = NULL;
int promisc_flag = 1;
int monitor_flag = 0;
int immediate_flag = 0;
int threads_flag = 1;
int debug_flag = 0;
int nodaemon_flag = 0;
int have_reports = 0;

extern void cip_net_indexer_init(void);
#if HAVE_LIBGEOIP
extern void country_indexer_init(void);
extern void asn_indexer_init(void);
#endif
extern void ParseConfig(const char *);
extern uint64_t minfree_bytes;
extern int n_pcap_offline;
extern md_array_printer xml_printer;
extern md_array_printer json_printer;
extern int output_format_xml;
extern int output_format_json;
extern int dump_reports_on_exit;

void
daemonize(void)
{
    int fd;
    pid_t pid;
    if ((pid = fork()) < 0) {
        dsyslogf(LOG_ERR, "fork failed: %s", strerror(errno));
        exit(1);
    }
    if (pid > 0)
        exit(0);
    if (setsid() < 0)
        dsyslogf(LOG_ERR, "setsid failed: %s", strerror(errno));
    closelog();
#ifdef TIOCNOTTY
    if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
        ioctl(fd, TIOCNOTTY, NULL);
        close(fd);
    }
#endif
    fd = open("/dev/null", O_RDWR);
    if (fd < 0) {
        dsyslogf(LOG_ERR, "/dev/null: %s\n", strerror(errno));
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
    int fd, flags;
    struct flock lock;

    if (!pid_file_name)
        return;

    /*
     * Open the PID file, create if it does not exist.
     */

    if ((fd = open(pid_file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) == -1) {
        dsyslogf(LOG_ERR, "unable to open PID file %s: %s", pid_file_name, strerror(errno));
        exit(2);
    }

    /*
     * Set close-on-exec flag
     */

    if ((flags = fcntl(fd, F_GETFD)) == -1) {
        dsyslogf(LOG_ERR, "unable to get PID file flags: %s", strerror(errno));
        exit(2);
    }

    flags |= FD_CLOEXEC;

    if (fcntl(fd, F_SETFD, flags) == 1) {
        dsyslogf(LOG_ERR, "unable to set PID file flags: %s", strerror(errno));
        exit(2);
    }

    /*
     * Lock the PID file
     */

    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    if (fcntl(fd, F_SETLK, &lock) == -1) {
        if (errno == EACCES || errno == EAGAIN) {
            dsyslog(LOG_ERR, "PID file locked by other process");
            exit(3);
        }

        dsyslogf(LOG_ERR, "unable to lock PID file: %s", strerror(errno));
        exit(2);
    }

    /*
     * Write our PID to the file
     */

    if (ftruncate(fd, 0) == -1) {
        dsyslogf(LOG_ERR, "unable to truncate PID file: %s", strerror(errno));
        exit(2);
    }

    dsyslogf(LOG_INFO, "writing PID to %s", pid_file_name);

    fp = fdopen(fd, "w");
    if (!fp || fprintf(fp, "%d\n", getpid()) < 1 || fflush(fp)) {
        dsyslogf(LOG_ERR, "unable to write to PID file: %s", strerror(errno));
        exit(2);
    }
}

int
disk_is_full(void)
{
    uint64_t avail_bytes;
#if HAVE_STATVFS
    struct statvfs s;
    if (statvfs(".", &s) < 0)
        return 0;                /* assume not */
    avail_bytes = (uint64_t) s.f_frsize * (uint64_t) s.f_bavail;
#else
    struct statfs s;
    if (statfs(".", &s) < 0)
        return 0;                /* assume not */
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
    fprintf(stderr,
        "\t-d\tDebug mode.  Exits after first write.\n"
        "\t-f\tForeground mode.  Don't become a daemon.\n"
        "\t-p\tDon't put interface in promiscuous mode.\n"
        "\t-m\tEnable monitor mode on interfaces.\n"
        "\t-i\tEnable immediate mode on interfaces.\n"
        "\t-T\tDisable usage of threads in pcap thread.\n"
        "\t-v\tPrint version and exit.\n"
    );
    exit(1);
}

void
version(void)
{
    printf("dsc version " PACKAGE_VERSION "\n");
    exit(0);
}

static int
dump_report(md_array_printer * printer)
{
    int fd;
    FILE *fp;
    char fname[128];
    char tname[128];

    if (disk_is_full()) {
        dsyslogf(LOG_NOTICE, "Not enough free disk space to write %s files", printer->format);
        return 1;
    }
    snprintf(fname, 128, "%d.dscdata.%s", Pcap_finish_time(), printer->extension);
    snprintf(tname, 128, "%s.XXXXXXXXX", fname);
    fd = mkstemp(tname);
    if (fd < 0) {
        dsyslogf(LOG_ERR, "%s: %s", tname, strerror(errno));
        return 1;
    }
    fp = fdopen(fd, "w");
    if (NULL == fp) {
        dsyslogf(LOG_ERR, "%s: %s", tname, strerror(errno));
        close(fd);
        return 1;
    }
    dfprintf(0, "writing to %s", tname);

    fputs(printer->start_file, fp);

    /* amalloc_report(); */
    pcap_report(fp, printer);
    dns_message_report(fp, printer);

    fputs(printer->end_file, fp);

    /*
     * XXX need chmod because files are written as root, but may be processed
     * by a non-priv user
     */
    fchmod(fd, 0664);
    fclose(fp);
    dfprintf(0, "renaming to %s", fname);

    if (rename(tname, fname)) {
        dsyslogf(LOG_ERR, "unable to move report from %s to %s: %s", tname, fname, strerror(errno));
    }
    return 0;
}

static int
dump_reports(void)
{
    int ret;

    if ( output_format_xml && (ret = dump_report(&xml_printer)) ) {
        return ret;
    }
    if ( output_format_json && (ret = dump_report(&json_printer)) ) {
        return ret;
    }

    return 0;
}

static void
sig_ignore(int signum)
{
    if (debug_flag)
        dsyslogf(LOG_INFO, "Received signal %d, ignoring", signum);

    return;
}

static void
sig_exit(int signum)
{
    dsyslogf(LOG_INFO, "Received signal %d, exiting", signum);

    exit(0);
}

int sig_while_processing = 0;
static void
sig_exit_dumping(int signum)
{
    if (have_reports) {
        dsyslogf(LOG_INFO, "Received signal %d while dumping reports, exiting later", signum);
        sig_while_processing = signum;
    }
    else {
        dsyslogf(LOG_INFO, "Received signal %d, exiting", signum);
        exit(0);
    }
}

int
main(int argc, char *argv[])
{
    int x;
    int result;
    struct timeval break_start = { 0, 0 };
    struct sigaction action;

    progname = xstrdup(strrchr(argv[0], '/') ? strchr(argv[0], '/') + 1 : argv[0]);
    if (NULL == progname)
        return 1;
    srandom(time(NULL));
    openlog(progname, LOG_PID | LOG_NDELAY, LOG_DAEMON);

    while ((x = getopt(argc, argv, "fpdvmiT")) != -1) {
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
        case 'm':
            monitor_flag = 1;
            break;
        case 'i':
            immediate_flag = 1;
            break;
        case 'T':
            threads_flag = 0;
            break;
        case 'v':
            version();
        default:
            usage();
            break;
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 1)
        usage();

    if (!promisc_flag)
        dsyslog(LOG_INFO, "disabling interface promiscuous mode");
    if (monitor_flag)
        dsyslog(LOG_INFO, "enabling interface monitor mode");
    if (immediate_flag)
        dsyslog(LOG_INFO, "enabling interface immediate mode");
    if (!threads_flag)
        dsyslog(LOG_INFO, "disabling usage of threads in pcap thread");

    dns_message_init();
    ParseConfig(argv[0]);
#if HAVE_LIBGEOIP
    country_indexer_init();
    asn_indexer_init();
#endif
    cip_net_indexer_init();
    if ( !output_format_xml && !output_format_json ) {
        output_format_xml = 1;
    }

    /*
     * Do not damonize if we only have offline files
     */
    if (n_pcap_offline) {
        nodaemon_flag = 1;
    }

    if (!nodaemon_flag)
        daemonize();
    write_pid_file();

    /*
     * Install signal handler for signals to ignore
     */

    memset(&action, 0, sizeof(action));
    action.sa_handler = sig_ignore;
    sigfillset(&action.sa_mask);

    if (sigaction(SIGHUP, &action, NULL))
        dsyslogf(LOG_ERR, "Unable to install signal handler for SIGHUP: %s", strerror(errno));
    if (sigaction(SIGCHLD, &action, NULL))
        dsyslogf(LOG_ERR, "Unable to install signal handler for SIGCHLD: %s", strerror(errno));
    if (sigaction(SIGPIPE, &action, NULL))
        dsyslogf(LOG_ERR, "Unable to install signal handler for SIGPIPE: %s", strerror(errno));
    if (sigaction(SIGUSR1, &action, NULL))
        dsyslogf(LOG_ERR, "Unable to install signal handler for SIGUSR1: %s", strerror(errno));
    if (sigaction(SIGUSR2, &action, NULL))
        dsyslogf(LOG_ERR, "Unable to install signal handler for SIGUSR2: %s", strerror(errno));

    /*
     * Do not ignore SIGINT if we are running in the foreground
     */

    if (!nodaemon_flag && sigaction(SIGINT, &action, NULL))
        dsyslogf(LOG_ERR, "Unable to install signal handler for SIGINT: %s", strerror(errno));

    /*
     * Install signal handler for signals to exit on
     */

    if (dump_reports_on_exit)
        action.sa_handler = sig_exit_dumping;
    else
        action.sa_handler = sig_exit;

    if (sigaction(SIGTERM, &action, NULL))
        dsyslogf(LOG_ERR, "Unable to install signal handler for SIGTERM: %s", strerror(errno));
    if (sigaction(SIGQUIT, &action, NULL))
        dsyslogf(LOG_ERR, "Unable to install signal handler for SIGQUIT: %s", strerror(errno));

    if (!debug_flag && 0 == n_pcap_offline) {
        dsyslogf(LOG_INFO, "Sleeping for %d seconds", 60 - (int) (time(NULL) % 60));
        sleep(60 - (time(NULL) % 60));
    }
    dsyslog(LOG_INFO, "Running");

    do {
        useArena();                /* Initialize a memory arena for data collection. */
        if (debug_flag && break_start.tv_sec > 0) {
            struct timeval now;
            gettimeofday(&now, NULL);
            dsyslogf(LOG_INFO, "inter-run processing delay: %ld ms",
                (now.tv_usec - break_start.tv_usec) / 1000 + 1000 * (now.tv_sec - break_start.tv_sec));
        }

        /* Indicate we might have reports to dump on exit */
        have_reports = 1;

        result = Pcap_run();
        if (debug_flag)
            gettimeofday(&break_start, NULL);

        if (0 == fork()) {
            dump_reports();
            _exit(0);
        }

        if (sig_while_processing) {
            dsyslogf(LOG_INFO, "Received signal %d before, exiting now", sig_while_processing);
            exit(0);
        }
        have_reports = 0;

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
                    dsyslogf(LOG_NOTICE, "child %d exited with signal %d", pid, WTERMSIG(cstatus));
                if (WIFEXITED(cstatus) && WEXITSTATUS(cstatus) != 0)
                    dsyslogf(LOG_NOTICE, "child %d exited with status %d", pid, WEXITSTATUS(cstatus));
            }
        }

    } while (result > 0 && debug_flag == 0);

    Pcap_close();

    return 0;
}
