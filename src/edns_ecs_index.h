/*
 * Copyright (c) 2008-2024 OARC, Inc.
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

#ifndef __dsc_edns_ecs_index_h
#define __dsc_edns_ecs_index_h

#include "dns_message.h"

int edns_ecs_indexer(const dns_message*);
int edns_ecs_iterator(const char** label);

int  edns_ecs_family_indexer(const dns_message*);
int  edns_ecs_family_iterator(const char** label);
void edns_ecs_family_reset(void);

int  edns_ecs_source_prefix_indexer(const dns_message*);
int  edns_ecs_source_prefix_iterator(const char** label);
void edns_ecs_source_prefix_reset(void);

int  edns_ecs_scope_prefix_indexer(const dns_message*);
int  edns_ecs_scope_prefix_iterator(const char** label);
void edns_ecs_scope_prefix_reset(void);

int  edns_ecs_address_indexer(const dns_message*);
int  edns_ecs_address_iterator(const char** label);
void edns_ecs_address_reset(void);

int  edns_ecs_subnet_indexer(const dns_message*);
int  edns_ecs_subnet_iterator(const char** label);
void edns_ecs_subnet_reset(void);

#endif /* __dsc_edns_ecs_index_h */
