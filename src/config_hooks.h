/*
 * Copyright (c) 2008-2018, OARC, Inc.
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

#ifndef __dsc_config_hooks_h
#define __dsc_config_hooks_h

#include "dataset_opt.h"
#include "geoip.h"

int open_interface(const char* interface);
int set_bpf_program(const char* s);
int add_local_address(const char* s, const char* m);
int set_run_dir(const char* dir);
int set_pid_file(const char* s);
int set_statistics_interval(const char* s);
int add_dataset(const char* name, const char* layer_ignored, const char* firstname, const char* firstindexer, const char* secondname, const char* secondindexer, const char* filtername, dataset_opt opts);
int set_bpf_vlan_tag_byte_order(const char* which);
int set_match_vlan(const char* s);
int set_minfree_bytes(const char* s);
int set_output_format(const char* output_format);
void set_dump_reports_on_exit(void);
int set_geoip_v4_dat(const char* dat, int options);
int set_geoip_v6_dat(const char* dat, int options);
int set_geoip_asn_v4_dat(const char* dat, int options);
int set_geoip_asn_v6_dat(const char* dat, int options);
int set_asn_indexer_backend(enum geoip_backend backend);
int set_country_indexer_backend(enum geoip_backend backend);
int set_maxminddb_asn(const char* file);
int set_maxminddb_country(const char* file);
int set_pcap_buffer_size(const char* s);
void set_no_wait_interval(void);
int set_pt_timeout(const char* s);
void set_drop_ip_fragments(void);
int set_dns_port(const char* s);

#endif /* __dsc_config_hooks_h */
