cmake_minimum_required(VERSION 3.10)

project(Speculos C)

include(ExternalProject)

set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_EXE_LINKER_FLAGS -static)

enable_testing()

option(WITH_VNC "Support for VNC" OFF)

# Set GIT_REVISION to the last commit hash.
# Please note that the variable is set at configuration time and might be
# outdated.
find_package(Git)
execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --always HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_REVISION
    OUTPUT_STRIP_TRAILING_WHITESPACE)

add_compile_options(-mthumb -W -Wall -fPIC)
add_definitions(-DOS_LITTLE_ENDIAN -DNATIVE_64BITS -DGIT_REVISION=\"${GIT_REVISION}\")

option(
  CODE_COVERAGE
  "Builds targets with code coverage instrumentation. (Requires GCC or Clang)"
  OFF
)
if (CODE_COVERAGE)
  # Always disable optimisations and build with debug symbols, when building for code coverage
  add_compile_options(-O0 -g)
  add_link_options(-g)
  if (CMAKE_C_COMPILER_ID MATCHES "(Apple)?[Cc]lang")
    # Options for clang
    message(STATUS "Building with clang code coverage...")
    add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
    add_link_options(-fprofile-instr-generate -fcoverage-mapping)
  elseif(CMAKE_C_COMPILER_ID MATCHES "GNU")
    # Options for gcc
    message(STATUS "Building with gcc code coverage...")
    add_compile_options(--coverage -fprofile-arcs -ftest-coverage)
    add_link_options(--coverage -fprofile-arcs -ftest-coverage)
  else()
    message(FATAL_ERROR "Unable to identify the compiler! Aborting...")
  endif()
endif()

include_directories(sdk src)

if (PRECOMPILED_DEPENDENCIES_DIR)
  message(STATUS "Using OpenSSL and cmoka from ${PRECOMPILED_DEPENDENCIES_DIR}")
  set(INSTALL_DIR ${PRECOMPILED_DEPENDENCIES_DIR})
  add_library(openssl STATIC IMPORTED)
else()
  message(STATUS "Building OpenSSL and cmoka...")
  set(INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/install)

  ExternalProject_Add(
    openssl
    URL https://www.openssl.org/source/openssl-1.1.1f.tar.gz
    URL_HASH SHA256=186c6bfe6ecfba7a5b48c47f8a1673d0f3b0e5ba2e25602dd23b629975da3f35
    CONFIGURE_COMMAND ./Configure --cross-compile-prefix=arm-linux-gnueabihf- no-afalgeng no-aria no-asan no-asm no-async no-autoalginit no-autoerrinit no-autoload-config no-bf no-buildtest-c++ no-camellia no-capieng no-cast no-chacha no-cmac no-cms no-comp no-crypto-mdebug no-crypto-mdebug-backtrace no-ct no-deprecated no-des no-devcryptoeng no-dgram no-dh no-dsa no-dso no-dtls no-ec2m no-ecdh no-egd no-engine no-err no-external-tests no-filenames no-fuzz-afl no-fuzz-libfuzzer no-gost no-heartbeats no-hw no-idea no-makedepend no-md2 no-md4 no-mdc2 no-msan no-multiblock no-nextprotoneg no-ocb no-ocsp no-pinshared no-poly1305 no-posix-io no-psk no-rc2 no-rc4 no-rc5 no-rdrand no-rfc3779 no-scrypt no-sctp no-seed no-shared no-siphash no-sm2 no-sm3 no-sm4 no-sock no-srp no-srtp no-sse2 no-ssl no-ssl3-method no-ssl-trace no-stdio no-tests no-threads no-tls no-ts no-ubsan no-ui-console no-unit-test no-whirlpool no-zlib no-zlib-dynamic linux-armv4 --prefix=${INSTALL_DIR}
    BUILD_COMMAND make CFLAGS=-mthumb
    INSTALL_COMMAND make install_sw
    BUILD_IN_SOURCE 1
  )

  ExternalProject_Add(cmocka
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/cmocka
    URL https://cmocka.org/files/1.1/cmocka-1.1.5.tar.xz
    URL_HASH SHA256=f0ccd8242d55e2fd74b16ba518359151f6f8383ff8aef4976e48393f77bba8b6
    CMAKE_ARGS += -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_C_FLAGS=-mthumb -DWITH_STATIC_LIB=true -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
  )
endif()

add_library(cmocka-static STATIC SHARED IMPORTED)
add_dependencies(cmocka-static cmocka)

include_directories(${INSTALL_DIR}/include)
link_directories(${INSTALL_DIR}/lib)

link_libraries(ssl crypto dl)

add_subdirectory(src)
add_subdirectory(tests/syscalls)

if (WITH_VNC)
    externalproject_add(vnc_server
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/vnc"
        BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/vnc"
        INSTALL_COMMAND ""
    )
endif()
