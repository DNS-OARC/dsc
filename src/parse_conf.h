/*
 * Copyright (c) 2016, OARC, Inc.
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

#ifndef __dsc_parse_conf_h
#define __dsc_parse_conf_h

#define PARSE_CONF_EINVAL   -2
#define PARSE_CONF_ERROR    -1
#define PARSE_CONF_OK       0
#define PARSE_CONF_LAST     1
#define PARSE_CONF_COMMENT  2
#define PARSE_CONF_EMPTY    3

#define PARSE_MAX_ARGS 64

typedef enum token_type token_type_t;
enum token_type {
    TOKEN_END = 0,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_STRINGS,
    TOKEN_NUMBERS,
    TOKEN_ANY
};

typedef struct token token_t;
struct token {
    token_type_t    type;
    const char*     token;
    size_t          length;
};

typedef struct token_syntax token_syntax_t;
struct token_syntax {
    const char*         token;
    int                 (*parse)(const token_t* tokens);
    const token_type_t  syntax[PARSE_MAX_ARGS];
};

int parse_conf(const char* file);

#endif /* __dsc_parse_conf_h */
