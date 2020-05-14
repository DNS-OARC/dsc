/*
 * Copyright (c) 2008-2020, OARC, Inc.
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

#include "config_hooks.h"
#include "xmalloc.h"
#include "syslog_debug.h"
#include "hashtbl.h"
#include "pcap.h"
#include "compat.h"
#include "response_time_index.h"
#include "input_mode.h"
#include "dnstap.h"

#include "knowntlds.inc"

#if defined(HAVE_LIBGEOIP) && defined(HAVE_GEOIP_H)
#define HAVE_GEOIP 1
#include <GeoIP.h>
#endif
#if defined(HAVE_LIBMAXMINDDB) && defined(HAVE_MAXMINDDB_H)
#define HAVE_MAXMINDDB 1
#include <maxminddb.h>
#endif
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <ctype.h>

extern int input_mode;
extern int promisc_flag;
extern int monitor_flag;
extern int immediate_flag;
extern int threads_flag;
uint64_t   minfree_bytes      = 0;
int        output_format_xml  = 0;
int        output_format_json = 0;
#define MAX_HASH_SIZE 512
static hashtbl* dataset_hash         = NULL;
uint64_t        statistics_interval  = 60; /* default interval in seconds*/
int             dump_reports_on_exit = 0;
char*           geoip_v4_dat         = NULL;
int             geoip_v4_options     = 0;
char*           geoip_v6_dat         = NULL;
int             geoip_v6_options     = 0;
char*           geoip_asn_v4_dat     = NULL;
int             geoip_asn_v4_options = 0;
char*           geoip_asn_v6_dat     = NULL;
int             geoip_asn_v6_options = 0;
int             pcap_buffer_size     = 0;
int             no_wait_interval     = 0;
int             pt_timeout           = 100;
int             drop_ip_fragments    = 0;
#ifdef HAVE_GEOIP
enum geoip_backend asn_indexer_backend     = geoip_backend_libgeoip;
enum geoip_backend country_indexer_backend = geoip_backend_libgeoip;
#else
#ifdef HAVE_MAXMINDDB
enum geoip_backend asn_indexer_backend     = geoip_backend_libmaxminddb;
enum geoip_backend country_indexer_backend = geoip_backend_libmaxminddb;
#else
enum geoip_backend asn_indexer_backend     = geoip_backend_none;
enum geoip_backend country_indexer_backend = geoip_backend_none;
#endif
#endif
char* maxminddb_asn     = NULL;
char* maxminddb_country = NULL;

extern int  ip_local_address(const char*, const char*);
extern void pcap_set_match_vlan(int);

int open_interface(const char* interface)
{
    if (input_mode != INPUT_NONE && input_mode != INPUT_PCAP) {
        dsyslog(LOG_ERR, "input mode already set");
        return 0;
    }
    input_mode = INPUT_PCAP;
    dsyslogf(LOG_INFO, "Opening interface %s", interface);
    Pcap_init(interface, promisc_flag, monitor_flag, immediate_flag, threads_flag, pcap_buffer_size);
    return 1;
}

int open_dnstap(enum dnstap_via via, const char* file_or_ip, const char* port, const char* user, const char* group, const char* umask)
{
    int   port_num = -1, mask = -1;
    uid_t uid = -1;
    gid_t gid = -1;

    if (input_mode != INPUT_NONE) {
        if (input_mode == INPUT_DNSTAP) {
            dsyslog(LOG_ERR, "only one DNSTAP input can be used at a time");
        } else {
            dsyslog(LOG_ERR, "input mode already set");
        }
        return 0;
    }
    if (port) {
        port_num = atoi(port);
        if (port_num < 0 || port_num > 65535) {
            dsyslog(LOG_ERR, "invalid port for DNSTAP");
            return 0;
        }
        dsyslogf(LOG_INFO, "Opening dnstap %s:%s", file_or_ip, port);
    } else {
        dsyslogf(LOG_INFO, "Opening dnstap %s", file_or_ip);
    }
    if (user && *user != 0) {
        struct passwd* pw = getpwnam(user);
        if (!pw) {
            dsyslog(LOG_ERR, "invalid USER for DNSTAP UNIX socket, does not exist");
            return 0;
        }
        uid = pw->pw_uid;
        dsyslogf(LOG_INFO, "Using user %s [%d] for DNSTAP", user, uid);
    }
    if (group) {
        struct group* gr = getgrnam(group);
        if (!gr) {
            dsyslog(LOG_ERR, "invalid GROUP for DNSTAP UNIX socket, does not exist");
            return 0;
        }
        gid = gr->gr_gid;
        dsyslogf(LOG_INFO, "Using group %s [%d] for DNSTAP", group, gid);
    }
    if (umask) {
        unsigned int m;
        if (sscanf(umask, "%o", &m) != 1) {
            dsyslog(LOG_ERR, "invalid UMASK for DNSTAP UNIX socket, should be octal");
            return 0;
        }
        if (m > 0777) {
            dsyslog(LOG_ERR, "invalid UMASK for DNSTAP UNIX socket, too large value, maximum 0777");
            return 0;
        }
        mask = (int)m;
        dsyslogf(LOG_INFO, "Using umask %04o for DNSTAP", mask);
    }
    dnstap_init(via, file_or_ip, port_num, uid, gid, mask);
    input_mode = INPUT_DNSTAP;
    return 1;
}

int set_bpf_program(const char* s)
{
    extern char* bpf_program_str;
    dsyslogf(LOG_INFO, "BPF program is: %s", s);
    if (bpf_program_str)
        xfree(bpf_program_str);
    bpf_program_str = xstrdup(s);
    if (NULL == bpf_program_str)
        return 0;
    return 1;
}

int add_local_address(const char* s, const char* m)
{
    dsyslogf(LOG_INFO, "adding local address %s%s%s", s, m ? " mask " : "", m ? m : "");
    return ip_local_address(s, m);
}

int set_run_dir(const char* dir)
{
    dsyslogf(LOG_INFO, "setting current directory to %s", dir);
    if (chdir(dir) < 0) {
        char errbuf[512];
        perror(dir);
        dsyslogf(LOG_ERR, "chdir: %s: %s", dir, dsc_strerror(errno, errbuf, sizeof(errbuf)));
        return 0;
    }
    return 1;
}

int set_pid_file(const char* s)
{
    extern char* pid_file_name;
    dsyslogf(LOG_INFO, "PID file is: %s", s);
    if (pid_file_name)
        xfree(pid_file_name);
    pid_file_name = xstrdup(s);
    if (NULL == pid_file_name)
        return 0;
    return 1;
}

static unsigned int
dataset_hashfunc(const void* key)
{
    return hashendian(key, strlen(key), 0);
}

static int
dataset_cmpfunc(const void* a, const void* b)
{
    return strcasecmp(a, b);
}

int set_statistics_interval(const char* s)
{
    dsyslogf(LOG_INFO, "Setting statistics interval to: %s", s);
    statistics_interval = strtoull(s, NULL, 10);
    if (statistics_interval == ULLONG_MAX) {
        char errbuf[512];
        dsyslogf(LOG_ERR, "strtoull: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
        return 0;
    }
    if (!statistics_interval) {
        dsyslog(LOG_ERR, "statistics_interval can not be zero");
        return 0;
    }
    return 1;
}

static int response_time_indexer_used = 0;

int add_dataset(const char* name, const char* layer_ignored,
    const char* firstname, const char* firstindexer,
    const char* secondname, const char* secondindexer, const char* filtername, dataset_opt opts)
{
    char* dup;

    if (!strcmp(firstindexer, "response_time") || !strcmp(secondindexer, "response_time")) {
        if (response_time_indexer_used) {
            dsyslogf(LOG_ERR, "unable to create dataset %s: response_time indexer already used, can only be used in one dataset", name);
            return 0;
        }
        response_time_indexer_used = 1;
    }

    if (!dataset_hash) {
        if (!(dataset_hash = hash_create(MAX_HASH_SIZE, dataset_hashfunc, dataset_cmpfunc, 0, xfree, xfree))) {
            dsyslogf(LOG_ERR, "unable to create dataset %s due to internal error", name);
            return 0;
        }
    }

    if (hash_find(name, dataset_hash)) {
        dsyslogf(LOG_ERR, "unable to create dataset %s: already exists", name);
        return 0;
    }

    if (!(dup = xstrdup(name))) {
        dsyslogf(LOG_ERR, "unable to create dataset %s due to internal error", name);
        return 0;
    }

    if (hash_add(dup, dup, dataset_hash)) {
        xfree(dup);
        dsyslogf(LOG_ERR, "unable to create dataset %s due to internal error", name);
        return 0;
    }

    dsyslogf(LOG_INFO, "creating dataset %s", name);
    return dns_message_add_array(name, firstname, firstindexer, secondname, secondindexer, filtername, opts);
}

int set_bpf_vlan_tag_byte_order(const char* which)
{
    extern int vlan_tag_needs_byte_conversion;
    dsyslogf(LOG_INFO, "bpf_vlan_tag_byte_order is %s", which);
    if (0 == strcmp(which, "host")) {
        vlan_tag_needs_byte_conversion = 0;
        return 1;
    }
    if (0 == strcmp(which, "net")) {
        vlan_tag_needs_byte_conversion = 1;
        return 1;
    }
    dsyslogf(LOG_ERR, "unknown bpf_vlan_tag_byte_order '%s'", which);
    return 0;
}

int set_match_vlan(const char* s)
{
    int i;
    dsyslogf(LOG_INFO, "match_vlan %s", s);
    i = atoi(s);
    if (0 == i && 0 != strcmp(s, "0"))
        return 0;
    pcap_set_match_vlan(i);
    return 1;
}

int set_minfree_bytes(const char* s)
{
    dsyslogf(LOG_INFO, "minfree_bytes %s", s);
    minfree_bytes = strtoull(s, NULL, 10);
    return 1;
}

int set_output_format(const char* output_format)
{
    dsyslogf(LOG_INFO, "output_format %s", output_format);

    if (!strcmp(output_format, "XML")) {
        output_format_xml = 1;
        return 1;
    } else if (!strcmp(output_format, "JSON")) {
        output_format_json = 1;
        return 1;
    }

    dsyslogf(LOG_ERR, "unknown output format '%s'", output_format);
    return 0;
}

void set_dump_reports_on_exit(void)
{
    dsyslog(LOG_INFO, "dump_reports_on_exit");

    dump_reports_on_exit = 1;
}

int set_geoip_v4_dat(const char* dat, int options)
{
    char errbuf[512];

    geoip_v4_options = options;
    if (geoip_v4_dat)
        xfree(geoip_v4_dat);
    if ((geoip_v4_dat = xstrdup(dat))) {
        dsyslogf(LOG_INFO, "GeoIP v4 dat %s %d", geoip_v4_dat, geoip_v4_options);
        return 1;
    }

    dsyslogf(LOG_ERR, "unable to set GeoIP v4 dat, strdup: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
    return 0;
}

int set_geoip_v6_dat(const char* dat, int options)
{
    char errbuf[512];

    geoip_v6_options = options;
    if (geoip_v6_dat)
        xfree(geoip_v6_dat);
    if ((geoip_v6_dat = xstrdup(dat))) {
        dsyslogf(LOG_INFO, "GeoIP v6 dat %s %d", geoip_v6_dat, geoip_v6_options);
        return 1;
    }

    dsyslogf(LOG_ERR, "unable to set GeoIP v6 dat, strdup: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
    return 0;
}

int set_geoip_asn_v4_dat(const char* dat, int options)
{
    char errbuf[512];

    geoip_asn_v4_options = options;
    if (geoip_asn_v4_dat)
        xfree(geoip_asn_v4_dat);
    if ((geoip_asn_v4_dat = xstrdup(dat))) {
        dsyslogf(LOG_INFO, "GeoIP ASN v4 dat %s %d", geoip_asn_v4_dat, geoip_asn_v4_options);
        return 1;
    }

    dsyslogf(LOG_ERR, "unable to set GeoIP ASN v4 dat, strdup: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
    return 0;
}

int set_geoip_asn_v6_dat(const char* dat, int options)
{
    char errbuf[512];

    geoip_asn_v6_options = options;
    if (geoip_asn_v6_dat)
        xfree(geoip_asn_v6_dat);
    if ((geoip_asn_v6_dat = xstrdup(dat))) {
        dsyslogf(LOG_INFO, "GeoIP ASN v6 dat %s %d", geoip_asn_v6_dat, geoip_asn_v6_options);
        return 1;
    }

    dsyslogf(LOG_ERR, "unable to set GeoIP ASN v6 dat, strdup: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
    return 0;
}

int set_asn_indexer_backend(enum geoip_backend backend)
{
    switch (backend) {
    case geoip_backend_libgeoip:
        dsyslog(LOG_INFO, "asn_indexer using GeoIP backend");
        break;
    case geoip_backend_libmaxminddb:
        dsyslog(LOG_INFO, "asn_indexer using MaxMind DB backend");
        break;
    default:
        return 0;
    }

    asn_indexer_backend = backend;

    return 1;
}

int set_country_indexer_backend(enum geoip_backend backend)
{
    switch (backend) {
    case geoip_backend_libgeoip:
        dsyslog(LOG_INFO, "country_indexer using GeoIP backend");
        break;
    case geoip_backend_libmaxminddb:
        dsyslog(LOG_INFO, "country_indexer using MaxMind DB backend");
        break;
    default:
        return 0;
    }

    country_indexer_backend = backend;

    return 1;
}

int set_maxminddb_asn(const char* file)
{
    char errbuf[512];

    if (maxminddb_asn)
        xfree(maxminddb_asn);
    if ((maxminddb_asn = xstrdup(file))) {
        dsyslogf(LOG_INFO, "Maxmind ASN database %s", maxminddb_asn);
        return 1;
    }

    dsyslogf(LOG_ERR, "unable to set Maxmind ASN database, strdup: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
    return 0;
}

int set_maxminddb_country(const char* file)
{
    char errbuf[512];

    if (maxminddb_country)
        xfree(maxminddb_country);
    if ((maxminddb_country = xstrdup(file))) {
        dsyslogf(LOG_INFO, "Maxmind ASN database %s", maxminddb_country);
        return 1;
    }

    dsyslogf(LOG_ERR, "unable to set Maxmind ASN database, strdup: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
    return 0;
}

int set_pcap_buffer_size(const char* s)
{
    dsyslogf(LOG_INFO, "Setting pcap buffer size to: %s", s);
    pcap_buffer_size = atoi(s);
    if (pcap_buffer_size < 0) {
        dsyslog(LOG_ERR, "pcap_buffer_size can not be negative");
        return 0;
    }
    return 1;
}

void set_no_wait_interval(void)
{
    dsyslog(LOG_INFO, "not waiting on interval sync to start");

    no_wait_interval = 1;
}

int set_pt_timeout(const char* s)
{
    dsyslogf(LOG_INFO, "Setting pcap-thread timeout to: %s", s);
    pt_timeout = atoi(s);
    if (pt_timeout < 0) {
        dsyslog(LOG_ERR, "pcap-thread timeout can not be negative");
        return 0;
    }
    return 1;
}

void set_drop_ip_fragments(void)
{
    dsyslog(LOG_INFO, "dropping ip fragments");

    drop_ip_fragments = 1;
}

int set_dns_port(const char* s)
{
    int port;
    dsyslogf(LOG_INFO, "dns_port %s", s);
    port = atoi(s);
    if (port < 0 || port > 65535) {
        dsyslog(LOG_ERR, "invalid dns_port");
        return 0;
    }
    port53 = port;
    return 1;
}

int set_response_time_mode(const char* s)
{
    if (!strcmp(s, "bucket")) {
        response_time_set_mode(response_time_bucket);
    } else if (!strcmp(s, "log10")) {
        response_time_set_mode(response_time_log10);
    } else if (!strcmp(s, "log2")) {
        response_time_set_mode(response_time_log2);
    } else {
        dsyslogf(LOG_ERR, "invalid response time mode %s", s);
        return 0;
    }
    dsyslogf(LOG_INFO, "set response time mode to %s", s);
    return 1;
}

int set_response_time_max_queries(const char* s)
{
    int max_queries = atoi(s);
    if (max_queries < 1) {
        dsyslogf(LOG_ERR, "invalid response time max queries %s", s);
        return 0;
    }
    response_time_set_max_queries(max_queries);
    dsyslogf(LOG_INFO, "set response time max queries to %d", max_queries);
    return 1;
}

int set_response_time_full_mode(const char* s)
{
    if (!strcmp(s, "drop_query")) {
        response_time_set_full_mode(response_time_drop_query);
    } else if (!strcmp(s, "drop_oldest")) {
        response_time_set_full_mode(response_time_drop_oldest);
    } else {
        dsyslogf(LOG_ERR, "invalid response time full mode %s", s);
        return 0;
    }
    dsyslogf(LOG_INFO, "set response time full mode to %s", s);
    return 1;
}

int set_response_time_max_seconds(const char* s)
{
    int max_seconds = atoi(s);
    if (max_seconds < 1) {
        dsyslogf(LOG_ERR, "invalid response time max seconds %s", s);
        return 0;
    }
    response_time_set_max_sec(max_seconds);
    dsyslogf(LOG_INFO, "set response time max seconds to %d", max_seconds);
    return 1;
}

int set_response_time_max_sec_mode(const char* s)
{
    if (!strcmp(s, "ceil")) {
        response_time_set_max_sec_mode(response_time_ceil);
    } else if (!strcmp(s, "timed_out")) {
        response_time_set_max_sec_mode(response_time_timed_out);
    } else {
        dsyslogf(LOG_ERR, "invalid response time max sec mode %s", s);
        return 0;
    }
    dsyslogf(LOG_INFO, "set response time max sec mode to %s", s);
    return 1;
}

int set_response_time_bucket_size(const char* s)
{
    int bucket_size = atoi(s);
    if (bucket_size < 1) {
        dsyslogf(LOG_ERR, "invalid response time bucket size %s", s);
        return 0;
    }
    response_time_set_bucket_size(bucket_size);
    dsyslogf(LOG_INFO, "set response time bucket size to %d", bucket_size);
    return 1;
}

const char** KnownTLDS = KnownTLDS_static;

int load_knowntlds(const char* file)
{
    FILE*   fp;
    char *  buffer        = 0, *p;
    size_t  bufsize       = 0;
    char**  new_KnownTLDS = 0;
    size_t  new_size      = 0;

    if (KnownTLDS != KnownTLDS_static) {
        dsyslog(LOG_ERR, "Known TLDs already loaded once");
        return 0;
    }

    if (!(fp = fopen(file, "r"))) {
        dsyslogf(LOG_ERR, "unable to open %s", file);
        return 0;
    }

    if (!(new_KnownTLDS = xrealloc(new_KnownTLDS, (new_size + 1) * sizeof(char*)))) {
        dsyslog(LOG_ERR, "out of memory");
        return 0;
    }
    new_KnownTLDS[new_size] = ".";
    new_size++;

    while (getline(&buffer, &bufsize, fp) > 0) {
        for (p = buffer; *p; p++) {
            if (*p == '\r' || *p == '\n') {
                *p = 0;
                break;
            }
            *p = tolower(*p);
        }
        if (buffer[0] == '#') {
            continue;
        }

        if (!(new_KnownTLDS = xrealloc(new_KnownTLDS, (new_size + 1) * sizeof(char*)))) {
            dsyslog(LOG_ERR, "out of memory");
            return 0;
        }
        new_KnownTLDS[new_size] = xstrdup(buffer);
        if (!new_KnownTLDS[new_size]) {
            dsyslog(LOG_ERR, "out of memory");
            return 0;
        }
        new_size++;
    }
    free(buffer);
    fclose(fp);

    if (!(new_KnownTLDS = xrealloc(new_KnownTLDS, (new_size + 1) * sizeof(char*)))) {
        dsyslog(LOG_ERR, "out of memory");
        return 0;
    }
    new_KnownTLDS[new_size] = 0;

    KnownTLDS = (const char**)new_KnownTLDS;
    dsyslogf(LOG_INFO, "loaded %zd known TLDs from %s", new_size - 1, file);

    return 1;
}
