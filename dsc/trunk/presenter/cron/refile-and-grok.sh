#!/bin/sh

# move incoming XML files into a filesystem hierarchy
# then pre-process them

set -e

sleep 20
cd /data/dns-oarc

exec >`basename $0`.stdout

for server in * ; do
	test -d $server || continue;
	cd $server
	for node in * ; do
		
		cd $node

		echo "$server/$node:"

		xmls=`ls | grep xml$` || true
		if test -n "$xmls" ; then
		    for h in $xmls ; do
			secs=`echo $h | awk -F. '{print $1}'`
			type=`echo $h | awk -F. '{print $2}'`
			yymmdd=`date -u -r $secs +%Y%m%d`
			test -d $yymmdd || mkdir $yymmdd
			test -d $yymmdd/$type || mkdir $yymmdd/$type
			$HOME/Edit/dsc/perl-gd/$type-extractor.pl $h
			mv $h $yymmdd/$type

		    done
		fi

		cd ..	# node

	done
	cd ..	# server
done
