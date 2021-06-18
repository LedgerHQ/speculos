---
sort: 2
---

# Docker

## How to use the Docker image

A docker image is available on [GitHub Packages](https://ghcr.io/ledgerhq/speculos). Pull the latest image:

```shell
docker pull ghcr.io/ledgerhq/speculos
docker image tag ghcr.io/ledgerhq/speculos speculos
```

And run the image with a few arguments from the root of the speculos project:

```shell
docker run --rm -it -v $(pwd)/apps:/speculos/apps --publish 41000:41000 speculos --display headless --vnc-port 41000 apps/btc.elf
```

- The app folder (here `$(pwd)/apps/`) is mounted thanks to `-v`
- The VNC server is available from the host thanks to `--publish`

The image can obviously run an interactive shell with `--entrypoint /bin/bash`.


### Arguments

All the arguments which are supported by `speculos.py` can be passed on the Docker command-line. Don't forget to publish container's ports when required using `-p`:

```shell
docker run --rm -it -v "$(pwd)"/apps:/speculos/apps \
-p 1234:1234 -p 5000:5000 -p 40000:40000 -p 41000:41000 speculos \
--model nanos ./apps/btc.elf --sdk 2.0 --seed "secret" --display headless --apdu-port 40000 --vnc-port 41000
```

### Debug

```shell
docker run --rm -it -v "$(pwd)"/apps:/speculos/apps -p 1234:1234 -p 5000:5000 -p 40000:40000 -p 41000:41000 --entrypoint /bin/bash speculos
```

### docker-compose setup

```shell
docker-compose up [-d]
```
> Default configuration is nanos / 2.0 / btc.elf / seed "secret"

Edit `docker-compose.yml` to configure port forwarding and environment variables that fit your needs.

## Build

The following command-line can be used to create a docker image based on a local
[build](../installation/build.md):

```shell
docker build ./ -t speculos
```
