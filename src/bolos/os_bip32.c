#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "os_bip32.h"

#include "bolos/exception.h"
#include "cx.h"
#include "cx_ec.h"
#include "cx_hmac.h"
#include "cx_math.h"
#include "cx_utils.h"
#include "emulate.h"

#define BIP32_HARDEN_MASK      0x80000000
#define BIP32_SECP_SEED_LENGTH 12
#define MAX_SEED_SIZE          64

#define cx_ecfp_generate_pair     sys_cx_ecfp_generate_pair
#define cx_ecfp_init_private_key  sys_cx_ecfp_init_private_key
#define cx_ecdsa_init_private_key cx_ecfp_init_private_key

/* glory promote mansion idle axis finger extra february uncover one trip
 * resource lawn turtle enact monster seven myth punch hobby comfort wild raise
 * skin */
static uint8_t default_seed[MAX_SEED_SIZE] =
    "\xb1\x19\x97\xfa\xff\x42\x0a\x33\x1b\xb4\xa4\xff\xdc\x8b\xdc\x8b\xa7\xc0"
    "\x17\x32\xa9\x9a\x30\xd8\x3d\xbb\xeb\xd4\x69\x66\x6c\x84\xb4\x7d\x09\xd3"
    "\xf5\xf4\x72\xb3\xb9\x38\x4a\xc6\x34\xbe\xba\x2a\x44\x0b\xa3\x6e\xc7\x66"
    "\x11\x44\x13\x2f\x35\xe2\x06\x87\x35\x64";

static uint8_t const BIP32_SECP_SEED[] = { 'B', 'i', 't', 'c', 'o', 'i',
                                           'n', ' ', 's', 'e', 'e', 'd' };

static uint8_t const BIP32_NIST_SEED[] = { 'N', 'i', 's', 't', '2', '5', '6',
                                           'p', '1', ' ', 's', 'e', 'e', 'd' };

static uint8_t const BIP32_ED_SEED[] = { 'e', 'd', '2', '5', '5', '1',
                                         '9', ' ', 's', 'e', 'e', 'd' };

static uint8_t const SLIP21_SEED[] = { 'S', 'y', 'm', 'm', 'e', 't',
                                       'r', 'i', 'c', ' ', 'k', 'e',
                                       'y', ' ', 's', 'e', 'e', 'd' };

static bool is_hardened_child(uint32_t child)
{
  return (child & 0x80000000) != 0;
}

static ssize_t get_seed_key(cx_curve_t curve, const uint8_t **sk)
{
  ssize_t sk_length;

  switch (curve) {
  case CX_CURVE_256K1:
    *sk = BIP32_SECP_SEED;
    sk_length = sizeof(BIP32_SECP_SEED);
    break;
  case CX_CURVE_256R1:
    *sk = BIP32_NIST_SEED;
    sk_length = sizeof(BIP32_NIST_SEED);
    break;
  case CX_CURVE_Ed25519:
    *sk = BIP32_ED_SEED;
    sk_length = sizeof(BIP32_ED_SEED);
    break;
  default:
    errx(1, "seed_key: unsupported curve");
    sk_length = -1;
    break;
  }

  return sk_length;
}

static ssize_t get_seed_key_slip21(const uint8_t **sk)
{
  ssize_t sk_length;

  *sk = SLIP21_SEED;
  sk_length = sizeof(SLIP21_SEED);

  return sk_length;
}

void expand_seed_ed25519(const uint8_t *sk, size_t sk_length, uint8_t *seed,
                         unsigned int seed_length, extended_private_key *key)
{
  uint8_t hash[CX_SHA512_SIZE];
  uint8_t buf[1 + MAX_SEED_SIZE];

  buf[0] = 0x01;
  memcpy(buf + 1, seed, seed_length);
  spec_cx_hmac_sha256(sk, sk_length, buf, 1 + seed_length, key->chain_code,
                      CX_SHA256_SIZE);

  spec_cx_hmac_sha512(sk, sk_length, seed, seed_length, hash, CX_SHA512_SIZE);

  memcpy(key->private_key, hash, 32);
  memcpy(key->chain_code, hash + 32, 32);

  while (key->private_key[31] & 0x20) {
    spec_cx_hmac_sha512(sk, sk_length, key->private_key, CX_SHA512_SIZE,
                        key->private_key, CX_SHA512_SIZE);
  }

  key->private_key[0] &= 0xF8;
  key->private_key[31] = (key->private_key[31] & 0x7F) | 0x40;
}

void expand_seed_slip10(const uint8_t *sk, size_t sk_length, uint8_t *seed,
                        unsigned int seed_length, extended_private_key *key)
{
  uint8_t hash[CX_SHA512_SIZE];

  spec_cx_hmac_sha512(sk, sk_length, seed, seed_length, hash, CX_SHA512_SIZE);

  memcpy(key->private_key, hash, 32);
  memcpy(key->chain_code, hash + 32, 32);
}

void expand_seed_ed25519_bip32(const uint8_t *sk, size_t sk_length,
                               uint8_t *seed, unsigned int seed_length,
                               extended_private_key *key)
{
  uint8_t buf[1 + MAX_SEED_SIZE];

  buf[0] = 0x01;
  memcpy(buf + 1, seed, seed_length);
  spec_cx_hmac_sha256(sk, sk_length, buf, 1 + seed_length, key->chain_code,
                      CX_SHA256_SIZE);

  spec_cx_hmac_sha512(sk, sk_length, seed, seed_length, key->private_key,
                      CX_SHA512_SIZE);

  while (key->private_key[31] & 0x20) {
    spec_cx_hmac_sha512(sk, sk_length, key->private_key, CX_SHA512_SIZE,
                        key->private_key, CX_SHA512_SIZE);
  }

  key->private_key[0] &= 0xF8;
  key->private_key[31] = (key->private_key[31] & 0x7F) | 0x40;
}

static void expand_seed(cx_curve_t curve, const uint8_t *sk, size_t sk_length,
                        uint8_t *seed, unsigned int seed_length,
                        extended_private_key *key)
{
  const cx_curve_domain_t *domain;
  uint8_t hash[CX_SHA512_SIZE];

  domain = cx_ecfp_get_domain(curve);

  spec_cx_hmac_sha512(sk, sk_length, seed, seed_length, hash, CX_SHA512_SIZE);

  memcpy(key->private_key, hash, 32);
  memcpy(key->chain_code, hash + 32, 32);

  /* ensure that the master key is valid */
  while (cx_math_is_zero(key->private_key, 32) ||
         cx_math_cmp(key->private_key, domain->n, 32) >= 0) {
    memcpy(hash, key->private_key, 32);
    memcpy(hash + 32, key->chain_code, 32);
    spec_cx_hmac_sha512(sk, sk_length, hash, CX_SHA512_SIZE, hash,
                        CX_SHA512_SIZE);
    memcpy(key->private_key, hash, 32);
    memcpy(key->chain_code, hash + 32, 32);
  }
}

int unhex(uint8_t *dst, size_t dst_size, const char *src, size_t src_size)
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

size_t get_seed_from_env(const char *name, uint8_t *seed, size_t max_size)
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

static int hdw_bip32_ed25519(extended_private_key *key, const uint32_t *path,
                             size_t length, uint8_t *private_key,
                             uint8_t *chain)
{
  const cx_curve_domain_t *domain;
  cx_ecfp_256_public_key_t pub;
  uint8_t *kP, *kLP, *kRP;
  unsigned int i, j, len;
  uint8_t tmp[1 + 64 + 4], x;
  uint8_t *ZR, *Z, *ZL;

  Z = tmp + 1;
  ZL = tmp + 1;
  ZR = tmp + 1 + 32;

  kP = key->private_key;
  kLP = key->private_key;
  kRP = &key->private_key[32];

  domain = cx_ecfp_get_domain(CX_CURVE_Ed25519);
  pub.W_len = 65;
  pub.W[0] = 0x04;

  for (i = 0; i < length; i++) {
    // compute the public key A = kL.G
    memcpy(pub.W + 1, domain->Gx, 32);
    memcpy(pub.W + 1 + 32, domain->Gy, 32);

    le2be(kLP, 32);
    sys_cx_ecfp_scalar_mult(CX_CURVE_Ed25519, pub.W, pub.W_len, kLP, 32);
    be2le(kLP, 32);

    sys_cx_edward_compress_point(CX_CURVE_Ed25519, pub.W, pub.W_len);

    // Step 1: compute kL/Kr child
    //  if less than 0x80000000 => setup 02|A|i in tmp
    if (!is_hardened_child(path[i])) {
      tmp[0] = 0x02;
      memcpy(tmp + 1, &pub.W[1], 32);
      len = 1 + 32;
    } else {
      //  else if greater-eq than 0x80000000 --> setup 00|k|i in tmp
      tmp[0] = 0x00;
      memcpy(tmp + 1, kP, 64);
      len = 1 + 32 * 2;
    }

    tmp[len + 0] = (path[i] >> 0) & 0xff;
    tmp[len + 1] = (path[i] >> 8) & 0xff;
    tmp[len + 2] = (path[i] >> 16) & 0xff;
    tmp[len + 3] = (path[i] >> 24) & 0xff;
    len += 4;

    // compute Z = Hmac(...)
    spec_cx_hmac_sha512(key->chain_code, 32, tmp, len, Z, CX_SHA512_SIZE);
    // kL = 8*Zl + kLP (use multm, but never overflow order, so ok)
    le2be(ZL, 32);
    le2be(kLP, 32);
    memset(ZL, 0, 4);
    cx_math_add(ZL, ZL, ZL, 32);
    cx_math_add(ZL, ZL, ZL, 32);
    cx_math_add(ZL, ZL, ZL, 32);
    cx_math_add(ZL, ZL, kLP, 32);
    be2le(ZL, 32);
    be2le(kLP, 32);
    // kR = Zr + kRP
    le2be(ZR, 32);
    le2be(kRP, 32);
    cx_math_add(ZR, ZR, kRP, 32);
    be2le(ZR, 32);
    be2le(kRP, 32);
    // store new kL,kP, but keep old on to compute new c
    for (j = 0; j < 64; j++) {
      x = kP[j];
      kP[j] = Z[j];
      Z[j] = x;
    }

    // Step2: compute chain code
    // if less than 0x80000000 => set up 03|A|i in tmp
    if (!is_hardened_child(path[i])) {
      tmp[0] = 0x03;
      memcpy(tmp + 1, &pub.W[1], 32);
      len = 1 + 32;
    }
    // else if greater-eq than 0x80000000 -->  01|k|i in tmp
    else {
      tmp[0] = 0x01;
      // kP already set
      len = 1 + 32 * 2;
    }
    tmp[len + 0] = (path[i] >> 0) & 0xff;
    tmp[len + 1] = (path[i] >> 8) & 0xff;
    tmp[len + 2] = (path[i] >> 16) & 0xff;
    tmp[len + 3] = (path[i] >> 24) & 0xff;
    len += 4;
    spec_cx_hmac_sha512(key->chain_code, 32, tmp, len, tmp, CX_SHA512_SIZE);
    // store new c
    memcpy(key->chain_code, tmp + 32, 32);
  }

  if (private_key != NULL) {
    memcpy(private_key, kP, 64);
  }

  if (chain != NULL) {
    memcpy(chain, key->chain_code, 32);
  }

  return 0;
}

static int hdw_slip10(extended_private_key *key, const uint32_t *path,
                      size_t length, uint8_t *private_key, uint8_t *chain)
{
  uint8_t tmp[1 + 64 + 4];
  unsigned int i;

  for (i = 0; i < length; i++) {
    if (is_hardened_child(path[i])) {
      tmp[0] = 0;
      memcpy(tmp + 1, key->private_key, 32);
    } else {
      warn("hdw_slip10: invalid path (%u:0x%x)", i, path[i]);
      return -1;
    }

    tmp[33] = (path[i] >> 24) & 0xff;
    tmp[34] = (path[i] >> 16) & 0xff;
    tmp[35] = (path[i] >> 8) & 0xff;
    tmp[36] = path[i] & 0xff;

    spec_cx_hmac_sha512(key->chain_code, 32, tmp, 37, tmp, CX_SHA512_SIZE);

    memcpy(key->private_key, tmp, 32);
    memcpy(key->chain_code, tmp + 32, 32);
  }

  if (private_key != NULL) {
    memcpy(private_key, key->private_key, 32);
  }

  if (chain != NULL) {
    memcpy(chain, key->chain_code, 32);
  }

  return 0;
}

static int hdw_bip32(extended_private_key *key, cx_curve_t curve,
                     const uint32_t *path, size_t length, uint8_t *private_key,
                     uint8_t *chain)
{
  const cx_curve_domain_t *domain;
  cx_ecfp_256_public_key_t pub;
  cx_ecfp_private_key_t priv;
  uint8_t tmp[1 + 64 + 4];
  unsigned int i;

  if (curve != CX_CURVE_256K1 && curve != CX_CURVE_SECP256R1) {
    warn("hdw_bip32: invalid curve (0x%x)", curve);
    return -1;
  }

  domain = cx_ecfp_get_domain(curve);

  for (i = 0; i < length; i++) {
    if (is_hardened_child(path[i])) {
      tmp[0] = 0;
      memcpy(tmp + 1, key->private_key, 32);
    } else {
      cx_ecdsa_init_private_key(curve, key->private_key, 32, &priv);
      cx_ecfp_generate_pair(curve, &pub, &priv, 1);
      pub.W[0] = (pub.W[64] & 1) ? 0x03 : 0x02;
      memcpy(tmp, pub.W, 33);
    }

    while (true) {
      tmp[33] = (path[i] >> 24) & 0xff;
      tmp[34] = (path[i] >> 16) & 0xff;
      tmp[35] = (path[i] >> 8) & 0xff;
      tmp[36] = path[i] & 0xff;

      spec_cx_hmac_sha512(key->chain_code, 32, tmp, 37, tmp, CX_SHA512_SIZE);

      if (cx_math_cmp(tmp, domain->n, 32) < 0) {
        cx_math_addm(tmp, tmp, key->private_key, domain->n, 32);
        if (cx_math_is_zero(tmp, 32) == 0) {
          break;
        }
      }

      tmp[0] = 1;
      memmove(tmp + 1, tmp + 32, 32);
    }

    memcpy(key->private_key, tmp, 32);
    memcpy(key->chain_code, tmp + 32, 32);
  }

  if (private_key != NULL) {
    memcpy(private_key, key->private_key, 32);
  }

  if (chain != NULL) {
    memcpy(chain, key->chain_code, 32);
  }

  return 0;
}

static int hdw_slip21(const uint8_t *sk, size_t sk_length, const uint8_t *seed,
                      size_t seed_size, const uint8_t *path, size_t path_len,
                      uint8_t *private_key)
{
  uint8_t node[CX_SHA512_SIZE];

  if (path == NULL || path_len == 0 || path[0] != 0) {
    warnx("hdw_slip21: invalid path");
    return -1;
  }

  /* derive master node */
  spec_cx_hmac_sha512(sk, sk_length, seed, seed_size, node, CX_SHA512_SIZE);

  /* derive child node */
  spec_cx_hmac_sha512(node, 32, path, path_len, node, CX_SHA512_SIZE);

  if (private_key != NULL) {
    memcpy(private_key, node + 32, 32);
  }

  return 0;
}

unsigned long sys_os_perso_derive_node_bip32_seed_key(
    unsigned int mode, cx_curve_t curve, const unsigned int *path,
    unsigned int pathLength, unsigned char *privateKey, unsigned char *chain,
    unsigned char *seed_key, unsigned int seed_key_length)
    __attribute__((weak, alias("sys_os_perso_derive_node_with_seed_key")));

unsigned long sys_os_perso_derive_node_with_seed_key(
    unsigned int mode, cx_curve_t curve, const unsigned int *path,
    unsigned int pathLength, unsigned char *privateKey, unsigned char *chain,
    unsigned char *seed_key, unsigned int seed_key_length)
{
  ssize_t sk_length;
  size_t seed_size;
  uint8_t seed[MAX_SEED_SIZE];
  extended_private_key key;
  const uint8_t *sk;
  int ret;

  // In SDK2, some curves don't have the same value:
  switch ((int)curve) {
  case 0x71:
    curve = CX_CURVE_Ed25519;
    break;
  case 0x72:
    curve = CX_CURVE_Ed448;
    break;
  case 0x81:
    curve = CX_CURVE_Curve25519;
    break;
  case 0x82:
    curve = CX_CURVE_Curve448;
    break;
  }

  if (seed_key == NULL || seed_key_length == 0) {
    if (mode != HDW_SLIP21) {
      sk_length = get_seed_key(curve, &sk);
    } else {
      sk_length = get_seed_key_slip21(&sk);
    }
    if (sk_length < 0) {
      THROW(EXCEPTION);
    }
  } else {
    sk = seed_key;
    sk_length = seed_key_length;
  }

  seed_size = get_seed_from_env("SPECULOS_SEED", seed, sizeof(seed));

  if (mode == HDW_SLIP21) {
    ret = hdw_slip21(sk, sk_length, seed, seed_size, (const uint8_t *)path,
                     pathLength, privateKey);
  } else if (mode == HDW_ED25519_SLIP10) {
    /* https://github.com/satoshilabs/slips/tree/master/slip-0010 */
    /* https://github.com/satoshilabs/slips/blob/master/slip-0010.md */
    if (curve == CX_CURVE_Ed25519) {
      expand_seed_slip10(sk, sk_length, seed, seed_size, &key);
      ret = hdw_slip10(&key, path, pathLength, privateKey, chain);
    } else {
      expand_seed(curve, sk, sk_length, seed, seed_size, &key);
      ret = hdw_bip32(&key, curve, path, pathLength, privateKey, chain);
    }
  } else {
    if (curve == CX_CURVE_Ed25519) {
      expand_seed_ed25519_bip32(sk, sk_length, seed, seed_size, &key);
      ret = hdw_bip32_ed25519(&key, path, pathLength, privateKey, chain);
    } else {
      expand_seed(curve, sk, sk_length, seed, seed_size, &key);
      ret = hdw_bip32(&key, curve, path, pathLength, privateKey, chain);
    }
  }

  if (ret < 0) {
    THROW(EXCEPTION);
  }

  return 0;
}

unsigned long sys_os_perso_derive_node_bip32(cx_curve_t curve,
                                             const uint32_t *path,
                                             size_t length,
                                             uint8_t *private_key,
                                             uint8_t *chain)
{
  return sys_os_perso_derive_node_with_seed_key(HDW_NORMAL, curve, path, length,
                                                private_key, chain, NULL, 0);
}
