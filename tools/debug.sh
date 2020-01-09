#!/bin/bash

set -e

function get_text_addr()
{
    local elf="$1"

    arm-none-eabi-readelf -WS "$elf" \
        | grep '\.text' \
        | awk '{ print "0x"$5 }'
}

function main()
{
    local app="$1"
    local launcher_text_addr=$(get_text_addr "build/src/launcher")

    cat >/tmp/x.gdb<<EOF
source ./tools/gdbinit
set architecture arm
target remote 127.0.0.1:1234
handle SIGILL nostop pass noprint
add-symbol-file "build/src/launcher" ${launcher_text_addr}
add-symbol-file "${app}" 0x40000000
b *0x40000000
c
EOF

    gdb-multiarch -q -nh -x /tmp/x.gdb
}

if [ $# -ne 1 ]; then
    echo "Usage: $0 <app.elf>"
    exit 1
fi

main "$1"
