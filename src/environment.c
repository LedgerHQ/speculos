#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "bolos/cx.h"
#include "bolos/endorsement.h"
#include "bolos/exception.h"
#include "emulate.h"
#include "environment.h"

/* SEED VARIABLES */

/* glory promote mansion idle axis finger extra february uncover one trip
 * resource lawn turtle enact monster seven myth punch hobby comfort wild raise
 * skin */
static const uint8_t default_seed[MAX_SEED_SIZE] =
    "\xb1\x19\x97\xfa\xff\x42\x0a\x33\x1b\xb4\xa4\xff\xdc\x8b\xdc\x8b\xa7\xc0"
    "\x17\x32\xa9\x9a\x30\xd8\x3d\xbb\xeb\xd4\x69\x66\x6c\x84\xb4\x7d\x09\xd3"
    "\xf5\xf4\x72\xb3\xb9\x38\x4a\xc6\x34\xbe\xba\x2a\x44\x0b\xa3\x6e\xc7\x66"
    "\x11\x44\x13\x2f\x35\xe2\x06\x87\x35\x64";

static const char SEED_ENV_NAME[] = "SPECULOS_SEED";

static struct {
  size_t size;
  uint8_t seed[MAX_SEED_SIZE];
} actual_seed = { 0 };

/* APP NAME and VERSION */
static const char APP_NAME_VERSION_ENV_NAME[] = "SPECULOS_APPNAME";
static const char APP_NAME_VERSION_ENV_NAME_BKP[] = "SPECULOS_DETECTED_APPNAME";

static env_sized_name_t app_name = { 3, "app\0" };
static env_sized_name_t app_version = { 6, "1.33.7\0" };

/* RNG VARIABLES */

static const char RNG_ENV_NAME[] = "RNG_SEED";
static unsigned int actual_rng = 0;

/* ENDORSEMENT VARIABLES */

static const char USER_KEY_ENV_NAME[] = "USER_PRIVATE_KEY";
static const char ATTESTATION_ENV_NAME[] = "ATTESTATION_PRIVATE_KEY";

static const uint8_t default_attestation_key[32] = {
  0x13, 0x8f, 0xb9, 0xb9, 0x1d, 0xa7, 0x45, 0xf1, 0x29, 0x77, 0xa2,
  0xb4, 0x6f, 0x0b, 0xce, 0x2f, 0x04, 0x18, 0xb5, 0x0f, 0xcb, 0x76,
  0x63, 0x1b, 0xaf, 0x0f, 0x08, 0xce, 0xef, 0xdb, 0x5d, 0x57
};

static const uint8_t default_user_private_key[32] = {
  0xe1, 0x5e, 0x01, 0xd4, 0x70, 0x82, 0xf0, 0xea, 0x47, 0x71, 0xc9,
  0x9f, 0xe3, 0x12, 0xf9, 0xd7, 0x00, 0x93, 0xc8, 0x9a, 0xf4, 0x77,
  0x87, 0xfd, 0xf8, 0x2e, 0x03, 0x1f, 0x67, 0x28, 0xb7, 0x10
};

static cx_ecfp_private_key_t attestation_key = { CX_CURVE_256K1, 32, {} };
static cx_ecfp_private_key_t user_private_key_1 = { CX_CURVE_256K1, 32, {} };
static cx_ecfp_private_key_t user_private_key_2 = { CX_CURVE_256K1, 32, {} };
static env_user_certificate_t user_certificate_1 = { 0 };
static env_user_certificate_t user_certificate_2 = { 0 };

/* UTILS */

static int unhex(uint8_t *dst, size_t dst_size, const char *src,
                 size_t src_size)
{
  unsigned int i;
  uint8_t acc;
  int8_t c;

  acc = 0;
  for (i = 0; i < src_size && (i >> 1) < dst_size; i++) {
    c = src[i];
    switch (c) {
    case '0' ... '9':
      acc = (acc << 4) + c - '0';
      break;
    case 'a' ... 'f':
      acc = (acc << 4) + c - 'a' + 10;
      break;
    case 'A' ... 'F':
      acc = (acc << 4) + c - 'A' + 10;
      break;
    default:
      return -1;
    }

    if (i % 2 != 0) {
      dst[i >> 1] = acc;
      acc = 0;
    }
  }

  if (i != src_size) {
    return -1;
  }

  return src_size / 2;
}

static void env_init_seed()
{
  ssize_t size;
  char *p;

  p = getenv(SEED_ENV_NAME);
  if (p != NULL) {
    size = unhex(actual_seed.seed, sizeof(actual_seed.seed), p, strlen(p));
    if (size < 0) {
      warnx("invalid seed passed through %s environment variable",
            SEED_ENV_NAME);
      p = NULL;
    } else {
      fprintf(stderr, "[*] Seed initialized from environment\n");
    }
  }

  if (p == NULL) {
    warnx("using default seed");
    memcpy(actual_seed.seed, default_seed, sizeof(default_seed));
    size = sizeof(default_seed);
  }
  actual_seed.size = size;
}

size_t env_get_seed(uint8_t *seed, size_t max_size)
{
  memcpy(seed, actual_seed.seed, max_size);
  return (actual_seed.size < max_size) ? actual_seed.size : max_size;
}

static void env_init_rng()
{
  char *p;
  p = getenv(RNG_ENV_NAME);
  if (p != NULL) {
    actual_rng = atoi(p);
    fprintf(stderr, "[*] RNG initialized from environment: '%u'\n", actual_rng);
  } else {
    actual_rng = time(NULL);
  }
}

unsigned int env_get_rng()
{
  return actual_rng;
}

static void env_init_user_hex_private_key(const char *ENV_NAME,
                                          cx_ecfp_private_key_t *dst,
                                          const uint8_t *default_key)
{
  ssize_t size;
  char *p;
  uint8_t tmp[dst->d_len];

  p = getenv(ENV_NAME);
  if (p != NULL) {
    size = unhex(tmp, dst->d_len, p, strlen(p));
    if (size < 0) {
      warnx("invalid  user key passed through %s environment variable: not an "
            "hex string",
            ENV_NAME);
      p = NULL;
    } else if ((unsigned int)size != dst->d_len) {
      warnx("invalid size for user key passed through %s environment variable: "
            "expecting '%u', got '%i'",
            ENV_NAME, dst->d_len, size);
      p = NULL;
    }
  }

  if (p == NULL) {
    memcpy(dst->d, default_key, dst->d_len);
  } else {
    memcpy(dst->d, tmp, dst->d_len);
    fprintf(stderr, "[*] Private key ('%s') initialized from environment\n",
            ENV_NAME);
  }
}

static void env_init_user_certificate(unsigned int index)
{
  env_user_certificate_t *certificate;
  uint8_t pkey[65] = { 0 };
  sys_os_endorsement_get_public_key(index, &pkey[0]);
  uint8_t hash[32] = { 0 };
  cx_sha256_t sha256;

  switch (index) {
  case 1:
    certificate = &user_certificate_1;
    break;
  case 2:
    certificate = &user_certificate_2;
    break;
  default:
    THROW(EXCEPTION);
    break;
  }

  sys_cx_sha256_init(&sha256);
  sys_cx_hash((cx_hash_t *)&sha256, 0, (unsigned char *)"\xfe", 1, NULL, 0);
  sys_cx_hash((cx_hash_t *)&sha256, CX_LAST, pkey, sizeof(pkey), hash,
              sizeof(hash));

  sys_cx_ecdsa_sign(&attestation_key, CX_RND_TRNG, CX_SHA256, hash,
                    sizeof(hash), certificate->buffer, MAX_CERT_SIZE, NULL);
  certificate->length = certificate->buffer[1] + 2;
}

static void env_init_endorsement()
{
  env_init_user_hex_private_key(ATTESTATION_ENV_NAME, &attestation_key,
                                default_attestation_key);
  env_init_user_hex_private_key(USER_KEY_ENV_NAME, &user_private_key_1,
                                default_user_private_key);
  env_init_user_certificate(1);
  env_init_user_hex_private_key(USER_KEY_ENV_NAME, &user_private_key_2,
                                default_user_private_key);
  env_init_user_certificate(2);
}

cx_ecfp_private_key_t *env_get_user_private_key(unsigned int index)
{
  switch (index) {
  case 1:
    return &user_private_key_1;
    break;
  case 2:
    return &user_private_key_2;
    break;
  default:
    THROW(EXCEPTION);
    break;
  }
}

env_user_certificate_t *env_get_user_certificate(unsigned int index)
{
  switch (index) {
  case 1:
    return &user_certificate_1;
    break;
  case 2:
    return &user_certificate_2;
    break;
  default:
    THROW(EXCEPTION);
    break;
  }
}

static void env_init_app_name_version()
{
  char *str;
  str = getenv(APP_NAME_VERSION_ENV_NAME);
  if (str == NULL) {
    str = getenv(APP_NAME_VERSION_ENV_NAME_BKP);
  }

  if (str == NULL) {
    warnx("using default app name & version");
    return;
  }

  char *char_ptr = strchr(str, ':');
  if (char_ptr == NULL) {
    warnx("Invalid '<name>:<version>' format in env variable '%s', falling "
          "back to default.",
          str);
    fprintf(stderr, "[*] Default app name: '%s'\n", app_name.name);
    fprintf(stderr, "[*] Default app version: '%s'\n", app_version.name);
    return;
  }

  app_name.length = (char_ptr - str);
  app_version.length = (strlen(str) - (size_t)(app_name.length + 1));
  str[app_name.length] = '\0';
  // + 1 to include trailing '\0'
  strncpy(app_name.name, str, app_name.length + 1);
  strncpy(app_version.name, str + app_name.length + 1, app_version.length + 1);

  fprintf(stderr, "[*] Env app name: '%s'\n", app_name.name);
  fprintf(stderr, "[*] Env app version: '%s'\n", app_version.name);
}

size_t env_get_app_tag(char *dst, size_t length, BOLOS_TAG tag)
{
  env_sized_name_t *field;
  switch (tag) {
  case BOLOS_TAG_APPNAME:
    field = &app_name;
    break;
  case BOLOS_TAG_APPVERSION:
    field = &app_version;
    break;
  default:
    return 0;
  }
  if (length < field->length) {
    warnx("Providing length to copy env variable too small: asked for %u, "
          "needs %u",
          length, field->length);
    return 0;
  }

  strncpy(dst, field->name, length);
  dst[field->length] = '\0';
  return field->length;
}

void init_environment()
{
  env_init_seed();
  env_init_rng();
  env_init_endorsement();
  env_init_app_name_version();
}
