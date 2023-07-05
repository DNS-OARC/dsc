#!/bin/sh -xe

rm -f 1688541706.dscdata.xml

../dsc "$srcdir/test_291.conf"

test -f 1688541706.dscdata.xml || sleep 1
test -f 1688541706.dscdata.xml || sleep 2
test -f 1688541706.dscdata.xml || sleep 3
test -f 1688541706.dscdata.xml
diff -u 1688541706.dscdata.xml "$srcdir/test_291.xml_gold"
