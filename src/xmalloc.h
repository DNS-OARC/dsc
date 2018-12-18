/*
 * Copyright (c) 2008-2018, OARC, Inc.
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

#ifndef __dsc_xmalloc_h
#define __dsc_xmalloc_h

#include <stddef.h>

/* The xmalloc family of functions syslogs an error if the alloc fails. */
void* xmalloc(size_t size);
void* xcalloc(size_t number, size_t size);
void* xrealloc(void* ptr, size_t size);
char* xstrdup(const char* s);
void xfree(void* ptr);

/* The amalloc family of functions allocates from an "arena", optimized for
 * making a large number of small allocations, and then freeing them all at
 * once.  This makes it possible to avoid memory leaks without a lot of
 * tedious tracking of many small allocations.  Like xmalloc, they will syslog
 * an error if the alloc fails.
 * You must call useArena() before using any of these allocators for the first
 * time or after calling freeArena().
 * The only way to free space allocated with these functions is with
 * freeArena(), which quickly frees _everything_ allocated by these functions.
 * afree() is actually a no-op, and arealloc() does not free the original;
 * these will waste space if used heavily.
 */
void  useArena();
void  freeArena();
void* amalloc(size_t size);
void* acalloc(size_t number, size_t size);
void* arealloc(void* ptr, size_t size);
char* astrdup(const char* s);
void afree(void* ptr);

#endif /* __dsc_xmalloc_h */
