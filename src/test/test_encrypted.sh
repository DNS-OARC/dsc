#!/bin/sh -xe

rm -f 1643283234.dscdata.xml

../dsc "$srcdir/dnstap_encrypted.conf"

test -f 1643283234.dscdata.xml || sleep 1
test -f 1643283234.dscdata.xml || sleep 2
test -f 1643283234.dscdata.xml || sleep 3
test -f 1643283234.dscdata.xml
diff -u 1643283234.dscdata.xml "$srcdir/dnstap_encrypted.gold"
