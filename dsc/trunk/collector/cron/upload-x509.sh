#!/bin/sh
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

CURL="curl --silent"
SRVAUTH="--cacert /usr/local/dsc/etc/cacert.pem"
URI="https://dns-oarc.measurement-factory.com"

for node in hq lgh sc ; do
	CLTAUTH="--cert /usr/local/dsc/etc/$node-cert.pem"
	cd /usr/local/dsc/run/$node
	sleep 5

	exec > $PROG.out
	exec 2>&1

	k=`ls -t | grep xml$ | head -500` || true
	test -n "$k" || continue
	TF=`mktemp /tmp/put.XXXXXXXXXXXXX`
	tar czf $TF $k
	mv $TF $TF.tar
	TF="$TF.tar"
	UPLOAD="--upload $TF"
	$CURL $SRVAUTH $CLTAUTH $UPLOAD $URI | awk '$1 == "Stored" {print $2}' | xargs rm -v
	rm -f $TF
done
