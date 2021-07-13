# cmake-toolchain configuration to build with gcc on Debian and Ubuntu
#
# Usage:
#
#   cmake -DCMAKE_TOOLCHAIN_FILE=gcc-arm.cmake -Bbuild -H. && cmake --build build
#
# Documentation: https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)

set(CMAKE_C_FLAGS "-mthumb" CACHE STRING "" FORCE)
