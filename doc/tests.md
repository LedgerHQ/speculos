## How to run tests

The application binaries launched by the tests should be placed in `apps/`.
Every type of app tested should have a dedicated test file that can be launched like this:

```console
pytest-3 -s -v tests/test_btc.py
```

Crypto syscalls are tested using the following command:

```console
make -C build/ test
```
