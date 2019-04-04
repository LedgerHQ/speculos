#!/bin/bash

set -e

function main()
{
    local app="$1"

    cat >/tmp/x.gdb<<EOF
source ./tools/gdbinit
set architecture arm
target remote 127.0.0.1:1234
handle SIGILL nostop pass noprint
add-symbol-file "${app}" 0x40000000
b *0x40000000
c
EOF

    gdb-multiarch -q -nh -x /tmp/x.gdb
    #qemu-arm-static -g 1234 -singlestep ./src/launcher ./apps/btc.elf &
}

if [ $# -ne 1 ]; then
    echo "Usage: $0 <app.elf>"
    exit 1
fi

main "$1"
