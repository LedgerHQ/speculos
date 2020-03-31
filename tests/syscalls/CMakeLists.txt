add_definitions(-DST31)

link_libraries(emu -lcmocka-static)

add_executable(hello hello.c)

add_executable(test_aes test_aes.c nist_cavp.c utils.c)
add_executable(test_blake2 test_blake2.c nist_cavp.c utils.c)
add_executable(test_ecdh test_ecdh.c utils.c)
add_executable(test_eddsa test_eddsa.c utils.c)
add_executable(test_ecdsa test_ecdsa.c utils.c)
add_executable(test_hmac test_hmac.c utils.c)
add_executable(test_ripemd test_ripemd.c utils.c)
add_executable(test_sha2 test_sha2.c nist_cavp.c utils.c)
add_executable(test_sha3 test_sha3.c nist_cavp.c utils.c)
add_executable(test_crc16 test_crc16.c)
add_executable(test_slip21 test_slip21.c)
add_executable(test_endorsement test_endorsement.c)
add_executable(test_bip32 test_bip32.c utils.c)

foreach(target hello test_aes test_blake2 test_ecdh test_eddsa test_ecdsa test_hmac test_ripemd test_sha2 test_sha3 test_crc16 test_slip21 test_endorsement test_bip32)
    add_test(NAME ${target} COMMAND qemu-arm-static ${target} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endforeach()
