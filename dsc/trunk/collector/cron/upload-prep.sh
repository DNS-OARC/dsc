#!/bin/sh

# this script allows collected files to go to multiple 
# archivers.  

set -e
#et -x

PATH=/usr/bin:/bin:/sbin:/usr/sbin:/usr/local/bin:/usr/local/sbin
export PATH
PROG=`basename $0`

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

sleep 3

for node in THIS THAT ; do
	cd $HOME/dsc/run-$node

	exec > $PROG.out
	exec 2>&1

	k=`ls -r | grep xml$ | head -400` || true

	for to in to-* ; do
		ln -f $k $to
	done
	rm -f $k
done
