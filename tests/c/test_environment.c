#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <cmocka.h>

#include "utils.h"

#include "bolos/cx.h"
#include "bolos/endorsement.h"
#include "environment.h"

static const char *SEED_ENV_NAME = "SPECULOS_SEED";
static const char *APP_NAME_VERSION_ENV_NAME = "SPECULOS_APPNAME";
static const char *APP_NAME_VERSION_ENV_NAME_BKP = "SPECULOS_DETECTED_APPNAME";
static const char *RNG_ENV_NAME = "RNG_SEED";
static const char *USER_KEY_ENV_NAME = "USER_PRIVATE_KEY";
static const char *ATTESTATION_ENV_NAME = "ATTESTATION_PRIVATE_KEY";
static uint8_t default_seed[MAX_SEED_SIZE] =
    "\xb1\x19\x97\xfa\xff\x42\x0a\x33\x1b\xb4\xa4\xff\xdc\x8b\xdc\x8b\xa7\xc0"
    "\x17\x32\xa9\x9a\x30\xd8\x3d\xbb\xeb\xd4\x69\x66\x6c\x84\xb4\x7d\x09\xd3"
    "\xf5\xf4\x72\xb3\xb9\x38\x4a\xc6\x34\xbe\xba\x2a\x44\x0b\xa3\x6e\xc7\x66"
    "\x11\x44\x13\x2f\x35\xe2\x06\x87\x35\x64";
static char *default_app_name = "app";
static char *default_app_version = "1.33.7";

static uint8_t default_user_private_key[KEY_LENGTH] = {
  0xe1, 0x5e, 0x01, 0xd4, 0x70, 0x82, 0xf0, 0xea, 0x47, 0x71, 0xc9,
  0x9f, 0xe3, 0x12, 0xf9, 0xd7, 0x00, 0x93, 0xc8, 0x9a, 0xf4, 0x77,
  0x87, 0xfd, 0xf8, 0x2e, 0x03, 0x1f, 0x67, 0x28, 0xb7, 0x10
};
#define MAX_CERT_LENGTH 75
static const uint8_t default_user_certificate[MAX_CERT_LENGTH] = {
  0x30, 0x45, 0x02, 0x21, 0x00, 0xbf, 0x23, 0x7e, 0x5b, 0x40, 0x06, 0x14,
  0x17, 0xf6, 0x62, 0xa6, 0xd0, 0x8a, 0x4b, 0xde, 0x1f, 0xe3, 0x34, 0x3b,
  0xd8, 0x70, 0x8c, 0xed, 0x04, 0x6c, 0x84, 0x17, 0x49, 0x5a, 0xd3, 0x6c,
  0xcf, 0x02, 0x20, 0x3d, 0x39, 0xa5, 0x32, 0xee, 0xca, 0xdf, 0xf6, 0xdf,
  0x20, 0x53, 0xe4, 0xab, 0x98, 0x96, 0xaa, 0x00, 0xf3, 0xbe, 0xf1, 0x5c,
  0x4b, 0xd1, 0x1c, 0x53, 0x66, 0x1e, 0x54, 0xfe, 0x5e, 0x2f, 0xf4
};

typedef enum {
  FIELD_SEED,
  FIELD_RNG,
  FIELD_USER_KEY,
  FIELD_APPNAME,
  FIELD_APPVERSION
} field_e;

static void check_is_default(field_e field)
{
  switch (field) {
  case FIELD_SEED: {
    uint8_t key[MAX_SEED_SIZE] = {};
    env_get_seed(&key[0], MAX_SEED_SIZE);
    assert_memory_equal(&key[0], default_seed, MAX_SEED_SIZE);
    break;
  }
  case FIELD_RNG: {
    unsigned int now = time(NULL);
    // RNG is initialized in the past, not that long ago, but still
    // assert_int_equal(env_get_rng(), time(NULL)); can sometimes fail
    assert_in_range(env_get_rng(), now - 1, now);
    break;
  }
  case FIELD_USER_KEY: {
    assert_memory_equal(env_get_user_private_key(1)->d,
                        default_user_private_key,
                        env_get_user_private_key(1)->d_len);
    assert_memory_equal(env_get_user_private_key(2)->d,
                        default_user_private_key,
                        env_get_user_private_key(2)->d_len);
    assert_memory_equal(env_get_user_certificate(1)->buffer,
                        default_user_certificate,
                        env_get_user_certificate(1)->length);
    assert_memory_equal(env_get_user_certificate(2)->buffer,
                        default_user_certificate,
                        env_get_user_certificate(2)->length);
    break;
  }
  case FIELD_APPNAME: {
    char buffer[10];
    size_t size = env_get_app_tag(buffer, 10, BOLOS_TAG_APPNAME);
    assert_string_equal(buffer, default_app_name);
    assert_int_equal(size, strlen(buffer));
    break;
  }
  case FIELD_APPVERSION: {
    char buffer[10];
    size_t size = env_get_app_tag(buffer, 10, BOLOS_TAG_APPVERSION);
    assert_string_equal(buffer, default_app_version);
    assert_int_equal(size, strlen(buffer));
    break;
  }
  default:
    assert_true(false);
    break;
  }
}

static void test_complete_default(void **state __attribute__((unused)))
{
  init_environment();
  check_is_default(FIELD_SEED);
  check_is_default(FIELD_RNG);
  check_is_default(FIELD_USER_KEY);
  check_is_default(FIELD_APPNAME);
  check_is_default(FIELD_APPVERSION);
}

static void test_change_seed(void **state __attribute__((unused)))
{
  char *seed = "0123456789abcdef";
  uint8_t expected_seed[8];
  uint8_t result_seed[8];
  hexstr2bin(seed, expected_seed, sizeof(expected_seed));

  setenv(SEED_ENV_NAME, seed, true);
  init_environment();

  env_get_seed(result_seed, 8);
  assert_memory_equal(expected_seed, result_seed, 8);

  check_is_default(FIELD_RNG);
  check_is_default(FIELD_USER_KEY);
  check_is_default(FIELD_APPNAME);
  check_is_default(FIELD_APPVERSION);
}

static void test_change_rng(void **state __attribute__((unused)))
{
  char *rng_str = "7654";
  int rng_int = atoi(rng_str);

  setenv(RNG_ENV_NAME, rng_str, true);

  init_environment();
  assert_int_equal(env_get_rng(), rng_int);

  check_is_default(FIELD_SEED);
  check_is_default(FIELD_USER_KEY);
  check_is_default(FIELD_APPNAME);
  check_is_default(FIELD_APPVERSION);
}

static void test_change_attestation(void **state __attribute__((unused)))
{
  setenv(ATTESTATION_ENV_NAME,
         "0123456789000000000000000000000000000000000000000000000000abcdef",
         true);
  init_environment();

  assert_memory_equal(env_get_user_private_key(1)->d, default_user_private_key,
                      env_get_user_private_key(1)->d_len);
  assert_memory_equal(env_get_user_private_key(2)->d, default_user_private_key,
                      env_get_user_private_key(2)->d_len);
  assert_memory_not_equal(env_get_user_certificate(1)->buffer,
                          default_user_certificate,
                          env_get_user_certificate(1)->length);
  assert_memory_not_equal(env_get_user_certificate(2)->buffer,
                          default_user_certificate,
                          env_get_user_certificate(2)->length);

  check_is_default(FIELD_SEED);
  check_is_default(FIELD_RNG);
  check_is_default(FIELD_APPNAME);
  check_is_default(FIELD_APPVERSION);
}

static void test_change_user_key(void **state __attribute__((unused)))
{
  setenv(USER_KEY_ENV_NAME,
         "abcdef0000000000000000000000000000000000000000000000000123456789",
         true);
  init_environment();

  assert_memory_not_equal(env_get_user_private_key(1)->d,
                          default_user_private_key,
                          env_get_user_private_key(1)->d_len);
  assert_memory_not_equal(env_get_user_private_key(2)->d,
                          default_user_private_key,
                          env_get_user_private_key(2)->d_len);
  assert_memory_not_equal(env_get_user_certificate(1)->buffer,
                          default_user_certificate,
                          env_get_user_certificate(1)->length);
  assert_memory_not_equal(env_get_user_certificate(2)->buffer,
                          default_user_certificate,
                          env_get_user_certificate(2)->length);

  check_is_default(FIELD_SEED);
  check_is_default(FIELD_RNG);
  check_is_default(FIELD_APPNAME);
  check_is_default(FIELD_APPVERSION);
}

static void test_change_appname_appversion(void **state __attribute__((unused)))
{
  char *name = "some_app";
  char *version = "some_version";
  // + 1 for ':' then + 1 for '\0'
  char value[strlen(name) + strlen(version) + 1 + 1];
  snprintf(value, sizeof(value), "%s:%s", name, version);

  setenv(APP_NAME_VERSION_ENV_NAME, value, true);
  init_environment();

  char buffer[strlen(name) < strlen(version) ? strlen(version) : strlen(name)];
  size_t str_len = 0;
  str_len = env_get_app_tag(buffer, sizeof(buffer), BOLOS_TAG_APPNAME);
  assert_string_equal(buffer, name);
  assert_int_equal(str_len, strlen(name));
  str_len = env_get_app_tag(buffer, sizeof(buffer), BOLOS_TAG_APPVERSION);
  assert_string_equal(buffer, version);
  assert_int_equal(str_len, strlen(version));

  check_is_default(FIELD_SEED);
  check_is_default(FIELD_RNG);
  check_is_default(FIELD_USER_KEY);
}

static int setup(void **state __attribute__((unused)))
{
  unsetenv(SEED_ENV_NAME);
  unsetenv(APP_NAME_VERSION_ENV_NAME);
  unsetenv(APP_NAME_VERSION_ENV_NAME_BKP);
  unsetenv(RNG_ENV_NAME);
  unsetenv(USER_KEY_ENV_NAME);
  unsetenv(ATTESTATION_ENV_NAME);
  return 0;
}

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test_setup(test_complete_default, setup),
    cmocka_unit_test_setup(test_change_seed, setup),
    cmocka_unit_test_setup(test_change_rng, setup),
    cmocka_unit_test_setup(test_change_attestation, setup),
    cmocka_unit_test_setup(test_change_user_key, setup),
    cmocka_unit_test_setup(test_change_appname_appversion, setup),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
