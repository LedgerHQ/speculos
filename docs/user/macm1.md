# Docker - for Mac M1

## How to build the Docker image

Build the builder docker image using the command:

```shell
docker build -t speculos-builder:latest -f build.Dockerfile .
```

Then build the Docker container image with the following command:

```shell
docker build ./ -t speculos
```

If the build is successful, you should have a docker image named speculos:

```shell
docker image ls

REPOSITORY                                  TAG       IMAGE ID       CREATED          SIZE
speculos                                    latest    634c66a13457   15 minutes ago   593MB
```

## How to use the Docker image

Run the image with a few arguments from the root of the speculos project:

1. Using VNC (require a VNC client)

```shell
docker run --rm -it \
    -v $(pwd)/apps:/speculos/apps \
    --publish 41000:41000 \
    --publish 5001:5001 \
    speculos --display headless --vnc-port 41000 --api-port 5001 apps/btc.elf
```

- The app folder (here `$(pwd)/apps/`) is mounted thanks to `-v`
- The VNC server is available from the host thanks to `--publish`

2. Using QT

Be sure to have XQuartz running and accepting connections by running `socat` command:

```shell
socat TCP-LISTEN:6000,reuseaddr,fork UNIX-CLIENT:\"$DISPLAY\"
```

then, run:

```shell
docker run --rm -it \
    -v $(pwd)/apps:/speculos/apps \
    -v /tmp/.X11-unix:/tmp/.X11-unix:rw \
    -e DISPLAY=host.docker.internal:0 \
    --publish 41000:41000 \
    --publish 5001:5001 \
    speculos --api-port 5001 apps/btc.elf
```

- The app folder (here `$(pwd)/apps/`) is mounted thanks to `-v`
- The QT app should be displayed on your screen

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
