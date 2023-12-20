add_definitions(-DST31)
link_libraries(emu -lcmocka-static)


add_subdirectory(syscalls)
