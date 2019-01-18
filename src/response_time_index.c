/*
 * Copyright (c) 2008-2018, OARC, Inc.
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

#include "response_time_index.h"
#include "hashtbl.h"
#include "inX_addr.h"
#include "xmalloc.h"
#include "syslog_debug.h"
#include "pcap.h"
#include "compat.h"

#include <math.h>
#include <assert.h>

#define TIMED_OUT 0
#define MISSING_QUERY 1
#define DROPPED_QUERY 2
#define INTERNAL_ERROR 3
#define FIRST_BUCKET 4

struct query;
struct query {
    struct query *    prev, *next;
    transport_message tm;
    dns_message       m;
};

#define MAX_ARRAY_SZ 65536
static hashtbl*                        theHash      = 0;
static enum response_time_mode         mode         = response_time_log10;
static time_t                          max_sec      = 5;
static enum response_time_max_sec_mode max_sec_mode = response_time_ceil;
static unsigned int                    bucket_size  = 100;
static size_t                          max_queries = 1000000, num_queries = 0;
static struct query *                  qfirst = 0, *qlast = 0;
static int                             max_iter = INTERNAL_ERROR, next_iter, flushing = 0;
static enum response_time_full_mode    full_mode = response_time_drop_query;

static unsigned int _hash(const struct query* q)
{
    if (q->m.qr)
        return inXaddr_hash(&q->tm.dst_ip_addr) ^ ((q->tm.dst_port & 0xffff) | (q->m.id << 16));
    return inXaddr_hash(&q->tm.src_ip_addr) ^ ((q->tm.src_port & 0xffff) | (q->m.id << 16));
}

static int _cmp(const struct query* a, const struct query* b)
{
    int cmp;

    // compare DNS ID
    if (a->m.id != b->m.id)
        return a->m.id < b->m.id ? -1 : 1;

    // compare IP version, since inXaddr_cmp() does not, and protocol
    if (a->tm.ip_version != b->tm.ip_version)
        return a->tm.ip_version < b->tm.ip_version ? -1 : 1;
    if (a->tm.proto != b->tm.proto)
        return a->tm.proto < b->tm.proto ? -1 : 1;

    // compare client IP&port first since other should be server and more static data
    if (!a->m.qr && !b->m.qr) {
        // both are queries, compare source address first as that is the client
        if ((cmp = inXaddr_cmp(&a->tm.src_ip_addr, &b->tm.src_ip_addr)))
            return cmp;
        if (a->tm.src_port != b->tm.src_port)
            return a->tm.src_port < b->tm.src_port ? -1 : 1;
        if ((cmp = inXaddr_cmp(&a->tm.dst_ip_addr, &b->tm.dst_ip_addr)))
            return cmp;
        if (a->tm.dst_port != b->tm.dst_port)
            return a->tm.dst_port < b->tm.dst_port ? -1 : 1;
    } else if (a->m.qr && b->m.qr) {
        // both are responses, compare destination address first as that is the client
        if ((cmp = inXaddr_cmp(&a->tm.dst_ip_addr, &b->tm.dst_ip_addr)))
            return cmp;
        if (a->tm.dst_port != b->tm.dst_port)
            return a->tm.dst_port < b->tm.dst_port ? -1 : 1;
        if ((cmp = inXaddr_cmp(&a->tm.src_ip_addr, &b->tm.src_ip_addr)))
            return cmp;
        if (a->tm.src_port != b->tm.src_port)
            return a->tm.src_port < b->tm.src_port ? -1 : 1;
    } else if (a->m.qr && !b->m.qr) {
        // a is a response and b is a query, compare a's destination with b's source first
        if ((cmp = inXaddr_cmp(&a->tm.dst_ip_addr, &b->tm.src_ip_addr)))
            return cmp;
        if (a->tm.dst_port != b->tm.src_port)
            return a->tm.dst_port < b->tm.src_port ? -1 : 1;
        if ((cmp = inXaddr_cmp(&a->tm.src_ip_addr, &b->tm.dst_ip_addr)))
            return cmp;
        if (a->tm.src_port != b->tm.dst_port)
            return a->tm.src_port < b->tm.dst_port ? -1 : 1;
    } else {
        // a is a query and b is a response, compare a's source with b's destination first
        if ((cmp = inXaddr_cmp(&a->tm.src_ip_addr, &b->tm.dst_ip_addr)))
            return cmp;
        if (a->tm.src_port != b->tm.dst_port)
            return a->tm.src_port < b->tm.dst_port ? -1 : 1;
        if ((cmp = inXaddr_cmp(&a->tm.dst_ip_addr, &b->tm.src_ip_addr)))
            return cmp;
        if (a->tm.dst_port != b->tm.src_port)
            return a->tm.dst_port < b->tm.src_port ? -1 : 1;
    }

    return 0;
}

int response_time_indexer(const dns_message* m)
{
    struct query       q, *obj;
    transport_message* tm  = m->tm;
    int                ret = -1;

    if (flushing) {
        dfprintf(1, "response_time: flushing %u %s", m->id, m->qname);
        return TIMED_OUT;
    }

    if (m->malformed) {
        return -1;
    }

    dfprintf(1, "response_time: %s %u %s", m->qr ? "response" : "query", m->id, m->qname);

    if (!theHash) {
        theHash = hash_create(MAX_ARRAY_SZ, (hashfunc*)_hash, (hashkeycmp*)_cmp, 0, 0, 0);
        if (!theHash)
            return INTERNAL_ERROR;
    }

    q.m     = *m;
    q.tm    = *tm;
    q.m.tm  = &q.tm;
    q.m.tld = 0;

    obj = hash_find(&q, theHash);

    if (m->qr) {
        struct timeval diff;
        unsigned long  us;
        int            iter;

        if (!obj) {
            // got a response without a query,
            dfprint(1, "response_time: missing query for response");
            return MISSING_QUERY;
        }

        // TODO: compare more?
        // - qclass/qtype, qname

        // found query, remove and calculate index
        if (obj->prev)
            obj->prev->next = obj->next;
        if (obj->next)
            obj->next->prev = obj->prev;
        if (obj == qfirst)
            qfirst = obj->next;
        if (obj == qlast)
            qlast = obj->prev;
        hash_remove(obj, theHash);
        num_queries--;

        q      = *obj;
        q.m.tm = &q.tm;
        xfree(obj);

        diff.tv_sec  = tm->ts.tv_sec - q.tm.ts.tv_sec;
        diff.tv_usec = tm->ts.tv_usec - q.tm.ts.tv_usec;
        if (diff.tv_usec >= 1000000) {
            diff.tv_sec += 1;
            diff.tv_usec -= 1000000;
        } else if (diff.tv_usec < 0) {
            diff.tv_sec -= 1;
            diff.tv_usec += 1000000;
        }

        if (diff.tv_sec < 0 || diff.tv_usec < 0) {
            dfprintf(1, "response_time: bad diff " PRItime ", " PRItime " - " PRItime, diff.tv_sec, diff.tv_usec, q.tm.ts.tv_sec, q.tm.ts.tv_usec, tm->ts.tv_sec, tm->ts.tv_usec);
            return INTERNAL_ERROR;
        }
        if (diff.tv_sec >= max_sec) {
            switch (max_sec_mode) {
            case response_time_ceil:
                dfprintf(2, "response_time: diff " PRItime " ceiled to " PRItime, diff.tv_sec, diff.tv_usec, max_sec, 0L);
                diff.tv_sec  = max_sec;
                diff.tv_usec = 0;
                break;
            case response_time_timed_out:
                dfprintf(1, "response_time: diff " PRItime " too old, timed out", diff.tv_sec, diff.tv_usec);
                return TIMED_OUT;
            default:
                dfprint(1, "response_time: bad max_sec_mode");
                return INTERNAL_ERROR;
            }
        }

        us = (diff.tv_sec * 1000000) + diff.tv_usec;
        switch (mode) {
        case response_time_bucket:
            iter = FIRST_BUCKET + (us / bucket_size);
            dfprintf(2, "response_time: found q/r us:%lu, put in bucket %d (%lu-%lu usec)", us, iter, (us / bucket_size) * bucket_size, ((us / bucket_size) + 1) * bucket_size);
            break;
        case response_time_log10: {
            double d = log10(us);
            if (d < 0) {
                dfprintf(1, "response_time: bad log10(%lu) ret %f", us, d);
                return INTERNAL_ERROR;
            }
            iter = FIRST_BUCKET + (int)d;
            dfprintf(2, "response_time: found q/r us:%lu, log10 %d (%.0f-%.0f usec)", us, iter, pow(10, (int)d), pow(10, (int)d + 1));
            break;
        }
        case response_time_log2: {
            double d = log2(us);
            if (d < 0) {
                dfprintf(1, "response_time: bad log2(%lu) ret %f", us, d);
                return INTERNAL_ERROR;
            }
            iter = FIRST_BUCKET + (int)d;
            dfprintf(2, "response_time: found q/r us:%lu, log2 %d (%.0f-%.0f usec)", us, iter, pow(2, (int)d), pow(2, (int)d + 1));
            break;
        }
        default:
            dfprint(1, "response_time: bad mode");
            return INTERNAL_ERROR;
        }

        if (iter > max_iter)
            max_iter = iter;
        return iter;
    }

    if (obj) {
        // Found another query in the hash so the old one have timed out,
        // reuse the obj for the new query
        obj->tm.ts = tm->ts;
        if (obj != qlast) {
            if (obj->prev)
                obj->prev->next = obj->next;
            if (obj->next) {
                if (obj == qfirst)
                    qfirst      = obj->next;
                obj->next->prev = obj->prev;
            }
            obj->prev = qlast;
            obj->next = 0;
            assert(qlast);
            qlast->next = obj;
            qlast       = obj;
        }
        dfprintf(1, "response_time: reuse %p, timed out", obj);
        return TIMED_OUT;
    }

    if (num_queries >= max_queries) {
        // We're at max, see if we can time out the oldest query
        ret = TIMED_OUT;
        assert(qfirst);
        if (tm->ts.tv_sec - qfirst->tm.ts.tv_sec < max_sec) {
            // no, so what to do?
            switch (full_mode) {
            case response_time_drop_query:
                dfprint(1, "response_time: full and oldest not old enough");
                return DROPPED_QUERY;
            case response_time_drop_oldest:
                ret = DROPPED_QUERY;
                dfprint(2, "response_time: full and dropping oldest");
                break;
            default:
                dfprint(1, "response_time: bad full_mode");
                return INTERNAL_ERROR;
            }
        }

        // remove oldest obj from hash and reuse it
        obj    = qfirst;
        qfirst = obj->next;
        if (qfirst)
            qfirst->prev = 0;
        hash_remove(obj, theHash);
        num_queries--;
        dfprintf(1, "response_time: reuse %p, too old", obj);
    } else {
        obj = xcalloc(1, sizeof(*obj));
        if (!obj) {
            dfprint(1, "response_time: failed to alloc obj");
            return INTERNAL_ERROR;
        }
    }

    *obj      = q;
    obj->m.tm = &obj->tm;
    if (hash_add(obj, obj, theHash)) {
        xfree(obj);
        dfprint(1, "response_time: failed to add to hash");
        return INTERNAL_ERROR;
    }

    obj->prev = qlast;
    obj->next = 0;
    if (qlast)
        qlast->next = obj;
    qlast           = obj;
    if (!qfirst)
        qfirst = obj;
    num_queries++;
    dfprintf(2, "response_time: add %p, %lu/%lu queries", obj, num_queries, max_queries);

    return ret;
}

int response_time_iterator(const char** label)
{
    char label_buf[128];

    if (!label) {
        next_iter = 0;
        return max_iter + 1;
    }
    if (next_iter > max_iter) {
        return -1;
    }

    if (next_iter < FIRST_BUCKET) {
        switch (next_iter) {
        case TIMED_OUT:
            *label = "timeouts";
            break;
        case MISSING_QUERY:
            *label = "missing_queries";
            break;
        case DROPPED_QUERY:
            *label = "dropped_queries";
            break;
        case INTERNAL_ERROR:
            *label = "internal_errors";
            break;
        default:
            return -1;
        }
    } else {
        switch (mode) {
        case response_time_bucket:
            snprintf(label_buf, 128, "%d-%d", (next_iter - FIRST_BUCKET) * bucket_size, (next_iter - FIRST_BUCKET + 1) * bucket_size);
            break;
        case response_time_log10:
            snprintf(label_buf, 128, "%.0f-%.0f", pow(10, next_iter - FIRST_BUCKET), pow(10, next_iter - FIRST_BUCKET + 1));
            break;
        case response_time_log2:
            snprintf(label_buf, 128, "%.0f-%.0f", pow(2, next_iter - FIRST_BUCKET), pow(2, next_iter - FIRST_BUCKET + 1));
            break;
        default:
            return -1;
        }
        *label = label_buf;
    }

    return next_iter++;
}

void response_time_reset()
{
    max_iter = INTERNAL_ERROR;
}

static struct query* flushed_obj = 0;

const dns_message* response_time_flush(enum flush_mode fm)
{
    switch (fm) {
    case flush_get:
        if (qfirst && last_ts.tv_sec - qfirst->tm.ts.tv_sec >= max_sec) {
            if (flushed_obj)
                xfree(flushed_obj);

            flushed_obj = qfirst;
            qfirst      = flushed_obj->next;
            if (qfirst)
                qfirst->prev = 0;
            hash_remove(flushed_obj, theHash);
            num_queries--;
            return &flushed_obj->m;
        }
        break;
    case flush_on:
        flushing = 1;
        break;
    case flush_off:
        if (flushed_obj) {
            xfree(flushed_obj);
            flushed_obj = 0;
        }
        flushing = 0;
        break;
    default:
        break;
    }

    return 0;
}
