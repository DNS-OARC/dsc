#!/bin/sh
set -e

RM="rm -rf"
TD=`mktemp -d /tmp/XXXXXXXXXX`
trap '${RM} $TD' EXIT

if test -f CVS/Root ; then
	CVSROOT=`cat CVS/Root`
	export CVSROOT
fi

cd $TD
TS=`date +%Y%m%d%H%M`
cvs export -r HEAD -d dsc-$TS dsc
${RM} update-tmfbase.sh
${RM} mk-release.sh
${RM} junk

(cd dsc-$TS/doc; make all clean-release)
tar czvf ../dsc-$TS.tar.gz dsc-$TS
