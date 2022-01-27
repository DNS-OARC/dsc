#!/bin/sh -x

# Special test for coverage using dnswire example

if [ -x ~/workspace/dnswire/examples/reader_sender ]; then
    mkdir -p dnstap
    (cd dnstap && rm -f *.xml)
    ../dsc -f "$srcdir/dnstap_tcp.conf" &
    sleep 2
    ~/workspace/dnswire/examples/reader_sender "$srcdir/test.dnstap" 127.0.0.1 6666
    sleep 1
    ~/workspace/dnswire/examples/reader_sender "$srcdir/test.dnstap" 127.0.0.1 6666
    sleep 1
    ~/workspace/dnswire/examples/reader_sender "$srcdir/test.dnstap" 127.0.0.1 6666
    sleep 1
    ~/workspace/dnswire/examples/reader_sender "$srcdir/test.dnstap" 127.0.0.1 6666
    sleep 1
    ~/workspace/dnswire/examples/reader_sender "$srcdir/test.dnstap" 127.0.0.1 6666
    sleep 1
    ~/workspace/dnswire/examples/reader_sender "$srcdir/test.dnstap" 127.0.0.1 6666
    sleep 5
    pkill -ou `id -un` dsc
    sleep 5
    pgrep -ou `id -un` dsc || exit 0
    pkill -ou `id -un` dsc
    sleep 5
    pgrep -ou `id -un` dsc || exit 0
    pkill -KILL -ou `id -un` dsc
    exit 1
fi
