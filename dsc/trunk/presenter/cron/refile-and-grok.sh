#!/bin/sh
set -e

cd /usr/local/dsc/data

PROG=`basename $0`
exec >$PROG.stdout
#exec 2>&1
#set -x
date

EXECDIR=/usr/local/dsc/libexec
TYPES=`ls $EXECDIR/*-extractor.pl | awk -F/ '{print $NF}' | awk -F- '{print $1}'`
export EXECDIR SERVER NODE TYPES

for SERVER in * ; do
	test -L $SERVER && continue;
	test -d $SERVER || continue;
	cd $SERVER
	for NODE in * ; do
		test -L $NODE && continue;
		test -d $NODE || continue;
		cd $NODE
		echo "$SERVER/$NODE:"
		sh $EXECDIR/refile-and-grok-node.sh &
		cd ..	# NODE
	done
	cd ..	# SERVER
done
wait
date
