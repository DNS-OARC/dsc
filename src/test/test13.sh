#!/bin/sh -xe

rm -f 1458044657.dscdata.json 1458044657.dscdata.xml

rm -f test13.conf
cp "$srcdir/1458044657.conf" test13.conf
echo "output_user `id -nu`;" >>test13.conf
echo "output_group `id -ng`;" >>test13.conf
echo "output_mod 0644;" >>test13.conf

../dsc test13.conf

test -f 1458044657.dscdata.json || sleep 1
test -f 1458044657.dscdata.json || sleep 2
test -f 1458044657.dscdata.json || sleep 3
test -f 1458044657.dscdata.json
test "`stat -c "%a" 1458044657.dscdata.json`" = "644" || test "`perl -e 'printf("%o\n", (stat("1458044657.dscdata.json"))[2])'`" = "100644"

test -f 1458044657.dscdata.xml || sleep 1
test -f 1458044657.dscdata.xml || sleep 2
test -f 1458044657.dscdata.xml || sleep 3
test -f 1458044657.dscdata.xml
test "`stat -c "%a" 1458044657.dscdata.xml`" = "644" || test "`perl -e 'printf("%o\n", (stat("1458044657.dscdata.xml"))[2])'`" = "100644"
