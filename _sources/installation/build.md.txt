---
sort: 1
---

# Linux

## Requirements

For Debian (version 10 "Buster" or later) and Ubuntu (version 18.04 or later):

```shell
sudo apt install \
    git cmake gcc-arm-linux-gnueabihf libc6-dev-armhf-cross gdb-multiarch \
    python3-pyqt5 python3-construct python3-flask-restful python3-jsonschema \
    python3-mnemonic python3-pil python3-pyelftools python3-requests \
    qemu-user-static libvncserver-dev
```

Please note that VNC support (the `libvncserver-dev` package), although necessary
for a Python installation, is optional when building only the launcher (see below).

## Build & install

Easiest way to build & install Speculos is with `pip`. It will not only compile
the launcher, but will also install the Python package wrapped around it, which
includes the high-level Speculos entrypoint, used (for instance) in the
[Ragger](https://ledgerhq.github.io/ragger/) framework.

Following command ought to be executed into a Python virtualenv, else admin
rights will be necessary.

```shell
pip install .
```

## Building the Speculos launcher only

### speculos

```shell
cmake -B build/ -S .
make -C build/
```

Please note that the first build can take some time because a tarball of OpenSSL
is downloaded (the integrity of the downloaded tarball is checked) before being
built. Further invocations of `make` skip this step.

The following command line can be used for a debug build:

```shell
cmake -B build/ -DCMAKE_BUILD_TYPE=Debug -S .
```

### VNC support (optional)

Pass the `WITH_VNC` option to CMake:

```shell
cmake -B build/ -DWITH_VNC=1 -S .
```
