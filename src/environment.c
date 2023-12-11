#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "bolos/cx.h"
#include "bolos/endorsement.h"
#include "bolos/exception.h"
#include "emulate.h"
#include "environment.h"

/* SEED VARIABLES */

/* glory promote mansion idle axis finger extra february uncover one trip
 * resource lawn turtle enact monster seven myth punch hobby comfort wild raise
 * skin */
static uint8_t default_seed[MAX_SEED_SIZE] =
    "\xb1\x19\x97\xfa\xff\x42\x0a\x33\x1b\xb4\xa4\xff\xdc\x8b\xdc\x8b\xa7\xc0"
    "\x17\x32\xa9\x9a\x30\xd8\x3d\xbb\xeb\xd4\x69\x66\x6c\x84\xb4\x7d\x09\xd3"
    "\xf5\xf4\x72\xb3\xb9\x38\x4a\xc6\x34\xbe\xba\x2a\x44\x0b\xa3\x6e\xc7\x66"
    "\x11\x44\x13\x2f\x35\xe2\x06\x87\x35\x64";

static const char *SEED_ENV_NAME = "SPECULOS_SEED";

static struct {
  size_t size;
  uint8_t seed[MAX_SEED_SIZE];
} actual_seed = { 0 };

/* RNG VARIABLES */

static const char *RNG_ENV_NAME = "RNG_SEED";
static unsigned int actual_rng = 0;

/* ENDORSEMENT VARIABLES */

static const char *USER_KEY_ENV_NAME = "USER_PRIVATE_KEY";
static const char *ATTESTATION_ENV_NAME = "ATTESTATION_PRIVATE_KEY";

static cx_ecfp_private_key_t attestation_key = {
    CX_CURVE_256K1,
    32,
    { 0x13, 0x8f, 0xb9, 0xb9, 0x1d, 0xa7, 0x45, 0xf1, 0x29, 0x77, 0xa2,
      0xb4, 0x6f, 0x0b, 0xce, 0x2f, 0x04, 0x18, 0xb5, 0x0f, 0xcb, 0x76,
      0x63, 0x1b, 0xaf, 0x0f, 0x08, 0xce, 0xef, 0xdb, 0x5d, 0x57},
};

static cx_ecfp_private_key_t user_private_key_1 = {
  CX_CURVE_256K1,
  32,
  { 0xe1, 0x5e, 0x01, 0xd4, 0x70, 0x82, 0xf0, 0xea, 0x47, 0x71, 0xc9,
    0x9f, 0xe3, 0x12, 0xf9, 0xd7, 0x00, 0x93, 0xc8, 0x9a, 0xf4, 0x77,
    0x87, 0xfd, 0xf8, 0x2e, 0x03, 0x1f, 0x67, 0x28, 0xb7, 0x10 },
};

static cx_ecfp_private_key_t user_private_key_2 = {
  CX_CURVE_256K1,
  32,
  { 0xe1, 0x5e, 0x01, 0xd4, 0x70, 0x82, 0xf0, 0xea, 0x47, 0x71, 0xc9,
    0x9f, 0xe3, 0x12, 0xf9, 0xd7, 0x00, 0x93, 0xc8, 0x9a, 0xf4, 0x77,
    0x87, 0xfd, 0xf8, 0x2e, 0x03, 0x1f, 0x67, 0x28, 0xb7, 0x10 },
};

static env_user_certificate_t user_certificate_1 = {
  71,
  // user_private_key_1 signed by test owner private key
  // "138fb9b91da745f12977a2b46f0bce2f0418b50fcb76631baf0f08ceefdb5d57"
  {  0x30, 0x45, 0x02, 0x21, 0x00, 0xbf, 0x23, 0x7e, 0x5b, 0x40, 0x06, 0x14,
     0x17, 0xf6, 0x62, 0xa6, 0xd0, 0x8a, 0x4b, 0xde, 0x1f, 0xe3, 0x34, 0x3b,
     0xd8, 0x70, 0x8c, 0xed, 0x04, 0x6c, 0x84, 0x17, 0x49, 0x5a, 0xd3, 0x6c,
     0xcf, 0x02, 0x20, 0x3d, 0x39, 0xa5, 0x32, 0xee, 0xca, 0xdf, 0xf6, 0xdf,
     0x20, 0x53, 0xe4, 0xab, 0x98, 0x96, 0xaa, 0x00, 0xf3, 0xbe, 0xf1, 0x5c,
     0x4b, 0xd1, 0x1c, 0x53, 0x66, 0x1e, 0x54, 0xfe, 0x5e, 0x2f, 0xf4 },
};

static env_user_certificate_t user_certificate_2 = {
  71,
  // user_private_key_2 signed by test owner private key
  // "138fb9b91da745f12977a2b46f0bce2f0418b50fcb76631baf0f08ceefdb5d57"
  { 0x30, 0x45, 0x02, 0x21, 0x00, 0xbf, 0x23, 0x7e, 0x5b, 0x40, 0x06, 0x14,
    0x17, 0xf6, 0x62, 0xa6, 0xd0, 0x8a, 0x4b, 0xde, 0x1f, 0xe3, 0x34, 0x3b,
    0xd8, 0x70, 0x8c, 0xed, 0x04, 0x6c, 0x84, 0x17, 0x49, 0x5a, 0xd3, 0x6c,
    0xcf, 0x02, 0x20, 0x3d, 0x39, 0xa5, 0x32, 0xee, 0xca, 0xdf, 0xf6, 0xdf,
    0x20, 0x53, 0xe4, 0xab, 0x98, 0x96, 0xaa, 0x00, 0xf3, 0xbe, 0xf1, 0x5c,
    0x4b, 0xd1, 0x1c, 0x53, 0x66, 0x1e, 0x54, 0xfe, 0x5e, 0x2f, 0xf4
  },
};

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
      fprintf(stderr, "[*] Seed initialized from environment: '%s'\n", p);
    }
  }

  if (p == NULL) {
    warnx("using default seed");
    memcpy(actual_seed.seed, default_seed, sizeof(default_seed));
    size = sizeof(default_seed);
    fprintf(stderr, "[*] Seed initialized from default value: '%s'\n",
            default_seed);
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
    fprintf(stderr, "[*] RNG initialized by default (time(NULL)): '%u'\n",
            actual_rng);
  }
}

unsigned int env_get_rng()
{
  return actual_rng;
}

static bool env_init_user_hex_private_key(const char *ENV_NAME, cx_ecfp_private_key_t *dst)
{
  ssize_t size;
  char *p;
  unsigned char tmp[dst->d_len];

  p = getenv(ENV_NAME);
  if (p != NULL) {
    size = unhex(tmp, dst->d_len, p, strlen(p));
    if (size < 0) {
      warnx(
          "invalid  user key passed through %s environment variable: not an hex string",
          USER_KEY_ENV_NAME);
      p = NULL;
    } else if ((unsigned int)size != dst->d_len) {
      warnx(
          "invalid size for user key passed through %s environment variable: expecting '%u', got '%i'",
          USER_KEY_ENV_NAME, dst->d_len, size);
      p = NULL;
    }
  }

  if (p == NULL) {
      fprintf(stderr, "[*] Private key ('%s') initialized with default value: '%p'\n",
              ENV_NAME, dst->d);
      return false;
  } else {
    memcpy(dst->d, tmp, dst->d_len);
    fprintf(stderr,
            "[*] Private key ('%s') initialized from environment: '%s'\n",
            ENV_NAME, p);
    return true;
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
  sys_cx_hash((cx_hash_t *)&sha256, CX_LAST, pkey, sizeof(pkey),
              hash, sizeof(hash));
  sys_cx_ecdsa_sign(&attestation_key, CX_RND_TRNG, CX_SHA256,
                    hash, sizeof(hash),
                    certificate->buffer, 6 + 33 * 2,
                    NULL);
  certificate->length = certificate->buffer[1] + 2;

  fprintf(stderr, "[*] User certificate %u initialized (size: %u)\n",
          index, certificate->length);
}

static void env_init_endorsement()
{
  bool attestation_changed =
      env_init_user_hex_private_key(ATTESTATION_ENV_NAME, &attestation_key);
  if (env_init_user_hex_private_key(USER_KEY_ENV_NAME, &user_private_key_1) ||
      attestation_changed) {
    env_init_user_certificate(1);
  }
  if (env_init_user_hex_private_key(USER_KEY_ENV_NAME, &user_private_key_2) ||
      attestation_changed) {
    env_init_user_certificate(2);
  }
}

cx_ecfp_private_key_t* env_get_user_private_key(unsigned int index)
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


env_user_certificate_t* env_get_user_certificate(unsigned int index)
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

void init_environment()
{
    env_init_seed();
    env_init_rng();
    env_init_endorsement();
}
