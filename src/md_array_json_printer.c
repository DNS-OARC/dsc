/*
 * Copyright (c) 2008-2019, OARC, Inc.
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

#include "md_array.h"
#include "pcap.h"
#include "base64.h"
#include "xmalloc.h"
#include "input_mode.h"
#include "dnstap.h"

extern int input_mode;

#include <string.h>
#include <assert.h>

static const char* d1_type_s; /* XXX barf */
static const char* d2_type_s; /* XXX barf */

static int array_comma   = 0;
static int data_comma    = 0;
static int element_comma = 0;

static void
start_array(void* pr_data, const char* name)
{
    FILE* fp = pr_data;
    assert(fp);

    if (array_comma)
        fprintf(fp, ",\n");
    else
        array_comma = 1;

    fprintf(fp, "{\n  \"name\": \"%s\",\n", name);
    if (input_mode == INPUT_DNSTAP) {
        fprintf(fp, "  \"start_time\": %d,\n", dnstap_start_time());
        fprintf(fp, "  \"stop_time\": %d,\n", dnstap_finish_time());
    } else {
        fprintf(fp, "  \"start_time\": %d,\n", Pcap_start_time());
        fprintf(fp, "  \"stop_time\": %d,\n", Pcap_finish_time());
    }
    fprintf(fp, "  \"dimensions\": [");
}

static void
finish_array(void* pr_data)
{
    FILE* fp = pr_data;

    data_comma = 0;
    fprintf(fp, "}");
}

static void
d1_type(void* pr_data, const char* t)
{
    FILE* fp = pr_data;

    fprintf(fp, " \"%s\"", t);
    d1_type_s = t;
}

static void
d2_type(void* pr_data, const char* t)
{
    FILE* fp = pr_data;

    fprintf(fp, ", \"%s\" ],\n", t);
    d2_type_s = t;
}

static const char* entity_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
                                  "0123456789._-:";

static void
d1_begin(void* pr_data, const char* l)
{
    FILE* fp = pr_data;
    int   ll = strlen(l);
    char* e  = NULL;

    if (strspn(l, entity_chars) != ll) {
        int x = base64_encode(l, ll, &e);
        assert(x);
        l = e;
    }

    if (data_comma)
        fprintf(fp, ",\n");
    else
        data_comma = 1;

    element_comma = 0;

    fprintf(fp, "    {\n");
    fprintf(fp, "      \"%s\": \"%s\",\n", d1_type_s, l);
    if (e)
        fprintf(fp, "      \"base64\": true,\n");
    fprintf(fp, "      \"%s\": [", d2_type_s);

    if (e)
        xfree(e);
}

static void
print_element(void* pr_data, const char* l, int val)
{
    FILE* fp = pr_data;
    int   ll = strlen(l);
    char* e  = NULL;

    if (strspn(l, entity_chars) != ll) {
        int x = base64_encode(l, ll, &e);
        assert(x);
        l = e;
    }

    if (element_comma)
        fprintf(fp, ",\n");
    else {
        fprintf(fp, "\n");
        element_comma = 1;
    }

    fprintf(fp, "        { \"val\": \"%s\"", l);
    if (e)
        fprintf(fp, ", \"base64\": true");
    fprintf(fp, ", \"count\": %d }", val);

    if (e)
        xfree(e);
}

static void
d1_end(void* pr_data, const char* l)
{
    FILE* fp = pr_data;

    if (element_comma)
        fprintf(fp, "\n      ");

    fprintf(fp, "]\n    }");
}

static void
start_data(void* pr_data)
{
    FILE* fp = pr_data;

    fprintf(fp, "  \"data\": [\n");
}

static void
finish_data(void* pr_data)
{
    FILE* fp = pr_data;

    if (data_comma)
        fprintf(fp, "\n");

    fprintf(fp, "  ]\n");
}

md_array_printer json_printer = {
    start_array,
    finish_array,
    d1_type,
    d2_type,
    start_data,
    finish_data,
    d1_begin,
    d1_end,
    print_element,
    "JSON",
    "[\n",
    "\n]\n",
    "json"
};
