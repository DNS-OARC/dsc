#!/bin/sh

set -e
#et -x

: ${SERVER:?}
: ${NODE:?}
: ${EXECDIR:?}
: ${TYPES:?}
export SERVER NODE

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
			secs=`expr $secs - 60`
			#yymmdd=`date -u -r $secs +%Y%m%d`
			yymmdd=`perl -e 'use POSIX; print strftime "%Y%m%d", localtime(time)'`
			test -d $yymmdd || mkdir $yymmdd
			test -d $yymmdd/$type || mkdir $yymmdd/$type
			#echo "Doing $type-extractor.pl $h"
			if test -s $yymmdd/$type/$h ; then
				ls -l $yymmdd/$type/$h $h 1>&2
				echo "removing dupe $SERVER/$NODE/$h" 1>&2
				rm -f $h
				continue
			elif test -f $yymmdd/$type/$h ; then
				echo "removing empty $SERVER/$NODE/$yymmdd/$type/$h" 1>&2
				rm -f $yymmdd/$type/$h
			fi
			if $EXECDIR/$type-extractor.pl $h ; then
				mv $h $yymmdd/$type
			else
				echo "error processing $SERVER/$NODE/$h" 1>&2
			fi
		done
	fi
done
date
