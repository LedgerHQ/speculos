The files in these folder are consumed by the tests to ensure that speculos
works correctly across various devices and apps.

DO NOT use them to test apps or your integrations, as these  binaries are old
and unmantained. Rather, follow the instructions in the app's repository
in order to build the most recent version.

### Naming

Examples:

- `nanosp#btc#1.5#5b6693b8.elf`

If the examples aren't enough, here is the naming convention that should be
used:

```
device model # app name # SDK version # git revision .elf
```

where:

- `device model` is one of the following: `nanosp`, `stax`
- `git revision` is the short hash (8 bytes) of the last commit
