---
sort: 2
---

# Tests

## How to run tests

The application binaries launched by the tests should be placed in `apps/`.
Every type of app tested should have a dedicated test file that can be launched like this:

```shell
python3 -m pytest -s -v tests/apps/
```

Crypto syscalls are tested using the following command:

```shell
make -C build/ test
```

Arguments can be given to `ctest`. For instance, to make the output of a
specific test verbose:

```shell
make -C build/ test ARGS='-V -R test_bip32'
```

## Code coverage

In order to build with code coverage instrumentation, the CMake configuration supports `CODE_COVERAGE` macro:
```shell
cmake -Bbuild -H. -DCODE_COVERAGE=ON
make -C build/ clean
make -C build/ all
```

When using gcc to build the project (which is by default), this enables instrumentation with gcov: these commands created `.gcno` files in `build/`.

[lcov](http://ltp.sourceforge.net/coverage/lcov.php) can be used to collect code coverage and generates a HTML report in `coverage/`:
```shell
lcov -d . --zerocounters
lcov -d . --capture --initial -o coverage.base
RNG_SEED=0 make -C build/ test
lcov -d . --rc lcov_branch_coverage=1 --capture -o coverage.capture
lcov -d . --add-tracefile coverage.base --add-tracefile coverage.capture -o coverage.total
genhtml coverage.total -o coverage
```
