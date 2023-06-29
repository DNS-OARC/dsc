#!/bin/sh -xe

rm -f 1683879752.dscdata.xml

../dsc "$srcdir/test_285.conf"

test -f 1683879752.dscdata.xml || sleep 1
test -f 1683879752.dscdata.xml || sleep 2
test -f 1683879752.dscdata.xml || sleep 3
test -f 1683879752.dscdata.xml
diff -u 1683879752.dscdata.xml "$srcdir/test_285.xml_gold"
