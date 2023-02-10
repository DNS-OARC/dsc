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

#include "tld_list.h"
#include "xmalloc.h"
#include "hashtbl.h"
#include "syslog_debug.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool have_tld_list = false;

#define MAX_ARRAY_SZ 65536
static hashtbl* theHash = NULL;

typedef struct
{
    char* tld;
    bool  is_tld; // True if this entry was present in the TLD list
    bool  has_children; // True if there are more entries "above" this
} tldlobj;

static unsigned int tldl_hashfunc(const void* key)
{
    return hashendian(key, strlen(key), 0);
}

static int tldl_cmpfunc(const void* a, const void* b)
{
    return strcasecmp(a, b);
}

int _add_tld(const char* tld, tldlobj* obj)
{
    if (!theHash) {
        theHash = hash_create(MAX_ARRAY_SZ, tldl_hashfunc, tldl_cmpfunc, 0, xfree, xfree);
        if (!theHash) {
            dsyslog(LOG_ERR, "tld_list: Unable to create hash, out of memory?");
            exit(1);
        }
    }
    tldlobj* found;
    if ((found = hash_find(tld, theHash))) {
        if (obj->is_tld)
            found->is_tld = true;
        if (obj->has_children)
            found->has_children = true;
        return 0;
    }
    obj->tld = strdup(tld);
    if (!obj->tld) {
        dsyslog(LOG_ERR, "tld_list: Unable to add entry to hash, out of memory");
        exit(1);
    }
    if (hash_add(obj->tld, obj, theHash)) {
        dsyslog(LOG_ERR, "tld_list: Unable to add entry to hash, out of memory?");
        exit(1);
    }
    return 1;
}

/*
 * Hash example layout:
 *
 * "ab.cd.ef" will create entries as follow:
 * - ef         is_tld=no, has_children=yes
 * - cd.ef      is_tld=no, has_children=yes
 * - ab.cd.ef   is_tld=yes, has_children=no
 *
 * adding "cd.ef" will result in:
 * - ef         is_tld=no, has_children=yes
 * - cd.ef      is_tld=yes, has_children=yes
 * - ab.cd.ef   is_tld=yes, has_children=no
 *
 */
void tld_list_add(const char* tld)
{
    const char* e = tld + strlen(tld) - 1;
    const char* t;
    int         state = 0; /* 0 = not in dots, 1 = in dots */
    while (*e == '.' && e > tld)
        e--;
    t = e;

    tldlobj* o = xmalloc(sizeof(tldlobj));
    if (!o) {
        dsyslog(LOG_ERR, "tld_list: Unable to create hash entry, out of memory?");
        exit(1);
    }

    while (t > tld) {
        t--;
        if ('.' == *t) {
            if (0 == state) {
                o->is_tld       = false;
                o->has_children = true;
                if (_add_tld(t + 1, o)) {
                    o = xmalloc(sizeof(tldlobj));
                    if (!o) {
                        dsyslog(LOG_ERR, "tld_list: Unable to create hash entry, out of memory?");
                        exit(1);
                    }
                }
            }
            state = 1;
        } else {
            state = 0;
        }
    }
    while (*t == '.' && t < e)
        t++;
    o->is_tld       = true;
    o->has_children = false;
    if (!_add_tld(t, o)) {
        xfree(o);
        return;
    }
    have_tld_list = true;
}

int tld_list_find(const char* tld)
{
    int      ret = 0;
    tldlobj* found;
    if ((found = hash_find(tld, theHash))) {
        if (found->is_tld)
            ret |= 1;
        if (found->has_children)
            ret |= 2;
    }
    return ret;
}
