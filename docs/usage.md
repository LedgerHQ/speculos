# Usage

## Basic usage

```console
./speculos.py apps/btc.elf
```

The Nano S is the default model; the Nano X and Blue can be specified on the
command-line:

```console
./speculos.py --model nanox apps/nanox#btc#1.2#57272a0f.elf
./speculos.py --model blue --sdk 1.5 apps/blue#btc#1.5#00000000.elf
```

The last SDK version is automatically selected. However, a specific version
be specified if the target app is not build against the last version of the SDK,
thanks to the `-k`/`--sdk` argument. For instance, to launch an app built
against the SDK `1.5` on the Nano S:

```console
./speculos.py --sdk 1.5 --model nanos apps/btc.elf
```

Supported SDK values for the `-k`/`--sdk` argument are:
|     | Nano S   | Nano X  | Blue            |
|-----|----------|---------|-----------------|
| SDK | 1.5, 1.6 | 1.2     | 1.5, blue-2.2.5 |

For more options, pass the `-h` or `--help` flag.

#### Keyboard control

- The keyboard left and right arrow keys are used instead of the Nano buttons.
  The down arrow can also be used as a more convenient shortcut.
- The `Q` key exits the application.

#### Display

Several display options are available through the `--display` parameter:

- `qt`: default, requires a X server
- `headless`: nothing is displayed
- `text`: the UI is displayed in the console (handy on Windows)

These options can be used along `--vnc-port` which spawns a VNC server on the
specified port. macOS users should also add `--vnc-password <password>` if using
the built-in VNC client because unauthenticated sessions doesn't seem to be
supported (issue #34).


## Docker

A docker image is available on
[Docker Hub](https://hub.docker.com/r/ledgerhq/speculos).

#### Run
From the root of the speculos project
```console
docker run -it -v "$(pwd)"/apps:/speculos/apps \
-p 1234:1234 -p 40000:40000 -p 41000:41000 -p 42000:42000 \
ledgerhq/speculos --model nanos ./apps/btc.elf --sdk 1.6 --seed "secret" --display headless \
--apdu-port 40000 --vnc-port 41000 --button-port 42000
```

#### Debug
```console
docker run -it -v "$(pwd)"/apps:/speculos/apps -p 1234:1234 -p 40000:40000 -p 41000:41000 -p 42000:42000 --entrypoint /bin/bash ledgerhq/speculos
```

#### docker-compose setup
```console
docker-compose up [-d]
```
> Default configuration is nanos / 1.6 / btc.elf / seed "secret"

Edit `docker-compose.yml` to configure port forwarding and environment variables that fit your needs.

#### Build

The following command-line can be used to create a docker image based on a local
[build](build.md):

```console
docker build ./ -t speculos
```

Replace `ledgerhq/speculos` with `speculos` in the sections above to use this
image.


## Bitcoin Testnet app

Launch the Bitcoin Testnet app, which requires the Bitcoin app:

```console
./speculos.py ./apps/btc-test.elf -l Bitcoin:./apps/btc.elf
```
