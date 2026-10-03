---
sort: 1
---

# Usage

> New to Speculos? Start with the [Quickstart](quickstart.md), which covers
> installation and running an app on every platform.

After having [installed the requirements and built](../installation/build.md) speculos
(or `pip install speculos`), run an app. With applications built by recent SDKs,
Speculos **automatically detects** the targeted device:

```shell
./speculos.py path/to/app.elf
```

> A demo app, `apps/boil.elf`, is bundled in the source tree if you just want to
> try Speculos without building or fetching an app:
> `./speculos.py apps/boil.elf`.

The docker image can also be used directly, as detailed in the specific [docker documentation page](docker.md).

For all options, pass the `-h` or `--help` flag.

## Keyboard control

- The keyboard left and right arrow keys are used instead of the Nano buttons.
  The down arrow can also be used as a more convenient shortcut.
- The `Q` key exits the application.

## Display

Several display options are available through the `--display` parameter:

- `qt`: default, requires a X server
- `headless`: nothing is displayed
- `text`: the UI is displayed in the console (handy on Windows)

These options can be used along `--vnc-port` which spawns a VNC server on the
specified port. macOS users should also add `--vnc-password <password>` if using
the built-in VNC client because unauthenticated sessions doesn't seem to be
supported (issue #34).

A recording of the screen can be saved as a GIF file thanks to the
`tools/gif-recorder.py` script.

## App name and version

On a real device, some parameters specific to the app to be installed (name and
version, icon, allowed derivation paths, etc.) are given during the
installation. This information isn't embedded in the .elf file itself and thus
cannot be retrieved by speculos.

The default app name and version are respectively `app` `1.33.7`, but these
values can be set through the `SPECULOS_APPNAME` environment variable. For
instance:

```shell
$ SPECULOS_APPNAME=blah:1.2.3.4 ./speculos.py ./apps/boil.elf &
$ echo 'b0 01 00 00 00' \
  | LEDGER_PROXY_ADDRESS=127.0.0.1 LEDGER_PROXY_PORT=9999 ledgerctl send - \
  | xxd -r -ps \
  | hd
00000000  01 04 62 6c 61 68 07 31  2e 32 2e 33 2e 34 01 00  |..blah.1.2.3.4..|
00000010  90 00                                             |..|
00000012
```

## Device PIN

An app holding `APPLICATION_FLAG_GLOBAL_PIN` can have the user enter the device
PIN (`os_global_pin_check`) and read its remaining tries
(`os_global_pin_retries`). The emulated device has the PIN `1234`, which
`--pin` changes (4 to 8 digits), and three tries: a wrong entry spends one,
down to zero, and a correct entry restores them.

```shell
./speculos.py --pin 5678 path/to/app.elf
```

## Loading a library app (example: Bitcoin Testnet)

Some apps call into another app at runtime (`os_lib_call`). Pass the dependency
with `-l name:path` (the name is optional if present in the ELF metadata). For
example, the Bitcoin Testnet app depends on the Bitcoin app:

```shell
./speculos.py ./apps/btc-test.elf -l Bitcoin:./apps/btc.elf
```

> `btc-test.elf` and `btc.elf` are **not bundled** with Speculos; build them from
> [`app-bitcoin-new`](https://github.com/LedgerHQ/app-bitcoin-new) (see
> [Get an app to run](getting_an_app.md)). The `-l` mechanism itself works with
> any app and its library dependency.

## OCR

OCR is available for Nano X, Nano S+, Flex, Stax and Apex+ with built-in character recognition.
