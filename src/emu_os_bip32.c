#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "cx.h"
#include "cx_ec.h"
#include "cx_math.h"
#include "emulate.h"
#include "exception.h"

#define BIP32_HARDEN_MASK      0x80000000
#define BIP32_SECP_SEED_LENGTH 12

#define cx_ecfp_generate_pair     sys_cx_ecfp_generate_pair
#define cx_ecfp_init_private_key  sys_cx_ecfp_init_private_key
#define cx_ecdsa_init_private_key cx_ecfp_init_private_key

typedef struct {
  cx_ecfp_256_private_key_t private_key;
  uint8_t chain_code[32];
} extended_private_key;

/* glory promote mansion idle axis finger extra february uncover one trip resource lawn turtle enact monster seven myth punch hobby comfort wild raise skin */
static uint8_t default_seed[64] = "\xb1\x19\x97\xfa\xff\x42\x0a\x33\x1b\xb4\xa4\xff\xdc\x8b\xdc\x8b\xa7\xc0\x17\x32\xa9\x9a\x30\xd8\x3d\xbb\xeb\xd4\x69\x66\x6c\x84\xb4\x7d\x09\xd3\xf5\xf4\x72\xb3\xb9\x38\x4a\xc6\x34\xbe\xba\x2a\x44\x0b\xa3\x6e\xc7\x66\x11\x44\x13\x2f\x35\xe2\x06\x87\x35\x64";

static uint8_t const BIP32_SECP_SEED[] = {
	'B', 'i', 't', 'c', 'o', 'i', 'n', ' ', 's', 'e', 'e', 'd'
};

static uint8_t const BIP32_NIST_SEED[] = {
	'N', 'i', 's', 't', '2', '5', '6', 'p', '1', ' ', 's', 'e', 'e', 'd'
};

static void expand_seed_bip32(cx_curve_t curve, uint8_t *seed, unsigned int seed_length, uint8_t *result, const cx_curve_domain_t *domain)
{
  const uint8_t *seed_key;
  size_t seed_key_length;

  switch (curve) {
  case CX_CURVE_256K1:
    seed_key = BIP32_SECP_SEED;
    seed_key_length = sizeof(BIP32_SECP_SEED);
    break;
  case CX_CURVE_256R1:
    seed_key = BIP32_NIST_SEED;
    seed_key_length = sizeof(BIP32_NIST_SEED);
    break;

  default:
    errx(1, "expand_seed_bip32: unsupported curve");
    THROW(EXCEPTION);
    break;
  }

  cx_hmac_sha512(seed_key, seed_key_length, seed, seed_length, result, CX_SHA512_SIZE);

  while (cx_math_is_zero(result, 32) || (cx_math_cmp(result, domain->n, 32) >= 0)) {
    cx_hmac_sha512(seed_key, seed_key_length, result, CX_SHA512_SIZE, result, CX_SHA512_SIZE);
  }
}

static int unhex(uint8_t *dst, size_t dst_size, char *src, size_t src_size)
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
        printf("[%02x]\n", c);
        return -1;
		}

		if (i % 2 != 0) {
      dst[i >> 1] = acc;
      acc = 0;
    }
	}

  if (i != src_size)
    return -1;

	return src_size / 2;
}

static size_t get_seed_from_env(const char *name, uint8_t *seed, size_t max_size)
{
  ssize_t seed_size;
  char *p;

  p = getenv(name);
  if (p != NULL) {
    seed_size = unhex(seed, max_size, p, strlen(p));
    if (seed_size < 0) {
      warnx("invalid seed passed through %s environment variable", name);
      p = NULL;
    }
  }

  if (p == NULL) {
    warnx("using default seed");
    memcpy(seed, default_seed, sizeof(default_seed));
    seed_size = sizeof(default_seed);
  }

  return seed_size;
}

unsigned long sys_os_perso_derive_node_bip32(cx_curve_t curve, const uint32_t *path, size_t length, uint8_t *private_key, uint8_t* chain)
{
  uint8_t tmpPrivateChain[CX_SHA512_SIZE + 32];
  cx_ecfp_256_public_key_t tmpEcPublic;
  cx_ecfp_private_key_t tmpEcPrivate;
  const cx_curve_domain_t *domain;
  uint8_t seed[64], tmp[1+64+4];
  ssize_t seed_size;
  unsigned int i;

  if (curve != CX_CURVE_256K1 && curve != CX_CURVE_SECP256R1) {
    errx(1, "os_perso_derive_node_bip32: curve not implemented (0x%x)", curve);
  }

  domain = cx_ecfp_get_domain(curve);

  seed_size = get_seed_from_env("SPECULOS_SEED", seed, sizeof(seed));

  expand_seed_bip32(curve, seed, seed_size, tmpPrivateChain, domain);

  for (i = 0; i < length; i++) {
    if (((path[i] >> 24) & 0x80) != 0) {
      tmp[0] = 0;
      memmove(tmp + 1, tmpPrivateChain, 32);
    }
    else {
      cx_ecdsa_init_private_key(curve, tmpPrivateChain, 32, &tmpEcPrivate);
      cx_ecfp_generate_pair(curve, &tmpEcPublic, &tmpEcPrivate, 1);
      tmpEcPublic.W[0] = ((tmpEcPublic.W[64] & 1) ? 0x03 : 0x02);
      memmove(tmp, tmpEcPublic.W, 33);
    }

    while (true) {
        tmp[33] = ((path[i] >> 24) & 0xff);
        tmp[34] = ((path[i] >> 16) & 0xff);
        tmp[35] = ((path[i] >> 8) & 0xff);
        tmp[36] = (path[i] & 0xff);

        cx_hmac_sha512(tmpPrivateChain + 32, 32, tmp, 37, tmp, 64);

        if (cx_math_cmp(tmp, domain->n, 32) < 0) {
          cx_math_addm(tmp, tmp, tmpPrivateChain, domain->n, 32);
          if (cx_math_is_zero(tmp, 32) == 0)
            break;
        }

        tmp[0] = 1;
        memmove(tmp + 1, tmp + 32, 32);
    }

    memmove(tmpPrivateChain, tmp, 32);
    memmove(tmpPrivateChain + 32, tmp + 32, 32);
  }

  if (private_key != NULL) {
    memmove(private_key, tmpPrivateChain, 32);
  }

  if (chain != NULL) {
    memmove(chain, tmpPrivateChain + 32, 32);
  }

  return 0;
}
