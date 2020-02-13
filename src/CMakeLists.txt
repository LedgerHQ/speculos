add_library(emu
        cx_aes.c
        cx_blake2b.c
        cx_crc.c
        cx_ec.c
        cx_ed25519.c
        cx_hmac.c
        cx_ripemd160.c
        cx_rng_rfc6979.c
        cx_scc.c
        cx_sha256.c
        cx_sha3.c
        cx_sha512.c
        cx_utils.c
        emulate.c
        emulate_1.5.c
        emulate_1.6.c
        emulate_blue_2.2.5.c
        emu_cx.c
        emu_cx_hash.c
        emu_cx_math.c
        emu_default.c
        emu_endorsement.c
        emu_os.c
        emu_os_1.5.c
        emu_os_1.6.c
        emu_os_bip32.c
        emu_seproxyhal.c)

target_link_libraries(emu ssl crypto)

add_executable(launcher launcher.c)
target_link_libraries(launcher PRIVATE emu)
