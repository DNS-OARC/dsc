#!/bin/sh
set -e

# sample cron job for pushing data from a collector to the analysis
# box.  Run every minute.

SERVICE="f-root"
NODES="sfo2"
HOME="dns-oarc.measurement-factory.com:/data/dns-oarc"


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

cd `dirname $0`
sleep 5

exec > $PROG.out
exec 2>&1

for d in $NODES ; do

        test -d $d || continue;
        cd $d

        k=`ls | grep xml$ | head -20` || true
        if test -n "$k" ; then
                rsync -az -e ssh $k ${HOME}/${SERVICE}/$d
                rm -f $k
        fi

        cd ..

done
rm -f $PIDF
