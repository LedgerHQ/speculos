# cmake-toolchain configuration to build with clang on Debian and Ubuntu
#
# Usage:
#
#   cmake -DCMAKE_TOOLCHAIN_FILE=clang-armv7m.cmake -Bbuild -H. && cmake --build build
#
# Documentation: https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET armv7m-linux-gnueabihf)

# Use files installed by libc6-dev-armhf-cross
# (This adds /usr/arm-linux-gnueabihf/include to CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES)
set(CMAKE_SYSROOT /usr/arm-linux-gnueabihf)

# Use LLVM linker to link ARM code, as using the system "ld" fails on Ubuntu with x86-64
add_link_options(-fuse-ld=lld)
