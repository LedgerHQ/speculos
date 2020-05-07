# Speculos

![screenshot btc nano s](doc/screenshot-btc-nanos.png)

The goal of this project is to emulate Ledger Nano and Ledger Blue apps on
standard desktop computers, without any hardware device. More information can
be found here:

- [build](doc/build.md)
- [usage](doc/usage.md)
- [internals](doc/internals.md)
- [tests](doc/tests.md)

Usage example: `./speculos.py apps/btc.elf`.


## Bugs and contributions

Feel free to open issues and create pull requests on this GitHub repository.

Please ensure that tests still pass.


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
[semihosting](doc/semihosting.md) features), thus don't run Speculos on
untrusted apps.

It's worth noting that the syscall implementation (`src/`) doesn't expect
malicious input. By the way, in Speculos, there is no privilege separation
between the app and the syscalls. This doesn't reflect the security of the
firmware on hardware devices where app and OS isolation is enforced.

Speculos is not part of Ledger bug bounty program.
