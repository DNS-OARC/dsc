#!/bin/sh

set -e
#et -x

: ${SERVER:?}
: ${NODE:?}
: ${EXECDIR:?}
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
 exec 2>$PROG.stderr
#set -x
date


	xmls=`ls | grep "\.xml$" | head -100 | sort -t. +1` || true
	if test -n "$xmls" ; then
		for h in $xmls ; do

			type=`echo $h | awk -F. '{print $2}'`

			# hack
			if test "$type" = "d0_bit" ; then
				toname=`echo $h | sed -e 's/d0_bit/do_bit/'`
				mv $h $toname
				type='do_bit'
				h=$toname
			fi

			secs=`echo $h | awk -F. '{print $1}'`
			secs=`expr $secs - 60`
			#yymmdd=`date -u -r $secs +%Y%m%d`
			yymmdd=`perl -e 'use POSIX; print strftime "%Y%m%d", gmtime(shift)' $secs`
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
			#if $EXECDIR/$type-extractor.pl $h ; then
			if $EXECDIR/dsc-xml-extractor.pl $h ; then
				mv $h $yymmdd/$type
			else
				case $? in
				254)
					echo "problem reading $type data file for $SERVER/$NODE"
					break
					;;
				*)
					test -d errors || mkdir errors
					echo "error processing $SERVER/$NODE/$h" 1>&2
					mv $h errors
					;;
				esac
			fi
		done
	fi
date

if test -s $PROG.stderr ; then
	/usr/bin/uniq $PROG.stderr \
		| head -20 \
		| /usr/bin/Mail -s "Cron <${USER}@`hostname -s`> $0" $USER
fi
