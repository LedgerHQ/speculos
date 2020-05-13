#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdbool.h>

#include <setjmp.h>
#include <cmocka.h>

#include "utils.h"

#include "cx.h"
#include "cx_ec.h"

static uint8_t const C_ED25519_G[] = {
  //uncompressed
  0x04,
  //x
  0x21, 0x69, 0x36, 0xd3, 0xcd, 0x6e, 0x53, 0xfe, 0xc0, 0xa4, 0xe2, 0x31, 0xfd, 0xd6, 0xdc, 0x5c,
  0x69, 0x2c, 0xc7, 0x60, 0x95, 0x25, 0xa7, 0xb2, 0xc9, 0x56, 0x2d, 0x60, 0x8f, 0x25, 0xd5, 0x1a,
  //y
  0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
  0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x58
};

void test_scalar_mult_ec25519(void **state __attribute__((unused))) {
  uint8_t Pxy[65];
  uint8_t s[32];
  uint8_t expected[65];

  memcpy(Pxy, C_ED25519_G, sizeof(Pxy));
  memcpy(s,
         "\x0f\x8a\xa5\xdb\x64\x07\x2f\x35\x91\xac\xf3\x1e\xc9\xd7\x2b\x62"
         "\xce\x9a\x31\xb8\xc2\xdd\x8e\x96\x73\xaf\x1a\x48\x2a\x86\x62\xef",
         sizeof(s));

  memcpy(expected,
         "\x04"
         "\x5f\xe2\x35\x09\xe2\x30\x63\x09\x53\xad\xcb\x5c\x4a\xdb\x97\x41"
         "\xbb\x53\x97\xbb\xf6\x29\xc0\xdb\x1f\x1d\x49\xf7\xcb\x2e\x0a\x19"
         "\x1b\x23\x68\xaf\x41\x3e\x50\x2f\x14\x21\x1e\x1d\x79\x68\xc6\x5e"
         "\x0d\x71\xa2\xe7\x24\x9b\x11\xc3\x5c\x33\xa9\x01\xe6\xb3\x78\x41",
         65);

  sys_cx_ecfp_scalar_mult(CX_CURVE_Ed25519, Pxy, sizeof(Pxy), s, sizeof(s));

  assert_memory_equal(Pxy, expected, sizeof(expected));
}

int main(void) {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_scalar_mult_ec25519),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
