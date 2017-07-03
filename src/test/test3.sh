#!/bin/sh -xe

! ../dsc -f "$srcdir/statinter.conf"
! ../dsc -f "$srcdir/statinter2.conf"
../dsc -f "$srcdir/cnetmask.conf"
! ../dsc -f "$srcdir/cnetmask2.conf"
! ../dsc -f "$srcdir/cnetmask3.conf"
! ../dsc -f "$srcdir/parseconf.conf"
../dsc -f "$srcdir/parseconf2.conf"
