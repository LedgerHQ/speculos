cmake_minimum_required(VERSION 3.10)

project(Speculos)

include(ExternalProject)

set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_EXE_LINKER_FLAGS -static)

enable_testing()

option(WITH_VNC "Support for VNC" OFF)

add_compile_options(-mthumb -W -Wall -fPIC)
add_definitions(-DOS_LITTLE_ENDIAN -DNATIVE_64BITS)

include_directories(sdk src openssl/include)
link_directories(openssl)

link_libraries(ssl crypto dl)

add_subdirectory(src)
add_subdirectory(tests)

if (WITH_VNC)
    externalproject_add(vnc_server
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vnc"
        BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/vnc"
        INSTALL_COMMAND ""
    )
endif()
