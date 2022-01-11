#!/bin/sh
#
# Copyright (c) 2008-2022, OARC, Inc.
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

# PURPOSE:
#
# This script copies (links) XML output files from the DSC collector
# to one or more "transfer" directories.  This allows the XML files
# to be sent to multiple "presenter" boxes independently.  If one
# destination is unreachable, XML files accumulate there until
# it becomes available again.  Meanwhile, recent files are still
# sent to the other destinations.

set -e
#et -x

PATH=/usr/bin:/bin:/sbin:/usr/sbin:/usr/local/bin:/usr/local/sbin
export PATH
PROG=`basename $0`

# exit if this script is already running
#
PIDF="/tmp/$PROG.pid"
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

# wait a few seconds for 'dsc' to finish writing its XML files to disk
#
sleep 3

for conf in /usr/local/dsc/etc/*.conf ; do
	#
	# extract run_dir from a dsc.conf file
	#
	rundir=`perl -ne 'print $1 if /^run_dir\s+"([^"]+)"/' $conf`
	cd $rundir;

	#xec > $PROG.out
	#xec 2>&1

	k=`ls -r | grep xml$ | head -400` || true
	test -z "$k" && continue

	for up in upload/* ; do
		if test "$up" = "upload/*" ; then
			echo "No upload dirs in $rundir, failing"
			continue 2
		fi
		ln -f $k $up
	done
	rm -f $k
done
