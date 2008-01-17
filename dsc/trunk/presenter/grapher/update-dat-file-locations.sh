#!/bin/sh

cd /usr/local/dsc/data
for SERVER in *; do
	test -d $SERVER || continue
	echo "Server $SERVER..."
	cd $SERVER
	for NODE in *; do
		test -d $NODE || continue
		echo "  Node $NODE..."
		cd $NODE
		for YYYYMMDD in ????????; do
			test -d $YYYYMMDD || continue
			echo "    Date $YYYYMMDD..."
			cd $YYYYMMDD
				for DAT in */*.dat; do
					test "$DAT" = '*/*.dat' && break;
					echo -n '      '
					mv -v $DAT .
				done
				rmdir *
			cd ..
		done
		cd ..
	done
	cd ..
done
