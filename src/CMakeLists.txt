add_library(emu
        bolos/cx_aes.c
        bolos/cx_aes_sdk2.c
        bolos/cx_blake2b.c
        bolos/cx_bn.c
        bolos/cx_curve25519.c
        bolos/cx_crc.c
        bolos/cx_ec.c
        bolos/cx_ec_domain.c
        bolos/cx_ecpoint.c
        bolos/cx_ed25519.c
        bolos/cx_hmac.c
        bolos/cx_hkdf.c
        bolos/cx_montgomery.c
        bolos/cx_mpi.c
        bolos/cx_ripemd160.c
        bolos/cx_rng_rfc6979.c
        bolos/cx_scc.c
        bolos/cx_sha256.c
        bolos/cx_sha3.c
        bolos/cx_sha512.c
        bolos/cx_twisted_edwards.c
        bolos/cx_utils.c
        bolos/cx_weierstrass.c
        bolos/cxlib.c
        bolos/bagl.c
        bolos/endorsement.c
        bolos/seproxyhal.c
        bolos/os.c
        bolos/os_1.2.c
        bolos/os_1.5.c
        bolos/os_1.6.c
        bolos/os_2.0.c
        bolos/os_bip32.c
        bolos/os_eip2333.c
        bolos/cx.c
        bolos/cx_hash.c
        bolos/cx_math.c
        bolos/default.c
        emulate.c
        emulate_1.2.c
        emulate_1.5.c
        emulate_1.6.c
        emulate_2.0.c
        emulate_blue_2.2.5.c
        svc.c)

add_dependencies(emu openssl)

add_executable(launcher launcher.c)
# ensure the loader .text section doesn't mess with cxlib
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Ttext-segment=0xf0000000")
target_link_libraries(launcher PRIVATE emu)
