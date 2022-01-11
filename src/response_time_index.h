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

#ifndef __dsc_response_time_index_h
#define __dsc_response_time_index_h

#include "dns_message.h"

enum response_time_mode {
    response_time_bucket,
    response_time_log10,
    response_time_log2
};

enum response_time_max_sec_mode {
    response_time_ceil,
    response_time_timed_out
};

enum response_time_full_mode {
    response_time_drop_oldest,
    response_time_drop_query
};

void response_time_set_mode(enum response_time_mode m);
void response_time_set_max_sec(time_t s);
void response_time_set_max_sec_mode(enum response_time_max_sec_mode m);
void response_time_set_bucket_size(unsigned int s);
void response_time_set_max_queries(size_t q);
void response_time_set_full_mode(enum response_time_full_mode m);

int                response_time_indexer(const dns_message*);
int                response_time_iterator(const char** label);
void               response_time_reset(void);
const dns_message* response_time_flush(enum flush_mode mode);

#endif /* __dsc_response_time_index_h */
