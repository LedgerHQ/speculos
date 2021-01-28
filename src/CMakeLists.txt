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
        bolos/emu_bagl.c
        bolos/emu_endorsement.c
        bolos/emu_seproxyhal.c
        bolos/emu_os.c
        bolos/emu_os_1.5.c
        bolos/emu_os_1.6.c
        bolos/emu_os_bip32.c
        bolos/emu_cx.c
        bolos/emu_cx_hash.c
        bolos/emu_cx_math.c
        bolos/emu_default.c
        emulate.c
        emulate_1.2.c
        emulate_1.5.c
        emulate_1.6.c
        emulate_blue_2.2.5.c
        svc.c)

add_dependencies(emu openssl)

add_executable(launcher launcher.c)
target_link_libraries(launcher PRIVATE emu)
