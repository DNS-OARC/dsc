#!/bin/sh -xe

rm -f 1458044657.dscdata.json

../dsc "$srcdir/1458044657.conf"

test -f 1458044657.dscdata.json || sleep 1
test -f 1458044657.dscdata.json || sleep 2
test -f 1458044657.dscdata.json || sleep 3
test -f 1458044657.dscdata.json
diff 1458044657.dscdata.json "$srcdir/1458044657.json_gold"

test -f 1458044657.dscdata.xml || sleep 1
test -f 1458044657.dscdata.xml || sleep 2
test -f 1458044657.dscdata.xml || sleep 3
test -f 1458044657.dscdata.xml
diff 1458044657.dscdata.xml "$srcdir/1458044657.xml_gold"
