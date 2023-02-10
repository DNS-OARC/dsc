/*
 * Copyright (c) 2008-2023, OARC, Inc.
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

#include "certain_qnames_index.h"

#include <string.h>

#define QNAME_LOCALHOST 0
#define QNAME_RSN 1
#define QNAME_OTHER 2

int certain_qnames_indexer(const dns_message* m)
{
    if (m->malformed)
        return -1;
    if (0 == strcmp(m->qname, "localhost"))
        return QNAME_LOCALHOST;
    if (0 == strcmp(m->qname + 1, ".root-servers.net"))
        return QNAME_RSN;
    return QNAME_OTHER;
}

int certain_qnames_iterator(const char** label)
{
    static int next_iter = 0;
    if (NULL == label) {
        next_iter = 0;
        return QNAME_OTHER + 1;
    }
    if (QNAME_LOCALHOST == next_iter)
        *label = "localhost";
    else if (QNAME_RSN == next_iter)
        *label = "X.root-servers.net";
    else if (QNAME_OTHER == next_iter)
        *label = "else";
    else
        return -1;
    return next_iter++;
}
