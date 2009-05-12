#!/bin/sh
# generate fresh configure and Makefile.ins
# only developers might need to run this script

run() {
	sh -e -x -c "$@"
}

run aclocal

# we need to run this manually because it does not create ltmain.sh when
# called from automake for some unknown reason (is it called at all?)
if fgrep -q AC_PROG_LIBTOOL configure.in
then
	run "libtoolize --automake --force --copy"
	echo "Warning: libtoolize 1.x does not update libtool.m4."
	echo ""
fi

run autoconf

run "autoheader --warnings=all"

run "automake --foreign --add-missing --copy --force-missing --no-force --warnings all"


echo "done."
