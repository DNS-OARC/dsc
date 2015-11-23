#!/bin/sh
set -e
#et -x

PATH=/usr/bin:/bin:/sbin:/usr/sbin:/usr/local/bin:/usr/local/sbin
export PATH
PROG=`basename $0`
PREFIX=/usr/local/dsc

NODE=$1; shift
DEST=$1; shift
LOGIN=$1; shift

if test -f /usr/bin/md5sum ; then
        MD5=/usr/bin/md5sum
else
        MD5=md5
fi

SSH="/usr/local/bin/ssh"
ID="$PREFIX/certs/$DEST/$NODE.id"

PIDF="/tmp/$PROG-$NODE-$DEST.pid"
if test -f $PIDF; then
	PID=`cat $PIDF`
	if kill -0 $PID ; then
		exit 0
	else
		true
	fi
fi
echo $$ >$PIDF
trap "rm -f $PIDF" EXIT

perl -e 'sleep((rand 10) + 5)'

cd $PREFIX/run/$NODE/upload/$DEST

#xec > $PROG.out
#xec 2>&1

YYYYMMDD=`ls | grep '^[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]$' | head -1`
test -n "$YYYYMMDD" || exit 0
cd $YYYYMMDD

k=`ls -r | grep \.xml$ | head -500` || true
if test -n "$k" ; then

    # dsc receiver doesn't like + in filename
    #
    NODE=`echo $NODE | sed -e 's/+/_/g'`

    tar czf - $k \
	| $SSH -i $ID $LOGIN "dsc $NODE" \
	| while read marker md5 file x; do
		# ignore lines not beginning with the word "MD5".
		case "$marker" in
		MD5) ;;
		*) echo >&2 "not MD5 in '$marker'"; continue ;;
		esac

		# disallow / in returned filename.
		case "$file" in
		*/*) echo >&2 "slash in '$file'"; continue ;;
		esac

		# if the MD5 matches, remove the local file.
		if [ "$md5" = "`cat $file | $MD5 | awk '{print $1}'`" ]; then
			echo -n "$file "
		fi
	  done \
	| xargs rm

fi

cd ..; rmdir $YYYYMMDD 2>/dev/null

exit 0
