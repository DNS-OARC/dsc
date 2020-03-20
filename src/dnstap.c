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

#include "dnstap.h"

#include "syslog_debug.h"
#include "dns_message.h"
#include "config_hooks.h"
#include "xmalloc.h"

char* dnstap_network_ip4  = 0;
char* dnstap_network_ip6  = 0;
int   dnstap_network_port = -1;

#include <string.h>

extern struct timeval last_ts;
static struct timeval start_ts, finish_ts;

#ifdef USE_DNSTAP

#include <uv.h>
#include <dnswire/reader.h>
#include <stdlib.h>
#include <stdio.h>

// print_dnstap():
#include <dnswire/dnstap.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUF_SIZE 4096

static const char* printable_string(const uint8_t* data, size_t len)
{
    static char buf[512];
    size_t      r = 0, w = 0;

    while (r < len && w < sizeof(buf) - 1) {
        if (isprint(data[r])) {
            buf[w++] = data[r++];
        } else {
            if (w + 4 >= sizeof(buf) - 1) {
                break;
            }

            sprintf(&buf[w], "\\x%02x", data[r++]);
            w += 4;
        }
    }
    if (w >= sizeof(buf)) {
        buf[sizeof(buf) - 1] = 0;
    } else {
        buf[w] = 0;
    }

    return buf;
}

static const char* printable_ip_address(const uint8_t* data, size_t len)
{
    static char buf[INET6_ADDRSTRLEN];

    buf[0] = 0;
    if (len == 4) {
        inet_ntop(AF_INET, data, buf, sizeof(buf));
    } else if (len == 16) {
        inet_ntop(AF_INET6, data, buf, sizeof(buf));
    }

    return buf;
}

static void print_dnstap(const struct dnstap* d)
{
    dsyslog(LOG_DEBUG, "---- dnstap");
    if (dnstap_has_identity(*d)) {
        dsyslogf(LOG_DEBUG, "identity: %s", printable_string(dnstap_identity(*d), dnstap_identity_length(*d)));
    }
    if (dnstap_has_version(*d)) {
        dsyslogf(LOG_DEBUG, "version: %s", printable_string(dnstap_version(*d), dnstap_version_length(*d)));
    }
    if (dnstap_has_extra(*d)) {
        dsyslogf(LOG_DEBUG, "extra: %s", printable_string(dnstap_extra(*d), dnstap_extra_length(*d)));
    }

    if (dnstap_type(*d) == DNSTAP_TYPE_MESSAGE && dnstap_has_message(*d)) {
        dsyslogf(LOG_DEBUG, "message:  type: %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*d)]);

        if (dnstap_message_has_query_time_sec(*d) && dnstap_message_has_query_time_nsec(*d)) {
            dsyslogf(LOG_DEBUG, "  query_time: %" PRIu64 ".%" PRIu32 "", dnstap_message_query_time_sec(*d), dnstap_message_query_time_nsec(*d));
        }
        if (dnstap_message_has_response_time_sec(*d) && dnstap_message_has_response_time_nsec(*d)) {
            dsyslogf(LOG_DEBUG, "  response_time: %" PRIu64 ".%" PRIu32 "", dnstap_message_response_time_sec(*d), dnstap_message_response_time_nsec(*d));
        }
        if (dnstap_message_has_socket_family(*d)) {
            dsyslogf(LOG_DEBUG, "  socket_family: %s", DNSTAP_SOCKET_FAMILY_STRING[dnstap_message_socket_family(*d)]);
        }
        if (dnstap_message_has_socket_protocol(*d)) {
            dsyslogf(LOG_DEBUG, "  socket_protocol: %s", DNSTAP_SOCKET_PROTOCOL_STRING[dnstap_message_socket_protocol(*d)]);
        }
        if (dnstap_message_has_query_address(*d)) {
            dsyslogf(LOG_DEBUG, "  query_address: %s", printable_ip_address(dnstap_message_query_address(*d), dnstap_message_query_address_length(*d)));
        }
        if (dnstap_message_has_query_port(*d)) {
            dsyslogf(LOG_DEBUG, "  query_port: %u", dnstap_message_query_port(*d));
        }
        if (dnstap_message_has_response_address(*d)) {
            dsyslogf(LOG_DEBUG, "  response_address: %s", printable_ip_address(dnstap_message_response_address(*d), dnstap_message_response_address_length(*d)));
        }
        if (dnstap_message_has_response_port(*d)) {
            dsyslogf(LOG_DEBUG, "  response_port: %u", dnstap_message_response_port(*d));
        }
        if (dnstap_message_has_query_zone(*d)) {
            dsyslogf(LOG_DEBUG, "  query_zone: %s", printable_string(dnstap_message_query_zone(*d), dnstap_message_query_zone_length(*d)));
        }
        if (dnstap_message_has_query_message(*d)) {
            dsyslogf(LOG_DEBUG, "  query_message_length: %lu", dnstap_message_query_message_length(*d));
            dsyslogf(LOG_DEBUG, "  query_message: %s", printable_string(dnstap_message_query_message(*d), dnstap_message_query_message_length(*d)));
        }
        if (dnstap_message_has_response_message(*d)) {
            dsyslogf(LOG_DEBUG, "  response_message_length: %lu", dnstap_message_response_message_length(*d));
            dsyslogf(LOG_DEBUG, "  response_message: %s", printable_string(dnstap_message_response_message(*d), dnstap_message_response_message_length(*d)));
        }
    }

    dsyslog(LOG_DEBUG, "----");
}

enum client_state {
    no_state,
    writing_start,
    started,
    writing_frame,
};

struct client;
struct client {
    struct client*    next;
    size_t            id;
    enum client_state state;
    uv_pipe_t         unix_conn;
    uv_tcp_t          tcp_conn;
    uv_udp_t          udp_conn;
    uv_stream_t*      stream;

    char    rbuf[BUF_SIZE];
    ssize_t read;
    size_t  pushed;

    uv_write_t            wreq;
    uv_buf_t              wbuf;
    uint8_t               _wbuf[BUF_SIZE];
    struct dnswire_reader reader;

    int finished;
};

static struct client* clients   = 0;
static size_t         client_id = 1;

static struct client* client_new()
{
    struct client* c = xmalloc(sizeof(struct client));
    if (c) {
        c->unix_conn.data = c;
        c->tcp_conn.data  = c;
        c->udp_conn.data  = c;
        c->next           = clients;
        c->id             = client_id++;
        c->state          = no_state;
        c->wbuf.base      = (void*)c->_wbuf;
        c->finished       = 0;
        if (dnswire_reader_init(&c->reader) != dnswire_ok) {
            xfree(c);
            return 0;
        }
        if (dnswire_reader_allow_bidirectional(&c->reader, true) != dnswire_ok) {
            dnswire_reader_destroy(c->reader);
            xfree(c);
            return 0;
        }
        clients = c;
    }
    return c;
}

static void client_close(uv_handle_t* handle)
{
    struct client* c = handle->data;

    dsyslogf(LOG_DEBUG, "client %zu closed/freed", c->id);

    if (clients == c) {
        clients = c->next;
    } else {
        struct client* prev = clients;

        while (prev) {
            if (prev->next == c) {
                prev->next = c->next;
                break;
            }
            prev = prev->next;
        }
    }

    dnswire_reader_destroy(c->reader);
    xfree(c);
}

static void client_alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
{
    buf->base = ((struct client*)handle->data)->rbuf;
    buf->len  = BUF_SIZE;
}

static int dnstap_handler(const struct dnstap* m);

static void client_write(uv_write_t* req, int status);

static int process_rbuf(uv_stream_t* handle, struct client* c)
{
    int done = 0;
    dsyslogf(LOG_DEBUG, "client %zu pushing %zu of %zd", c->id, c->pushed, c->read);
    if (c->pushed >= c->read) {
        done = 1;
    }
    while (!done) {
        size_t              out_len = sizeof(c->_wbuf);
        enum dnswire_result res     = dnswire_reader_push(&c->reader, (uint8_t*)&c->rbuf[c->pushed], c->read - c->pushed, c->_wbuf, &out_len);

        c->pushed += dnswire_reader_pushed(c->reader);
        if (c->pushed >= c->read) {
            done = 1;
        }
        dsyslogf(LOG_DEBUG, "client %zu pushed %zu of %zd", c->id, c->pushed, c->read);

        switch (res) {
        case dnswire_have_dnstap:
            dnstap_handler(dnswire_reader_dnstap(c->reader));
            done = 0;
            break;
        case dnswire_need_more:
            break;
        case dnswire_again:
            done = 0;
            break;
        case dnswire_endofdata:
            if (out_len) {
                c->finished = 1;
                dsyslogf(LOG_DEBUG, "client %zu finishing %zu", c->id, out_len);
                uv_read_stop(handle);
                c->wbuf.len = out_len;
                uv_write((uv_write_t*)&c->wreq, handle, &c->wbuf, 1, client_write);
                return 0;
            }
            uv_close((uv_handle_t*)handle, client_close);
            return 0;
        default:
            dsyslog(LOG_ERR, "dnswire_reader_push() error");
            uv_close((uv_handle_t*)handle, client_close);
            return 0;
        }

        if (out_len) {
            dsyslogf(LOG_DEBUG, "client %zu writing %zu", c->id, out_len);
            uv_read_stop(handle);
            c->wbuf.len = out_len;
            uv_write((uv_write_t*)&c->wreq, handle, &c->wbuf, 1, client_write);
            return 0;
        }
    }

    return 1;
}

static void client_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
{
    struct client* c = handle->data;

    if (nread < 0) {
        if (nread != UV_EOF) {
            dsyslogf(LOG_ERR, "client_read() error: %s", uv_err_name(nread));
        } else {
            dsyslogf(LOG_DEBUG, "client %zu disconnected", c->id);
        }
        uv_close((uv_handle_t*)handle, client_close);
        return;
    }
    if (nread > 0) {
        dsyslogf(LOG_DEBUG, "client %zu read %zd", c->id, nread);
        c->read   = nread;
        c->pushed = 0;
        process_rbuf(handle, c);
    }
}

static void client_write(uv_write_t* req, int status)
{
    if (status) {
        dsyslogf(LOG_ERR, "client_write() error: %s", uv_strerror(status));
        uv_close((uv_handle_t*)req->handle, client_close);
        return;
    }

    struct client* c = req->handle->data;
    if (process_rbuf(req->handle, c)) {
        if (c->finished) {
            dsyslogf(LOG_DEBUG, "client %zu finished", c->id);
            uv_close((uv_handle_t*)req->handle, client_close);
            return;
        }
        uv_read_start(c->stream, client_alloc_buffer, client_read);
    }
}

static void on_new_unix_connection(uv_stream_t* server, int status)
{
    if (status < 0) {
        dsyslogf(LOG_ERR, "on_new_unix_connection() error: %s", uv_strerror(status));
        return;
    }

    struct client* client = client_new();
    if (!client) {
        dsyslog(LOG_ERR, "on_new_unix_connection() out of memory");
        return;
    }

    uv_pipe_init(uv_default_loop(), &client->unix_conn, 0);
    client->stream = (uv_stream_t*)&client->unix_conn;
    if (uv_accept(server, client->stream) == 0) {
        dsyslogf(LOG_DEBUG, "client %zu connected", client->id);

        uv_read_start(client->stream, client_alloc_buffer, client_read);
    } else {
        uv_close((uv_handle_t*)client->stream, client_close);
    }
}

static void on_new_tcp_connection(uv_stream_t* server, int status)
{
    dsyslog(LOG_DEBUG, "new tcp conn");

    if (status < 0) {
        dsyslogf(LOG_ERR, "on_new_tcp_connection() error: %s", uv_strerror(status));
        return;
    }

    struct client* client = client_new();
    if (!client) {
        dsyslog(LOG_ERR, "on_new_tcp_connection() out of memory");
        return;
    }

    uv_tcp_init(uv_default_loop(), &client->tcp_conn);
    client->stream = (uv_stream_t*)&client->tcp_conn;
    if (uv_accept(server, (uv_stream_t*)client->stream) == 0) {
        dsyslogf(LOG_DEBUG, "client %zu connected", client->id);

        uv_read_start((uv_stream_t*)client->stream, client_alloc_buffer, client_read);
    } else {
        uv_close((uv_handle_t*)client->stream, client_close);
    }
}

static void on_new_udp_connection(uv_stream_t* server, int status)
{
    if (status < 0) {
        dsyslogf(LOG_ERR, "on_new_udp_connection() error: %s", uv_strerror(status));
        return;
    }

    struct client* client = client_new();
    if (!client) {
        dsyslog(LOG_ERR, "on_new_udp_connection() out of memory");
        return;
    }

    uv_udp_init(uv_default_loop(), &client->udp_conn);
    client->stream = (uv_stream_t*)&client->udp_conn;
    if (uv_accept(server, client->stream) == 0) {
        dsyslogf(LOG_DEBUG, "client %zu connected", client->id);

        uv_read_start(client->stream, client_alloc_buffer, client_read);
    } else {
        uv_close((uv_handle_t*)client->stream, client_close);
    }
}

static uv_pipe_t unix_server;
static uv_tcp_t  tcp_server;
static uv_udp_t  udp_server;

extern int dns_protocol_handler(const u_char* buf, int len, void* udata);
extern uint64_t statistics_interval;

static int _set_ipv(transport_message* tm, const struct dnstap* m)
{
    switch (dnstap_message_socket_family(*m)) {
    case DNSTAP_SOCKET_FAMILY_INET:
        tm->ip_version = 4;
        break;
    case DNSTAP_SOCKET_FAMILY_INET6:
        tm->ip_version = 6;
        break;
    default:
        return -1;
    }
    return 0;
}

static int _set_proto(transport_message* tm, const struct dnstap* m)
{
    switch (dnstap_message_socket_protocol(*m)) {
    case DNSTAP_SOCKET_PROTOCOL_UDP:
        tm->proto = IPPROTO_UDP;
        break;
    case DNSTAP_SOCKET_PROTOCOL_TCP:
        tm->proto = IPPROTO_TCP;
        break;
    default:
        return -1;
    }
    return 0;
}

static int _set_addr(inX_addr* addr, const uint8_t* data, const size_t len)
{
    if (len == sizeof(struct in_addr)) {
        return inXaddr_assign_v4(addr, (const struct in_addr*)data);
    } else if (len == sizeof(struct in6_addr)) {
        return inXaddr_assign_v6(addr, (const struct in6_addr*)data);
    }
    return -1;
}

static int dnstap_handler(const struct dnstap* m)
{
    transport_message tm;

    if (!dnstap_message_has_socket_family(*m)
        || !dnstap_message_has_socket_protocol(*m)
        || (dnstap_message_has_query_message(*m) && (!dnstap_message_has_query_time_sec(*m) || !dnstap_message_has_query_time_nsec(*m)))
        || (dnstap_message_has_response_message(*m) && (!dnstap_message_has_response_time_sec(*m) || !dnstap_message_has_response_time_nsec(*m)))
        || (dnstap_message_has_query_port(*m) && !dnstap_message_has_query_address(*m))
        || (dnstap_message_has_response_port(*m) && !dnstap_message_has_response_address(*m))) {
        dsyslog(LOG_ERR, "missing part of dnstap message");
        return -1;
    }

    memset(&tm, 0, sizeof(tm));

    print_dnstap(m);

    switch (dnstap_message_type(*m)) {
    case DNSTAP_MESSAGE_TYPE_AUTH_QUERY:
        if (!dnstap_message_has_query_message(*m) || !dnstap_message_has_query_address(*m)) {
            dsyslogf(LOG_ERR, "missing parts of %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
            break;
        }

        if (_set_ipv(&tm, m)
            || _set_proto(&tm, m)
            || _set_addr(&tm.src_ip_addr, dnstap_message_query_address(*m), dnstap_message_query_address_length(*m))) {
            dsyslogf(LOG_ERR, "unable to extract values from %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
            break;
        }
        tm.src_port   = dnstap_message_query_port(*m);
        tm.ts.tv_sec  = dnstap_message_query_time_sec(*m);
        tm.ts.tv_usec = dnstap_message_query_time_nsec(*m) / 1000;

        if (dnstap_message_has_response_address(*m)) {
            if (_set_addr(&tm.dst_ip_addr, dnstap_message_response_address(*m), dnstap_message_response_address_length(*m))) {
                dsyslogf(LOG_ERR, "unable to extract values from %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
                break;
            }
            tm.dst_port = dnstap_message_response_port(*m);
        } else {
            if (tm.ip_version == 4)
                inXaddr_pton(dnstap_network_ip4, &tm.dst_ip_addr);
            else
                inXaddr_pton(dnstap_network_ip6, &tm.dst_ip_addr);
            tm.dst_port = dnstap_network_port;
        }

        last_ts = tm.ts;
        dns_protocol_handler(dnstap_message_query_message(*m), dnstap_message_query_message_length(*m), &tm);
        break;

    case DNSTAP_MESSAGE_TYPE_AUTH_RESPONSE:
        if (!dnstap_message_has_response_message(*m) || !dnstap_message_has_query_address(*m)) {
            dsyslogf(LOG_ERR, "missing parts of %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
            break;
        }

        if (_set_ipv(&tm, m)
            || _set_proto(&tm, m)
            || _set_addr(&tm.dst_ip_addr, dnstap_message_query_address(*m), dnstap_message_query_address_length(*m))) {
            dsyslogf(LOG_ERR, "unable to extract values from %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
            break;
        }
        tm.dst_port   = dnstap_message_query_port(*m);
        tm.ts.tv_sec  = dnstap_message_response_time_sec(*m);
        tm.ts.tv_usec = dnstap_message_response_time_nsec(*m) / 1000;

        if (dnstap_message_has_response_address(*m)) {
            if (_set_addr(&tm.src_ip_addr, dnstap_message_response_address(*m), dnstap_message_response_address_length(*m))) {
                dsyslogf(LOG_ERR, "unable to extract values from %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
                break;
            }
            tm.src_port = dnstap_message_response_port(*m);
        } else {
            if (tm.ip_version == 4)
                inXaddr_pton(dnstap_network_ip4, &tm.src_ip_addr);
            else
                inXaddr_pton(dnstap_network_ip6, &tm.src_ip_addr);
            tm.src_port = dnstap_network_port;
        }

        last_ts = tm.ts;
        dns_protocol_handler(dnstap_message_response_message(*m), dnstap_message_response_message_length(*m), &tm);
        break;

    case DNSTAP_MESSAGE_TYPE_RESOLVER_QUERY:
        if (!dnstap_message_has_query_message(*m) || !dnstap_message_has_response_address(*m)) {
            dsyslogf(LOG_ERR, "missing parts of %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
            break;
        }

        if (_set_ipv(&tm, m)
            || _set_proto(&tm, m)
            || _set_addr(&tm.dst_ip_addr, dnstap_message_response_address(*m), dnstap_message_response_address_length(*m))) {
            dsyslogf(LOG_ERR, "unable to extract values from %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
            break;
        }
        tm.dst_port   = dnstap_message_response_port(*m);
        tm.ts.tv_sec  = dnstap_message_query_time_sec(*m);
        tm.ts.tv_usec = dnstap_message_query_time_nsec(*m) / 1000;

        if (dnstap_message_has_query_address(*m)) {
            if (_set_addr(&tm.src_ip_addr, dnstap_message_query_address(*m), dnstap_message_query_address_length(*m))) {
                dsyslogf(LOG_ERR, "unable to extract values from %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
                break;
            }
            tm.src_port = dnstap_message_query_port(*m);
        } else {
            if (tm.ip_version == 4)
                inXaddr_pton(dnstap_network_ip4, &tm.src_ip_addr);
            else
                inXaddr_pton(dnstap_network_ip6, &tm.src_ip_addr);
            tm.src_port = dnstap_network_port;
        }

        last_ts = tm.ts;
        dns_protocol_handler(dnstap_message_query_message(*m), dnstap_message_query_message_length(*m), &tm);
        break;

    case DNSTAP_MESSAGE_TYPE_RESOLVER_RESPONSE:
        if (!dnstap_message_has_response_message(*m) || !dnstap_message_has_response_address(*m)) {
            dsyslogf(LOG_ERR, "missing parts of %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
            break;
        }

        if (_set_ipv(&tm, m)
            || _set_proto(&tm, m)
            || _set_addr(&tm.src_ip_addr, dnstap_message_response_address(*m), dnstap_message_response_address_length(*m))) {
            dsyslogf(LOG_ERR, "unable to extract values from %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
            break;
        }
        tm.src_port   = dnstap_message_response_port(*m);
        tm.ts.tv_sec  = dnstap_message_response_time_sec(*m);
        tm.ts.tv_usec = dnstap_message_response_time_nsec(*m) / 1000;

        if (dnstap_message_has_query_address(*m)) {
            if (_set_addr(&tm.dst_ip_addr, dnstap_message_query_address(*m), dnstap_message_query_address_length(*m))) {
                dsyslogf(LOG_ERR, "unable to extract values from %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
                break;
            }
            tm.dst_port = dnstap_message_query_port(*m);
        } else {
            if (tm.ip_version == 4)
                inXaddr_pton(dnstap_network_ip4, &tm.dst_ip_addr);
            else
                inXaddr_pton(dnstap_network_ip6, &tm.dst_ip_addr);
            tm.dst_port = dnstap_network_port;
        }

        last_ts = tm.ts;
        dns_protocol_handler(dnstap_message_response_message(*m), dnstap_message_response_message_length(*m), &tm);
        break;

    case DNSTAP_MESSAGE_TYPE_CLIENT_QUERY:
        if (!dnstap_message_has_query_message(*m) || !dnstap_message_has_query_address(*m)) {
            dsyslogf(LOG_ERR, "missing parts of %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
            break;
        }

        if (_set_ipv(&tm, m)
            || _set_proto(&tm, m)
            || _set_addr(&tm.src_ip_addr, dnstap_message_query_address(*m), dnstap_message_query_address_length(*m))) {
            dsyslogf(LOG_ERR, "unable to extract values from %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
            break;
        }
        tm.src_port   = dnstap_message_query_port(*m);
        tm.ts.tv_sec  = dnstap_message_query_time_sec(*m);
        tm.ts.tv_usec = dnstap_message_query_time_nsec(*m) / 1000;

        if (dnstap_message_has_response_address(*m)) {
            if (_set_addr(&tm.dst_ip_addr, dnstap_message_response_address(*m), dnstap_message_response_address_length(*m))) {
                dsyslogf(LOG_ERR, "unable to extract values from %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
                break;
            }
            tm.dst_port = dnstap_message_response_port(*m);
        } else {
            if (tm.ip_version == 4)
                inXaddr_pton(dnstap_network_ip4, &tm.dst_ip_addr);
            else
                inXaddr_pton(dnstap_network_ip6, &tm.dst_ip_addr);
            tm.dst_port = dnstap_network_port;
        }

        last_ts = tm.ts;
        dns_protocol_handler(dnstap_message_query_message(*m), dnstap_message_query_message_length(*m), &tm);
        break;

    case DNSTAP_MESSAGE_TYPE_CLIENT_RESPONSE:
        if (!dnstap_message_has_response_message(*m) || !dnstap_message_has_query_address(*m)) {
            dsyslogf(LOG_ERR, "missing parts of %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
            break;
        }

        if (_set_ipv(&tm, m)
            || _set_proto(&tm, m)
            || _set_addr(&tm.dst_ip_addr, dnstap_message_query_address(*m), dnstap_message_query_address_length(*m))) {
            dsyslogf(LOG_ERR, "unable to extract values from %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
            break;
        }
        tm.dst_port   = dnstap_message_query_port(*m);
        tm.ts.tv_sec  = dnstap_message_response_time_sec(*m);
        tm.ts.tv_usec = dnstap_message_response_time_nsec(*m) / 1000;

        if (dnstap_message_has_response_address(*m)) {
            if (_set_addr(&tm.src_ip_addr, dnstap_message_response_address(*m), dnstap_message_response_address_length(*m))) {
                dsyslogf(LOG_ERR, "unable to extract values from %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
                break;
            }
            tm.src_port = dnstap_message_response_port(*m);
        } else {
            if (tm.ip_version == 4)
                inXaddr_pton(dnstap_network_ip4, &tm.src_ip_addr);
            else
                inXaddr_pton(dnstap_network_ip6, &tm.src_ip_addr);
            tm.src_port = dnstap_network_port;
        }

        last_ts = tm.ts;
        dns_protocol_handler(dnstap_message_response_message(*m), dnstap_message_response_message_length(*m), &tm);
        break;

    case DNSTAP_MESSAGE_TYPE_STUB_QUERY:
    case DNSTAP_MESSAGE_TYPE_FORWARDER_QUERY:
    case DNSTAP_MESSAGE_TYPE_TOOL_QUERY:
        if (!dnstap_message_has_query_message(*m) || !dnstap_message_has_response_address(*m)) {
            dsyslogf(LOG_ERR, "missing parts of %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
            break;
        }

        if (_set_ipv(&tm, m)
            || _set_proto(&tm, m)
            || _set_addr(&tm.dst_ip_addr, dnstap_message_response_address(*m), dnstap_message_response_address_length(*m))) {
            dsyslogf(LOG_ERR, "unable to extract values from %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
            break;
        }
        tm.dst_port   = dnstap_message_response_port(*m);
        tm.ts.tv_sec  = dnstap_message_query_time_sec(*m);
        tm.ts.tv_usec = dnstap_message_query_time_nsec(*m) / 1000;

        if (dnstap_message_has_query_address(*m)) {
            if (_set_addr(&tm.src_ip_addr, dnstap_message_query_address(*m), dnstap_message_query_address_length(*m))) {
                dsyslogf(LOG_ERR, "unable to extract values from %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
                break;
            }
            tm.src_port = dnstap_message_query_port(*m);
        } else {
            if (tm.ip_version == 4)
                inXaddr_pton(dnstap_network_ip4, &tm.src_ip_addr);
            else
                inXaddr_pton(dnstap_network_ip6, &tm.src_ip_addr);
            tm.src_port = dnstap_network_port;
        }

        last_ts = tm.ts;
        dns_protocol_handler(dnstap_message_query_message(*m), dnstap_message_query_message_length(*m), &tm);
        break;

    case DNSTAP_MESSAGE_TYPE_STUB_RESPONSE:
    case DNSTAP_MESSAGE_TYPE_FORWARDER_RESPONSE:
    case DNSTAP_MESSAGE_TYPE_TOOL_RESPONSE:
        if (!dnstap_message_has_response_message(*m) || !dnstap_message_has_response_address(*m)) {
            dsyslogf(LOG_ERR, "missing parts of %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
            break;
        }

        if (_set_ipv(&tm, m)
            || _set_proto(&tm, m)
            || _set_addr(&tm.src_ip_addr, dnstap_message_response_address(*m), dnstap_message_response_address_length(*m))) {
            dsyslogf(LOG_ERR, "unable to extract values from %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
            break;
        }
        tm.src_port   = dnstap_message_response_port(*m);
        tm.ts.tv_sec  = dnstap_message_response_time_sec(*m);
        tm.ts.tv_usec = dnstap_message_response_time_nsec(*m) / 1000;

        if (dnstap_message_has_query_address(*m)) {
            if (_set_addr(&tm.dst_ip_addr, dnstap_message_query_address(*m), dnstap_message_query_address_length(*m))) {
                dsyslogf(LOG_ERR, "unable to extract values from %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
                break;
            }
            tm.dst_port = dnstap_message_query_port(*m);
        } else {
            if (tm.ip_version == 4)
                inXaddr_pton(dnstap_network_ip4, &tm.dst_ip_addr);
            else
                inXaddr_pton(dnstap_network_ip6, &tm.dst_ip_addr);
            tm.dst_port = dnstap_network_port;
        }

        last_ts = tm.ts;
        dns_protocol_handler(dnstap_message_response_message(*m), dnstap_message_response_message_length(*m), &tm);
        break;

    default:
        dsyslogf(LOG_DEBUG, "unknown or unsupported message type %s", DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*m)]);
        return 0;
    }

    return 0;
}

static void stop_uv(uv_timer_t* handle)
{
    uv_stop(uv_default_loop());
}

static FILE*          _file = 0;
struct dnswire_reader _file_reader;

#endif // USE_DNSTAP

extern int no_wait_interval;

void dnstap_init(enum dnstap_via via, const char* sock_or_host, int port)
{
    if (!dnstap_network_ip4)
        dnstap_network_ip4 = strdup("127.0.0.1");
    if (!dnstap_network_ip6)
        dnstap_network_ip6 = strdup("::1");
    if (dnstap_network_port < 0)
        dnstap_network_port = 53;

    last_ts.tv_sec = last_ts.tv_usec = 0;
    finish_ts.tv_sec = finish_ts.tv_usec = 0;

#ifdef USE_DNSTAP
    struct sockaddr_storage addr;
    int                     r;

    switch (via) {
    case dnstap_via_file:
        if (!(_file = fopen(sock_or_host, "r"))) {
            dsyslogf(LOG_ERR, "fopen() failed: %s", strerror(errno));
            return;
        }
        if (dnswire_reader_init(&_file_reader) != dnswire_ok) {
            dsyslog(LOG_ERR, "Unable to initialize dnswire reader");
            return;
        }
        no_wait_interval = 1;
        break;

    case dnstap_via_unixsock:
        uv_pipe_init(uv_default_loop(), &unix_server, 0);
        if ((r = uv_pipe_bind(&unix_server, sock_or_host))) {
            dsyslogf(LOG_ERR, "uv_pipe_bind() failed: %s", uv_strerror(r));
            return;
        }
        if ((r = uv_listen((uv_stream_t*)&unix_server, 128, on_new_unix_connection))) {
            dsyslogf(LOG_ERR, "uv_listen() failed: %s", uv_strerror(r));
        }
        break;

    case dnstap_via_tcp:
        if (strchr(sock_or_host, ':')) {
            uv_ip6_addr(sock_or_host, port, (struct sockaddr_in6*)&addr);
        } else {
            uv_ip4_addr(sock_or_host, port, (struct sockaddr_in*)&addr);
        }

        uv_tcp_init(uv_default_loop(), &tcp_server);
        if ((r = uv_tcp_bind(&tcp_server, (const struct sockaddr*)&addr, 0))) {
            dsyslogf(LOG_ERR, "uv_tcp_bind() failed: %s", uv_strerror(r));
            return;
        }
        if ((r = uv_listen((uv_stream_t*)&tcp_server, 128, on_new_tcp_connection))) {
            dsyslogf(LOG_ERR, "uv_listen() failed: %s", uv_strerror(r));
            return;
        }
        break;

    case dnstap_via_udp:
        if (strchr(sock_or_host, ':')) {
            uv_ip6_addr(sock_or_host, port, (struct sockaddr_in6*)&addr);
        } else {
            uv_ip4_addr(sock_or_host, port, (struct sockaddr_in*)&addr);
        }

        uv_udp_init(uv_default_loop(), &udp_server);
        if ((r = uv_udp_bind(&udp_server, (const struct sockaddr*)&addr, 0))) {
            dsyslogf(LOG_ERR, "uv_udp_bind() failed: %s", uv_strerror(r));
            return;
        }
        if ((r = uv_listen((uv_stream_t*)&udp_server, 128, on_new_udp_connection))) {
            dsyslogf(LOG_ERR, "uv_listen() failed: %s", uv_strerror(r));
            return;
        }
        break;
    }
#else
    dsyslog(LOG_ERR, "No DNSTAP support built in");
#endif
}

int dnstap_start_time(void)
{
    return (int)start_ts.tv_sec;
}

int dnstap_finish_time(void)
{
    return (int)finish_ts.tv_sec;
}

extern int sig_while_processing;

int dnstap_run(void)
{
#ifdef USE_DNSTAP
    if (_file) {
        if (finish_ts.tv_sec > 0) {
            start_ts.tv_sec = finish_ts.tv_sec;
            finish_ts.tv_sec += statistics_interval;
        } else {
            /*
             * First run, need to read one DNSTAP message and find
             * the first start time
             */

            int done = 0;
            while (!done) {
                switch (dnswire_reader_fread(&_file_reader, _file)) {
                case dnswire_have_dnstap:
                    dnstap_handler(dnswire_reader_dnstap(_file_reader));
                    done = 1;
                    break;
                case dnswire_again:
                case dnswire_need_more:
                    break;
                case dnswire_endofdata:
                    done = 1;
                    break;
                default:
                    dsyslog(LOG_ERR, "dnswire_reader_fread() error");
                    return 0;
                }
            }

            if (!start_ts.tv_sec
                || last_ts.tv_sec < start_ts.tv_sec
                || (last_ts.tv_sec == start_ts.tv_sec && last_ts.tv_usec < start_ts.tv_usec)) {
                start_ts = last_ts;
            }

            if (!start_ts.tv_sec) {
                return 0;
            }

            finish_ts.tv_sec  = ((start_ts.tv_sec / statistics_interval) + 1) * statistics_interval;
            finish_ts.tv_usec = 0;
        }

        while (last_ts.tv_sec < finish_ts.tv_sec) {
            switch (dnswire_reader_fread(&_file_reader, _file)) {
            case dnswire_have_dnstap:
                dnstap_handler(dnswire_reader_dnstap(_file_reader));
                break;
            case dnswire_again:
            case dnswire_need_more:
                break;
            case dnswire_endofdata:
                // EOF
                finish_ts = last_ts;
                return 0;
            default:
                if (sig_while_processing) {
                    break;
                }
                dsyslog(LOG_ERR, "dnswire_reader_fread() error");
                return 0;
            }

            if (sig_while_processing) {
                /*
                 * We got a signal, nothing more to do
                 */
                finish_ts = last_ts;
                return 0;
            }
        }
    } else {
        gettimeofday(&start_ts, NULL);
        gettimeofday(&last_ts, NULL);
        finish_ts.tv_sec  = ((start_ts.tv_sec / statistics_interval) + 1) * statistics_interval;
        finish_ts.tv_usec = 0;

        uv_timer_t stop_timer;
        uv_timer_init(uv_default_loop(), &stop_timer);
        uv_timer_start(&stop_timer, stop_uv, (finish_ts.tv_sec - start_ts.tv_sec) * 1000, 0);

        uv_run(uv_default_loop(), UV_RUN_DEFAULT);

        if (sig_while_processing)
            finish_ts = last_ts;
    }
    return 1;
#else
    dsyslog(LOG_ERR, "No DNSTAP support built in");
    return 0;
#endif
}

void dnstap_stop(void)
{
#ifdef USE_DNSTAP
    if (!_file) {
        uv_stop(uv_default_loop());
    }
#endif
}

void dnstap_close(void)
{
#ifdef USE_DNSTAP
    if (_file) {
        dnswire_reader_destroy(_file_reader);
        fclose(_file);
        _file = 0;
    } else {
        uv_stop(uv_default_loop());
    }
#endif
}
