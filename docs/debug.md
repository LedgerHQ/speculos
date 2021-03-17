# Debug

Debug an app thanks to gdb:

```console
./speculos.py -d apps/btc.elf &
./tools/debug.sh apps/btc.elf
```

Some useful tricks:

- Use the `-t` (`--trace`) argument to trace every syscalls.
- [Semihosting](semihosting.md) features can be used as an additional debug mechanism.
