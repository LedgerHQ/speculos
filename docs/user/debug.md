---
sort: 4
---

# Debug: how to use GDB

Debug an app thanks to GDB:

```shell
./speculos.py -d apps/btc.elf &
./tools/debug.sh apps/btc.elf
```

Some useful tricks:

- Use the `-t` (`--trace`) argument to trace every syscalls.
- [Semihosting](semihosting.md) features can be used as an additional debug mechanism.
