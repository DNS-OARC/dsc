#!/bin/sh

set -e
EXECDIR=/usr/local/dsc/libexec

cd /data/dns-oarc

PROG=`basename $0`
PIDF="/var/tmp/$PROG.pid"
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

exec >`basename $0`.stdout
#exec 2>&1
#set -x
date

uptime >> /httpd/htdocs/loadav/loadav.log

for server in * ; do
	test -d $server || continue;
	cd $server
	for node in * ; do
		
		cd $node

		echo "$server/$node:"

		xmls=`ls -r | grep xml$ | head -50` || true
		if test -n "$xmls" ; then
		    for h in $xmls ; do
			secs=`echo $h | awk -F. '{print $1}'`
			type=`echo $h | awk -F. '{print $2}'`
			yymmdd=`date -u -r $secs +%Y%m%d`
			test -d $yymmdd || mkdir $yymmdd
			test -d $yymmdd/$type || mkdir $yymmdd/$type
			if test -f $EXECDIR/$type-extractor.pl ; then
				$EXECDIR/$type-extractor.pl $h
				mv $h $yymmdd/$type
			fi

		    done
		fi

		cd ..	# node

	done
	cd ..	# server
done
date
