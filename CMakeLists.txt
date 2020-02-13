cmake_minimum_required(VERSION 3.10)

project(Speculos C)

include(ExternalProject)

# set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
# set(CMAKE_EXE_LINKER_FLAGS -static)

enable_testing()

option(WITH_VNC "Support for VNC" OFF)

# add_compile_options(-mthumb -W -Wall -fPIC -march=armv6t2+vfpv2)
add_definitions(-DOS_LITTLE_ENDIAN -DNATIVE_64BITS)

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

find_library(openssl REQUIRED)

include_directories(sdk src)

if (PRECOMPILED_DEPENDENCIES_DIR)
  message(STATUS "Using OpenSSL and cmoka from ${PRECOMPILED_DEPENDENCIES_DIR}")
  set(INSTALL_DIR ${PRECOMPILED_DEPENDENCIES_DIR})
  add_library(openssl STATIC IMPORTED)
else()
  message(STATUS "Building OpenSSL and cmoka...")
  set(INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/install)
endif()

add_library(cmocka UNKNOWN IMPORTED)

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
