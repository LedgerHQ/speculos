# Docker

## How to use the Docker image

A docker image is available on [GitHub Packages](https://ghcr.io/ledgerhq/speculos). Pull the latest image:

```shell
docker pull ghcr.io/ledgerhq/speculos
docker image tag ghcr.io/ledgerhq/speculos speculos
```

And run the image with a few arguments from the root of the speculos project:

```shell
docker run --rm -it -v $(pwd)/apps:/speculos/apps --publish 41000:41000 speculos --display headless --vnc-port 41000 apps/boil.elf
```

- The app folder (here `$(pwd)/apps/`) is mounted thanks to `-v`
- The VNC server is available from the host thanks to `--publish`

The image can obviously run an interactive shell with `--entrypoint /bin/bash`.

### Arguments

All the arguments which are supported by `speculos.py` can be passed on the Docker command-line. Don't forget to publish container's ports when required using `-p`:

```shell
docker run --rm -it -v "$(pwd)"/apps:/speculos/apps \
-p 1234:1234 -p 5000:5000 -p 40000:40000 -p 41000:41000 speculos \
./apps/boil.elf --seed "secret" --display headless --apdu-port 40000 --vnc-port 41000
# Official images set SPECULOS_HOST=0.0.0.0. Outside Docker, pass --host 0.0.0.0
# when you need non-localhost clients.
```

### Debug

```shell
docker run --rm -it -v "$(pwd)"/apps:/speculos/apps -p 1234:1234 -p 5000:5000 -p 40000:40000 -p 41000:41000 --entrypoint /bin/bash speculos
```

### docker-compose setup

```shell
docker-compose up [-d]
```
> Default configuration is boil.elf / seed "secret" (model auto-detected)

Edit `docker-compose.yml` to configure port forwarding and environment variables that fit your needs.

## Build

The following command-line can be used to create a docker image based on a local
[build](../installation/build.md):

```shell
docker build ./ -t speculos
```
