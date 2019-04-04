add_definitions(-DST31)

add_library(cmocka cmocka/src/cmocka.c)
target_include_directories(cmocka INTERFACE cmocka/include)

target_compile_definitions(cmocka PRIVATE HAVE_MALLOC_H HAVE_INTTYPES_H HAVE_SIGNAL_H HAVE_STRINGS_H)
target_include_directories(cmocka PRIVATE cmocka/include)

add_executable(hello hello.c)

add_executable(test_aes test_aes.c nist_cavp.c utils.c)
target_link_libraries(test_aes emu cmocka)

add_executable(test_ecdh test_ecdh.c utils.c)
target_link_libraries(test_ecdh emu cmocka)

add_executable(test_hmac test_hmac.c utils.c)
target_link_libraries(test_hmac emu cmocka)

add_executable(test_ripemd test_ripemd.c utils.c)
target_link_libraries(test_ripemd emu cmocka)

add_executable(test_sha2 test_sha2.c nist_cavp.c utils.c)
target_link_libraries(test_sha2 emu cmocka)

add_executable(test_crc16 test_crc16.c)
target_link_libraries(test_crc16 emu cmocka)

foreach(target hello test_aes test_ecdh test_hmac test_ripemd test_sha2 test_crc16)
    add_test(NAME ${target} COMMAND qemu-arm-static ${target} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()