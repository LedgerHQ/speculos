cmake_minimum_required(VERSION 3.10)

project(Speculos C)

include(CTest)
include(ExternalProject)

if (CMAKE_TOOLCHAIN_FILE)
  message(STATUS "Using toolchain ${CMAKE_TOOLCHAIN_FILE}")
else ()
  # By default, use gcc cross-compiler for ARM-Thumb
  set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
  add_compile_options(-mthumb)
  message(STATUS "Using default compiler ${CMAKE_C_COMPILER}")
endif ()

# add_link_options(-static) should be used but was introduced into CMake version 3.13
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

add_compile_options(-W -Wall -fPIC)
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
  message(STATUS "Using OpenSSL and cmocka from ${PRECOMPILED_DEPENDENCIES_DIR}")
  set(INSTALL_DIR ${PRECOMPILED_DEPENDENCIES_DIR})
  add_library(openssl STATIC IMPORTED)
else()
  message(STATUS "Building OpenSSL and cmocka...")
  set(INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/install)

  set(OPENSSL_CFLAGS "${CMAKE_C_FLAGS}")
  get_directory_property(compile_options COMPILE_OPTIONS)
  foreach (opt ${compile_options})
    string(APPEND OPENSSL_CFLAGS " ${opt}")
  endforeach ()
  if (CMAKE_C_COMPILER_TARGET)
    string(APPEND OPENSSL_CFLAGS " --target=${CMAKE_C_COMPILER_TARGET}")
  endif ()
  if (CMAKE_SYSROOT)
    string(APPEND OPENSSL_CFLAGS " -isystem${CMAKE_SYSROOT}/include")
  endif ()
  string(APPEND OPENSSL_CFLAGS " -Wno-unused-parameter -Wno-missing-field-initializers")

  ExternalProject_Add(
    openssl
    URL https://www.openssl.org/source/openssl-1.1.1k.tar.gz
    URL_HASH SHA256=892a0875b9872acd04a9fde79b1f943075d5ea162415de3047c327df33fbaee5
    CONFIGURE_COMMAND ./Configure "CC=${CMAKE_C_COMPILER}" "CFLAGS=${OPENSSL_CFLAGS}" no-afalgeng no-aria no-asan no-asm no-async no-autoalginit no-autoerrinit no-autoload-config no-bf no-buildtest-c++ no-camellia no-capieng no-cast no-chacha no-cmac no-cms no-comp no-crypto-mdebug no-crypto-mdebug-backtrace no-ct no-deprecated no-des no-devcryptoeng no-dgram no-dh no-dsa no-dso no-dtls no-ecdh no-egd no-engine no-err no-external-tests no-filenames no-fuzz-afl no-fuzz-libfuzzer no-gost no-heartbeats no-hw no-idea no-makedepend no-md2 no-md4 no-mdc2 no-msan no-multiblock no-nextprotoneg no-ocb no-ocsp no-pinshared no-poly1305 no-posix-io no-psk no-rc2 no-rc4 no-rc5 no-rdrand no-rfc3779 no-scrypt no-sctp no-seed no-shared no-siphash no-sm2 no-sm3 no-sm4 no-sock no-srp no-srtp no-sse2 no-ssl no-ssl3-method no-ssl-trace no-stdio no-tests no-threads no-tls no-ts no-ubsan no-ui-console no-unit-test no-whirlpool no-zlib no-zlib-dynamic linux-armv4 --prefix=${INSTALL_DIR}
    BUILD_COMMAND make
    INSTALL_COMMAND make install_sw
    BUILD_IN_SOURCE 1
  )

  ExternalProject_Add(
    blst
    URL https://github.com/supranational/blst/archive/d0bc304a132df43856d8302e15dabee97d3d8a95.tar.gz
    URL_HASH SHA256=70127766f8031cde3df4224d88f7b33dec6c33fc7ac6b8e4308d4f7d0bdffd7b
    CONFIGURE_COMMAND ""
    BUILD_COMMAND sh build.sh CC=arm-linux-gnueabihf-gcc
    INSTALL_COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/blst-prefix/src/blst/libblst.a ${INSTALL_DIR}/lib/
    COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/blst-prefix/src/blst/bindings/blst.h ${INSTALL_DIR}/include/
    COMMAND cp ${CMAKE_CURRENT_BINARY_DIR}/blst-prefix/src/blst/bindings/blst_aux.h ${INSTALL_DIR}/include/
    BUILD_IN_SOURCE 1
  )

  if (BUILD_TESTING)
    set(CMOCKA_CMAKE_ARGS
      -DWITH_STATIC_LIB=true
      "-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}"
      "-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
      "-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}"
    )
    if (CMAKE_TOOLCHAIN_FILE)
      get_filename_component(abs_cmake_toolchain_file "${CMAKE_TOOLCHAIN_FILE}" ABSOLUTE)
      list(APPEND CMOCKA_CMAKE_ARGS "-DCMAKE_TOOLCHAIN_FILE=${abs_cmake_toolchain_file}")
    endif ()
    ExternalProject_Add(cmocka
      PREFIX ${CMAKE_CURRENT_BINARY_DIR}/cmocka
      URL https://cmocka.org/files/1.1/cmocka-1.1.5.tar.xz
      URL_HASH SHA256=f0ccd8242d55e2fd74b16ba518359151f6f8383ff8aef4976e48393f77bba8b6
      CMAKE_ARGS ${CMOCKA_CMAKE_ARGS}
    )
  endif()
endif()

if (BUILD_TESTING)
  add_library(cmocka-static STATIC SHARED IMPORTED)
  add_dependencies(cmocka-static cmocka)
endif()

include_directories(${INSTALL_DIR}/include)
link_directories(${INSTALL_DIR}/lib)

link_libraries(ssl crypto dl blst)

add_subdirectory(src)

if (BUILD_TESTING)
  add_subdirectory(tests/c/)
endif()

add_custom_target(
  copy-launcher ALL
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_BINARY_DIR}/src/launcher
    ${CMAKE_CURRENT_SOURCE_DIR}/speculos/resources/launcher
  DEPENDS src/launcher
  COMMENT Copy the launcher in the Python part of Speculos
  VERBATIM)

if (WITH_VNC)
  externalproject_add(vnc_server
    SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/vnc"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/vnc"
    INSTALL_COMMAND ""
  )

  add_custom_target(
    copy-vnc-server ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${CMAKE_CURRENT_BINARY_DIR}/vnc/vnc_server
      ${CMAKE_CURRENT_SOURCE_DIR}/speculos/resources/vnc_server
    DEPENDS vnc_server
    COMMENT Copy the launcher in the Python part of Speculos
    VERBATIM)
endif()
