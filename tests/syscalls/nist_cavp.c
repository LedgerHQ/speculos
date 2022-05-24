#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <setjmp.h>
// must come after setjmp.h
#include <cmocka.h>

#include "bolos/cx_hash.h"
#include "nist_cavp.h"
#include "utils.h"

#define CX_MAX_DIGEST_SIZE CX_SHA512_SIZE

void test_cavp_short_msg_with_size(const char *filename, cx_md_t md_type,
                                   size_t digest_size)
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
    uint8_t md[CX_MAX_DIGEST_SIZE], expected[CX_MAX_DIGEST_SIZE];
    assert_int_equal(spec_cx_hash_init_ex(&ctx, md_type, digest_size), 1);

    assert_int_equal(md_len, spec_cx_hash_get_size(&ctx));
    assert_int_equal(hexstr2bin(pos1 + 1, data, data_len), data_len);
    assert_int_equal(hexstr2bin(pos2 + 1, expected, md_len), md_len);

    assert_int_equal(spec_cx_hash_update(&ctx, data, data_len), 1);
    assert_int_equal(spec_cx_hash_final(&ctx, md), 1);
    assert_memory_equal(expected, md, md_len);
  }
  fclose(f);
}

void test_cavp_short_msg_with_single(const char *filename, single_hash_t hash,
                                     size_t digest_size)
{
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
    uint8_t md[CX_MAX_DIGEST_SIZE], expected[CX_MAX_DIGEST_SIZE];

    assert_int_equal(md_len, digest_size);
    assert_int_equal(hexstr2bin(pos1 + 1, data, data_len), data_len);
    assert_int_equal(hexstr2bin(pos2 + 1, expected, md_len), md_len);

    assert_int_equal(hash(data, data_len, md, md_len), (int)digest_size);
    assert_memory_equal(expected, md, md_len);
  }
  fclose(f);
}

#define MAX_CAVP_LINE_LENGTH 65536

void test_cavp_long_msg_with_size(const char *filename, cx_md_t md_type,
                                  size_t digest_size)
{
  cx_hash_ctx ctx;
  char *result;
  char *line = malloc(MAX_CAVP_LINE_LENGTH);
  assert_non_null(line);

  FILE *f = fopen(filename, "r");
  assert_non_null(f);

  while (!feof(f)) {
    result = fgets(line, MAX_CAVP_LINE_LENGTH, f);
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

    uint8_t *data = malloc(data_len);
    assert_non_null(data);

    uint8_t md[CX_MAX_DIGEST_SIZE], expected[CX_MAX_DIGEST_SIZE];

    assert_int_equal(spec_cx_hash_init_ex(&ctx, md_type, digest_size), 1);
    assert_int_equal(md_len, spec_cx_hash_get_size(&ctx));

    assert_int_equal(hexstr2bin(pos1 + 1, data, data_len), data_len);
    assert_int_equal(hexstr2bin(pos2 + 1, expected, md_len), md_len);

    assert_int_equal(spec_cx_hash_update(&ctx, data, data_len), 1);
    assert_int_equal(spec_cx_hash_final(&ctx, md), 1);

    assert_memory_equal(expected, md, md_len);
    free(data);
  }
  fclose(f);
  free(line);
}

void test_cavp_monte_with_size(cx_md_t md_type, uint8_t *initial_seed,
                               const uint8_t *expected_seed, size_t digest_size)
{
  cx_hash_ctx ctx;

  uint8_t md0[CX_MAX_DIGEST_SIZE], md1[CX_MAX_DIGEST_SIZE],
      md2[CX_MAX_DIGEST_SIZE];
  uint8_t tmp[CX_MAX_DIGEST_SIZE];
  uint8_t *seed = initial_seed;

  assert_int_equal(spec_cx_hash_init_ex(&ctx, md_type, digest_size), 1);
  size_t md_len = (size_t)spec_cx_hash_get_size(&ctx);

  for (int j = 0; j < 100; j++) {
    memcpy(md0, seed, md_len);
    memcpy(md1, seed, md_len);
    memcpy(md2, seed, md_len);

    for (int i = 0; i < 1000; i++) {
      assert_int_equal(spec_cx_hash_init_ex(&ctx, md_type, digest_size), 1);
      assert_int_equal(spec_cx_hash_update(&ctx, md0, md_len), 1);
      assert_int_equal(spec_cx_hash_update(&ctx, md1, md_len), 1);
      assert_int_equal(spec_cx_hash_update(&ctx, md2, md_len), 1);
      assert_int_equal(spec_cx_hash_final(&ctx, tmp), 1);

      memcpy(md0, md1, md_len);
      memcpy(md1, md2, md_len);
      memcpy(md2, tmp, md_len);
    }
    memcpy(seed, md2, md_len);
  }
  assert_memory_equal(seed, expected_seed, md_len);
}

void test_cavp_short_msg(const char *filename, cx_md_t md_type)
{
  const cx_hash_info_t *info = spec_cx_hash_get_info(md_type);
  assert_non_null(info);
  assert_int_not_equal(info->output_size, 0);
  return test_cavp_short_msg_with_size(filename, md_type, info->output_size);
}

void test_cavp_long_msg(const char *filename, cx_md_t md_type)
{
  const cx_hash_info_t *info = spec_cx_hash_get_info(md_type);
  assert_non_null(info);
  assert_int_not_equal(info->output_size, 0);
  return test_cavp_long_msg_with_size(filename, md_type, info->output_size);
}

void test_cavp_monte(cx_md_t md_type, uint8_t *initial_seed,
                     const uint8_t *expected_seed)
{
  const cx_hash_info_t *info = spec_cx_hash_get_info(md_type);
  assert_non_null(info);
  assert_int_not_equal(info->output_size, 0);
  return test_cavp_monte_with_size(md_type, initial_seed, expected_seed,
                                   info->output_size);
}
