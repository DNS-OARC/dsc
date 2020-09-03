#!/bin/sh -xe

cp "$srcdir/dns6.conf" dns6.conf.run

rm -f 1543333920.dscdata.xml

../dsc dns6.conf.run

test -f 1543333920.dscdata.xml || sleep 1
test -f 1543333920.dscdata.xml || sleep 2
test -f 1543333920.dscdata.xml || sleep 3
test -f 1543333920.dscdata.xml
diff -u 1543333920.dscdata.xml "$srcdir/dns6.gold"

for dir in /var/lib/GeoIP /usr/share/GeoIP /usr/local/share/GeoIP "$HOME/GeoIP"; do
    if [ -f "$dir/GeoLite2-ASN.mmdb" -a -f "$dir/GeoLite2-Country.mmdb" ]; then
        echo "dataset cc ip All:null CountryCode:country any;
dataset asn ip All:null ASN:asn any;
output_format XML;
asn_indexer_backend maxminddb;
country_indexer_backend maxminddb;
maxminddb_asn \"$dir/GeoLite2-ASN.mmdb\";
maxminddb_country \"$dir/GeoLite2-Country.mmdb\";" >> dns6.conf.run

rm -f 1543333920.dscdata.xml

../dsc dns6.conf.run

test -f 1543333920.dscdata.xml || sleep 1
test -f 1543333920.dscdata.xml || sleep 2
test -f 1543333920.dscdata.xml || sleep 3
test -f 1543333920.dscdata.xml
grep -q ASN 1543333920.dscdata.xml
grep -q CountryCode 1543333920.dscdata.xml

        break
    fi
done
