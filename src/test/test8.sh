#!/bin/sh -xe

rm -f 1515583363.dscdata.xml

../dsc -dddd "$srcdir/dnso1tcp.conf"

test -f 1515583363.dscdata.xml || sleep 1
test -f 1515583363.dscdata.xml || sleep 2
test -f 1515583363.dscdata.xml || sleep 3
test -f 1515583363.dscdata.xml
diff -u 1515583363.dscdata.xml "$srcdir/dnso1tcp.gold"
