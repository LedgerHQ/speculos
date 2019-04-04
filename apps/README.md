The files in these folder are consumed by the tests to ensure that speculos
works correctly across various devices and apps.


### Naming

Examples:

- `nanos#btc#1.5#5b6693b8.elf`
- `blue#vault#blue-2.2.5#ca89b330.elf`

If the examples aren't enough, here is the naming convention that should be
used:

```
device model # app name # SDK version # git revision .elf
```

where:

- `device model` is one of the following: `nanos`, `blue`
- `git revision` is the short hash (8 bytes) of the last commit
