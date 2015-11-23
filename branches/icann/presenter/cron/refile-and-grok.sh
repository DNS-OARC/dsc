#!/bin/sh
set -e

cd /usr/local/dsc/data

PROG=`basename $0`
exec >$PROG.stdout
#exec 2>&1
#set -x
date

EXECDIR=/usr/local/dsc/libexec
export EXECDIR SERVER NODE

for SERVER in * ; do
	test -L $SERVER && continue;
	test -d $SERVER || continue;
	cd $SERVER
	for NODE in * ; do
		test -L $NODE && continue;
		test -d $NODE || continue;
		cd $NODE
		echo "$SERVER/$NODE:"
		$EXECDIR/dsc-xml-extractor.pl >dsc-xml-extractor.out 2>&1 # &
		cd ..	# NODE
		# sleep 1
	done
	cd ..	# SERVER
done
wait
date
