#!/bin/sh
set -e

PATH=/usr/bin:/bin:/usr/local/ssh2/bin:/usr/local/bin:/udir/wessels/bin:/usr/local/bin:/udir/wessels/bin
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

CURL=curl
SRVAUTH="--cacert $HOME/dsc/etc/cacert.pem"
URI="https://dns-oarc.measurement-factory.com"

for d in pao1 ; do

	CLTAUTH="--cert $HOME/dsc/etc/$d-cert.pem"
	cd $HOME/dsc/run-$d
	sleep 5

	exec > $PROG.out
	exec 2>&1

	k=`ls -r | grep xml$ | head -50` || true
	if test -n "$k" ; then
		for f in $k ; do
			UPLOAD="--upload $f"
			$CURL $SRVAUTH $CLTAUTH $UPLOAD $URI
			rm -f $f
		done
	fi

done
