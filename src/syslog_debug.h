/*
 * Copyright (c) 2016, OARC, Inc.
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

#ifndef __dsc_syslog_debug_h
#define __dsc_syslog_debug_h

extern int debug_flag;

/* This syslog() macro normally calls the real syslog() function, unless
 * debug_flag is on, in which case it does fprintf to stderr.  It relies on
 * the variable argument count feature of C99 macros.  Unfortunately, C99 does
 * not allow you to use 0 variable args, so where you would normally write
 * syslog(p, "string"), you should instead write syslog(p, "%s", string).
 */

#define syslog(priority, format, ...) \
    do { \
        if (debug_flag) \
            fprintf(stderr, format "\n", __VA_ARGS__); \
        else \
            (syslog)(priority, format, __VA_ARGS__); \
    } while (0)


/* This dfprintf() macro won't call syslog(), only fprintf to stderr if
 * debug_flag is on.
 */

#define dfprintf(lvl, format, ...) \
    do { \
        if (debug_flag > lvl) \
            fprintf(stderr, format "\n", __VA_ARGS__); \
    } while (0)


#endif /* __dsc_syslog_debug_h */
