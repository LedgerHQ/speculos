# Speculos

[![codecov](https://codecov.io/gh/LedgerHQ/speculos/branch/master/graph/badge.svg)](https://codecov.io/gh/LedgerHQ/speculos)
[![lgtm](https://img.shields.io/lgtm/alerts/g/LedgerHQ/speculos.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/LedgerHQ/speculos/alerts/)

![screenshot btc nano s](https://raw.githubusercontent.com/LedgerHQ/speculos/master/docs/screenshot-api-nanos-btc.png)

The goal of this project is to emulate Ledger Nano S/S+, Nano X, Blue, Flex and Stax apps on
standard desktop computers, without any hardware device. More information can
be found here in the
[documentation website](https://ledgerhq.github.io/speculos) (or in the
[docs/ folder](docs/) directly).

Usage example:

```shell
./speculos.py apps/btc.elf --model nanos
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

There is absolutely no guarantee that apps will have the same behavior on
hardware devices and Speculos, though the differences are limited.

### Syscalls

The emulator handles only a few syscalls made by common apps. For instance,
syscalls related to app install, firmware update or OS info can't be
implemented.

Invalid syscall parameters might throw an exception on a real device while
being ignored on Speculos.
Notably, this is the case for application allowed derivation path and curve and
application settings flags which are enforced by the device OS, but ignored by
Speculos.

### Memory alignment

Attempts to perform unaligned accesses when not allowed (eg. dereferencing a
misaligned pointer) will cause an alignment fault on a Ledger Nano S device but
not on Speculos. Note that such unaligned accesses are supported by other
Ledger devices.

Following code crashes on LNS device, but not on Speculos nor on other devices.
```
uint8_t buffer[20];
for (int i = 0; i < 20; i++) {
    buffer[i] = i;
}
uint32_t display_value = *((uint32_t*) (buffer + 1));
PRINTF("display_value: %d\n", display_value);
```

### Watchdog

NanoX, Flex and Stax devices use an internal watchdog enforcing usage of regular
calls to `io_seproxyhal_io_heartbeat();`. This watchdog is not emulated on
Speculos.


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
- [Go on Discord](https://developers.ledger.com/discord-pro/) to chat with developer support and the developer community.
