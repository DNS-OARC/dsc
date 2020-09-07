#!/bin/sh -xe

rm -f 1458044657.dscdata.xml

../dsc -d "$srcdir/test11.conf" 2>test11.out

test -f 1458044657.dscdata.xml || sleep 1
test -f 1458044657.dscdata.xml || sleep 2
test -f 1458044657.dscdata.xml || sleep 3
test -f 1458044657.dscdata.xml

grep -v "writing to 1458044657.dscdata.xml." test11.out | grep -v "_index: No database loaded for" > test11.out.tmp
mv test11.out.tmp test11.out

diff -u test11.out "$srcdir/test11.gold"
