## How to use the Docker image

Pull the latest image from
[Docker Hub](https://hub.docker.com/r/ledgerhq/speculos):

```shell
docker pull ledgerhq/speculos
```

And run the image with a few arguments:

```shell
docker run -v $(pwd)/apps:/speculos/apps --publish 5900:5900 -it ledgerhq/speculos --display headless --vnc-port 5900 apps/btc.elf
```

- The app folder (here `$(pwd)/apps/`) is mounted thanks to `-v`
- The VNC server is available from the host thanks to `--publish`

The image can obviously run an interactive shell with `--entrypoint /bin/bash`.
