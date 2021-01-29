add_library(emu
        bolos/cx_aes.c
        bolos/cx_blake2b.c
        bolos/cx_curve25519.c
        bolos/cx_crc.c
        bolos/cx_ec.c
        bolos/cx_ed25519.c
        bolos/cx_hmac.c
        bolos/cx_ripemd160.c
        bolos/cx_rng_rfc6979.c
        bolos/cx_scc.c
        bolos/cx_sha256.c
        bolos/cx_sha3.c
        bolos/cx_sha512.c
        bolos/cx_utils.c
        bolos/bagl.c
        bolos/endorsement.c
        bolos/seproxyhal.c
        bolos/os.c
        bolos/os_1.5.c
        bolos/os_1.6.c
        bolos/os_bip32.c
        bolos/cx.c
        bolos/cx_hash.c
        bolos/cx_math.c
        bolos/default.c
        emulate.c
        emulate_1.2.c
        emulate_1.5.c
        emulate_1.6.c
        emulate_blue_2.2.5.c
        svc.c)

add_dependencies(emu openssl)

add_executable(launcher launcher.c)
target_link_libraries(launcher PRIVATE emu)
