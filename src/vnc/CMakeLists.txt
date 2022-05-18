cmake_minimum_required(VERSION 3.10)

project(VncServer C)

add_compile_options(-Wall)

if (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "aarch64")
    add_compile_options(-DDISABLE_SANDBOX)
endif ()

add_executable(vnc_server vnc_server.c cursor.c)
# add DISABLE_SANDBOX to build vnc_server without seccomp-bpf
target_compile_definitions(vnc_server PRIVATE CUSTOM_CURSOR)
target_link_libraries(vnc_server PRIVATE vncserver)
