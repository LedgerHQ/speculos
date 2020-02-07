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
`1.5.5`:

```console
./speculos.py --sdk 1.5.5 apps/btc.elf
```


For more options, pass the `-h` or `--help` flag.

#### Keyboard control

- The keyboard left and right arrow keys are use instead of the Nano buttons.
- The `Q` key exits the application.

## Docker

#### Build
```console
docker build ./ -t speculos
```

#### Run
From the root of the speculos project
```console
docker run -it -v "$(pwd)"/apps:/speculos/apps -e DEVICE_MODEL=nanos -e SDK_VERSION=1.6 -e APP_FILE=btc.elf -e DEVICE_SEED=<SEED> speculos
```

#### docker-compose setup
```console
docker-compose up [-d]
```
> Default configuration is nanos / 1.6 / btc.elf / <EMPTY_SEED>

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

### blue-loader-python (ledgerblue)

Most clients relies on the
[blue-loader-python](https://github.com/LedgerHQ/blue-loader-python/) Python
library which supports Speculos since release
[0.1.24](https://pypi.org/project/ledgerblue/0.1.24/). This library can be
installed through pip using the following command-line:

```console
pip3 install ledgerblue
```

If the environment variables `LEDGER_PROXY_ADDRESS` and `LEDGER_PROXY_PORT` are
set, the library tries to use the device emulated by Speculos. For instance, the
following command-line sends the APDU `e0 c4 00 00 00` (Bitcoin app APDU to get
the version):

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
