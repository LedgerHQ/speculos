add_definitions(-DST31)
link_libraries(emu -lcmocka-static)

add_executable(test_environment test_environment.c utils.c)

add_test(NAME test_environment COMMAND qemu-arm-static test_environment WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(syscalls)
