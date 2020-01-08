#!/bin/bash

set -e

function main()
{
    local app="$1"

    if [[ "${app}" == *"/launcher" ]]; then
        # special case for launcher binary
        local add_symbol="add-symbol-file \"${app}\" 0x101c0"
    else
        local add_symbol="add-symbol-file \"${app}\" 0x40000000"$'\n'"b *0x40000000"
    fi

    cat >/tmp/x.gdb<<EOF
source ./tools/gdbinit
set architecture arm
target remote 127.0.0.1:1234
handle SIGILL nostop pass noprint
${add_symbol}
c
EOF

    gdb-multiarch -q -nh -x /tmp/x.gdb
}

if [ $# -ne 1 ]; then
    echo "Usage: $0 <app.elf>"
    exit 1
fi

main "$1"
