---
sort: 1
---

# Usage

After having [installed the requirements and built](../installation/build.md) speculos:

```shell
./speculos.py apps/btc.elf
```

The docker image can also be used directly, as detailed in the specific [docker documentation page](docker.md).

The Nano S is the default model; the Nano X and Blue can be specified on the
command-line:

```shell
./speculos.py --model nanox apps/nanox#btc#2.0.2#1c8db8da.elf
./speculos.py --model blue --sdk 1.5 apps/blue#btc#1.5#00000000.elf
```

The last SDK version is automatically selected. However, a specific version
be specified if the target app is not build against the last version of the SDK,
thanks to the `-k`/`--sdk` argument. For instance, to launch an app built
against the SDK `1.5` on the Nano S:

```shell
./speculos.py --sdk 1.5 --model nanos apps/btc.elf
```

Supported SDK values for each device are defined in [src/sdk.h](https://github.com/LedgerHQ/speculos/blob/master/src/sdk.h).
You main choose the SDK using `-k`/`--sdk` argument:

|     | Nano S             | Nano X          | Blue            |
|-----|--------------------|-----------------|-----------------|
| SDK | 1.5, 1.6, 2.0, 2.1 | 1.2, 2.0, 2.0.2 | 1.5, blue-2.2.5 |

For more options, pass the `-h` or `--help` flag.

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
$ SPECULOS_APPNAME=blah:1.2.3.4 ./speculos.py ./apps/btc.elf &
$ echo 'b0 01 00 00 00' \
  | LEDGER_PROXY_ADDRESS=127.0.0.1 LEDGER_PROXY_PORT=9999 ledgerctl send - \
  | xxd -r -ps \
  | hd
00000000  01 04 62 6c 61 68 07 31  2e 32 2e 33 2e 34 01 00  |..blah.1.2.3.4..|
00000010  90 00                                             |..|
00000012
```

# Bitcoin Testnet app

Launch the Bitcoin Testnet app, which requires the Bitcoin app:

```shell
./speculos.py ./apps/btc-test.elf -l Bitcoin:./apps/btc.elf
```
