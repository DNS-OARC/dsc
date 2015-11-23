#!/bin/sh
# generate fresh configure and Makefile.ins
# only developers might need to run this script

run() {
	sh -e -x -c "$@"
}

run aclocal
# backport AC_LIBTOOL_TAGS from Libtool 1.6
# http://www.mail-archive.com/libtool@gnu.org/msg04504.html
cat cfgaux/libtooltags.m4 >> aclocal.m4

#workaround for Automake 1.5
if grep m4_regex aclocal.m4 >/dev/null
then
    perl -i.bak -p -e 's/m4_patsubst/m4_bpatsubst/g; s/m4_regexp/m4_bregexp/g;' aclocal.m4
fi

run autoheader
config_h_in=src/include/Hapy/config.h.in
if grep PACKAGE_ $config_h_in > /dev/null
then
    perl -i.bak -p -e 's/#undef PACKAGE_/#undef HAPY_PACKAGE_/g;' $config_h_in
fi

run "automake --foreign --add-missing"

run autoconf

echo "done."
