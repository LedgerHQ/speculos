#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <setjmp.h>
// must come after setjmp.h
#include <cmocka.h>

#include "bolos/cx_hash.h"
#include "nist_cavp.h"
#include "utils.h"

/*
// Exception-related functions. Exceptions are not handled, functions are just
// defined so that tests can compile.
try_context_t *try_context_get(void) { return NULL; }

void os_longjmp(unsigned int exception) {
  longjmp(try_context_get()->jmp_buf, exception);
}

#include "cx_ram.h"

union cx_u G_cx;
*/

#define CX_MAX_DIGEST_SIZE CX_SHA512_SIZE

void test_cavp_sha3_monte_carlo(cx_md_t md_type, uint8_t *initial_seed,
                                const uint8_t *expected_seed,
                                size_t digest_size)
{
  cx_sha3_t ctx;
  // uint8_t ctx[1024]; // haxx, get max real digest size

  uint8_t md[CX_MAX_DIGEST_SIZE];

  assert_int_equal(
      spec_cx_hash_init_ex((cx_hash_ctx *)&ctx, md_type, digest_size), 1);
  size_t md_len = (size_t)spec_cx_hash_get_size((cx_hash_ctx *)&ctx);
  assert_int_equal(md_len, digest_size);
  memcpy(md, initial_seed, md_len);

  for (int i = 0; i < 100; i++) {
    for (int j = 0; j < 1000; j++) {
      assert_int_equal(
          spec_cx_hash_init_ex((cx_hash_ctx *)&ctx, md_type, digest_size), 1);
      assert_int_equal(spec_cx_hash_update((cx_hash_ctx *)&ctx, md, md_len), 1);
      assert_int_equal(spec_cx_hash_final((cx_hash_ctx *)&ctx, md), 1);
    }
  }
  assert_memory_equal(md, expected_seed, md_len);
}

void test_cavp_sha3_xof_monte_carlo(cx_md_t md_type,
                                    const uint8_t *initial_seed,
                                    const uint8_t *expected_output,
                                    size_t expected_output_length,
                                    size_t min_output_length,
                                    size_t max_output_length)
{

  cx_hash_ctx ctx;
  uint8_t md[max_output_length];

  memcpy(md, initial_seed, 16);

  size_t output_length, new_output_length;
  size_t output_range_length = max_output_length - min_output_length + 1;
  output_length = max_output_length;

  new_output_length = output_length;
  for (int i = 0; i < 100; i++) {
    for (int j = 0; j < 1000; j++) {
      output_length = new_output_length;
      assert_int_equal(spec_cx_hash_init_ex(&ctx, md_type, output_length), 1);
      assert_int_equal(spec_cx_hash_update(&ctx, md, 16), 1);

      // The first 16 bytes of md are hashed in the next iteration. However,
      // output_length can be lower than 16 bytes. In that case, output is
      // padded with zeroes.
      memset(md, 0, 16);
      assert_int_equal(spec_cx_hash_final(&ctx, md), 1);

      assert_in_range(output_length, 2, max_output_length);
      uint32_t right_bits =
          ((md[output_length - 2]) << 8) | md[output_length - 1];
      new_output_length =
          min_output_length + (right_bits % output_range_length);
    }
  }
  assert_int_equal(output_length, expected_output_length);
  assert_memory_equal(md, expected_output, expected_output_length);
}

void test_cavp_sha3_xof(const char *filename, cx_md_t md_type)
{
  cx_hash_ctx ctx;
  char line[1024];
  char *result;

  FILE *f = fopen(filename, "r");
  assert_non_null(f);

  while (!feof(f)) {
    result = fgets(line, sizeof(line), f);
    assert_non_null(result);
    char *pos1 = strchr(line, ':');
    assert_non_null(pos1);
    char *pos2 = strchr(pos1 + 1, ':');
    assert_non_null(pos2);
    char *pos3 = strchr(pos1 + 1, '\n');

    *pos1 = *pos2 = 0;
    if (pos3) {
      *pos3 = 0;
    }

    size_t data_len = strlen(pos1 + 1) / 2;
    size_t md_len = strlen(pos2 + 1) / 2;

    uint8_t data[data_len];
    uint8_t md[md_len], expected[md_len];

    assert_int_equal(spec_cx_hash_init_ex(&ctx, md_type, md_len), 1);

    assert_int_equal(md_len, spec_cx_hash_get_size(&ctx));
    assert_int_equal(hexstr2bin(pos1 + 1, data, data_len), data_len);
    assert_int_equal(hexstr2bin(pos2 + 1, expected, md_len), md_len);

    assert_int_equal(spec_cx_hash_update(&ctx, data, data_len), 1);
    assert_int_equal(spec_cx_hash_final(&ctx, md), 1);
    assert_memory_equal(expected, md, md_len);
  }
  fclose(f);
}

void test_sha3_224_short_msg(void **state __attribute__((unused)))
{
  test_cavp_short_msg_with_size(TESTS_PATH "cavp/sha3_224_short_msg.data",
                                CX_SHA3, 28);
}

void test_sha3_224_long_msg(void **state __attribute__((unused)))
{
  test_cavp_long_msg_with_size(TESTS_PATH "cavp/sha3_224_long_msg.data",
                               CX_SHA3, 28);
}

void test_sha3_224_monte_carlo(void **state __attribute__((unused)))
{
  uint8_t initial_seed[] = { 0x3a, 0x94, 0x15, 0xd4, 0x01, 0xae, 0xb8,
                             0x56, 0x7e, 0x6f, 0x0e, 0xce, 0xe3, 0x11,
                             0xf4, 0xf7, 0x16, 0xb3, 0x9e, 0x86, 0x04,
                             0x5c, 0x8a, 0x51, 0x38, 0x3d, 0xb2, 0xb6 };
  const uint8_t expected_output[] = {
    0x91, 0xde, 0xfb, 0xe2, 0x30, 0xb5, 0x14, 0xd7, 0xdb, 0x13,
    0xd9, 0x15, 0xa8, 0x23, 0x68, 0xd3, 0x2d, 0x48, 0xf5, 0x5d,
    0xb3, 0x1d, 0x16, 0xe3, 0xae, 0x7f, 0xbb, 0xd0
  };

  test_cavp_sha3_monte_carlo(CX_SHA3, initial_seed, expected_output, 28);
}

void test_sha3_256_short_msg(void **state __attribute__((unused)))
{
  test_cavp_long_msg_with_size(TESTS_PATH "cavp/sha3_256_short_msg.data",
                               CX_SHA3, 32);
}

void test_sha3_256_long_msg(void **state __attribute__((unused)))
{
  test_cavp_long_msg_with_size(TESTS_PATH "cavp/sha3_256_long_msg.data",
                               CX_SHA3, 32);
}

void test_sha3_256_monte_carlo(void **state __attribute__((unused)))
{
  uint8_t initial_seed[] = { 0xaa, 0x64, 0xf7, 0x24, 0x5e, 0x21, 0x77, 0xc6,
                             0x54, 0xeb, 0x4d, 0xe3, 0x60, 0xda, 0x87, 0x61,
                             0xa5, 0x16, 0xfd, 0xc7, 0x57, 0x8c, 0x34, 0x98,
                             0xc5, 0xe5, 0x82, 0xe0, 0x96, 0xb8, 0x73, 0x0c };
  const uint8_t expected_output[] = { 0x45, 0x6f, 0x2e, 0xd7, 0xf5, 0x43, 0x3b,
                                      0xb4, 0xe5, 0x6d, 0x77, 0x80, 0xa2, 0x1a,
                                      0x95, 0x3e, 0x95, 0xd6, 0xa5, 0xeb, 0x53,
                                      0xbb, 0x4c, 0x97, 0x4c, 0x57, 0xa9, 0x0e,
                                      0x67, 0x7f, 0x31, 0x97 };

  test_cavp_sha3_monte_carlo(CX_SHA3, initial_seed, expected_output, 32);
}

void test_sha3_384_short_msg(void **state __attribute__((unused)))
{
  test_cavp_long_msg_with_size(TESTS_PATH "cavp/sha3_384_short_msg.data",
                               CX_SHA3, 48);
}

void test_sha3_384_long_msg(void **state __attribute__((unused)))
{
  test_cavp_long_msg_with_size(TESTS_PATH "cavp/sha3_384_long_msg.data",
                               CX_SHA3, 48);
}

void test_sha3_384_monte_carlo(void **state __attribute__((unused)))
{
  uint8_t initial_seed[] = { 0x7a, 0x00, 0x79, 0x1f, 0x6f, 0x65, 0xc2, 0x1f,
                             0x1c, 0x97, 0xc5, 0x8f, 0xa3, 0xc0, 0x52, 0x0c,
                             0xfc, 0x85, 0xcd, 0x7e, 0x3d, 0x39, 0x8c, 0xf0,
                             0x19, 0x50, 0x81, 0x9f, 0xa7, 0x17, 0x19, 0x50,
                             0x65, 0xa3, 0x63, 0xe7, 0x7d, 0x07, 0x75, 0x36,
                             0x47, 0xcb, 0x0c, 0x13, 0x0e, 0x99, 0x72, 0xad };

  const uint8_t expected_output[] = {
    0x02, 0xc9, 0xba, 0xbd, 0x4a, 0xdd, 0x11, 0xa5, 0xf2, 0x3c, 0x18, 0x08,
    0xf7, 0x2e, 0x3d, 0xc8, 0x32, 0x5c, 0xed, 0xc3, 0x1d, 0x28, 0x21, 0x3a,
    0x04, 0xd9, 0x99, 0xda, 0xc8, 0xf4, 0x6b, 0x86, 0x6f, 0x84, 0xba, 0x3d,
    0xbf, 0xbc, 0xf1, 0xa8, 0x63, 0xcc, 0x54, 0xd8, 0x08, 0xff, 0xad, 0xca
  };

  test_cavp_sha3_monte_carlo(CX_SHA3, initial_seed, expected_output, 48);
}

void test_sha3_512_short_msg(void **state __attribute__((unused)))
{
  test_cavp_long_msg_with_size(TESTS_PATH "cavp/sha3_512_short_msg.data",
                               CX_SHA3, 64);
}

void test_sha3_512_long_msg(void **state __attribute__((unused)))
{
  test_cavp_long_msg_with_size(TESTS_PATH "cavp/sha3_512_long_msg.data",
                               CX_SHA3, 64);
}

void test_sha3_512_monte_carlo(void **state __attribute__((unused)))
{
  uint8_t initial_seed[] = { 0x76, 0x4a, 0x55, 0x11, 0xf0, 0x0d, 0xbb, 0x0e,
                             0xae, 0xf2, 0xeb, 0x27, 0xad, 0x58, 0xd3, 0x5f,
                             0x74, 0xf5, 0x63, 0xb8, 0x8f, 0x78, 0x9f, 0xf5,
                             0x3f, 0x6c, 0xf3, 0xa4, 0x70, 0x60, 0xc7, 0x5c,
                             0xeb, 0x45, 0x54, 0x44, 0xcd, 0x17, 0xb6, 0xd4,
                             0x38, 0xc0, 0x42, 0xe0, 0x48, 0x39, 0x19, 0xd2,
                             0x49, 0xf2, 0xfd, 0x37, 0x27, 0x74, 0x64, 0x7d,
                             0x25, 0x45, 0xcb, 0xfa, 0xd2, 0x0b, 0x4d, 0x31 };

  const uint8_t expected_output[] = {
    0x76, 0x08, 0x24, 0xa4, 0x39, 0xb0, 0x68, 0x1f, 0xcd, 0x5d, 0x22,
    0xf8, 0x46, 0x7d, 0x92, 0x7a, 0x76, 0x4f, 0xeb, 0xc4, 0x57, 0xfd,
    0x1e, 0xb6, 0x25, 0x84, 0xca, 0x82, 0xb0, 0x0e, 0x1a, 0x07, 0x90,
    0x5a, 0x01, 0x17, 0xa9, 0x55, 0x04, 0x18, 0x92, 0xd2, 0xc9, 0xd8,
    0x49, 0xc0, 0x96, 0x06, 0x7e, 0xd2, 0x89, 0x3a, 0xca, 0x5c, 0x84,
    0x1f, 0x8a, 0xa3, 0x2d, 0xab, 0xe6, 0x42, 0xbc, 0x82
  };

  test_cavp_sha3_monte_carlo(CX_SHA3, initial_seed, expected_output, 64);
}

void test_shake128_short_msg(void **state __attribute__((unused)))
{
  test_cavp_short_msg_with_size(TESTS_PATH "cavp/shake128_short_msg.data",
                                CX_SHAKE128, 16);
}

void test_shake128_long_msg(void **state __attribute__((unused)))
{
  test_cavp_long_msg_with_size(TESTS_PATH "cavp/shake128_long_msg.data",
                               CX_SHAKE128, 16);
}

void test_shake128_variable_output(void **state __attribute__((unused)))
{
  test_cavp_sha3_xof(TESTS_PATH "cavp/shake128_variable_output.data",
                     CX_SHAKE128);
}

void test_shake128_monte_carlo(void **state __attribute__((unused)))
{
  const uint8_t seed[] = { 0xc8, 0xb3, 0x10, 0xcb, 0x97, 0xef, 0xa3, 0x85,
                           0x54, 0x34, 0x99, 0x8f, 0xa8, 0x1c, 0x76, 0x74 };
  const uint8_t result[] = { 0x4a, 0xa3, 0x71, 0xf0, 0x09, 0x9b, 0x04,
                             0xa9, 0x09, 0xf9, 0xb1, 0x68, 0x0e, 0x8b,
                             0x52, 0xa2, 0x1c, 0x65, 0x10, 0xea, 0x26,
                             0x40, 0x13, 0x7d, 0x50, 0x1f, 0xfa, 0x11,
                             0x4b, 0xf8, 0x47, 0x17, 0xb1, 0xf7, 0x25,
                             0xd6, 0x4b, 0xae, 0x4a, 0xe5, 0xd8, 0x7a };
  test_cavp_sha3_xof_monte_carlo(CX_SHAKE128, seed, result, sizeof(result),
                                 128 / 8, 1120 / 8);
}

void test_shake256_short_msg(void **state __attribute__((unused)))
{
  test_cavp_short_msg_with_size(TESTS_PATH "cavp/shake256_short_msg.data",
                                CX_SHAKE256, 32);
}

void test_shake256_long_msg(void **state __attribute__((unused)))
{
  test_cavp_long_msg_with_size(TESTS_PATH "cavp/shake256_long_msg.data",
                               CX_SHAKE256, 32);
}

void test_shake256_variable_output(void **state __attribute__((unused)))
{
  test_cavp_sha3_xof(TESTS_PATH "cavp/shake256_variable_output.data",
                     CX_SHAKE256);
}

void test_shake256_monte_carlo(void **state __attribute__((unused)))
{
  const uint8_t seed[] = { 0x48, 0xa0, 0x32, 0x1b, 0x36, 0x53, 0xe4, 0xe8,
                           0x64, 0x46, 0xd0, 0x0f, 0x6a, 0x03, 0x6e, 0xfd };
  const uint8_t result[] = { 0xd4, 0xc8, 0xc2, 0x6d, 0xed, 0x38, 0xcc, 0xa4,
                             0x26, 0xd8, 0xd1, 0xc8, 0xf8, 0xae, 0xdb, 0x5c,
                             0x54, 0x35, 0x41, 0x33, 0x38, 0x39, 0xde, 0xca,
                             0x87, 0x13, 0xcf, 0xd8, 0x68, 0x44, 0x80, 0xfe,
                             0x92, 0x3f, 0x57, 0xc3, 0xa5, 0xc8, 0x9c, 0xb6,
                             0x14, 0x27, 0xc2, 0x20, 0xc7 };
  test_cavp_sha3_xof_monte_carlo(CX_SHAKE256, seed, result, sizeof(result),
                                 16 / 8, 2000 / 8);
}

void test_keccak_224_short_msg(void **state __attribute__((unused)))
{
  test_cavp_short_msg_with_size(TESTS_PATH "cavp/keccak_224_short_msg.data",
                                CX_KECCAK, 28);
}

void test_keccak_224_long_msg(void **state __attribute__((unused)))
{
  test_cavp_long_msg_with_size(TESTS_PATH "cavp/keccak_224_long_msg.data",
                               CX_KECCAK, 28);
}

void test_keccak_256_short_msg(void **state __attribute__((unused)))
{
  test_cavp_short_msg_with_size(TESTS_PATH "cavp/keccak_256_short_msg.data",
                                CX_KECCAK, 32);
}

void test_keccak_256_long_msg(void **state __attribute__((unused)))
{
  test_cavp_long_msg_with_size(TESTS_PATH "cavp/keccak_256_long_msg.data",
                               CX_KECCAK, 32);
}

void test_keccak_384_short_msg(void **state __attribute__((unused)))
{
  test_cavp_short_msg_with_size(TESTS_PATH "cavp/keccak_384_short_msg.data",
                                CX_KECCAK, 48);
}

void test_keccak_384_long_msg(void **state __attribute__((unused)))
{
  test_cavp_long_msg_with_size(TESTS_PATH "cavp/keccak_384_long_msg.data",
                               CX_KECCAK, 48);
}

void test_keccak_512_short_msg(void **state __attribute__((unused)))
{
  test_cavp_short_msg_with_size(TESTS_PATH "cavp/keccak_512_short_msg.data",
                                CX_KECCAK, 64);
}

void test_keccak_512_long_msg(void **state __attribute__((unused)))
{
  test_cavp_long_msg_with_size(TESTS_PATH "cavp/keccak_512_long_msg.data",
                               CX_KECCAK, 64);
}

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_sha3_224_short_msg),
    cmocka_unit_test(test_sha3_224_long_msg),
    cmocka_unit_test(test_sha3_224_monte_carlo),
    cmocka_unit_test(test_sha3_256_short_msg),
    cmocka_unit_test(test_sha3_256_long_msg),
    cmocka_unit_test(test_sha3_256_monte_carlo),
    cmocka_unit_test(test_sha3_384_short_msg),
    cmocka_unit_test(test_sha3_384_long_msg),
    cmocka_unit_test(test_sha3_384_monte_carlo),
    cmocka_unit_test(test_sha3_512_short_msg),
    cmocka_unit_test(test_sha3_512_long_msg),
    cmocka_unit_test(test_sha3_512_monte_carlo),
    cmocka_unit_test(test_shake128_short_msg),
    cmocka_unit_test(test_shake128_long_msg),
    cmocka_unit_test(test_shake128_variable_output),
    cmocka_unit_test(test_shake128_monte_carlo),
    cmocka_unit_test(test_shake256_short_msg),
    cmocka_unit_test(test_shake256_long_msg),
    cmocka_unit_test(test_shake256_variable_output),
    cmocka_unit_test(test_shake256_monte_carlo),
    cmocka_unit_test(test_keccak_224_short_msg),
    cmocka_unit_test(test_keccak_224_long_msg),
    cmocka_unit_test(test_keccak_256_short_msg),
    cmocka_unit_test(test_keccak_256_long_msg),
    cmocka_unit_test(test_keccak_384_short_msg),
    cmocka_unit_test(test_keccak_384_long_msg),
    cmocka_unit_test(test_keccak_512_short_msg),
    cmocka_unit_test(test_keccak_512_long_msg)
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
