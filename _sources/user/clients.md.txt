---
sort: 3
---

# Clients: how to send APDUs

Clients can communicate with the emulated device using APDUs, as usual. Speculos
embbeds a TCP server (listening on `127.0.0.1:9999`) to forward APDUs to the
target app.

## ledgerctl (ledgerwallet)

[ledgerwallet](https://github.com/LedgerHQ/ledgerctl) is a library to control
Ledger devices, also available through the command `ledgerctl` (it can be
installed thanks to [pip](https://pypi.org/project/ledgerwallet/)):

```shell
pip3 install ledgerwallet
```

If the environment variables `LEDGER_PROXY_ADDRESS` and `LEDGER_PROXY_PORT` are
set, the library tries to use the device emulated by Speculos. For instance, the
following command-line sends the APDU `e0 c4 00 00 00` (Bitcoin app APDU to get
the version):

```shell
$ echo 'e0c4000000' | LEDGER_PROXY_ADDRESS=127.0.0.1 LEDGER_PROXY_PORT=9999 ledgerctl send -
13:37:35.096:apdu: > e0c4000000
13:37:35.099:apdu: < 1b3001030e0100039000
1b3001030e0100039000
```

## blue-loader-python (ledgerblue)

Most clients relies on the
[blue-loader-python](https://github.com/LedgerHQ/blue-loader-python/) Python
library which supports Speculos since release
[0.1.24](https://pypi.org/project/ledgerblue/0.1.24/). This library can be
installed through pip using the following command-line:

```shell
pip3 install ledgerblue
```

The usage is similar to `ledgerctl`:

```shell
$ ./speculos.py ./apps/btc.elf &
$ echo 'e0c4000000' | LEDGER_PROXY_ADDRESS=127.0.0.1 LEDGER_PROXY_PORT=9999 python3 -m ledgerblue.runScript --apdu
=> b'e0c4000000'
<= b'1b30010308010003'9000
<= Clear bytearray(b'\x1b0\x01\x03\x08\x01\x00\x03')
```

## btchip-python

Use [btchip-python](https://github.com/LedgerHQ/btchip-python) without a real device:

```shell
PYTHONPATH=$(pwd) LEDGER_PROXY_ADDRESS=127.0.0.1 LEDGER_PROXY_PORT=9999 python tests/testMultisigArmory.py
```

Note: `btchip-python` relies on its own library to communicate with devices
(physical or emulated) instead of `ledgerblue` to transmit APDUs.

## ledger-live-common

```shell
./tools/ledger-live-http-proxy.py &
DEBUG_COMM_HTTP_PROXY=http://127.0.0.1:9998 ledger-live getAddress -c btc --path "m/49'/0'/0'/0/0" --derivationMode segwit
```
