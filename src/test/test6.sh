#!/bin/sh -xe

mmdb=
for dir in /var/lib/GeoIP /usr/share/GeoIP /usr/local/share/GeoIP "$HOME/GeoIP"; do
    if [ -f "$dir/GeoLite2-ASN.mmdb" -a -f "$dir/GeoLite2-Country.mmdb" ]; then
        mmdb="$dir"
        break
    fi
done
if [ -z "$mmdb" ]; then
    exit 0
fi

cp "$srcdir/mmdb.conf" mmdb.conf.run
echo "maxminddb_asn \"$mmdb/GeoLite2-ASN.mmdb\";" >> mmdb.conf.run
echo "maxminddb_country \"$mmdb/GeoLite2-Country.mmdb\";" >> mmdb.conf.run

rm -f 1458044657.dscdata.xml

../dsc mmdb.conf.run

test -f 1458044657.dscdata.xml || sleep 1
test -f 1458044657.dscdata.xml || sleep 2
test -f 1458044657.dscdata.xml || sleep 3
test -f 1458044657.dscdata.xml
grep -q ASN 1458044657.dscdata.xml
grep -q CountryCode 1458044657.dscdata.xml
