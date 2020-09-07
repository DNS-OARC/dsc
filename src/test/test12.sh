#!/bin/sh -xe

rm -f 1458044657.dscdata.xml

../dsc -d "$srcdir/test12.conf" 2>test12.out

test -f 1458044657.dscdata.xml || sleep 1
test -f 1458044657.dscdata.xml || sleep 2
test -f 1458044657.dscdata.xml || sleep 3
test -f 1458044657.dscdata.xml

grep -qF "loaded 6 known TLDs from ./knowntlds.txt.dist" test12.out
