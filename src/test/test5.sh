#!/bin/sh -xe

../dsc "$srcdir/1573730567.conf"

test -f 1573730567.dscdata.xml || sleep 1
test -f 1573730567.dscdata.xml || sleep 2
test -f 1573730567.dscdata.xml || sleep 3
test -f 1573730567.dscdata.xml
diff 1573730567.dscdata.xml "$srcdir/1573730567.gold"
