#!/bin/sh

set -e

if test $# -ne 1; then
	echo "usage: $0 start|stop"
	exit 1
fi

action=$1 ; shift

# default PIDFILE location
PIDFILE=/var/run/dsc.pid
PREFIX=/usr/local/dsc

get_pid() {
	if test -f $PIDFILE ; then
		PID=`cat $PIDFILE`
	else
		PID=''
	fi
}

case $action in
start)
	get_pid
	if test -n "$PID" ; then
		if kill -0 $PID; then
			echo "looks like DSC is already running (PID $PID)"
			exit 0;
		fi
	fi
	echo 'Starting DSC'
	$PREFIX/bin/dsc $PREFIX/etc/dsc.conf
	;;
stop)
	get_pid
	if test -n "$PID" ; then
		echo 'Stopping DSC'
		kill -INT $PID
	else
		echo "looks like DSC is not currently running"
		exit 0;
	fi
	;;
*)
	echo "unknown action: $action"
	exit 1
	;;
esac
