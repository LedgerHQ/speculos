#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <setjmp.h>
// must come after setjmp.h
#include <cmocka.h>

#include "utils.h"

#include "bolos/cx.h"
#include "bolos/cx_hash.h"
#include "emulate.h"

#define cx_hash sys_cx_hash

typedef struct {
  const char *msg;
  const char *md;
} test_vector;

void test_ripemd160_kat1(void **state __attribute__((unused)))
{
  // Test vectors found at
  // http://homes.esat.kuleuven.be/~bosselae/ripemd160.html
  cx_ripemd160_t ctx;
  uint8_t out[CX_RIPEMD160_SIZE], expected[CX_RIPEMD160_SIZE];
  size_t msg_len;
  unsigned int i;

  // Basic tests
  test_vector tv[] = {
    { "", "9c1185a5c5e9fc54612808977ee8f548b2258d31" },
    { "61", "0bdc9d2d256b3ee9daae347be6f4dc835a467ffe" },     // "a"
    { "616263", "8eb208f7e05d987a9b044a8e98c6b087f15a0bfc" }, // "abc"
    { "6d65737361676520646967657374",
      "5d0689ef49d2fae572b881b123a85ffa21595f36" }, // "message digest"
    { "6162636465666768696a6b6c6d6e6f707172737475767778797a",
      "f71c27109c692c1b56bbdceb5b9d2865b3708dbc" }, // "a...z"
    { "6162636462636465636465666465666765666768666768696768696a68696a6b696a6b6"
      "c6a6b6c6d6b6c6d6e6c6d6e6f6d6e6f706e6f7071",
      "12a053384a9c0c88e405a06c27dcf49ada62eb2b" }, // abcdbcde...nopq
    { "4142434445464748494a4b4c4d4e4f505152535455565758595a6162636465666768696"
      "a6b6c6d6e6f707172737475767778797a30313233343536373839",
      "b0e20b6e3116640286ed3a87a5713079b21f5189" } // A...Za...z0...9
  };

  for (i = 0; i < sizeof(tv) / sizeof(tv[0]); i++) {
    msg_len = strlen(tv[i].msg) / 2;

    uint8_t msg[msg_len];
    assert_int_equal(hexstr2bin(tv[i].msg, msg, msg_len), msg_len);
    assert_int_equal(hexstr2bin(tv[i].md, expected, CX_RIPEMD160_SIZE),
                     CX_RIPEMD160_SIZE);

    assert_int_equal(cx_ripemd160_init(&ctx), CX_RIPEMD160);
    spec_cx_ripemd160_update(&ctx, msg, msg_len);
    spec_cx_ripemd160_final(&ctx, out);
    // assert_int_equal(cx_hash((cx_hash_t *)&ctx, CX_LAST, msg, msg_len, out,
    //                          CX_RIPEMD160_SIZE),
    //                  CX_RIPEMD160_SIZE);

    assert_memory_equal(out, expected, CX_RIPEMD160_SIZE);
  }
}

void test_ripemd160_kat2(void **state __attribute__((unused)))
{
  // 8 times "1234567890"

  cx_ripemd160_t ctx;
  uint8_t out[CX_RIPEMD160_SIZE];
  const uint8_t expected[CX_RIPEMD160_SIZE] = { 0x9b, 0x75, 0x2e, 0x45, 0x57,
                                                0x3d, 0x4b, 0x39, 0xf4, 0xdb,
                                                0xd3, 0x32, 0x3c, 0xab, 0x82,
                                                0xbf, 0x63, 0x32, 0x6b, 0xfb };

  assert_int_equal(cx_ripemd160_init(&ctx), CX_RIPEMD160);
  for (int i = 0; i < 8; i++) {
    cx_hash((cx_hash_t *)&ctx, 0, (uint8_t *)"1234567890", 10, NULL, 0);
  }
  assert_int_equal(
      cx_hash((cx_hash_t *)&ctx, CX_LAST, NULL, 0, out, CX_RIPEMD160_SIZE),
      CX_RIPEMD160_SIZE);
  assert_memory_equal(out, expected, CX_RIPEMD160_SIZE);
}

void test_ripemd160_kat3(void **state __attribute__((unused)))
{
  // 1 million times "a"

  cx_ripemd160_t ctx;
  uint8_t out[CX_RIPEMD160_SIZE];
  const uint8_t expected[CX_RIPEMD160_SIZE] = { 0x52, 0x78, 0x32, 0x43, 0xc1,
                                                0x69, 0x7b, 0xdb, 0xe1, 0x6d,
                                                0x37, 0xf9, 0x7f, 0x68, 0xf0,
                                                0x83, 0x25, 0xdc, 0x15, 0x28 };
  assert_int_equal(cx_ripemd160_init(&ctx), CX_RIPEMD160);
  for (int i = 0; i < 1000000; i++) {
    cx_hash((cx_hash_t *)&ctx, 0, (uint8_t *)"a", 1, NULL, 0);
  }
  assert_int_equal(
      cx_hash((cx_hash_t *)&ctx, CX_LAST, NULL, 0, out, CX_RIPEMD160_SIZE),
      CX_RIPEMD160_SIZE);
  assert_memory_equal(out, expected, CX_RIPEMD160_SIZE);
}

int main(void)
{
  const struct CMUnitTest tests[] = { cmocka_unit_test(test_ripemd160_kat1),
                                      cmocka_unit_test(test_ripemd160_kat2),
                                      cmocka_unit_test(test_ripemd160_kat3) };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
