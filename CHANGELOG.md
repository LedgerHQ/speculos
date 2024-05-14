# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.9.1] 2024-05-14

### Fix

- `importlib.resources` does not exists on Python 3.8

## [0.9.0] - 2024-05-06

### Added

- Finger swipe capabilities (this feature is currently only available on Flex, using the capability
  will have no effect on other devices)
- Add support of API_LEVEL_19 for Flex

### Fixed

- Replacing deprecated `pkg_resources` usages with `importlib.resources` equivalents

## [0.8.6] - 2024-04-11

### Fixed

- Fix race condition on screenshot generation
- Fix ticker pause not waiting for end of tick processing

## [0.8.5] - 2024-04-11

### Fixed

- Fix race condition on screenshot generation

## [0.8.4] - 2024-04-10

### Fixed

- Removing PyQt5 from Speculos Docker images as it is unused and triggers error on MAC.
- Fix OCR screen content reset mechanism.
- Fix library mode fonts not unloaded at os_lib_end.

## [0.8.3] - 2024-04-08

### Fixed

- Fix library mode fonts not loaded.

## [0.8.2] - 2024-04-05

### Fixed

- Fix NanoX and NanoSP handling when BAGL is used but fonts are not found in the elf.

## [0.8.1] - 2024-04-04

### Fixed

- Fixed logger initialization that lead to root log handler not being propagated to children

## [0.8.0] - 2024-04-03

### Added

- NBGL support for non-Stax devices
- Add Flex support
- Add support of API_LEVEL_18 for NanoX, NanoS+ and Flex

## [0.7.1] - 2024-02-26

### Fixed

- Specific `cache` mechanism for Python3.8 (`functools.cache` does not exists yet)

## [0.7.0] - 2024-02-26

### Changed
- Significative performance improvement on display/snapshot management
- Simplified HTTP API thread management

## [0.6.0] - 2024-02-21

### Added
- Add support for API_LEVEL_15 for Stax

## [0.5.1] - 2024-02-15

### Added
- Add possibility to set up a timeout for APDU exchange with default value to 5min

## [0.5.0] - 2024-01-11

### Added
- Attestation key or user private keys can now be configured with the new `--attestation-key`
  and `--user-private-key` arguments (or `ATTESTATION_PRIVATE_KEY` and `USER_PRIVATE_KEY` through
  environment variables). User certificates are correctly calculated, signed from the user private
  keys and the attestation key.

### Changed
- Seed, RNG, application name and version are now fetched from the environment when Speculos starts
  then stored internally for further use, rather than fetched when needed during runtime. This
  avoids several Speculos instances from messing up with each other's environment variables.

## [0.4.1] - 2023-12-19

### Fixed
- CX: Fix AES implementation on NanoS

## [0.4.0] - 2023-12-04

### Fixed
- bolos/os_bip32.c: Improve syscall emulation

### Added
- API_LEVEL: Add support for API_LEVEL_14 for Ledger Stax

## [0.3.5] - 2023-11-10

### Fixed
- CX: Update AES implementation to be compatible with API levels >= 12

## [0.3.4] - 2023-11-07

### Features

- Implement cx_bn_gf2_n_mul()

### Miscellaneous Tasks

- Add missing `binutils-arm-none-eabi` package

### README

- Update Limitations section

## [0.3.3] - 2023-10-26

### Fixed
- Launcher: Fix missing RAM relocation emulation on NanoX, NanoSP and Stax

## [0.3.2] - 2023-09-28

### Fixed
- API: the API thread is asked to stop when Speculos exits

## [0.3.1] - 2023-09-28

### Fixed
- OCR: Prevent null dereference when expected font is not in ELF file

## [0.3.0] - 2023-09-11

### Added
- API_LEVEL: Add support for API_LEVEL_13 for corresponding device

## [0.2.10] - 2023-09-01

### Changed
- OCR: Apps using unified SDK don't use OCR anymore. Font info is parsed from .elf file.

## [0.2.9] - 2023-08-31

### Fixed
- Seproxyhal: default status_sent value upon app launch is unset.

## [0.2.8] - 2023-07-31

### Changed
- OCR: Change Stax OCR method. Don't use Tesseract anymore.
- CI: Remove CI job dependency to allow deployment if wanted

### Added
- API_LEVEL: Add support for API_LEVEL_12 for corresponding device

## [0.2.7] - 2023-06-30

### Fixed
- Seproxyhal: Fix SeProxyHal emulation

## [0.2.6] - 2023-06-26

### Fixed
- Seproxyhal: Fix SeProxyHal issue when on LNSP / LNX and HAVE_PRINTF is enabled

## [0.2.5] - 2023-06-21

### Added
- API: Add a ticker/ endpoint to allow control of the ticks sent to the app

### Fixed
- OCR: Fix OCR on NanoX and NanoSP based on API_LEVEL_5 and upper
- Seproxyhal: Fix SeProxyHal emulation to match real devices behavior

## [0.2.4] - 2023-06-13

### Changed
- OCR: Lazy evaluation of screenshot content, performance improvement on Stax

### Fixed
- OCR: screenshot publish is no longer desynchronized with event publish

## [0.2.3] - 2023-06-05

### Fixed
- svc: Fixed emulation of os_lib_call for latest SDK API levels

## [0.2.2] - 2023-06-01

### Changed
- docker: Add blst library to the docker image
- launcher: If Speculos is able to determine the location of the SVC_Call and SVC_cx_call symbols in
            the application elf, it will only try to patch `svc 1` inside the functions.

## [0.2.1] - 2023-05-30

### Fixed
- deployment: re-ordering pypi.org package automatic push in order to avoid incomplete deployments

## [0.2.0] - 2023-05-30

### Changed
- package: Version is not longer customly incremented, but inferred from tag then bundled into the
           package thanks to `setuptools_scm`

## [0.1.265] - 2023-05-12

## [0.1.0] - 2021-07-21

- First release of Speculos on PyPi
