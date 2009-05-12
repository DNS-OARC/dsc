#!/bin/sh
set -e
#cvs status TmfBase| grep ^File | grep -v Up-to-date
rm -rfv tmp
mkdir tmp
cd tmp
svn export svn+ssh://cvs.measurement-factory.com/usr/local/svn/hapy/trunk
mv trunk Hapy
cd ..
rsync -av tmp/ TmfBase/
