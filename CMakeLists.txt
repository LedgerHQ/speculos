cmake_minimum_required(VERSION 3.10)

project(Speculos C)

include(ExternalProject)

set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_EXE_LINKER_FLAGS -static)

enable_testing()

option(WITH_VNC "Support for VNC" OFF)

add_compile_options(-mthumb -W -Wall -fPIC)
add_definitions(-DOS_LITTLE_ENDIAN -DNATIVE_64BITS)

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
    CONFIGURE_COMMAND ./Configure --cross-compile-prefix=arm-linux-gnueabihf- no-asm no-threads no-shared no-sock linux-armv4 --prefix=${INSTALL_DIR}
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
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vnc"
        BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/vnc"
        INSTALL_COMMAND ""
    )
endif()
