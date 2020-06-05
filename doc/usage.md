# Usage

## Basic usage

```console
./speculos.py apps/btc.elf
```

The Ledger Nano S is the default model and the Ledger Blue can be specified on
the command-line:

```console
./speculos.py --model blue apps/btc-blue.elf
```

If the target app is not build against the version `1.6` of the SDK, use the
`-k`/`--sdk` argument. For instance, to launch an app built against the SDK
`1.5`:

```console
./speculos.py --sdk 1.5 apps/btc.elf
```
Supported SDK values for the `-k`/`--sdk` argument are:
|     | Nano S emulation | Blue emulation |
|-----|------------------|----------------|
| SDK |     1.5, 1.6     |   blue-2.2.5   |

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

#### Build
```console
docker build ./ -t speculos
```

#### Debug
```console
docker run -it -v "$(pwd)"/apps:/speculos/apps -p 1234:1234 -p 40000:40000 -p 41000:41000 -p 42000:42000 --entrypoint /bin/bash speculos
```

#### Run
From the root of the speculos project
```console
docker run -it -v "$(pwd)"/apps:/speculos/apps \
-p 1234:1234 -p 40000:40000 -p 41000:41000 -p 42000:42000 \
speculos --model nanos ./apps/btc.elf --sdk 1.6 --seed "secret" --display headless \
--apdu-port 40000 --vnc-port 41000 --button-port 42000
```

#### docker-compose setup
```console
docker-compose up [-d]
```
> Default configuration is nanos / 1.6 / btc.elf / seed "secret"

Edit `docker-compose.yml` to configure port forwarding and environment variables that fit your needs.

## Bitcoin Testnet app

Launch the Bitcoin Testnet app, which requires the Bitcoin app:

```console
./speculos.py ./apps/btc-test.elf -l Bitcoin:./apps/btc.elf
```


## Debug

Debug an app thanks to gdb:

```console
./speculos.py -d apps/btc.elf &
./tools/debug.sh apps/btc.elf
```

[Semihosting](semihosting.md) features can be used as an additional debug
mechanism.

## Clients

Clients can communicate with the emulated device using APDUs, as usual. Speculos
embbeds a TCP server (listening on `127.0.0.1:9999`) to forward APDUs to the
target app.

### ledgerctl (ledgerwallet)

[ledgerwallet](https://github.com/LedgerHQ/ledgerctl) is a library to control
Ledger devices, also available through the command `ledgerctl` (it can be
installed thanks to [pip](https://pypi.org/project/ledgerwallet/):

```console
pip3 install ledgerwallet
```

If the environment variables `LEDGER_PROXY_ADDRESS` and `LEDGER_PROXY_PORT` are
set, the library tries to use the device emulated by Speculos. For instance, the
following command-line sends the APDU `e0 c4 00 00 00` (Bitcoin app APDU to get
the version):

```console
$ echo 'e0c4000000' | LEDGER_PROXY_ADDRESS=127.0.0.1 LEDGER_PROXY_PORT=9999 ledgerctl send -
13:37:35.096:apdu: > e0c4000000
13:37:35.099:apdu: < 1b3001030e0100039000
1b3001030e0100039000
```

### blue-loader-python (ledgerblue)

Most clients relies on the
[blue-loader-python](https://github.com/LedgerHQ/blue-loader-python/) Python
library which supports Speculos since release
[0.1.24](https://pypi.org/project/ledgerblue/0.1.24/). This library can be
installed through pip using the following command-line:

```console
pip3 install ledgerblue
```

The usage is similar to `ledgerctl`:

```console
$ ./speculos.py ./apps/btc.elf &
$ echo 'e0c4000000' | LEDGER_PROXY_ADDRESS=127.0.0.1 LEDGER_PROXY_PORT=9999 python3 -m ledgerblue.runScript --apdu
=> b'e0c4000000'
<= b'1b30010308010003'9000
<= Clear bytearray(b'\x1b0\x01\x03\x08\x01\x00\x03')
```

### btchip-python

Use [btchip-python](https://github.com/LedgerHQ/btchip-python) without a real device:

```console
PYTHONPATH=$(pwd) LEDGER_PROXY_ADDRESS=127.0.0.1 LEDGER_PROXY_PORT=9999 python tests/testMultisigArmory.py
```

Note: `btchip-python` relies on its own library to communicate with devices
(physical or emulated) instead of `ledgerblue` to transmit APDUs.

### ledger-live-common

```console
./tools/ledger-live-http-proxy.py &
DEBUG_COMM_HTTP_PROXY=http://127.0.0.1:9998 ledger-live getAddress -c btc --path "m/49'/0'/0'/0/0" --derivationMode segwit
```
