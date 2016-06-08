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

#if HAVE_LIBGEOIP

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <GeoIP.h>

#include "xmalloc.h"
#include "dns_message.h"
#include "md_array.h"
#include "hashtbl.h"
#include "syslog_debug.h"

extern int debug_flag;
extern char * geoip_asn_v4_dat;
extern int geoip_asn_v4_options;
extern char * geoip_asn_v6_dat;
extern int geoip_asn_v6_options;
static hashfunc asn_hashfunc;
static hashkeycmp asn_cmpfunc;

#define MAX_ARRAY_SZ 65536
static hashtbl *theHash = NULL;
static int next_idx = 0;
static GeoIP *geoip = NULL;
static GeoIP *geoip6 = NULL;
static char *ipstr;
static char *nodb = "noGeoIPDB";
static char *unknown = "unknown";
static char *unknown_v4 = "unknown IPv4";
static char *unknown_v6 = "unknown IPv6";

typedef struct {
    char *asn;
    int index;
}      asnobj;

char *
asn_get_from_message(dns_message * m)
{
    transport_message *tm;
    char *asn;
    char *truncate;

    tm = m->tm;

    if (!inXaddr_ntop(&tm->src_ip_addr, ipstr, 80)) {
        dfprint(0, "asn_index: Error converting IP address");
        return(strdup(unknown));
    }
    dfprintf(0, "asn_index: IP %s is IPv%d", ipstr, tm->ip_version);

    switch(tm->ip_version) {
    case 4:
        if (geoip) {
            asn = GeoIP_name_by_addr(geoip,ipstr);
            if (asn == NULL) {
                asn = strdup(unknown_v4); /* strdup uses malloc, just like libgeoip */
            }
        } else {
	        asn = strdup(nodb);
        }
        break;
    case 6:
        if (geoip6) {
            asn = GeoIP_name_by_addr_v6(geoip6,ipstr);
            if (asn == NULL) {
                asn = strdup(unknown_v6); /* strdup uses malloc, just like libgeoip */
            }
            break;
        } else {
	        asn = strdup(nodb);
        }
    default:
        asn = strdup(unknown);
    }
    if (asn == NULL) {
        dfprint(0, "asn_index: Error allocating memory");
        return(NULL);
    }

    dfprintf(0, "asn_index: full network name: %s", asn);

    /* libgeoip reports for networks with the same ASN different network names.
       Probably it uses the network description, not the AS description. Therefore,
       we truncate after the first space and only use the AS number. Mappings
       to AS names must be done in the presenter.
    */
    truncate = strchr(asn,' ');
    if (truncate) {
        *truncate='\0';
    }

    dfprintf(0, "asn_index: truncated network name: %s", asn);
    return (asn);
}

int
asn_indexer(const void *vp)
{
    const dns_message *m = vp;
    char *asn;
    asnobj *obj;
    if (m->malformed)
        return -1;
    asn = asn_get_from_message((dns_message *) m);
    if (asn == NULL)
        return -1;
    if (NULL == theHash) {
        theHash = hash_create(MAX_ARRAY_SZ, asn_hashfunc, asn_cmpfunc,
            1, afree, afree);
        if (NULL == theHash) {
            free(asn);
            return -1;
        }
    }
    if ((obj = hash_find(asn, theHash))) {
        free(asn);
        return obj->index;
    }
    obj = acalloc(1, sizeof(*obj));
    if (NULL == obj) {
        free(asn);
        return -1;
    }
    /* If I directly use the buffer from libgeoip with
         obj->asn = asn;
       then valgrind complains that the buffer is not freed.
       But it does not complain when copying the buffer in my
       own buffer and freeing the buffer from libgeoip */
    obj->asn = astrdup(asn);
    free(asn);
    if (NULL == obj->asn) {
        afree(obj);
        return -1;
    }
    obj->index = next_idx;
    if (0 != hash_add(obj->asn, obj, theHash)) {
        afree(obj->asn);
        afree(obj);
        return -1;
    }
    next_idx++;
    return obj->index;
}

int
asn_iterator(char **label)
{
    asnobj *obj;
    static char label_buf[128];
    if (0 == next_idx)
        return -1;
    if (NULL == label) {
        /* initialize and tell caller how big the array is */
        hash_iter_init(theHash);
        return next_idx;
    }
    if ((obj = hash_iterate(theHash)) == NULL)
        return -1;
    snprintf(label_buf, 128, "%s", obj->asn);
    *label = label_buf;
    return obj->index;
}

void
asn_reset()
{
    theHash = NULL;
    next_idx = 0;
}

static unsigned int
asn_hashfunc(const void *key)
{
    return hashendian(key, strlen(key), 0);
}

static int
asn_cmpfunc(const void *a, const void *b)
{
    return strcasecmp(a, b);
}

void
asn_indexer_init()
{
    if (geoip_asn_v4_dat) {
        geoip = GeoIP_open(geoip_asn_v4_dat, geoip_asn_v4_options);
        if (geoip == NULL) {
            dsyslog(LOG_ERR, "asn_index: Error opening IPv4 ASNum DB. Make sure libgeoip's GeoIPASNum.dat file is available");
            exit(1);
        }
    }
    if (geoip_asn_v6_dat) {
        geoip6 = GeoIP_open(geoip_asn_v6_dat, geoip_asn_v6_options);
        if (geoip6 == NULL) {
            dsyslog(LOG_ERR, "asn_index: Error opening IPv6 ASNum DB. Make sure libgeoip's GeoIPASNumv6.dat file is available");
            exit(1);
        }
    }
    ipstr = malloc(80);
    if (!ipstr) {
        dsyslog(LOG_ERR, "asn_index: Error allocating memory");
        exit(1);
    }
    if (geoip || geoip6) {
        dsyslog(LOG_INFO, "asn_index: Sucessfully initialized GeoIP ASN");
    }
    else {
        dsyslog(LOG_INFO, "asn_index: No database loaded for GeoIP ASN");
    }
}

#endif

