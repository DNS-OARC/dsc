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
#include <GeoIP.h>

#include "xmalloc.h"
#include "dns_message.h"
#include "md_array.h"
#include "hashtbl.h"

extern int debug_flag;
static hashfunc country_hashfunc;
static hashkeycmp country_cmpfunc;

#define MAX_ARRAY_SZ 65536
static hashtbl *theHash = NULL;
static int next_idx = 0;
static GeoIP *geoip;
static GeoIP *geoip6;
static char *ipstr;
static char *unknown = "??";
static char *unknown_v4 = "?4";
static char *unknown_v6 = "?6";

typedef struct
{
    char *country;
    int index;
} countryobj;

const char *
country_get_from_message(dns_message * m)
{
    transport_message *tm;
    const char *cc;

    tm = m->tm;
    if (!inXaddr_ntop(&tm->src_ip_addr, ipstr, 80)) {
        if (debug_flag)
            fprintf(stderr, "country_index: Error converting IP address.\n");
        return(unknown);
    }
    switch(tm->ip_version) {
    case 4:
        cc = GeoIP_country_code_by_addr(geoip,ipstr);
        if (cc == NULL) {
            cc = unknown_v4;
        }
        break;
    case 6:
        cc = GeoIP_country_code_by_addr_v6(geoip6,ipstr);
        if (cc == NULL) {
            cc = unknown_v6;
        }
        break;
    default:
        cc = unknown;
    }
    if (debug_flag)
        fprintf(stderr, "country_index: country code: %s\n", cc);
    return (cc);
}

int
country_indexer(const void *vp)
{
    const dns_message *m = vp;
    const char *country;
    countryobj *obj;
    if (m->malformed)
        return -1;
    country = country_get_from_message((dns_message *) m);
    if (NULL == theHash) {
        theHash = hash_create(MAX_ARRAY_SZ, country_hashfunc, country_cmpfunc, 1, afree, afree);
        if (NULL == theHash)
            return -1;
    }
    if ((obj = hash_find(country, theHash)))
        return obj->index;
    obj = acalloc(1, sizeof(*obj));
    if (NULL == obj)
        return -1;
    obj->country = astrdup(country);
    if (NULL == obj->country) {
        afree(obj);
        return -1;
    }
    obj->index = next_idx;
    if (0 != hash_add(obj->country, obj, theHash)) {
        afree(obj->country);
        afree(obj);
        return -1;
    }
    next_idx++;
    return obj->index;
}

int
country_iterator(char **label)
{
    countryobj *obj;
    static char label_buf[MAX_QNAME_SZ];
    if (0 == next_idx)
        return -1;
    if (NULL == label) {
        /* initialize and tell caller how big the array is */
        hash_iter_init(theHash);
        return next_idx;
    }
    if ((obj = hash_iterate(theHash)) == NULL)
        return -1;
    snprintf(label_buf, MAX_QNAME_SZ, "%s", obj->country);
    *label = label_buf;
    return obj->index;
}

void
country_reset()
{
    theHash = NULL;
    next_idx = 0;
}

static unsigned int
country_hashfunc(const void *key)
{
    return hashendian(key, strlen(key), 0);
}

static int
country_cmpfunc(const void *a, const void *b)
{
    return strcasecmp(a, b);
}

void
country_indexer_init()
{
    geoip = GeoIP_open_type(GEOIP_COUNTRY_EDITION, GEOIP_MEMORY_CACHE | GEOIP_CHECK_CACHE);
    if (geoip == NULL) {
        fprintf(stderr, "country_index: Error opening IPv4 Country DB. Make sure libgeoip's GeoIP.dat file is available.\n");
        exit(1);
    }
    geoip6 = GeoIP_open_type(GEOIP_COUNTRY_EDITION_V6, GEOIP_MEMORY_CACHE | GEOIP_CHECK_CACHE);
    if (geoip6 == NULL) {
        fprintf(stderr, "country_index: Error opening IPv6 Country DB. Make sure libgeoip's GeoIPv6.dat file is available.\n");
        exit(1);
    }
    ipstr = malloc(80);
    if (!ipstr) {
        fprintf(stderr, "country_index: Error allocating memory.\n");
        exit(1);
    }
    if (debug_flag)
        fprintf(stderr, "country_index: Sucessfully initialized GeoIP.\n");
}

#endif

