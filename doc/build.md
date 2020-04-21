## Requirements

```console
sudo apt install qemu-user-static python3-pyqt5 python3-construct python3-jsonschema python3-mnemonic python3-pyelftools gcc-arm-linux-gnueabihf libc6-dev-armhf-cross gdb-multiarch
```

For optional VNC support, please also install `libvncserver-dev`:

```console
sudo apt install libvncserver-dev
```


## Build

### speculos

```console
cmake -Bbuild -H.
make -C build/
```

Please note that the first build can take some time because a tarball of OpenSSL
is downloaded (the integrity of the downloaded tarball is checked) before being
built. Further invocations of `make` skip this step.

The following command line can be used for a debug build:

```console
cmake -Bbuild -DCMAKE_BUILD_TYPE=Debug -H.
```

### VNC support (optional)

Pass the `WITH_VNC` option to CMake:

```console
cmake -Bbuild -H. -DWITH_VNC=1
```
