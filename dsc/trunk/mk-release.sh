#!/bin/sh
set -e

RM="rm -rf"
TD=`mktemp -d /tmp/XXXXXXXXXX`
trap '${RM} $TD' EXIT

ROOT=`svn info | awk '$1 == "Repository" && $2 == "Root:" {print $3}'`
URL=`svn info | awk '$1 == "URL:" {print $2}'`
BRANCH=`echo $URL | sed -e "s@^$ROOT/dsc@@"`

# this goofyness turns a path like "/branches/sql" into "-sql"
# and turns "/trunk" into ""
BRANCH2=`echo $BRANCH | sed -e 's@/[a-z][a-z]*@@' -e 's@/@-@g'`

cd $TD
TS=`date +%Y%m%d%H%M`
DIR="dsc-${TS}${BRANCH2}"

echo $DIR
exit 0

svn export $ROOT/dsc/$BRANCH $DIR

${RM} $DIR/collector/update-tmfbase.sh
${RM} $DIR/mk-release.sh

(cd $DIR/doc; make all clean-release)
tar czvf ../$DIR.tar.gz $DIR
