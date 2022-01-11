/*
 * Copyright (c) 2008-2022, OARC, Inc.
 * Copyright (c) 2007-2008, Internet Systems Consortium, Inc.
 * Copyright (c) 2003-2007, The Measurement Factory, Inc.
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

#include "xmalloc.h"
#include "pcap.h"
#include "syslog_debug.h"
#include "parse_conf.h"
#include "compat.h"
#include "pcap-thread/pcap_thread.h"

#include "input_mode.h"
#include "dnstap.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
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
#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#include <signal.h>
#if HAVE_PTHREAD
#include <pthread.h>
#endif

char* progname       = NULL;
char* pid_file_name  = NULL;
int   promisc_flag   = 1;
int   monitor_flag   = 0;
int   immediate_flag = 0;
int   threads_flag   = 1;
int   debug_flag     = 0;
int   nodaemon_flag  = 0;
int   have_reports   = 0;
int   input_mode     = INPUT_NONE;

extern uint64_t         minfree_bytes;
extern int              n_pcap_offline;
extern md_array_printer xml_printer;
extern md_array_printer json_printer;
extern int              output_format_xml;
extern int              output_format_json;
extern int              dump_reports_on_exit;
extern uint64_t         statistics_interval;
extern int              no_wait_interval;
extern pcap_thread_t    pcap_thread;

void daemonize(void)
{
    char  errbuf[512];
    int   fd;
    pid_t pid;
    if ((pid = fork()) < 0) {
        dsyslogf(LOG_ERR, "fork failed: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
        exit(1);
    }
    if (pid > 0) {
#ifdef GCOV_FLUSH
        __gcov_flush();
#endif
        _exit(0);
    }
    if (setsid() < 0)
        dsyslogf(LOG_ERR, "setsid failed: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
    closelog();
#ifdef TIOCNOTTY
    if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
        ioctl(fd, TIOCNOTTY, NULL);
        close(fd);
    }
#endif
    fd = open("/dev/null", O_RDWR);
    if (fd < 0) {
        dsyslogf(LOG_ERR, "/dev/null: %s\n", dsc_strerror(errno, errbuf, sizeof(errbuf)));
    } else {
        dup2(fd, 0);
        dup2(fd, 1);
        dup2(fd, 2);
        close(fd);
    }
    openlog(progname, LOG_PID | LOG_NDELAY, LOG_DAEMON);
}

void write_pid_file(void)
{
    char         errbuf[512];
    FILE*        fp;
    int          fd, flags;
    struct flock lock;

    if (!pid_file_name)
        return;

    /*
     * Open the PID file, create if it does not exist.
     */

    if ((fd = open(pid_file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) == -1) {
        dsyslogf(LOG_ERR, "unable to open PID file %s: %s", pid_file_name, dsc_strerror(errno, errbuf, sizeof(errbuf)));
        exit(2);
    }

    /*
     * Set close-on-exec flag
     */

    if ((flags = fcntl(fd, F_GETFD)) == -1) {
        dsyslogf(LOG_ERR, "unable to get PID file flags: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
        exit(2);
    }

    flags |= FD_CLOEXEC;

    if (fcntl(fd, F_SETFD, flags) == 1) {
        dsyslogf(LOG_ERR, "unable to set PID file flags: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
        exit(2);
    }

    /*
     * Lock the PID file
     */

    lock.l_type   = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start  = 0;
    lock.l_len    = 0;

    if (fcntl(fd, F_SETLK, &lock) == -1) {
        if (errno == EACCES || errno == EAGAIN) {
            dsyslog(LOG_ERR, "PID file locked by other process");
            exit(3);
        }

        dsyslogf(LOG_ERR, "unable to lock PID file: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
        exit(2);
    }

    /*
     * Write our PID to the file
     */

    if (ftruncate(fd, 0) == -1) {
        dsyslogf(LOG_ERR, "unable to truncate PID file: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
        exit(2);
    }

    dsyslogf(LOG_INFO, "writing PID to %s", pid_file_name);

    fp = fdopen(fd, "w");
    if (!fp || fprintf(fp, "%d\n", getpid()) < 1 || fflush(fp)) {
        dsyslogf(LOG_ERR, "unable to write to PID file: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
        exit(2);
    }
}

int disk_is_full(void)
{
    uint64_t avail_bytes;
#if HAVE_STATVFS
    struct statvfs s;
    if (statvfs(".", &s) < 0)
        return 0; /* assume not */
    avail_bytes = (uint64_t)s.f_frsize * (uint64_t)s.f_bavail;
#else
    struct statfs s;
    if (statfs(".", &s) < 0)
        return 0; /* assume not */
    avail_bytes = (uint64_t)s.f_bsize * (uint64_t)s.f_bavail;
#endif
    if (avail_bytes < minfree_bytes)
        return 1;
    return 0;
}

void usage(void)
{
    fprintf(stderr, "usage: %s [opts] dsc.conf\n", progname);
    fprintf(stderr,
        "\t-d\tDebug mode.  Exits after first write.\n"
        "\t-f\tForeground mode.  Don't become a daemon.\n"
        "\t-p\tDon't put interface in promiscuous mode.\n"
        "\t-m\tEnable monitor mode on interfaces.\n"
        "\t-i\tEnable immediate mode on interfaces.\n"
        "\t-T\tDisable the usage of threads.\n"
        "\t-D\tDon't exit after first write when in debug mode.\n"
        "\t-v\tPrint version and exit.\n");
    exit(1);
}

void version(void)
{
    printf("dsc version " PACKAGE_VERSION "\n");
    exit(0);
}

static int
dump_report(md_array_printer* printer)
{
    char  errbuf[512];
    int   fd;
    FILE* fp;
    char  fname[128];
    char  tname[256];

    if (disk_is_full()) {
        dsyslogf(LOG_NOTICE, "Not enough free disk space to write %s files", printer->format);
        return 1;
    }
    if (input_mode == INPUT_DNSTAP)
        snprintf(fname, sizeof(fname), "%d.dscdata.%s", dnstap_finish_time(), printer->extension);
    else
        snprintf(fname, sizeof(fname), "%d.dscdata.%s", Pcap_finish_time(), printer->extension);
    snprintf(tname, sizeof(tname), "%s.XXXXXXXXX", fname);
    fd = mkstemp(tname);
    if (fd < 0) {
        dsyslogf(LOG_ERR, "%s: %s", tname, dsc_strerror(errno, errbuf, sizeof(errbuf)));
        return 1;
    }
    fp = fdopen(fd, "w");
    if (NULL == fp) {
        dsyslogf(LOG_ERR, "%s: %s", tname, dsc_strerror(errno, errbuf, sizeof(errbuf)));
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
        dsyslogf(LOG_ERR, "unable to move report from %s to %s: %s", tname, fname, dsc_strerror(errno, errbuf, sizeof(errbuf)));
    }
    return 0;
}

static int
dump_reports(void)
{
    int ret;

    if (output_format_xml && (ret = dump_report(&xml_printer))) {
        return ret;
    }
    if (output_format_json && (ret = dump_report(&json_printer))) {
        return ret;
    }

    return 0;
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
        switch (input_mode) {
        case INPUT_PCAP:
            Pcap_stop();
            break;
        case INPUT_DNSTAP:
            dnstap_stop();
            break;
        default:
            break;
        }
    } else {
        dsyslogf(LOG_INFO, "Received signal %d, exiting", signum);
        exit(0);
    }
}

#if HAVE_PTHREAD
static void*
sig_thread(void* arg)
{
    sigset_t* set = (sigset_t*)arg;
    int       sig, err;

    if ((err = sigwait(set, &sig))) {
        dsyslogf(LOG_DEBUG, "Error sigwait(): %d", err);
        return 0;
    }

    if (dump_reports_on_exit)
        sig_exit_dumping(sig);
    else
        sig_exit(sig);

    return 0;
}
#endif

typedef int (*run_func)(void);
typedef void (*close_func)(void);

int main(int argc, char* argv[])
{
    char           errbuf[512];
    int            x, dont_exit = 0;
    int            result;
    struct timeval break_start = { 0, 0 };
#if HAVE_PTHREAD
    pthread_t sigthread;
#endif
    int            err;
    struct timeval now;
    run_func       runf;
    close_func     closef;

    progname = xstrdup(strrchr(argv[0], '/') ? strrchr(argv[0], '/') + 1 : argv[0]);
    if (NULL == progname)
        return 1;
    openlog(progname, LOG_PID | LOG_NDELAY, LOG_DAEMON);

    while ((x = getopt(argc, argv, "fpdvmiTD")) != -1) {
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
        case 'D':
            dont_exit = 1;
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
        dsyslog(LOG_INFO, "disabling the usage of threads");

    pcap_thread_set_activate_mode(&pcap_thread, PCAP_THREAD_ACTIVATE_MODE_DELAYED);

    dns_message_filters_init();
    if (parse_conf(argv[0])) {
        return 1;
    }
    dns_message_indexers_init();
    if (!output_format_xml && !output_format_json) {
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
     * Handle signal when using pthreads
     */

#if HAVE_PTHREAD
    if (threads_flag) {
        sigset_t set;

        sigfillset(&set);
        if ((err = pthread_sigmask(SIG_BLOCK, &set, 0))) {
            dsyslogf(LOG_ERR, "Unable to set signal mask: %s", dsc_strerror(err, errbuf, sizeof(errbuf)));
            exit(1);
        }

        sigemptyset(&set);
        sigaddset(&set, SIGTERM);
        sigaddset(&set, SIGQUIT);
        if (nodaemon_flag)
            sigaddset(&set, SIGINT);

        if ((err = pthread_create(&sigthread, 0, &sig_thread, (void*)&set))) {
            dsyslogf(LOG_ERR, "Unable to start signal thread: %s", dsc_strerror(err, errbuf, sizeof(errbuf)));
            exit(1);
        }
    } else
#endif
    {
        /*
         * Handle signal without pthreads
         */

        sigset_t         set;
        struct sigaction action;

        sigfillset(&set);
        sigdelset(&set, SIGTERM);
        sigdelset(&set, SIGQUIT);
        if (nodaemon_flag)
            sigdelset(&set, SIGINT);

        if (sigprocmask(SIG_BLOCK, &set, 0))
            dsyslogf(LOG_ERR, "Unable to set signal mask: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));

        memset(&action, 0, sizeof(action));
        sigfillset(&action.sa_mask);

        if (dump_reports_on_exit)
            action.sa_handler = sig_exit_dumping;
        else
            action.sa_handler = sig_exit;

        if (sigaction(SIGTERM, &action, NULL))
            dsyslogf(LOG_ERR, "Unable to install signal handler for SIGTERM: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
        if (sigaction(SIGQUIT, &action, NULL))
            dsyslogf(LOG_ERR, "Unable to install signal handler for SIGQUIT: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
        if (nodaemon_flag && sigaction(SIGINT, &action, NULL))
            dsyslogf(LOG_ERR, "Unable to install signal handler for SIGINT: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
    }

    if (!debug_flag && 0 == n_pcap_offline && !no_wait_interval) {
        struct timespec nano;

        gettimeofday(&now, NULL);

        if ((now.tv_sec + 1) % statistics_interval)
            nano.tv_sec = statistics_interval - ((now.tv_sec + 1) % statistics_interval);
        else
            nano.tv_sec = 0;

        if (now.tv_usec > 1000000)
            nano.tv_nsec = 0;
        else
            nano.tv_nsec = (1000000 - now.tv_usec) * 1000;

        dsyslogf(LOG_INFO, "Sleeping for %ld.%ld seconds", (long)nano.tv_sec, nano.tv_nsec);
        nanosleep(&nano, NULL);
    }

    switch (input_mode) {
    case INPUT_PCAP:
        if ((err = pcap_thread_activate(&pcap_thread))) {
            dsyslogf(LOG_ERR, "unable to activate pcap thread: %s", pcap_thread_strerr(err));
            exit(1);
        }
        if (pcap_thread_filter_errno(&pcap_thread)) {
            dsyslogf(LOG_NOTICE, "detected non-fatal error during pcap activation, filters may run in userland [%d]: %s",
                pcap_thread_filter_errno(&pcap_thread),
                dsc_strerror(pcap_thread_filter_errno(&pcap_thread), errbuf, sizeof(errbuf)));
        }
        runf   = Pcap_run;
        closef = Pcap_close;
        break;
    case INPUT_DNSTAP:
        runf   = dnstap_run;
        closef = dnstap_close;
        break;
    default:
        dsyslog(LOG_ERR, "No input in config");
        exit(1);
    }

    dsyslog(LOG_INFO, "Running");

    do {
        useArena(); /* Initialize a memory arena for data collection. */
        if (debug_flag && break_start.tv_sec > 0) {
            gettimeofday(&now, NULL);
            dsyslogf(LOG_INFO, "inter-run processing delay: %lld ms",
                (long long int)((now.tv_usec - break_start.tv_usec) / 1000 + 1000 * (now.tv_sec - break_start.tv_sec)));
        }

        /* Indicate we might have reports to dump on exit */
        have_reports = 1;

        result = runf();
        if (debug_flag)
            gettimeofday(&break_start, NULL);

        dns_message_flush_arrays();

        if (0 == fork()) {
            struct sigaction action;

            /*
             * Remove the blocking of signals
             */

#if HAVE_PTHREAD
            if (threads_flag) {
                sigset_t set;

                /*
                 * Reset the signal process mask since the signal thread
                 * will not make the fork
                 */

                sigemptyset(&set);
                sigaddset(&set, SIGTERM);
                sigaddset(&set, SIGQUIT);
                sigaddset(&set, SIGINT);

                sigprocmask(SIG_UNBLOCK, &set, 0);
            }
#endif

            memset(&action, 0, sizeof(action));
            sigfillset(&action.sa_mask);
            action.sa_handler = SIG_DFL;

            sigaction(SIGTERM, &action, NULL);
            sigaction(SIGQUIT, &action, NULL);
            sigaction(SIGINT, &action, NULL);

            dump_reports();
#ifdef GCOV_FLUSH
            __gcov_flush();
#endif
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
            int   cstatus = 0;
            pid_t pid;
            while ((pid = waitpid(0, &cstatus, WNOHANG)) > 0) {
                if (WIFSIGNALED(cstatus))
                    dsyslogf(LOG_NOTICE, "child %d exited with signal %d", pid, WTERMSIG(cstatus));
                if (WIFEXITED(cstatus) && WEXITSTATUS(cstatus) != 0)
                    dsyslogf(LOG_NOTICE, "child %d exited with status %d", pid, WEXITSTATUS(cstatus));
            }
        }

    } while (result > 0 && (debug_flag == 0 || dont_exit));

    closef();

    return 0;
}
