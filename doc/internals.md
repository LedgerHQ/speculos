# Internals

The emulator is actually a userland application cross-compiled for the ARM
architecture. It opens the target app (`bin.elf`) from the filesystem and maps
it as is in memory. The emulator is launched with `qemu-arm-static` and
eventually jumps to the app entrypoint.

The `svc` instruction is replaced with `udf` (undefined) to generate a `SIGILL`
signal upon execution. It allows to catch syscalls and emulate them.

Apps can be debugged with `gdb-multiarch` thanks to `qemu-arm-static`.
