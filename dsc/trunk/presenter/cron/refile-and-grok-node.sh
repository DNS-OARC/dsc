#!/bin/sh

set -e

: ${SERVER:?}
: ${NODE:?}
: ${EXECDIR:?}
: ${TYPES:?}

PROG=`basename $0`
PIDF="/var/tmp/$PROG-$SERVER-$NODE.pid"
if test -f $PIDF ; then
	PID=`cat $PIDF`
	if test -n "$PID" ; then
		if kill -0 $PID ; then
			exit 0
		fi
	fi
fi

trap "rm -f $PIDF" EXIT
echo $$ > $PIDF

exec >$PROG.stdout
#exec 2>&1
#set -x
date


for type in $TYPES ; do
	xmls=`ls -r | grep "\.$type\.xml$" | head -5` || true
	if test -n "$xmls" ; then
		for h in $xmls ; do
			secs=`echo $h | awk -F. '{print $1}'`
			# convert end time to start time
			secs=`expr $secs - 60`
			yymmdd=`date -u -r $secs +%Y%m%d`
			test -d $yymmdd || mkdir $yymmdd
			test -d $yymmdd/$type || mkdir $yymmdd/$type
			$EXECDIR/$type-extractor.pl $h
			mv $h $yymmdd/$type
		done
	fi
done
date
