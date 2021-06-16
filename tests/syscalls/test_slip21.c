#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <cmocka.h>

#include "bolos/cx_ec.h"
#include "bolos/os_bip32.h"
#include "emulate.h"

void test_slip21(void **state __attribute__((unused)))
{
  const char SLIP77_LABEL[10] = {
    0, 'S', 'L', 'I', 'P', '-', '0', '0', '2', '1'
  };
  uint8_t key[32] = { 0 };
  const uint8_t expected_key[32] = { 0x1d, 0x06, 0x5e, 0x3a, 0xc1, 0xbb, 0xe5,
                                     0xc7, 0xfa, 0xd3, 0x2c, 0xf2, 0x30, 0x5f,
                                     0x7d, 0x70, 0x9d, 0xc0, 0x70, 0xd6, 0x72,
                                     0x04, 0x4a, 0x19, 0xe6, 0x10, 0xc7, 0x7c,
                                     0xdf, 0x33, 0xde, 0x0d };

  // Seed for BIP-39 mnemonic "all all all all all all all all all all all all"
  assert_int_equal(
      setenv("SPECULOS_SEED",
             "c76c4ac4f4e4a00d6b274d5c39c700bb4a7ddc04fbc6f78e85ca75007b5b495f7"
             "4a9043eeb77bdd53aa6fc3a0e31462270316fa04b8c19114c8798706cd02ac8",
             1),
      0);

  sys_os_perso_derive_node_with_seed_key(HDW_SLIP21, CX_CURVE_SECP256K1,
                                         (uint32_t *)SLIP77_LABEL, 10, key,
                                         NULL, NULL, 0);
  assert_memory_equal(key, expected_key, sizeof(key));
};

int main(void)
{
  const struct CMUnitTest tests[] = { cmocka_unit_test(test_slip21) };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
