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
		#
		# Uncomment the below test to skip nodes that don't
		# have an incoming directory.  It is useful if you
		# receive pre-grokked data to this presenter, but
		# NOT useful if you need backward compatibility with
		# older collectors that do not put XMLs into an
		# incoming subdirectory
		#
		#test -d $NODE/incoming || continue;
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
