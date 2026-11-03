---
sort: 8
---

# Troubleshooting

Common errors when getting started, and how to fix them.

## Install & launch

**`speculos: error: argument -m/--model: invalid choice: 'nanos'`**
The original Nano S is no longer supported. Valid models are `nanox`, `nanosp`
(Nano S+), `stax`, `flex` and `apex_p`. In most cases you can drop `--model`
entirely — recent apps are auto-detected.

**`Invalid api_level 0` when launching a master build**
This is *not* a version mismatch. An API level of `0` is a reserved value meaning
"built against the SDK master branch". Speculos refuses to guess which API level
to emulate for such a binary, so you must pass one explicitly with `-a`/`--apiLevel`
(or `-k/--sdk`). An app built against a **released** SDK — including the bundled
`apps/boil.elf` — embeds a real, non-zero API level and is auto-detected.

**`Invalid api_level in <path> (X vs Y)`**
You passed `-a Y` (or `-k`) but the app's embedded API level is `X`. Drop the flag
to let it auto-detect, or pass the value that matches the app. The binaries under
`apps/` are intentionally old; build the current app instead (see
[Get an app to run](getting_an_app.md)).

**`No such file or directory: 'apps/btc.elf'`**
The repository no longer ships `btc.elf`. Use the bundled `apps/boil.elf`, or
your own `.elf` (see [Get an app to run](getting_an_app.md)).

**`pip install speculos` fails to build on macOS**
Use the Docker image instead — it bundles the toolchain and all dependencies. See
[macOS (Apple Silicon)](macos.md).

## Display

**Qt window does not open / `qt.qpa.plugin: Could not load the Qt platform plugin`**
Qt needs a graphical session. On a headless server or container, use
`--display headless` and the web UI (port 5000) or `--vnc-port` instead.

**Nothing shows up on Windows (WSL 2)**
Qt requires an X server exported from Windows. Either follow the
[WSL 2 guide](../installation/wsl.md) to set up `DISPLAY` + VcXSrv, or use
`--display text` / the web UI, which need no X server.

**macOS Screen Sharing won't connect to the VNC port**
macOS' built-in client requires authentication. Add `--vnc-password <pwd>` to
the Speculos command.

## Docker

**`docker run` starts but I can't reach the web UI / API**
Publish the port *and* tell Speculos to listen on it:
`docker run ... -p 5000:5000 speculos --api-port 5000 ...`. A published port that
the app isn't listening on does nothing. The official image sets
`SPECULOS_HOST=0.0.0.0` so published ports stay reachable; if you override the
entrypoint or run Speculos on the host, pass `--host 0.0.0.0` when you need
LAN/Docker access (the default bind is `127.0.0.1`).

**`exec format error` / wrong architecture on Apple Silicon**
The official image is multi-arch and runs natively on `arm64`; pull the latest
tag (`docker pull ghcr.io/ledgerhq/speculos:latest`). You no longer need to edit
the Dockerfile or use an `-aarch64` builder — if you read that somewhere, the doc
is outdated.

**The app isn't found inside the container**
Mount your apps folder and reference it with the in-container path:
`-v "$(pwd)"/apps:/speculos/apps` then `apps/boil.elf`.

## Talking to the device

**`Connection refused` on port 9999 or 5000**
Speculos isn't running, or you didn't expose that port. Start it with
`--api-port 5000 --apdu-port 9999` (and `-p` those ports if using Docker). Check
the process is up before connecting.

**My APDU gets no response / the app seems stuck on a screen**
The app is probably waiting for a button press or touch. Read the current screen
with `GET /events?currentscreenonly=true`, then press the right button
(`POST /button/right`) or touch the screen (`POST /finger`). See
[Driving Speculos from code or an agent](agent.md).

**Addresses / signatures differ from a real device**
Speculos uses a default test seed. Set your own with `--seed "<mnemonic>"` (or
`--seed hex:<hex>`) to get deterministic, reproducible keys.

## Still stuck?

Open an issue on [GitHub](https://github.com/LedgerHQ/speculos/issues) with your
exact command line, OS, and the full error output.
