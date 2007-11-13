#!/bin/sh
set -e

RM="rm -rf"
TD=`mktemp -d /tmp/XXXXXXXXXX`
trap '${RM} $TD' EXIT

ROOT=`svn info | awk '$1 == "Repository" && $2 == "Root:" {print $3}'`

cd $TD
TS=`date +%Y%m%d%H%M`
svn export $ROOT/dsc/trunk dsc-$TS

${RM} dsc-$TS/collector/update-tmfbase.sh
${RM} dsc-$TS/mk-release.sh

(cd dsc-$TS/doc; make all clean-release)
tar czvf ../dsc-$TS.tar.gz dsc-$TS
