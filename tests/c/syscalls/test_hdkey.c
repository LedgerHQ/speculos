#define _SDK_2_0_

#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <setjmp.h>
// must come after setjmp.h
#include <cmocka.h>

#include "cxlib.h"
#include "environment.h"
#include "bolos/hdkey/include/hdkey.h"
#include "bolos/hdkey/include/hdkey_zip32.h"
#include "bolos/hdkey/include/hdkey_bls12377.h"

#define ARRAYLEN(array) (sizeof(array) / sizeof(array[0]))

static void setup_context(HDKEY_params_t *params, bolos_bool_t app_caller,
                          HDKEY_derive_mode_t mode, cx_curve_t curve,
                          const uint32_t *path, size_t path_len)
{
    params->from_app = app_caller;
    params->mode     = mode;
    params->curve    = curve;
    params->path     = path;
    params->path_len = path_len;
}

static void setup_context_result(HDKEY_params_t *params,
                                 uint8_t        *private_key,
                                 size_t          private_key_len,
                                 uint8_t        *chain,
                                 size_t          chain_len)
{
    params->private_key     = private_key;
    params->private_key_len = private_key_len;
    params->chain           = chain;
    params->chain_len       = chain_len;
}

#define TEST_SEED                                                            \
    "000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F"     \
    "202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F"

static uint32_t test_path[] = {0x80000001, 0x80000002, 0x80000003};

static int test_setup(void **state)
{
    (void) state;
    assert_int_equal(setenv("SPECULOS_SEED", TEST_SEED, 1), 0);
    init_environment();
    return 0;
}

/**
 * @brief Test function for HDKEY_derive with ZIP32 Sapling derivation.
 */
static void test_HDKEY_derive_zip32_sapling(void **state)
{
    (void) state; /* unused */
    HDKEY_params_t     params                               = {0};
    uint8_t            private_key[4 * HDKEY_ZIP32_KEY_LEN] = {0};
    uint8_t            chain[HDKEY_ZIP32_KEY_LEN]           = {0};
    size_t             offset                               = 0;
    // ask
    uint8_t ask[HDKEY_ZIP32_KEY_LEN]
        = {0xcc, 0xb7, 0xb3, 0x60, 0xc6, 0x49, 0x14, 0xa0, 0xdd, 0x0d, 0xa7,
           0xb7, 0x10, 0xfa, 0x4f, 0x85, 0x17, 0x01, 0x37, 0xb3, 0x36, 0xe0,
           0x84, 0x5c, 0x24, 0x00, 0x38, 0xa8, 0x68, 0x7b, 0xbf, 0x03};
    // nsk
    uint8_t nsk[HDKEY_ZIP32_KEY_LEN]
        = {0x87, 0x6c, 0x89, 0x37, 0x0a, 0x70, 0x42, 0x31, 0xc9, 0xba, 0x1c,
           0x19, 0x8f, 0x9f, 0xbf, 0xe6, 0x11, 0x61, 0x93, 0x86, 0x76, 0x1f,
           0xe6, 0x78, 0x36, 0x50, 0x3f, 0x21, 0xac, 0x23, 0xca, 0x0a};
    // ovk
    uint8_t ovk[HDKEY_ZIP32_KEY_LEN]
        = {0xf7, 0x68, 0xf4, 0x6d, 0x04, 0x1d, 0xd9, 0x3c, 0x85, 0x22, 0xd1,
           0x25, 0x3d, 0x9c, 0xb9, 0xd7, 0x74, 0xf0, 0x44, 0xbd, 0x43, 0xe2,
           0xb4, 0x54, 0x87, 0x85, 0x27, 0x49, 0xf4, 0xe6, 0x8d, 0xb9};
    // dk
    uint8_t dk[HDKEY_ZIP32_KEY_LEN]
        = {0x5f, 0x76, 0x07, 0x62, 0x6a, 0x2e, 0xdf, 0xd6, 0x4d, 0x20, 0x44,
           0x2c, 0xdb, 0xe8, 0x1a, 0x4c, 0x09, 0x83, 0xc9, 0xa9, 0x07, 0xda,
           0x53, 0x20, 0x29, 0x57, 0xb8, 0x53, 0xb3, 0x80, 0x08, 0x86};
    // c
    uint8_t cc[HDKEY_ZIP32_KEY_LEN]
        = {0x7f, 0x59, 0x4c, 0x97, 0x2b, 0x75, 0x73, 0xb1, 0xb9, 0x2d, 0x49,
           0x89, 0x63, 0xf3, 0xd5, 0xf5, 0xc2, 0xec, 0x59, 0x86, 0x8a, 0x71,
           0x68, 0x0b, 0x98, 0x6d, 0x63, 0x0f, 0xd2, 0x14, 0xc9, 0x45};

    // Set up context
    setup_context(&params,
                  BOLOS_TRUE,
                  HDKEY_DERIVE_MODE_ZIP32_SAPLING,
                  CX_CURVE_JUBJUB,
                  test_path,
                  ARRAYLEN(test_path));
    // Set up result pointers
    setup_context_result(&params, private_key, sizeof(private_key), chain, sizeof(chain));

    assert_int_equal(HDKEY_derive(&params), OS_SUCCESS);
    assert_memory_equal(private_key, ask, sizeof(ask));
    offset = HDKEY_ZIP32_KEY_LEN;
    assert_memory_equal(&private_key[offset], nsk, sizeof(nsk));
    offset += HDKEY_ZIP32_KEY_LEN;
    assert_memory_equal(&private_key[offset], ovk, sizeof(ovk));
    offset += HDKEY_ZIP32_KEY_LEN;
    assert_memory_equal(&private_key[offset], dk, sizeof(dk));
    assert_memory_equal(chain, cc, sizeof(cc));
}

/**
 * @brief Test function for HDKEY_derive with ZIP32 Orchard derivation.
 */
static void test_HDKEY_derive_zip32_orchard(void **state)
{
    (void) state; /* unused */
    uint8_t            private_key[HDKEY_ZIP32_KEY_LEN] = {0};
    uint8_t            chain[HDKEY_ZIP32_KEY_LEN]       = {0};
    HDKEY_params_t     params                           = {0};
    // sk
    uint8_t sk[HDKEY_ZIP32_KEY_LEN]
        = {0x51, 0xf9, 0xed, 0x7c, 0xb2, 0xba, 0x22, 0x0a, 0xc9, 0xf4, 0x9c,
           0xad, 0x4d, 0xcc, 0x50, 0xe0, 0x0d, 0x62, 0x56, 0x75, 0xba, 0x1f,
           0x39, 0xc8, 0xc1, 0x16, 0x55, 0xa8, 0xc5, 0x73, 0xf4, 0xae};
    // chain code
    uint8_t cc[HDKEY_ZIP32_KEY_LEN]
        = {0x92, 0x4c, 0xa8, 0x16, 0xac, 0xf1, 0x06, 0xd7, 0x38, 0x9a, 0x59,
           0x12, 0xcc, 0x77, 0xc6, 0x3e, 0x2b, 0x76, 0xc0, 0x41, 0xc3, 0x18,
           0x9f, 0x25, 0x99, 0x4b, 0xdf, 0x4d, 0x4a, 0x53, 0x62, 0x55};

    // Set up context
    setup_context(
        &params, BOLOS_TRUE, HDKEY_DERIVE_MODE_ZIP32_ORCHARD, CX_CURVE_NONE, test_path, ARRAYLEN(test_path));
    // Set up result pointers
    setup_context_result(&params, private_key, sizeof(private_key), chain, sizeof(chain));

    assert_int_equal(HDKEY_derive(&params), OS_SUCCESS);
    assert_memory_equal(private_key, sk, sizeof(sk));
    assert_memory_equal(chain, cc, sizeof(cc));
}

/**
 * @brief Test function for HDKEY_derive with ZIP32 registered derivation.
 */
static void test_HDKEY_derive_zip32_registered(void **state)
{
  (void)state; /* unused */
  uint8_t private_key[HDKEY_ZIP32_KEY_LEN] = { 0 };
  uint8_t chain[HDKEY_ZIP32_KEY_LEN] = { 0 };
  HDKEY_params_t params = { 0 };
  // seed_key contains: tag || len(context_str) || context_str || tag ||
  // len(tag_seq_1)
  // || tag_seq_1 || tag || len(tag_seq_2) || tag_seq_2
  uint8_t seed_key[] = { 0x16, 0x12, 0x5a, 0x63, 0x61, 0x73, 0x68, 0x20, 0x74,
                         0x65, 0x73, 0x74, 0x20, 0x76, 0x65, 0x63, 0x74, 0x6f,
                         0x72, 0x73, 0xa0, 0x1b, 0x54, 0x61, 0x67, 0x20, 0x73,
                         0x65, 0x71, 0x75, 0x65, 0x6e, 0x63, 0x65, 0x20, 0x66,
                         0x6f, 0x72, 0x20, 0x30, 0x78, 0x38, 0x30, 0x30, 0x30,
                         0x30, 0x30, 0x30, 0x32, 0xa0, 0x00 };
  // sk
  uint8_t sk[HDKEY_ZIP32_KEY_LEN] = { 0xd9, 0xd5, 0xac, 0xc7, 0x27, 0xfe, 0xff,
                                      0xd6, 0x8f, 0xed, 0x71, 0x33, 0x9d, 0x04,
                                      0xee, 0x3f, 0xce, 0xf1, 0xe6, 0x80, 0x2a,
                                      0xdb, 0x30, 0x11, 0xac, 0x48, 0xa9, 0xc1,
                                      0x53, 0xb3, 0x81, 0x30 };
  // chain code
  uint8_t cc[HDKEY_ZIP32_KEY_LEN] = { 0xf4, 0x86, 0x2e, 0x29, 0xeb, 0xc1, 0x3f,
                                      0x56, 0x02, 0x0b, 0x6e, 0x9f, 0x49, 0xd4,
                                      0x51, 0x08, 0x07, 0x12, 0x8a, 0xde, 0x72,
                                      0xa1, 0x6a, 0xde, 0x6d, 0x55, 0x0c, 0xb0,
                                      0x30, 0xec, 0x67, 0x90 };

    // Set up context
    setup_context(&params,
                  BOLOS_TRUE,
                  HDKEY_DERIVE_MODE_ZIP32_REGISTERED,
                  CX_CURVE_NONE,
                  test_path,
                  ARRAYLEN(test_path));
    // Set up result pointers
    setup_context_result(&params, private_key, sizeof(private_key), chain, sizeof(chain));

    // Set up seed key for context string and tag sequences
    params.seed_key     = seed_key;
    params.seed_key_len = sizeof(seed_key);

    assert_int_equal(HDKEY_derive(&params), OS_SUCCESS);
    assert_memory_equal(private_key, sk, sizeof(sk));
    assert_memory_equal(chain, cc, sizeof(cc));
}

/**
 * @brief Test function for HDKEY_derive with BLS12-377.
 */
static void test_HDKEY_derive_bls12377(void **state)
{
    (void) state; /* unused */
    // Test vector:
    // Root seed:
    // fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542
    // {
    //     path: "m/0'/2147483647'",
    //     chainCode:
    //         'c0cbe8cce5b29f2a787e1f77915af2c2e71fe65d4dcb7ad797b3584fcb42bcb5',
    //     seed: '2e6695fe0bd1e628215e5f8c0aa6b374233c6fa06019057d6d786ae66d937ef5'
    // },

    HDKEY_params_t     params;
    uint32_t           path[]          = {0x80000000, 0xffffffff};
    uint8_t            private_key[32] = {0};
    uint8_t            chain[32]       = {0};
    const char *seed = "fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542";
    uint8_t expected_key[]   = {0x2e, 0x66, 0x95, 0xfe, 0x0b, 0xd1, 0xe6, 0x28, 0x21, 0x5e, 0x5f,
                                0x8c, 0x0a, 0xa6, 0xb3, 0x74, 0x23, 0x3c, 0x6f, 0xa0, 0x60, 0x19,
                                0x05, 0x7d, 0x6d, 0x78, 0x6a, 0xe6, 0x6d, 0x93, 0x7e, 0xf5};
    uint8_t expected_chain[] = {0xc0, 0xcb, 0xe8, 0xcc, 0xe5, 0xb2, 0x9f, 0x2a, 0x78, 0x7e, 0x1f,
                                0x77, 0x91, 0x5a, 0xf2, 0xc2, 0xe7, 0x1f, 0xe6, 0x5d, 0x4d, 0xcb,
                                0x7a, 0xd7, 0x97, 0xb3, 0x58, 0x4f, 0xcb, 0x42, 0xbc, 0xb5};

    memset(&params, 0, sizeof(HDKEY_params_t));

    // Set up context
    setup_context(&params,
                  BOLOS_TRUE,
                  HDKEY_DERIVE_MODE_BLS12377_ALEO,
                  CX_CURVE_BLS12_377_G1,
                  path,
                  ARRAYLEN(path));
    // Set up result pointers
    setup_context_result(&params, private_key, sizeof(private_key), chain, sizeof(chain));
    // Init seed
    assert_int_equal(setenv("SPECULOS_SEED", seed, 1), 0);
    init_environment();

    assert_int_equal(HDKEY_derive(&params), OS_SUCCESS);
    assert_memory_equal(expected_key, private_key, HDKEY_BLS12377_KEY_LEN);
    assert_memory_equal(expected_chain, chain, HDKEY_BLS12377_KEY_LEN);
}

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test_setup(test_HDKEY_derive_zip32_sapling, test_setup),
    cmocka_unit_test_setup(test_HDKEY_derive_zip32_orchard, test_setup),
    cmocka_unit_test_setup(test_HDKEY_derive_zip32_registered, test_setup),
    cmocka_unit_test_setup(test_HDKEY_derive_bls12377, NULL),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
