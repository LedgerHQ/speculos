---
sort: 4
---

# Internals

The emulator is actually a userland application cross-compiled for the ARM
architecture. It opens the target app (`app.elf`) from the filesystem and maps
it as is in memory. The emulator is launched with `qemu-arm-static` and
eventually jumps to the app entrypoint.

Apps can be debugged with `gdb-multiarch` thanks to `qemu-arm-static`.


## Syscall hooks

The `svc` instruction is replaced with `udf` (undefined) to generate a `SIGILL`
signal upon execution. It allows to catch syscalls and emulate them. It can
unfortunately lead to unexpected bytes being patched if `\x01\xdf` is found in
the binary (and isn't the `svc` instruction).

A disassembler could give better results, but it doesn't look worth it. As a
side note, the `SVC_Call()` function can't be hooked because some syscalls are
inlined.


Other alternatives were considered (for instance `seccomp` or `ptrace`) but they
seem not practicable because QEMU don't support the
[associated syscalls](https://github.com/qemu/qemu/blob/1c4c6fcd1a2ae56e98be3a441e19a0933c508a51/linux-user/syscall.c#L7402).
