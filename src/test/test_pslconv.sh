#!/bin/sh -xe

"$srcdir/../dsc-psl-convert" -h

! "$srcdir/../dsc-psl-convert" --no-skip-idna-err "$srcdir/public_suffix_list.dat" > /dev/null

"$srcdir/../dsc-psl-convert" "$srcdir/public_suffix_list.dat" > /dev/null

"$srcdir/../dsc-psl-convert" --all "$srcdir/public_suffix_list.dat" > tld_list.dat

diff -u tld_list.dat "$srcdir/tld_list.dat.gold"

cat "$srcdir/public_suffix_list.dat" | "$srcdir/../dsc-psl-convert" - --all  > tld_list.dat

diff -u tld_list.dat "$srcdir/tld_list.dat.gold"
