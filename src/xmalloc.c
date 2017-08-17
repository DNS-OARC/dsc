/*
 * Copyright (c) 2016-2017, OARC, Inc.
 * Copyright (c) 2007, The Measurement Factory, Inc.
 * Copyright (c) 2007, Internet Systems Consortium, Inc.
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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <syslog.h>
#if defined(__SVR4) && defined(__sun)
#include <string.h>
#include <strings.h>
#endif
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include "xmalloc.h"
#include "syslog_debug.h"
#include "compat.h"

/********** xmalloc **********/

void* xmalloc(size_t size)
{
    char  errbuf[512];
    void* p = malloc(size);
    if (NULL == p)
        dsyslogf(LOG_CRIT, "malloc: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
    return p;
}

void* xcalloc(size_t number, size_t size)
{
    char  errbuf[512];
    void* p = calloc(number, size);
    if (NULL == p)
        dsyslogf(LOG_CRIT, "calloc: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
    return p;
}

void* xrealloc(void* p, size_t size)
{
    char errbuf[512];
    p = realloc(p, size);
    if (NULL == p)
        dsyslogf(LOG_CRIT, "realloc: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
    return p;
}

char* xstrdup(const char* s)
{
    char  errbuf[512];
    void* p = strdup(s);
    if (NULL == p)
        dsyslogf(LOG_CRIT, "strdup: %s", dsc_strerror(errno, errbuf, sizeof(errbuf)));
    return p;
}

void xfree(void* p)
{
    free(p);
}

/********** amalloc **********/

typedef struct arena {
    struct arena* prevArena;
    u_char*       end;
    u_char*       nextAlloc;
} Arena;

Arena* currentArena = NULL;

#define align(size, a) (((size_t)(size) + ((a)-1)) & ~((a)-1))
#define ALIGNMENT 4
#define HEADERSIZE align(sizeof(Arena), ALIGNMENT)
#define CHUNK_SIZE (1 * 1024 * 1024 + 1024)

static Arena*
newArena(size_t size)
{
    char   errbuf[512];
    Arena* arena;
    size  = align(size, ALIGNMENT);
    arena = malloc(HEADERSIZE + size);
    if (NULL == arena) {
        dsyslogf(LOG_CRIT, "amalloc %d: %s", (int)size, dsc_strerror(errno, errbuf, sizeof(errbuf)));
        return NULL;
    }
    arena->prevArena = NULL;
    arena->nextAlloc = (u_char*)arena + HEADERSIZE;
    arena->end       = arena->nextAlloc + size;
    return arena;
}

void useArena()
{
    currentArena = newArena(CHUNK_SIZE);
}

void freeArena()
{
    while (currentArena) {
        Arena* prev = currentArena->prevArena;
        free(currentArena);
        currentArena = prev;
    }
}

void* amalloc(size_t size)
{
    void* p;
    size = align(size, ALIGNMENT);
    if (currentArena->end - currentArena->nextAlloc <= size) {
        if (size >= (CHUNK_SIZE >> 2)) {
            /* Create a new dedicated chunk for this large allocation, and
             * continue to use the current chunk for future smaller
             * allocations. */
            Arena* new              = newArena(size);
            new->prevArena          = currentArena->prevArena;
            currentArena->prevArena = new;
            return new->nextAlloc;
        }
        /* Move on to a new chunk. */
        Arena* new = newArena(CHUNK_SIZE);
        if (NULL == new)
            return NULL;
        new->prevArena = currentArena;
        currentArena   = new;
    }
    p = currentArena->nextAlloc;
    currentArena->nextAlloc += size;
    return p;
}

void* acalloc(size_t number, size_t size)
{
    void* p;
    size *= number;
    p = amalloc(size);
    return memset(p, 0, size);
}

void* arealloc(void* p, size_t size)
{
    return memcpy(amalloc(size), p, size);
}

char* astrdup(const char* s)
{
    size_t size = strlen(s) + 1;
    return memcpy(amalloc(size), s, size);
}

void afree(void* p)
{
    return;
}
