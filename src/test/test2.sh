#!/bin/sh -xe

../dsc -f "$srcdir/pid.conf"

test -s pid.pid
