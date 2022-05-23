# Speculos

[![codecov](https://codecov.io/gh/LedgerHQ/speculos/branch/master/graph/badge.svg)](https://codecov.io/gh/LedgerHQ/speculos)
[![lgtm](https://img.shields.io/lgtm/alerts/g/LedgerHQ/speculos.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/LedgerHQ/speculos/alerts/)

![screenshot btc nano s](docs/screenshot-api-nanos-btc.png)

The goal of this project is to emulate Ledger Nano S, Nano X and Blue apps on
standard desktop computers, without any hardware device. More information can
be found here in the
[documentation website](https://ledgerhq.github.io/speculos) (or in the
[docs/ folder](docs/) directly).

Usage example:

```shell
./speculos.py apps/btc.elf
# ... and open a browser on http://127.0.0.1:5000
```

## Bugs and contributions

Feel free to open issues and create pull requests on this GitHub repository.

The `master` branch is protected to disable force pushing. Contributions should
be made through pull requests, which are reviewed by @LedgerHQ members before
being merged to `master`:

- @LedgerHQ members can create branches directly on the repository (if member of
  a team with write access to the repository)
- External contributors should fork the repository


## Limitations

The emulator handles only a few syscalls made by common apps; for instance,
syscalls related to app install, firmware update or OS info can't be
implemented.

There is absolutely no guarantee that apps will have the same behavior on
hardware devices and Speculos:

- Invalid syscall parameters might throw an exception on a real device while
  being ignored on Speculos.
- Attempts to perform unaligned accesses when not allowed (eg. dereferencing a
  misaligned pointer) will cause an alignment fault on a hardware device.


## Security

Apps can make arbitrary Linux system calls (and use QEMU
[semihosting](docs/user/semihosting.md) features), thus don't run Speculos on
untrusted apps.

It's worth noting that the syscall implementation (`src/`) doesn't expect
malicious input. By the way, in Speculos, there is no privilege separation
between the app and the syscalls. This doesn't reflect the security of the
firmware on hardware devices where app and OS isolation is enforced.

Speculos is not part of Ledger bug bounty program.


## Are you developing a Nano App as an external developer?

For a smooth and quick integration:
- Follow the developers documentation on the [Developer Portal](https://developers.ledger.com/docs/nano-app/introduction/) and 
- Go on Discord to chat with developer support and the developer community. See you there! If you are new to Ledger OP3N Discord server [click here](), otherwise directly join [the Nano App channel](https://discord.com/channels/885256081289379850/907623554542080070).
