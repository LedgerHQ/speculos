#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <setjmp.h>
// must come after setjmp.h
#include <cmocka.h>

#include "utils.h"

#include "bolos/cx.h"
#include "bolos/cx_hmac.h"
#include "emulate.h"

void cx_scc_struct_check_hashmac(const cx_hmac_t *UNUSED(hmac))
{
}

#define MAX_CAVP_LINE_LENGTH 65536
#define CX_MAX_DIGEST_SIZE   CX_SHA512_SIZE

static cx_md_t hmac_get_hash_id_from_name(const char *name)
{
  cx_md_t md_type = CX_NONE;

  if (strcmp(name, "hmac-sha224") == 0) {
    md_type = CX_SHA224;
  } else if (strcmp(name, "hmac-sha256") == 0) {
    md_type = CX_SHA256;
  } else if (strcmp(name, "hmac-sha384") == 0) {
    md_type = CX_SHA384;
  } else if (strcmp(name, "hmac-sha512") == 0) {
    md_type = CX_SHA512;
  }

  return md_type;
}

void test_cavp_long_msg_with_size(const char *filename)
{
  cx_hmac_ctx ctx;
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
    char *pos3 = strchr(pos2 + 1, ':');
    assert_non_null(pos3);
    char *pos4 = strchr(pos3 + 1, '\n');

    *pos1 = *pos2 = *pos3 = 0;
    if (pos4) {
      *pos4 = 0;
    }

    size_t key_len = strlen(pos1 + 1) / 2;
    size_t data_len = strlen(pos2 + 1) / 2;
    size_t mac_len = strlen(pos3 + 1) / 2;
    size_t returned_mac_len;

    uint8_t *data = malloc(data_len);
    assert_non_null(data);
    uint8_t *key = malloc(key_len);
    assert_non_null(key);

    uint8_t mac[CX_MAX_DIGEST_SIZE], expected[CX_MAX_DIGEST_SIZE];

    assert_int_equal(hexstr2bin(pos1 + 1, key, key_len), key_len);
    assert_int_equal(hexstr2bin(pos2 + 1, data, data_len), data_len);
    assert_int_equal(hexstr2bin(pos3 + 1, expected, mac_len), mac_len);

    cx_md_t md = hmac_get_hash_id_from_name(line);
    assert_int_not_equal(md, CX_NONE);

    returned_mac_len = mac_len;
    assert_int_equal(spec_cx_hmac_init(&ctx, md, key, key_len), 1);
    assert_int_equal(spec_cx_hmac_update(&ctx, data, data_len), 1);
    assert_int_equal(spec_cx_hmac_final(&ctx, mac, &returned_mac_len), 1);

    assert_int_equal(returned_mac_len, mac_len);
    assert_memory_equal(mac, expected, mac_len);
    free(data);
  }
  fclose(f);
  free(line);
}

void test_cavp_long_msg_with_size_old_api(const char *filename)
{
  char *result;
  char *line = malloc(MAX_CAVP_LINE_LENGTH);
  assert_non_null(line);

  cx_hmac_sha256_t hmac_sha256_ctx;
  cx_hmac_sha512_t hmac_sha512_ctx;

  FILE *f = fopen(filename, "r");
  assert_non_null(f);

  while (!feof(f)) {
    result = fgets(line, MAX_CAVP_LINE_LENGTH, f);
    assert_non_null(result);
    char *pos1 = strchr(line, ':');
    assert_non_null(pos1);
    char *pos2 = strchr(pos1 + 1, ':');
    assert_non_null(pos2);
    char *pos3 = strchr(pos2 + 1, ':');
    assert_non_null(pos3);
    char *pos4 = strchr(pos3 + 1, '\n');

    *pos1 = *pos2 = *pos3 = 0;
    if (pos4) {
      *pos4 = 0;
    }

    size_t key_len = strlen(pos1 + 1) / 2;
    size_t data_len = strlen(pos2 + 1) / 2;
    size_t mac_len = strlen(pos3 + 1) / 2;

    uint8_t *data = malloc(data_len);
    assert_non_null(data);
    uint8_t *key = malloc(key_len);
    assert_non_null(key);

    uint8_t mac[CX_MAX_DIGEST_SIZE], expected[CX_MAX_DIGEST_SIZE];

    assert_int_equal(hexstr2bin(pos1 + 1, key, key_len), key_len);
    assert_int_equal(hexstr2bin(pos2 + 1, data, data_len), data_len);
    assert_int_equal(hexstr2bin(pos3 + 1, expected, mac_len), mac_len);

    cx_md_t md = hmac_get_hash_id_from_name(line);
    assert_int_not_equal(md, CX_NONE);

    if (md == CX_SHA256) {
      assert_int_equal(cx_hmac_sha256_init(&hmac_sha256_ctx, key, key_len),
                       CX_SHA256);
      assert_int_equal(
          cx_hmac(&hmac_sha256_ctx, CX_LAST, data, data_len, mac, mac_len),
          mac_len);
      assert_memory_equal(mac, expected, mac_len);

      assert_int_equal(cx_hmac_sha256_init(&hmac_sha256_ctx, key, key_len),
                       CX_SHA256);
      assert_int_equal(cx_hmac(&hmac_sha256_ctx, 0, data, data_len, NULL, 0),
                       0);
      assert_int_equal(
          cx_hmac(&hmac_sha256_ctx, CX_LAST, NULL, 0, mac, mac_len), mac_len);
      assert_memory_equal(mac, expected, mac_len);
    } else if (md == CX_SHA512) {
      assert_int_equal(cx_hmac_sha512_init(&hmac_sha512_ctx, key, key_len),
                       CX_SHA512);
      assert_int_equal(
          cx_hmac(&hmac_sha512_ctx, CX_LAST, data, data_len, mac, mac_len),
          mac_len);
      assert_memory_equal(mac, expected, mac_len);

      assert_int_equal(cx_hmac_sha512_init(&hmac_sha512_ctx, key, key_len),
                       CX_SHA512);
      assert_int_equal(cx_hmac(&hmac_sha512_ctx, 0, data, data_len, NULL, 0),
                       0);
      assert_int_equal(
          cx_hmac(&hmac_sha512_ctx, CX_LAST, NULL, 0, mac, mac_len), mac_len);
      assert_memory_equal(mac, expected, mac_len);
    }

    free(data);
  }
  fclose(f);
  free(line);
}

void test_hmac_sha256_old_api(void **state __attribute__((unused)))
{
  test_cavp_long_msg_with_size_old_api(TESTS_PATH "cavp/hmac.data");
}

void test_hmac_sha2(void **state __attribute__((unused)))
{
  test_cavp_long_msg_with_size(TESTS_PATH "cavp/hmac.data");
}

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_hmac_sha2), cmocka_unit_test(test_hmac_sha256_old_api)
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
