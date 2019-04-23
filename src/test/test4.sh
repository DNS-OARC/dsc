#!/bin/sh -xe

rm -f 1458044657.dscdata.xml

../dsc "$srcdir/response_time.conf"

test -f 1458044657.dscdata.xml || sleep 1
test -f 1458044657.dscdata.xml || sleep 2
test -f 1458044657.dscdata.xml || sleep 3
test -f 1458044657.dscdata.xml
diff 1458044657.dscdata.xml "$srcdir/response_time.gold"

rm -f 1458044657.dscdata.xml

../dsc "$srcdir/response_time2.conf"

test -f 1458044657.dscdata.xml || sleep 1
test -f 1458044657.dscdata.xml || sleep 2
test -f 1458044657.dscdata.xml || sleep 3
test -f 1458044657.dscdata.xml
diff 1458044657.dscdata.xml "$srcdir/response_time2.gold"

rm -f 1458044657.dscdata.xml

../dsc "$srcdir/response_time3.conf"

test -f 1458044657.dscdata.xml || sleep 1
test -f 1458044657.dscdata.xml || sleep 2
test -f 1458044657.dscdata.xml || sleep 3
test -f 1458044657.dscdata.xml
diff 1458044657.dscdata.xml "$srcdir/response_time3.gold"
