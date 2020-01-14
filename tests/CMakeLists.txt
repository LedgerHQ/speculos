add_definitions(-DST31)

link_libraries(emu -lcmocka-static)

add_executable(hello hello.c)

add_executable(test_aes test_aes.c nist_cavp.c utils.c)
add_executable(test_ecdh test_ecdh.c utils.c)
add_executable(test_eddsa test_eddsa.c utils.c)
add_executable(test_hmac test_hmac.c utils.c)
add_executable(test_ripemd test_ripemd.c utils.c)
add_executable(test_sha2 test_sha2.c nist_cavp.c utils.c)
add_executable(test_sha3 test_sha3.c nist_cavp.c utils.c)
add_executable(test_crc16 test_crc16.c)

foreach(target hello test_aes test_ecdh test_eddsa test_hmac test_ripemd test_sha2 test_sha3 test_crc16)
    add_test(NAME ${target} COMMAND qemu-arm-static ${target} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()