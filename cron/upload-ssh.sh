#!/bin/sh
#
# Copyright (c) 2008-2020, OARC, Inc.
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

set -e

PATH=/usr/bin:/bin:/sbin:/usr/sbin:/usr/local/bin:/usr/local/sbin
export PATH
PROG=`basename $0`
PREFIX=/usr/local/dsc

NODE=$1; shift
DEST=$1; shift
LOGIN=$1; shift

if test -f /usr/bin/md5sum ; then
        MD5=/usr/bin/md5sum
else
        MD5=md5
fi

SSH="/usr/local/bin/ssh"
ID="$PREFIX/certs/$DEST/$NODE.id"

PIDF="/tmp/$PROG-$NODE-$DEST.pid"
if test -f $PIDF; then
	PID=`cat $PIDF`
	if kill -0 $PID ; then
		exit 0
	else
		true
	fi
fi
echo $$ >$PIDF
trap "rm -f $PIDF" EXIT

perl -e 'sleep((rand 10) + 5)'

cd $PREFIX/run/$NODE/upload/$DEST

#xec > $PROG.out
#xec 2>&1

YYYYMMDD=`ls | grep '^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]$' | head -1`
test -n "$YYYYMMDD" || exit 0
cd $YYYYMMDD

k=`ls -r | grep \.xml$ | head -500` || true
if test -n "$k" ; then

    # dsc receiver doesn't like + in filename
    #
    NODE=`echo $NODE | sed -e 's/+/_/g'`

    tar czf - $k \
	| $SSH -i $ID $LOGIN "dsc $NODE" \
	| while read marker md5 file x; do
		# ignore lines not beginning with the word "MD5".
		case "$marker" in
		MD5) ;;
		*) echo >&2 "not MD5 in '$marker'"; continue ;;
		esac

		# disallow / in returned filename.
		case "$file" in
		*/*) echo >&2 "slash in '$file'"; continue ;;
		esac

		# if the MD5 matches, remove the local file.
		if [ "$md5" = "`cat $file | $MD5 | awk '{print $1}'`" ]; then
			echo -n "$file "
		fi
	  done \
	| xargs rm

fi

cd ..; rmdir $YYYYMMDD 2>/dev/null

exit 0
