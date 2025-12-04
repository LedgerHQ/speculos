#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "os_bip32.h"

#include "appflags.h"
#include "bolos/exception.h"
#include "cx.h"
#include "cx_ec.h"
#include "cx_utils.h"
#include "emulate.h"
#include "environment.h"
#include "launcher.h"

#define BIP32_HARDEN_MASK      0x80000000
#define BIP32_SECP_SEED_LENGTH 12

#define DERIVE_AUTH_256K1      0x01
#define DERIVE_AUTH_256R1      0x02
#define DERIVE_AUTH_ED25519    0x04
#define DERIVE_AUTH_SLIP21     0x08
#define DERIVE_AUTH_BLS12381G1 0x10

#define BIP32_WILDCARD_VALUE (0x7fffffff)

#define cx_ecfp_generate_pair     sys_cx_ecfp_generate_pair
#define cx_ecfp_init_private_key  sys_cx_ecfp_init_private_key
#define cx_ecdsa_init_private_key cx_ecfp_init_private_key

static uint8_t const BIP32_SECP_SEED[] = { 'B', 'i', 't', 'c', 'o', 'i',
                                           'n', ' ', 's', 'e', 'e', 'd' };

static uint8_t const BIP32_NIST_SEED[] = { 'N', 'i', 's', 't', '2', '5', '6',
                                           'p', '1', ' ', 's', 'e', 'e', 'd' };

static uint8_t const BIP32_ED_SEED[] = { 'e', 'd', '2', '5', '5', '1',
                                         '9', ' ', 's', 'e', 'e', 'd' };

static uint8_t const SLIP21_SEED[] = { 'S', 'y', 'm', 'm', 'e', 't',
                                       'r', 'i', 'c', ' ', 'k', 'e',
                                       'y', ' ', 's', 'e', 'e', 'd' };

static uint32_t sys_os_perso_derive_node_with_seed_key_internal(
    uint32_t mode, cx_curve_t curve, const uint32_t *path, uint32_t pathLength,
    uint8_t *privateKey, uint8_t *chain, uint8_t *seed_key,
    uint32_t seed_key_length, bool fromApp);

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

static void os_perso_derive_node_bip32_check_path(unsigned int mode,
                                                  cx_curve_t curve,
                                                  const unsigned int *path,
                                                  unsigned int pathLength,
                                                  bool fromApp)
{
  unsigned char pathValid = 0;
  unsigned int derive_path_length;
  unsigned char *derive_path;

  // TODO : Harden against glitches
  // Check the path against the current application path
  derive_path_length = (fromApp) ? get_app_derivation_path(&derive_path) : 0;

  // No information specified, everything is valid
  if (derive_path_length != 0) {
    unsigned char curveMask;
    unsigned int checkOffset = 1;
    // Abort if not authorized to operate on this curve
    if (mode == HDW_SLIP21) {
      curveMask = DERIVE_AUTH_SLIP21;
    } else {
      switch (curve) {
      case CX_CURVE_256K1:
        curveMask = DERIVE_AUTH_256K1;
        break;
      case CX_CURVE_256R1:
        curveMask = DERIVE_AUTH_256R1;
        break;
      case CX_CURVE_Ed25519:
        curveMask = DERIVE_AUTH_ED25519;
        break;
      default:
        THROW(SWO_PAR_VAL_13);
      }
    }
    if ((derive_path[0] & curveMask) != curveMask) {
      THROW(SWO_PAR_VAL_14);
    }
    // if only the curve was specified, all paths are valid
    if (derive_path_length > 1) {
      while ((checkOffset < derive_path_length) && !pathValid) {
        unsigned char subPathLength = derive_path[checkOffset];
        unsigned char slip21Marker = ((subPathLength & 0x80) != 0);
        unsigned char slip21Request = (mode == HDW_SLIP21);
        subPathLength &= 0x7F;
        // Authorize if a subpath is valid
        if (pathLength >= subPathLength) {
          pathValid = 1;
          if (slip21Marker != slip21Request) {
            pathValid = 0;
          } else {
            if (slip21Request) {
              // compare path as bytes as slip21 path may be a string.
              // NOTE: interpret path first byte (len) as a number of bytes in
              // the path
              if (memcmp(path, &derive_path[checkOffset + 1], subPathLength)) {
                pathValid = 0;
              }
            } else {
              for (unsigned int i = 0; i < subPathLength; i++) {
                uint32_t allowed_prefix =
                    U4BE(derive_path, (checkOffset + 1 + 4 * i));
                if (path[i] != allowed_prefix) {
                  // path is considered to be valid if the allowed prefix
                  // contains the 'wildcard' value 0x7fffffff
                  if (allowed_prefix != BIP32_WILDCARD_VALUE) {
                    pathValid = 0;
                    break;
                  }
                }
              }
            }
          }
        }
        checkOffset += 1 + (slip21Marker ? 1 : 4) * subPathLength;
      }
    } else {
      pathValid = 1;
    }
  } else {
    pathValid = 1;
  }
  if (!pathValid) {
    THROW(SWO_PAR_VAL_15);
  }
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
  return sys_os_perso_derive_node_with_seed_key_internal(
      mode, curve, path, pathLength, privateKey, chain, seed_key,
      seed_key_length, true);
}

static uint32_t sys_os_perso_derive_node_with_seed_key_internal(
    uint32_t mode, cx_curve_t curve, const uint32_t *path, uint32_t pathLength,
    uint8_t *privateKey, uint8_t *chain, uint8_t *seed_key,
    uint32_t seed_key_length, bool fromApp)
{
  ssize_t sk_length;
  size_t seed_size;
  uint8_t seed[MAX_SEED_SIZE];
  extended_private_key key;
  const uint8_t *sk;
  int ret;

  if (path == NULL) {
    THROW(EXCEPTION);
  }

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

  // If APPLICATION_FLAG_DERIVE_MASTER is not set for an application, forbid
  // deriving from seed with a null path
  if ((mode != HDW_SLIP21) && (fromApp) &&
      !(app_flags & APPLICATION_FLAG_DERIVE_MASTER)) {
    uint32_t i;
    for (i = 0; i < pathLength; i++) {
      if ((path[i] & 0x80000000) != 0) {
        break;
      }
    }
    if (i == pathLength) {
      THROW(SWO_PAR_VAL_12);
    }
  }

  os_perso_derive_node_bip32_check_path(mode, curve, path, pathLength, fromApp);

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

  seed_size = env_get_seed(seed, sizeof(seed));

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

unsigned long sys_os_perso_get_master_key_identifier(uint8_t *identifier,
                                                     size_t identifier_length)
{
  /* Input parameters verification */
  if (identifier == NULL) {
    errx(1, "%s: identifier is NULL", __func__);
    THROW(EXCEPTION);
  }
  if (identifier_length < CX_RIPEMD160_SIZE) {
    errx(1, "%s: wrong input length", __func__);
    THROW(EXCEPTION);
  }
  unsigned char raw_private_key[64] = { 0 };
  /* Required for sys_os_perso_derive_node_with_seed_key() */
  unsigned int path = 0;

  /* Getting raw private key */
  uint32_t ret = sys_os_perso_derive_node_with_seed_key_internal(
      HDW_NORMAL, CX_CURVE_SECP256K1, &path, 0, raw_private_key, NULL, NULL, 0,
      false);
  if (ret != 0) {
    errx(1, "%s: sys_os_perso_derive_node_with_seed_key() failed with %u",
         __func__, ret);
    THROW(EXCEPTION);
  }

  /* Converting it to structure form */
  cx_ecfp_private_key_t private_key;
  sys_cx_ecfp_init_private_key(CX_CURVE_SECP256K1, raw_private_key, 32,
                               &private_key);

  /* Getting corresponding public key */
  cx_ecfp_public_key_t public_key;
  sys_cx_ecfp_generate_pair(CX_CURVE_SECP256K1, &public_key, &private_key, 1);
  if (public_key.W_len != 65) {
    errx(1, "perso_get_master_key_identifier: wrong output public key length");
    THROW(EXCEPTION);
  }

  /* Getting raw public key */
  uint8_t raw_pubkey[65] = { 0 };
  memcpy(raw_pubkey, public_key.W, public_key.W_len);

  /* Compressing public key */
  uint8_t compressed_pubkey[33] = { 0 };
  if (raw_pubkey[0] != 0x04) {
    errx(1, "perso_get_master_key_identifier: wrong public key");
    THROW(EXCEPTION);
  }
  compressed_pubkey[0] = (raw_pubkey[64] % 2 == 1) ? 0x03 : 0x02;
  memcpy(compressed_pubkey + 1, raw_pubkey + 1, 32); // copy x

  /* Getting sha256 hash of public key */
  uint8_t sha256_hash[CX_SHA256_SIZE];
  int res =
      sys_cx_hash_sha256(compressed_pubkey, 33, sha256_hash, CX_SHA256_SIZE);
  if (res != CX_SHA256_SIZE) {
    errx(1, "perso_get_master_key_identifier: unexpected error in sha256 "
            "computation");
    THROW(EXCEPTION);
  }

  /* Getting ripemd160 hash of sha256 hash */
  uint8_t ripemd160_hash[CX_RIPEMD160_SIZE];
  res = sys_cx_hash_ripemd160(sha256_hash, CX_SHA256_SIZE, ripemd160_hash,
                              CX_RIPEMD160_SIZE);
  if (res != CX_RIPEMD160_SIZE) {
    errx(1, "perso_get_master_key_identifier: unexpected error in ripemd160 "
            "computation");
    THROW(EXCEPTION);
  }

  /* Copying the result */
  memcpy(identifier, ripemd160_hash, CX_RIPEMD160_SIZE);

  return 0;
}
