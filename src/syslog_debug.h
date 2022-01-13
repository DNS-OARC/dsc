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

#ifndef __dsc_syslog_debug_h
#define __dsc_syslog_debug_h

#include <syslog.h>

extern int debug_flag;

/*
 * This syslog()/syslogf() macro normally calls the real syslog()
 * function, unless debug_flag is on, in which case it does fprintf
 * to stderr.
 */

#define dsyslog(priority, format)         \
    {                                     \
        if (debug_flag)                   \
            fprintf(stderr, format "\n"); \
        else                              \
            syslog(priority, format);     \
    }

#define dsyslogf(priority, format, ...)                \
    {                                                  \
        if (debug_flag)                                \
            fprintf(stderr, format "\n", __VA_ARGS__); \
        else                                           \
            syslog(priority, format, __VA_ARGS__);     \
    }

/*
 * This dfprint()/dfprintf() macro won't call syslog(), only fprintf
 * to stderr if debug_flag is on.
 */

#define dfprint(lvl, format, ...)         \
    {                                     \
        if (debug_flag > lvl)             \
            fprintf(stderr, format "\n"); \
    }

#define dfprintf(lvl, format, ...)                     \
    {                                                  \
        if (debug_flag > lvl)                          \
            fprintf(stderr, format "\n", __VA_ARGS__); \
    }

#endif /* __dsc_syslog_debug_h */
