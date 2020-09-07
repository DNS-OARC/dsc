#!/bin/sh -xe

user=`id -n -u`
group=`id -n -g`

echo "dnstap_unixsock /tmp/dnstap.sock INVALID:INVALID;" > test10.conf
! ../dsc -dddd test10.conf 2>test10.out
grep -qF "invalid USER for DNSTAP UNIX socket, does not exist" test10.out

echo "dnstap_unixsock /tmp/dnstap.sock $user:INVALID;" > test10.conf
! ../dsc -dddd test10.conf 2>test10.out
grep -qF "invalid GROUP for DNSTAP UNIX socket, does not exist" test10.out

echo "dnstap_unixsock /tmp/dnstap.sock $user:$group INVALID;" > test10.conf
! ../dsc -dddd test10.conf 2>test10.out
grep -qF "invalid UMASK for DNSTAP UNIX socket, should be octal" test10.out

echo "dnstap_unixsock /tmp/dnstap.sock $user:$group 0777777;" > test10.conf
! ../dsc -dddd test10.conf 2>test10.out
grep -qF "invalid UMASK for DNSTAP UNIX socket, too large value, maximum 0777" test10.out
