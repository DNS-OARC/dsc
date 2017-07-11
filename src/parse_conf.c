/*
 * Copyright (c) 2016-2017, OARC, Inc.
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

#ifdef __FreeBSD__
#define _WITH_GETLINE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_LIBGEOIP
#include <GeoIP.h>
#endif
#include <errno.h>

#include "parse_conf.h"
#include "config_hooks.h"
#include "dns_message.h"
#include "syslog_debug.h"
#include "compat.h"
#include "client_ip_net_index.h"

#define PARSE_CONF_EINVAL   -2
#define PARSE_CONF_ERROR    -1
#define PARSE_CONF_OK       0
#define PARSE_CONF_LAST     1
#define PARSE_CONF_COMMENT  2
#define PARSE_CONF_EMPTY    3

#define PARSE_MAX_ARGS 64

typedef enum conf_token_type conf_token_type_t;
enum conf_token_type {
    TOKEN_END = 0,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_STRINGS,
    TOKEN_NUMBERS,
    TOKEN_ANY
};

typedef struct conf_token conf_token_t;
struct conf_token {
    conf_token_type_t   type;
    const char*         token;
    size_t              length;
};

typedef struct conf_token_syntax conf_token_syntax_t;
struct conf_token_syntax {
    const char*             token;
    int                     (*parse)(const conf_token_t* tokens);
    const conf_token_type_t syntax[PARSE_MAX_ARGS];
};

int parse_conf_token(char** conf, size_t* length, conf_token_t* token) {
    int quoted = 0, end = 0;

    if (!conf || !*conf || !length || !token) {
        return PARSE_CONF_EINVAL;
    }
    if (!*length) {
        return PARSE_CONF_ERROR;
    }
    if (**conf == ' ' || **conf == '\t' || **conf == ';' || !**conf || **conf == '\n' || **conf == '\r') {
        return PARSE_CONF_ERROR;
    }
    if (**conf == '#') {
        return PARSE_CONF_COMMENT;
    }

    if (**conf == '"') {
        quoted = 1;
        (*conf)++;
        (*length)--;
        token->type = TOKEN_STRING;
    }
    else {
        token->type = TOKEN_NUMBER;
    }

    token->token = *conf;
    token->length = 0;

    for (; **conf && length; (*conf)++, (*length)--) {
        if (quoted && **conf == '"') {
            end = 1;
            quoted = 0;
            continue;
        }
        else if ((!quoted || end) && (**conf == ' ' || **conf == '\t' || **conf == ';')) {
            while (length && (**conf == ' ' || **conf == '\t')) {
                (*conf)++;
                (*length)--;
            }
            if (**conf == ';') {
                return PARSE_CONF_LAST;
            }
            return PARSE_CONF_OK;
        }
        else if (end || **conf == '\n' || **conf == '\r' || !**conf) {
            return PARSE_CONF_ERROR;
        }

        if (**conf < '0' || **conf > '9') {
            token->type = TOKEN_STRING;
        }

        token->length++;
    }

    return PARSE_CONF_ERROR;
}

int parse_conf_interface(const conf_token_t* tokens) {
    char* interface = strndup(tokens[1].token, tokens[1].length);
    int ret;

    if (!interface) {
        errno = ENOMEM;
        return -1;
    }

    ret = open_interface(interface);
    free(interface);
    return ret == 1 ? 0 : 1;
}

int parse_conf_run_dir(const conf_token_t* tokens) {
    char* run_dir = strndup(tokens[1].token, tokens[1].length);
    int ret;

    if (!run_dir) {
        errno = ENOMEM;
        return -1;
    }

    ret = set_run_dir(run_dir);
    free(run_dir);
    return ret == 1 ? 0 : 1;
}

int parse_conf_minfree_bytes(const conf_token_t* tokens) {
    char* minfree_bytes = strndup(tokens[1].token, tokens[1].length);
    int ret;

    if (!minfree_bytes) {
        errno = ENOMEM;
        return -1;
    }

    ret = set_minfree_bytes(minfree_bytes);
    free(minfree_bytes);
    return ret == 1 ? 0 : 1;
}

int parse_conf_pid_file(const conf_token_t* tokens) {
    char* pid_file = strndup(tokens[1].token, tokens[1].length);
    int ret;

    if (!pid_file) {
        errno = ENOMEM;
        return -1;
    }

    ret = set_pid_file(pid_file);
    free(pid_file);
    return ret == 1 ? 0 : 1;
}

int parse_conf_statistics_interval(const conf_token_t* tokens) {
    char* statistics_interval = strndup(tokens[1].token, tokens[1].length);
    int ret;

    if (!statistics_interval) {
        errno = ENOMEM;
        return -1;
    }

    ret = set_statistics_interval(statistics_interval);
    free(statistics_interval);
    return ret == 1 ? 0 : 1;
}

int parse_conf_local_address(const conf_token_t* tokens) {
    char* local_address = strndup(tokens[1].token, tokens[1].length);
    char* local_mask = 0;
    int ret;

    if (!local_address) {
        errno = ENOMEM;
        return -1;
    }
    if (tokens[2].token != TOKEN_END && !(local_mask = strndup(tokens[2].token, tokens[2].length))) {
        free(local_address);
        errno = ENOMEM;
        return -1;
    }

    ret = add_local_address(local_address, local_mask);
    free(local_address);
    free(local_mask);
    return ret == 1 ? 0 : 1;
}

int parse_conf_bpf_program(const conf_token_t* tokens) {
    char* bpf_program = strndup(tokens[1].token, tokens[1].length);
    int ret;

    if (!bpf_program) {
        errno = ENOMEM;
        return -1;
    }

    ret = set_bpf_program(bpf_program);
    free(bpf_program);
    return ret == 1 ? 0 : 1;
}

int parse_conf_dataset(const conf_token_t* tokens) {
    char* name = strndup(tokens[1].token, tokens[1].length);
    char* layer = strndup(tokens[2].token, tokens[2].length);
    char* dim1_name = strndup(tokens[3].token, tokens[3].length);
    char* dim1_indexer;
    char* dim2_name = strndup(tokens[4].token, tokens[4].length);
    char* dim2_indexer;
    char* filter = strndup(tokens[5].token, tokens[5].length);
    int ret;
    dataset_opt opts;
    size_t i;

    if (!name || !layer || !dim1_name || !dim2_name || !filter) {
        free(name);
        free(layer);
        free(dim1_name);
        free(dim2_name);
        free(filter);
        errno = ENOMEM;
        return -1;
    }

    opts.min_count = 0;	// min cell count to report
    opts.max_cells = 0;	// max 2nd dim cells to print

    for (i = 6; tokens[i].type != TOKEN_END; i++) {
        char* opt = strndup(tokens[i].token, tokens[i].length);
        char* arg;
        ret = 0;

        if (!opt) {
            errno = ENOMEM;
            ret = -1;
        }
        else if (!(arg = strchr(opt, '='))) {
            ret = 1;
        }
        else {
            *arg = 0;
            arg++;

            if (!*arg) {
                ret = 1;
            }
            else if (!strcmp(opt, "min-count")) {
                opts.min_count = atoi(arg);
            }
            else if (!strcmp(opt, "max-cells")) {
                opts.max_cells = atoi(arg);
            }
            else {
                ret = 1;
            }
        }

        free(opt);
        if (ret) {
            free(name);
            free(layer);
            free(dim1_name);
            free(dim2_name);
            free(filter);
            return ret;
        }
    }

    if (!(dim1_indexer = strchr(dim1_name, ':'))
        || !(dim2_indexer = strchr(dim2_name, ':')))
    {
        ret = 1;
    }
    else {
        *dim1_indexer = *dim2_indexer = 0;
        dim1_indexer++;
        dim2_indexer++;
        ret = add_dataset(name, layer, dim1_name, dim1_indexer, dim2_name, dim2_indexer, filter, opts);
    }
    free(name);
    free(layer);
    free(dim1_name);
    free(dim2_name);
    free(filter);
    return ret == 1 ? 0 : 1;
}

int parse_conf_bpf_vlan_tag_byte_order(const conf_token_t* tokens) {
    char* bpf_vlan_tag_byte_order = strndup(tokens[1].token, tokens[1].length);
    int ret;

    if (!bpf_vlan_tag_byte_order) {
        errno = ENOMEM;
        return -1;
    }

    ret = set_bpf_vlan_tag_byte_order(bpf_vlan_tag_byte_order);
    free(bpf_vlan_tag_byte_order);
    return ret == 1 ? 0 : 1;
}

int parse_conf_output_format(const conf_token_t* tokens) {
    char* output_format = strndup(tokens[1].token, tokens[1].length);
    int ret;

    if (!output_format) {
        errno = ENOMEM;
        return -1;
    }

    ret = set_output_format(output_format);
    free(output_format);
    return ret == 1 ? 0 : 1;
}

int parse_conf_match_vlan(const conf_token_t* tokens) {
    int ret = 0;
    size_t i;

    for (i = 1; tokens[i].type != TOKEN_END; i++) {
        char* match_vlan = strndup(tokens[i].token, tokens[i].length);

        if (!match_vlan) {
            errno = ENOMEM;
            return -1;
        }

        ret = set_match_vlan(match_vlan);
        free(match_vlan);
        if (ret != 1) {
            break;
        }
    }

    return ret == 1 ? 0 : 1;
}

int parse_conf_qname_filter(const conf_token_t* tokens) {
    char* name = strndup(tokens[1].token, tokens[1].length);
    char* re = strndup(tokens[2].token, tokens[2].length);
    int ret;

    if (!name || !re) {
        free(name);
        free(re);
        errno = ENOMEM;
        return -1;
    }

    ret = add_qname_filter(name, re);
    free(name);
    free(re);
    return ret == 1 ? 0 : 1;
}

int parse_conf_dump_reports_on_exit(const conf_token_t* tokens) {
    set_dump_reports_on_exit();
    return 0;
}

#if HAVE_LIBGEOIP
int parse_conf_geoip_options(const conf_token_t* tokens, int* options) {
    size_t i;

    for (i = 2; tokens[i].type != TOKEN_END; i++) {
        if (!strncmp(tokens[i].token, "STANDARD", tokens[i].length)) {
            *options |= GEOIP_STANDARD;
        }
        else if (!strncmp(tokens[i].token, "MEMORY_CACHE", tokens[i].length)) {
            *options |= GEOIP_MEMORY_CACHE;
        }
        else if (!strncmp(tokens[i].token, "CHECK_CACHE", tokens[i].length)) {
            *options |= GEOIP_CHECK_CACHE;
        }
        else if (!strncmp(tokens[i].token, "INDEX_CACHE", tokens[i].length)) {
            *options |= GEOIP_INDEX_CACHE;
        }
        else if (!strncmp(tokens[i].token, "MMAP_CACHE", tokens[i].length)) {
            *options |= GEOIP_MMAP_CACHE;
        }
        else {
            return 1;
        }
    }

    return 0;
}
#endif

int parse_conf_geoip_v4_dat(const conf_token_t* tokens) {
    char* geoip_v4_dat = strndup(tokens[1].token, tokens[1].length);
    int ret, options = 0;

    if (!geoip_v4_dat) {
        errno = ENOMEM;
        return -1;
    }

#if HAVE_LIBGEOIP
    if ((ret = parse_conf_geoip_options(tokens, &options))) {
        free(geoip_v4_dat);
        return ret;
    }
#endif

    ret = set_geoip_v4_dat(geoip_v4_dat, options);
    free(geoip_v4_dat);
    return ret == 1 ? 0 : 1;
}

int parse_conf_geoip_v6_dat(const conf_token_t* tokens) {
    char* geoip_v6_dat = strndup(tokens[1].token, tokens[1].length);
    int ret, options = 0;

    if (!geoip_v6_dat) {
        errno = ENOMEM;
        return -1;
    }

#if HAVE_LIBGEOIP
    if ((ret = parse_conf_geoip_options(tokens, &options))) {
        free(geoip_v6_dat);
        return ret;
    }
#endif

    ret = set_geoip_v6_dat(geoip_v6_dat, options);
    free(geoip_v6_dat);
    return ret == 1 ? 0 : 1;
}

int parse_conf_geoip_asn_v4_dat(const conf_token_t* tokens) {
    char* geoip_asn_v4_dat = strndup(tokens[1].token, tokens[1].length);
    int ret, options = 0;

    if (!geoip_asn_v4_dat) {
        errno = ENOMEM;
        return -1;
    }

#if HAVE_LIBGEOIP
    if ((ret = parse_conf_geoip_options(tokens, &options))) {
        free(geoip_asn_v4_dat);
        return ret;
    }
#endif

    ret = set_geoip_asn_v4_dat(geoip_asn_v4_dat, options);
    free(geoip_asn_v4_dat);
    return ret == 1 ? 0 : 1;
}

int parse_conf_geoip_asn_v6_dat(const conf_token_t* tokens) {
    char* geoip_asn_v6_dat = strndup(tokens[1].token, tokens[1].length);
    int ret, options = 0;

    if (!geoip_asn_v6_dat) {
        errno = ENOMEM;
        return -1;
    }

#if HAVE_LIBGEOIP
    if ((ret = parse_conf_geoip_options(tokens, &options))) {
        free(geoip_asn_v6_dat);
        return ret;
    }
#endif

    ret = set_geoip_asn_v6_dat(geoip_asn_v6_dat, options);
    free(geoip_asn_v6_dat);
    return ret == 1 ? 0 : 1;
}

int parse_conf_pcap_buffer_size(const conf_token_t* tokens) {
    char* pcap_buffer_size = strndup(tokens[1].token, tokens[1].length);
    int ret;

    if (!pcap_buffer_size) {
        errno = ENOMEM;
        return -1;
    }

    ret = set_pcap_buffer_size(pcap_buffer_size);
    free(pcap_buffer_size);
    return ret == 1 ? 0 : 1;
}

int parse_conf_no_wait_interval(const conf_token_t* tokens) {
    set_no_wait_interval();
    return 0;
}

int parse_conf_pcap_thread_timeout(const conf_token_t* tokens) {
    char* timeout = strndup(tokens[1].token, tokens[1].length);
    int ret;

    if (!timeout) {
        errno = ENOMEM;
        return -1;
    }

    ret = set_pt_timeout(timeout);
    free(timeout);
    return ret == 1 ? 0 : 1;
}

int parse_conf_drop_ip_fragments(const conf_token_t* tokens) {
    set_drop_ip_fragments();
    return 0;
}

int parse_conf_client_v4_mask(const conf_token_t* tokens) {
    char* mask = strndup(tokens[1].token, tokens[1].length);
    int ret;

    if (!mask) {
        errno = ENOMEM;
        return -1;
    }

    ret = cip_net_v4_mask_set(mask);
    free(mask);
    return ret == 1 ? 0 : 1;
}

int parse_conf_client_v6_mask(const conf_token_t* tokens) {
    char* mask = strndup(tokens[1].token, tokens[1].length);
    int ret;

    if (!mask) {
        errno = ENOMEM;
        return -1;
    }

    ret = cip_net_v6_mask_set(mask);
    free(mask);
    return ret == 1 ? 0 : 1;
}

static conf_token_syntax_t _syntax[] = {
    {
        "interface",
        parse_conf_interface,
        { TOKEN_STRING, TOKEN_END }
    },
    {
        "run_dir",
        parse_conf_run_dir,
        { TOKEN_STRING, TOKEN_END }
    },
    {
        "minfree_bytes",
        parse_conf_minfree_bytes,
        { TOKEN_NUMBER, TOKEN_END }
    },
    {
        "pid_file",
        parse_conf_pid_file,
        { TOKEN_STRING, TOKEN_END }
    },
    {
        "statistics_interval",
        parse_conf_statistics_interval,
        { TOKEN_NUMBER, TOKEN_END }
    },
    {
        "local_address",
        parse_conf_local_address,
        { TOKEN_STRING, TOKEN_ANY, TOKEN_END }
    },
    {
        "bpf_program",
        parse_conf_bpf_program,
        { TOKEN_STRING, TOKEN_END }
    },
    {
        "dataset",
        parse_conf_dataset,
        { TOKEN_STRING, TOKEN_STRING, TOKEN_STRING, TOKEN_STRING, TOKEN_STRING, TOKEN_STRINGS, TOKEN_END }
    },
    {
        "bpf_vlan_tag_byte_order",
        parse_conf_bpf_vlan_tag_byte_order,
        { TOKEN_STRING, TOKEN_END }
    },
    {
        "output_format",
        parse_conf_output_format,
        { TOKEN_STRING, TOKEN_END }
    },
    {
        "match_vlan",
        parse_conf_match_vlan,
        { TOKEN_NUMBER, TOKEN_NUMBERS, TOKEN_END }
    },
    {
        "qname_filter",
        parse_conf_qname_filter,
        { TOKEN_STRING, TOKEN_STRING, TOKEN_END }
    },
    {
        "dump_reports_on_exit",
        parse_conf_dump_reports_on_exit,
        { TOKEN_END }
    },
    {
        "geoip_v4_dat",
        parse_conf_geoip_v4_dat,
        { TOKEN_STRING, TOKEN_STRINGS, TOKEN_END }
    },
    {
        "geoip_v6_dat",
        parse_conf_geoip_v6_dat,
        { TOKEN_STRING, TOKEN_STRINGS, TOKEN_END }
    },
    {
        "geoip_asn_v4_dat",
        parse_conf_geoip_asn_v4_dat,
        { TOKEN_STRING, TOKEN_STRINGS, TOKEN_END }
    },
    {
        "geoip_asn_v6_dat",
        parse_conf_geoip_asn_v6_dat,
        { TOKEN_STRING, TOKEN_STRINGS, TOKEN_END }
    },
    {
        "pcap_buffer_size",
        parse_conf_pcap_buffer_size,
        { TOKEN_NUMBER, TOKEN_END }
    },
    {
        "no_wait_interval",
        parse_conf_no_wait_interval,
        { TOKEN_END }
    },
    {
        "pcap_thread_timeout",
        parse_conf_pcap_thread_timeout,
        { TOKEN_NUMBER, TOKEN_END }
    },
    {
        "drop_ip_fragments",
        parse_conf_drop_ip_fragments,
        { TOKEN_END }
    },
    {
        "client_v4_mask",
        parse_conf_client_v4_mask,
        { TOKEN_STRING, TOKEN_END }
    },
    {
        "client_v6_mask",
        parse_conf_client_v6_mask,
        { TOKEN_STRING, TOKEN_END }
    },

    { 0, 0, { TOKEN_END } }
};

int parse_conf_tokens(const conf_token_t* tokens, size_t token_size, size_t line) {
    const conf_token_syntax_t* syntax;
    const conf_token_type_t*   type;
    size_t i;

    if (!tokens || !token_size) {
        fprintf(stderr, "CONFIG ERROR [line:%lu]: Internal error, please report!\n", line);
        return 1;
    }

    if (tokens[0].type != TOKEN_STRING) {
        fprintf(stderr, "CONFIG ERROR [line:%lu]: Wrong first token, expected a string\n", line);
        return 1;
    }

    for (syntax = _syntax; syntax->token; syntax++) {
        if (!strncmp(tokens[0].token, syntax->token, tokens[0].length)) {
            break;
        }
    }
    if (!syntax->token) {
        fprintf(stderr, "CONFIG ERROR [line:%lu]: Unknown configuration option: ", line);
        fwrite(tokens[0].token, tokens[0].length, 1, stderr);
        fprintf(stderr, "\n");
        return 1;
    }

    for (type = syntax->syntax, i = 1; *type != TOKEN_END && i < token_size; i++) {
        if (*type == TOKEN_STRINGS) {
            if (tokens[i].type != TOKEN_STRING) {
                fprintf(stderr, "CONFIG ERROR [line:%lu]: Wrong token for argument %lu, expected a string\n", line, i);
                return 1;
            }
            continue;
        }
        if (*type == TOKEN_NUMBERS) {
            if (tokens[i].type != TOKEN_NUMBER) {
                fprintf(stderr, "CONFIG ERROR [line:%lu]: Wrong token for argument %lu, expected a number\n", line, i);
                return 1;
            }
            continue;
        }
        if (*type == TOKEN_ANY) {
            if (tokens[i].type != TOKEN_STRING && tokens[i].type != TOKEN_NUMBER) {
                fprintf(stderr, "CONFIG ERROR [line:%lu]: Wrong token for argument %lu, expected a string or number\n", line, i);
                return 1;
            }
            continue;
        }

        if (tokens[i].type != *type) {
            fprintf(stderr, "CONFIG ERROR [line:%lu]: Wrong token for argument %lu", line, i);
            if (*type == TOKEN_STRING) {
                fprintf(stderr, ", expected a string\n");
            }
            else if (*type == TOKEN_NUMBER) {
                fprintf(stderr, ", expected a number\n");
            }
            else {
                fprintf(stderr, "\n");
            }
            return 1;
        }
        type++;
    }

    if (syntax->parse) {
        int ret = syntax->parse(tokens);

        if (ret < 0) {
            char errbuf[512];
            fprintf(stderr, "CONFIG ERROR [line:%lu]: %s\n", line, dsc_strerror(errno, errbuf, sizeof(errbuf)));
        }
        if (ret > 0) {
            fprintf(stderr, "CONFIG ERROR [line:%lu]: Unable to configure ", line);
            fwrite(tokens[0].token, tokens[0].length, 1, stderr);
            fprintf(stderr, "\n");
        }
        return ret ? 1 : 0;
    }

    return 0;
}

int parse_conf(const char* file) {
    FILE* fp;
    char* buffer = 0;
    size_t bufsize = 0;
    char* buf;
    size_t s, i, line = 0;
    conf_token_t tokens[PARSE_MAX_ARGS];
    int ret, ret2;

    if (!file) {
        return 1;
    }

    if (!(fp = fopen(file, "r"))) {
        return 1;
    }
    while ((ret2 = getline(&buffer, &bufsize, fp)) > 0) {
        memset(tokens, 0, sizeof(conf_token_t) * PARSE_MAX_ARGS);
        line++;
        /*
         * Go to the first non white-space character
         */
        for (ret = PARSE_CONF_OK, buf = buffer, s = bufsize; *buf && s; buf++, s--) {
            if (*buf != ' ' && *buf != '\t') {
                if (*buf == '\n' || *buf == '\r') {
                    ret = PARSE_CONF_EMPTY;
                }
                break;
            }
        }
        /*
         * Parse all the tokens
         */
        for (i = 0; i < PARSE_MAX_ARGS && ret == PARSE_CONF_OK; i++) {
            ret = parse_conf_token(&buf, &s, &tokens[i]);
        }

        if (ret == PARSE_CONF_COMMENT) {
            /*
             * Line ended with comment, reduce the number of tokens
             */
            i--;
            if (!i) {
                /*
                 * Comment was the only token so the line is empty
                 */
                continue;
            }
        }
        else if (ret == PARSE_CONF_EMPTY) {
            continue;
        }
        else if (ret == PARSE_CONF_OK) {
            if (i > 0 && tokens[0].type == TOKEN_STRING) {
                fprintf(stderr, "CONFIG ERROR [line:%lu]: Too many arguments for ", line);
                fwrite(tokens[0].token, tokens[0].length, 1, stderr);
                fprintf(stderr, " at line %lu\n", line);
            }
            else {
                fprintf(stderr, "CONFIG ERROR [line:%lu]: Too many arguments at line %lu\n", line, line);
            }
            free(buffer);
            fclose(fp);
            return 1;
        }
        else if (ret != PARSE_CONF_LAST) {
            if (i > 0 && tokens[0].type == TOKEN_STRING) {
                fprintf(stderr, "CONFIG ERROR [line:%lu]: Invalid syntax for ", line);
                fwrite(tokens[0].token, tokens[0].length, 1, stderr);
                fprintf(stderr, " at line %lu\n", line);
            }
            else {
                fprintf(stderr, "CONFIG ERROR [line:%lu]: Invalid syntax at line %lu\n", line, line);
            }
            free(buffer);
            fclose(fp);
            return 1;
        }

        /*
         * Configure using the tokens
         */
        if (parse_conf_tokens(tokens, i, line)) {
            free(buffer);
            fclose(fp);
            return 1;
        }
    }
    if (ret2 < 0) {
        long pos;
        char errbuf[512];

        pos = ftell(fp);
        if (fseek(fp, 0, SEEK_END)) {
            fprintf(stderr, "CONFIG ERROR [line:%lu]: fseek(): %s\n", line, dsc_strerror(errno, errbuf, sizeof(errbuf)));
        }
        else if (ftell(fp) < pos) {
            fprintf(stderr, "CONFIG ERROR [line:%lu]: getline(): %s\n", line, dsc_strerror(errno, errbuf, sizeof(errbuf)));
        }
    }
    free(buffer);
    fclose(fp);

    return 0;
}
