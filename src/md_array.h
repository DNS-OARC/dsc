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

#ifndef __dsc_md_array_h
#define __dsc_md_array_h

typedef struct indexer          indexer;
typedef struct filter_defn      filter_defn;
typedef struct filter_list      filter_list;
typedef struct md_array_node    md_array_node;
typedef struct md_array         md_array;
typedef struct md_array_printer md_array_printer;
typedef struct md_array_list    md_array_list;

#include "dataset_opt.h"
#include "dns_message.h"

#include <stdio.h>

typedef int (*filter_func)(const dns_message* m, const void* context);

enum flush_mode {
    flush_on,
    flush_get,
    flush_off
};

struct indexer {
    const char* name;
    void (*init_fn)(void);
    int (*index_fn)(const dns_message*);
    int (*iter_fn)(const char**);
    void (*reset_fn)(void);
    const dns_message* (*flush_fn)(enum flush_mode);
};

struct filter_defn {
    const char* name;
    filter_func func;
    const void* context;
};

struct filter_list {
    filter_defn*        filter;
    struct filter_list* next;
};

struct md_array_node {
    int  alloc_sz;
    int* array;
};

struct md_array {
    const char*  name;
    filter_list* filter_list;
    struct
    {
        indexer*    indexer;
        const char* type;
        int         alloc_sz;
    } d1;
    struct
    {
        indexer*    indexer;
        const char* type;
        int         alloc_sz;
    } d2;
    dataset_opt    opts;
    md_array_node* array;
};

struct md_array_printer {
    void (*start_array)(void*, const char*);
    void (*finish_array)(void*);
    void (*d1_type)(void*, const char*);
    void (*d2_type)(void*, const char*);
    void (*start_data)(void*);
    void (*finish_data)(void*);
    void (*d1_begin)(void*, const char*);
    void (*d1_end)(void*, const char*);
    void (*print_element)(void*, const char*, int);
    const char* format;
    const char* start_file;
    const char* end_file;
    const char* extension;
};

struct md_array_list {
    md_array*      theArray;
    md_array_list* next;
};

md_array* md_array_create(const char* name, filter_list*, const char*, indexer*, const char*, indexer*);
void md_array_clear(md_array*);
int  md_array_count(md_array*, const void*);
void md_array_flush(md_array* a);
int md_array_print(md_array* a, md_array_printer* pr, FILE* fp);
filter_list** md_array_filter_list_append(filter_list** fl, filter_defn* f);
filter_defn* md_array_create_filter(const char* name, filter_func, const void* context);

#endif /* __dsc_md_array_h */
