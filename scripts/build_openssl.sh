#!/usr/bin/env bash
set -euxo pipefail

git submodule update --init
cd openssl/
./Configure --cross-compile-prefix=arm-linux-gnueabihf- no-asm no-threads no-shared linux-armv4
make CFLAGS=-mthumb