#!/bin/sh

# Special test for coverage using dnswire example

if [ -x ~/workspace/dnswire/examples/reader_sender ]; then
    mkdir -p dnstap
    cd dnstap
    rm -f *.xml
    ../../dsc -f "$srcdir/../dnstap_tcp.conf" &
    sleep 2
    ~/workspace/dnswire/examples/reader_sender "$srcdir/../test.dnstap" 127.0.0.1 6666
    sleep 1
    ~/workspace/dnswire/examples/reader_sender "$srcdir/../test.dnstap" 127.0.0.1 6666
    sleep 1
    ~/workspace/dnswire/examples/reader_sender "$srcdir/../test.dnstap" 127.0.0.1 6666
    sleep 1
    ~/workspace/dnswire/examples/reader_sender "$srcdir/../test.dnstap" 127.0.0.1 6666
    sleep 1
    ~/workspace/dnswire/examples/reader_sender "$srcdir/../test.dnstap" 127.0.0.1 6666
    sleep 1
    ~/workspace/dnswire/examples/reader_sender "$srcdir/../test.dnstap" 127.0.0.1 6666
    sleep 5
    pkill -ou sonar dsc
    sleep 5
    pgrep -ou sonar dsc || exit 0
    pkill -ou sonar dsc
    sleep 5
    pgrep -ou sonar dsc || exit 0
    pkill -KILL -ou sonar dsc
    exit 1
fi
