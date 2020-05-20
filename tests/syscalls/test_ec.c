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

  // Verify that 1*G = G
  memcpy(Pxy, C_ED25519_G, sizeof(Pxy));
  memcpy(s,
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01",
         sizeof(s));
  memcpy(expected, C_ED25519_G, sizeof(Pxy));
  sys_cx_ecfp_scalar_mult(CX_CURVE_Ed25519, Pxy, sizeof(Pxy), s, sizeof(s));
  assert_memory_equal(Pxy, expected, sizeof(expected));

  // Verify that order*G = (0,1)
  memcpy(Pxy, C_ED25519_G, sizeof(Pxy));
  memcpy(s,
         "\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
         "\x14\xde\xf9\xde\xa2\xf7\x9c\xd6\x58\x12\x63\x1a\x5c\xf5\xd3\xed",
         sizeof(s));
  memcpy(expected,
         "\x04"
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01",
         65);
  sys_cx_ecfp_scalar_mult(CX_CURVE_Ed25519, Pxy, sizeof(Pxy), s, sizeof(s));
  assert_memory_equal(Pxy, expected, sizeof(expected));

  // Test vectors from https://tools.ietf.org/html/rfc8032#section-7.1
  // Pre-compute private = SHA512(secret), truncated, adjusted and in Big Endian
  // secret = 9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60
  memcpy(Pxy, C_ED25519_G, sizeof(Pxy));
  memcpy(s,
         "\x4f\xe9\x4d\x90\x06\xf0\x20\xa5\xa3\xc0\x80\xd9\x68\x27\xff\xfd"
         "\x3c\x01\x0a\xc0\xf1\x2e\x7a\x42\xcb\x33\x28\x4f\x86\x83\x7c\x30",
         sizeof(s));
  memcpy(expected,
         "\x04"
         "\x55\xd0\xe0\x9a\x2b\x9d\x34\x29\x22\x97\xe0\x8d\x60\xd0\xf6\x20"
         "\xc5\x13\xd4\x72\x53\x18\x7c\x24\xb1\x27\x86\xbd\x77\x76\x45\xce"
         "\x1a\x51\x07\xf7\x68\x1a\x02\xaf\x25\x23\xa6\xda\xf3\x72\xe1\x0e"
         "\x3a\x07\x64\xc9\xd3\xfe\x4b\xd5\xb7\x0a\xb1\x82\x01\x98\x5a\xd7",
         65);
  sys_cx_ecfp_scalar_mult(CX_CURVE_Ed25519, Pxy, sizeof(Pxy), s, sizeof(s));
  assert_memory_equal(Pxy, expected, sizeof(expected));

  // secret = 4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb
  memcpy(Pxy, C_ED25519_G, sizeof(Pxy));
  memcpy(s,
         "\x51\x2e\x50\x2e\xb0\x24\x9a\x25\x5e\x1c\x82\x7f\x3b\x6b\x6c\x7f"
         "\x0a\x79\xf4\xca\x85\x75\xa9\x15\x28\xd5\x82\x58\xd7\x9e\xbd\x68",
         sizeof(s));
  memcpy(expected,
         "\x04"
         "\x74\xad\x28\x20\x5b\x4f\x38\x4b\xc0\x81\x3e\x65\x85\x86\x4e\x52"
         "\x80\x85\xf9\x1f\xb6\xa5\x09\x6f\x24\x4a\xe0\x1e\x57\xde\x43\xae"
         "\x0c\x66\xf4\x2a\xf1\x55\xcd\xc0\x8c\x96\xc4\x2e\xcf\x2c\x98\x9c"
         "\xbc\x7e\x1b\x4d\xa7\x0a\xb7\x92\x5a\x89\x43\xe8\xc3\x17\x40\x3d",
         65);
  sys_cx_ecfp_scalar_mult(CX_CURVE_Ed25519, Pxy, sizeof(Pxy), s, sizeof(s));
  assert_memory_equal(Pxy, expected, sizeof(expected));

  // secret = 833fe62409237b9d62ec77587520911e9a759cec1d19755b7da901b96dca3d42
  memcpy(Pxy, C_ED25519_G, sizeof(Pxy));
  memcpy(s,
         "\x45\xb6\x41\x72\xc7\x52\x8f\x1a\xf4\xa5\xa8\x5d\xd6\xdb\xd8\x72"
         "\x92\xa0\x07\x9b\xf1\x13\x57\x0b\xec\x4b\xe0\x59\x4f\xce\xdd\x30",
         sizeof(s));
  memcpy(expected,
         "\x04"
         "\x58\xb4\x01\xb9\xdf\x6f\x65\xa3\x46\x25\x40\x0a\x43\xfa\x6e\x89"
         "\xdd\x5a\xe7\x44\x0e\x98\x99\xc9\xc9\x6e\xea\x99\x5b\x72\xfc\x2f"
         "\x3f\xe2\x67\x34\x68\x19\xf8\xeb\x64\x4d\xfd\x2e\xef\x67\x54\xc3"
         "\x34\x50\x24\xe1\x70\x2c\x93\xf4\x3b\x56\x5e\xad\x93\x2b\x17\xec",
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
