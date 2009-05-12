#!/bin/sh

#
# dsc_enable=YES
# dsc_instances="node1 node2"
# 

. /etc/rc.subr

name="dsc"
rcvar=`set_rcvar`
load_rc_config $name

case "$dsc_enable" in
[Yy][Ee][Ss])
	;;
*)
	exit 0
esac

PIDDIR=/var/run
PREFIX=/usr/local/dsc

get_pid() {
	instance=$1
	if test -f $PIDDIR/dsc-$instance.pid ; then
		PID=`cat $PIDDIR/dsc-$instance.pid`
	else
		PID=''
	fi
}

do_status() {
	instance=$1
	if test ! -n "$PID" ; then
		echo "dsc-$instance is not running"
		false
	elif kill -0 $PID 2>/dev/null ; then
		echo "dsc-$instance is running as PID $PID"
	else
		echo "dsc-$instance is not running"
		false
	fi
}

do_start() {
	instance=$1
	if test -n "$PID" ; then
		if kill -0 $PID 2>/dev/null ; then
			echo "dsc-$instance is already running as PID $PID"
			true
			return
		fi
	fi
	if test -s $PREFIX/etc/dsc-$instance.conf ; then
		echo "Starting dsc-$instance"
		$PREFIX/bin/dsc $PREFIX/etc/dsc-$instance.conf
	else
		echo "$PREFIX/etc/dsc-$instance.conf not found"
		false
	fi
}

do_stop() {
	if test -n "$PID" ; then
		echo "Stopping dsc-$instance"
		kill -INT $PID
	else
		echo "dsc-$instance is not running"
		exit 0;
	fi
}


action=$1 ; shift

if test $# -eq 0 -a -n "$dsc_instances" ; then
	set $dsc_instances
fi

for instance ; do
    get_pid $instance
    case $action in
    start|faststart)
	do_start $instance
	;;
    stop)
	do_stop $instance
	;;
    restart)
	do_stop $instance
	do_start $instance
	;;
    status)
	do_status $instance
	;;
    *)
	echo "unknown action: $action"
	exit 1
	;;
    esac
done
