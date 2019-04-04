#!/usr/bin/env bash

# We write this script because every RUN create a new layer
# and we dont really need multi-stage build.
# and docker squash is not stable

install_python_deps() {
    pip install --upgrade pip pipenv
    pipenv install
    source $(pipenv --venv)/bin/activate
}

install_openssl() {
    git submodule update --init
    cd openssl/
    ./Configure --cross-compile-prefix=arm-linux-gnueabihf- no-asm no-threads no-shared linux-armv4
    make CFLAGS=-mthumb
}

install_deps() {
    # build openssl, vnc, and python deps
    install_openssl &
    install_python_deps &
    make -C vnc/ &
    wait

    # build speculos
    make -C src/
}

set -euxo pipefail

run_deps=$(cat <<EOF
qemu-user-static
libvncserver-dev
EOF
)
build_deps=$(cat <<EOF
gcc-arm-linux-gnueabihf
libc6-dev-armhf-cross
gcc-arm-linux-gnueabihf
build-essential
git
EOF
)

apt-get update
apt-get install -y ${run_deps} ${build_deps}
install_deps

# remove dev deps & builds tools
apt-get remove --purge ${build_deps} -y
apt-get clean
rm -rf -- /var/lib/apt/lists/*
# remove git history
rm -rf .git

exit 0
