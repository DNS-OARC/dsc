#!/bin/sh -xe

for conf in "$srcdir/test9/"*.conf; do
    base=`dirname "$conf"`
    name=`basename "$conf" .conf`
    ! ../dsc -dddd "$conf" 2>test9.out
    cat test9.out
    if [ -f "$base/$name.grep" ]; then
        grep=`cat "$base/$name.grep"`
        grep -qF "$grep" test9.out
    fi
done
