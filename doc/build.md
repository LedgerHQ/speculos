## Requirements

```console
sudo apt install qemu-user-static python3-pyqt5 python3-construct python3-mnemonic python3-pyelftools gcc-arm-linux-gnueabihf libc6-dev-armhf-cross gdb-multiarch
```

For optional VNC support, please also install `libvncserver-dev`:

```console
sudo apt install libvncserver-dev
```


## Build

### speculos

```console
mkdir -p build
cmake -Bbuild -H.
make -C build/
```

### VNC support (optional)

Pass the `WITH_VNC` option to CMake:

```console
cmake -Bbuild -H. -DWITH_VNC=1
```
