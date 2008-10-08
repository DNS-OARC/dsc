#!/bin/sh

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
