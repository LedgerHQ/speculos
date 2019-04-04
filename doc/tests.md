## How to run tests

The following tests ensures that apps in the `apps/` are successfuly
launched:

```console
pytest-3 -s -v tests/test_apps.py
```

Crypto syscalls are tested using the following command:

```console
make -C build/ test
```
