cmake_minimum_required(VERSION 3.10)

project(VncServer)

add_compile_options(-Wall)

add_executable(vnc_server vnc_server.c cursor.c)
target_compile_definitions(vnc_server PRIVATE CUSTOM_CURSOR)
target_link_libraries(vnc_server PRIVATE vncserver)
