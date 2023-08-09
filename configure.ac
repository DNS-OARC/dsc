# Copyright (c) 2008-2023, OARC, Inc.
# Copyright (c) 2007-2008, Internet Systems Consortium, Inc.
# Copyright (c) 2003-2007, The Measurement Factory, Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

AC_PREREQ(2.61)
AC_INIT([DSC], [2.15.0], [dsc@dns-oarc.net], [dsc], [https://github.com/DNS-OARC/dsc/issues])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AC_CONFIG_SRCDIR([src/md_array.c])
AC_CONFIG_HEADER([src/config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
PKG_PROG_PKG_CONFIG

# Check --enable-warn-all
AC_ARG_ENABLE([warn-all], [AS_HELP_STRING([--enable-warn-all], [Enable all compiler warnings])], [AX_CFLAGS_WARN_ALL()])

# Check --with-extra-cflags
AC_ARG_WITH([extra-cflags], [AS_HELP_STRING([--with-extra-cflags=CFLAGS], [Add extra CFLAGS])], [
  AC_MSG_NOTICE([appending extra CFLAGS... $withval])
  AS_VAR_APPEND(CFLAGS, [" $withval"])
])

# Check --with-extra-ldflags
AC_ARG_WITH([extra-ldflags], [AS_HELP_STRING([--with-extra-ldflags=LDFLAGS], [Add extra LDFLAGS])], [
  AC_MSG_NOTICE([appending extra LDFLAGS... $withval])
  AS_VAR_APPEND(LDFLAGS, [" $withval"])
])

# pcap thread
AC_ARG_ENABLE(threads,
  [AS_HELP_STRING([--enable-threads],
    [enable the usage of threads (default disabled)])],
  [AX_PCAP_THREAD],
  [AX_PCAP_THREAD_PCAP])

# dnstap
use_dnstap=no
AC_ARG_ENABLE([dnstap], [AS_HELP_STRING([--enable-dnstap], [DNSTAP input support])], [
  AC_DEFINE([USE_DNSTAP], [1], [Define to 1 if DNSTAP support is built in.])
  PKG_CHECK_MODULES([libdnswire], [libdnswire >= 0.4.0])
  PKG_CHECK_MODULES([libuv], [libuv])
  use_dnstap=yes
])
AM_CONDITIONAL([USE_DNSTAP], [test x$use_dnstap = xyes])

# Check --enable-gcov
AC_ARG_ENABLE([gcov], [AS_HELP_STRING([--enable-gcov], [Enable coverage testing])], [
  coverage_cflags="--coverage -g -O0 -fno-inline -fno-inline-small-functions -fno-default-inline"
  AC_MSG_NOTICE([enabling coverage testing... $coverage_cflags])
  AS_VAR_APPEND(CFLAGS, [" -DGCOV_FLUSH=1 $coverage_cflags"])
])
AM_CONDITIONAL([ENABLE_GCOV], [test "x$enable_gcov" != "xno"])
AM_EXTRA_RECURSIVE_TARGETS([gcov])

# Checks for libraries.
AC_CHECK_LIB([resolv], [inet_aton])
AC_CHECK_LIB([nsl], [gethostbyname])
AC_CHECK_LIB([socket], [connect])
AC_CHECK_LIB([GeoIP], [GeoIP_open])
PKG_CHECK_MODULES([libmaxminddb], [libmaxminddb], [AC_DEFINE([HAVE_LIBMAXMINDDB], [1], [Define to 1 if you have libmaxminddb.])], [:])
AC_CHECK_LIB([m], [log10])

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_HEADER_TIME
AC_CHECK_HEADERS([arpa/nameser_compat.h arpa/inet.h fcntl.h memory.h])
AC_CHECK_HEADERS([netdb.h netinet/in.h stdint.h stdlib.h string.h])
AC_CHECK_HEADERS([strings.h sys/mount.h sys/param.h sys/socket.h])
AC_CHECK_HEADERS([sys/statfs.h sys/statvfs.h sys/time.h syslog.h])
AC_CHECK_HEADERS([unistd.h netinet/ip_compat.h pcap/sll.h])
AC_CHECK_HEADERS([GeoIP.h maxminddb.h])
AC_CHECK_HEADERS([endian.h sys/endian.h machine/endian.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_STDBOOL
AC_TYPE_INT8_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_STAT
AC_CHECK_FUNCS([dup2 gettimeofday memset regcomp select strcasecmp strchr])
AC_CHECK_FUNCS([strdup strerror strrchr strspn strstr strtoull statvfs])

# pid file
AC_ARG_WITH(pid-file,
  [AS_HELP_STRING([--with-pid-file=FILE], [write pid to FILE [/run/dsc.pid]])],
  [],
  [with_pid_file=/run/dsc.pid])

AC_SUBST([DSC_PID_FILE], [$with_pid_file])

# data dir
AC_ARG_WITH(data-dir,
  [AS_HELP_STRING([--with-data-dir=DIR], [use DIR for DSC data [LOCALSTATEDIR/lib/dsc]])],
  [],
  [with_data_dir=${localstatedir}/lib/dsc])

AC_SUBST([DSC_DATA_DIR], [$with_data_dir])

# Generate
AC_CONFIG_FILES([
  Makefile
  src/Makefile
  src/test/Makefile
])
AC_OUTPUT
