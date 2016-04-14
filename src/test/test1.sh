#!/bin/sh -e

../dsc ./1458044657.conf

test -f 1458044657.dscdata.json || sleep 1
test -f 1458044657.dscdata.json || sleep 2
test -f 1458044657.dscdata.json || sleep 3
test -f 1458044657.dscdata.json
diff 1458044657.dscdata.json 1458044657.json_gold

test -f 1458044657.dscdata.xml || sleep 1
test -f 1458044657.dscdata.xml || sleep 2
test -f 1458044657.dscdata.xml || sleep 3
test -f 1458044657.dscdata.xml
diff 1458044657.dscdata.xml 1458044657.xml_gold
