#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <cmocka.h>

#include "utils.h"
#include "cx_ec.h"
#include "emu_os_bip32.h"
#include "emulate.h"

typedef struct {
  const size_t path_len;
  const uint32_t path[10];
} bip32_path;

typedef struct {
  const char *seed;
  const bip32_path bip32_path;
  const char *private_key;
} bip32_test_vector;

const bip32_test_vector secp256k1_test_vector[] = {
  {"9432fc1828af6f78f1f525a0c8208d7557b87bf89a15dab07f4e0acb649a48ab"
    "3f4a146f7a9567e38281fa19c062c5cf42d9687106dab9dacb8a2dc97d5c19ed",
    { 0x03, {0x8000002C, 0x80000000, 0x00000000}},
    "32FD0928F588847821BCA61BC5E566C8BF0061DAF91BB1AB6E6BA91E909E6B93"}
};

const bip32_test_vector secp256r1_test_vector[] = {
  {"9432fc1828af6f78f1f525a0c8208d7557b87bf89a15dab07f4e0acb649a48ab"
   "3f4a146f7a9567e38281fa19c062c5cf42d9687106dab9dacb8a2dc97d5c19ed",
    { 0x03, {0x8000002C, 0x80000000, 0x00000000}},
    "1C8E56B389A574305550B419A93F7A8616DE4287C545C78B0A4A8C1D62B61173"}
};


void test_bip32(cx_curve_t curve, const bip32_test_vector* tv, size_t tv_len) {
  uint8_t key[32] = {0};
  uint8_t expected_key[32];

  for(size_t i; i < tv_len; i++){
    assert_int_equal(hexstr2bin(tv[i].private_key, expected_key, sizeof(expected_key)), sizeof(expected_key));

    // Seed for BIP-39 mnemonic "dose bike detect wedding history hazard blast surprise hundred ankle sorry charge ozone often gauge photo sponsor faith business taste front differ bounce chaos"
    assert_int_equal(setenv("SPECULOS_SEED", tv[i].seed, 1), 0);

    sys_os_perso_derive_node_bip32(curve, tv[i].bip32_path.path, tv[i].bip32_path.path_len, key, NULL);
    assert_memory_equal(key, expected_key, sizeof(expected_key));
  }  
};

void test_bip32_secp256k1(void **state __attribute__((unused))){
  test_bip32(CX_CURVE_256K1, secp256k1_test_vector, sizeof(secp256k1_test_vector) / sizeof(secp256k1_test_vector[0]));
}

void test_bip32_secp256r1(void **state __attribute__((unused))){
  test_bip32(CX_CURVE_256R1, secp256r1_test_vector, sizeof(secp256r1_test_vector) / sizeof(secp256r1_test_vector[0]));
}

int main() {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_bip32_secp256k1),
    cmocka_unit_test(test_bip32_secp256r1)
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
