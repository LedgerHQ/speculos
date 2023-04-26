#define _SDK_2_0_
#include "cxlib.h"
#include <string.h>

// HMAC-Based Key Kerivation Function defined at
// https://tools.ietf.org/html/rfc5869 Used to implement the EIP-2333 for
// BLS12-381 key generation

void cx_hkdf_extract(const cx_md_t hash_id, const unsigned char *ikm,
                     unsigned int ikm_len, unsigned char *salt,
                     unsigned int salt_len, unsigned char *prk)
{
  cx_hmac_t hmac_ctx;
  const cx_hash_info_t *hash_info;
  size_t md_len;

  hash_info = spec_cx_hash_get_info(hash_id);
  md_len = hash_info->output_size;
  if (salt == NULL) {
    memset(hmac_ctx.key, 0, md_len);
    salt = hmac_ctx.key;
    salt_len = md_len;
  }

  spec_cx_hmac_init(&hmac_ctx, hash_id, salt, salt_len);
  spec_cx_hmac_update(&hmac_ctx, ikm, ikm_len);
  spec_cx_hmac_final(&hmac_ctx, prk, &md_len);
}

void spec_cx_hkdf_expand(const cx_md_t hash_id, const unsigned char *prk,
                         unsigned int prk_len, unsigned char *info,
                         unsigned int info_len, unsigned char *okm,
                         unsigned int okm_len)
{
  unsigned char i;
  unsigned int offset = 0;
  unsigned char T[MAX_HASH_SIZE];
  cx_hmac_t hmac_ctx;
  const cx_hash_info_t *hash_info;
  size_t md_len;

  hash_info = spec_cx_hash_get_info(hash_id);
  md_len = hash_info->output_size;

  for (i = 1; okm_len > 0; i++) {
    spec_cx_hmac_init(&hmac_ctx, hash_id, prk, prk_len);
    if (i > 1) {
      spec_cx_hmac_update(&hmac_ctx, T, offset);
    }
    spec_cx_hmac_update(&hmac_ctx, info, info_len);
    spec_cx_hmac_update(&hmac_ctx, &i, sizeof(i));
    spec_cx_hmac_final(&hmac_ctx, T, &md_len);
    offset = (okm_len < md_len) ? okm_len : md_len;
    memcpy(okm + (i - 1) * md_len, T, offset);
    okm_len -= offset;
  }
}

// Hash 32 random bytes into the subgroup of the BLS12-381 privates keys
// Input: ikm = a secret octet string >= 256 bits: ikm = ikm || I2OSP(0, 1)
//        ikm_len = ikm length
//        key_info = an optional string: key_info = key_info || I2OSP(L, 2)
//        key_info_len = key_info length
// Output: sk = the secret key, an integer s.t 0 <= sk < r, r is the subgroup
// order
// 1. salt = "BLS-SIG-KEYGEN-SALT-"
// 2. SK = 0
// 3. while SK == 0:
// 4.     salt = SHA256(salt)
// 5.     PRK = HKDF-Extract(salt, IKM || I2OSP(0, 1))
// 6.     OKM = HKDF-Expand(PRK, key_info || I2OSP(L, 2), L)
// 7.     SK = OS2IP(OKM) mod r
// 8. return SK

void cx_hkdf_mod_r(const unsigned char *ikm, unsigned int ikm_len,
                   const uint8_t *salt, unsigned int salt_len,
                   unsigned char *key_info, unsigned int key_info_len,
                   unsigned char *sk)
{
  unsigned char prk[CX_SHA256_SIZE];
  unsigned char okm[CX_HKDF_MOD_R_CEIL];
  unsigned char HKDF_MOD_R_SALT[] = { 'B', 'L', 'S', '-', 'S', 'I', 'G',
                                      '-', 'K', 'E', 'Y', 'G', 'E', 'N',
                                      '-', 'S', 'A', 'L', 'T', '-' };
  const cx_curve_domain_t *domain;
  unsigned char key_salt[CX_SHA256_SIZE] = { 0 };
  unsigned char used_salt[CX_SHA256_SIZE] = { 0 };
  unsigned int used_salt_len;

  domain = cx_ecdomain(CX_CURVE_BLS12_381_G1);

  if (NULL == salt) {
    sys_cx_hash_sha256(HKDF_MOD_R_SALT, sizeof(HKDF_MOD_R_SALT), key_salt,
                       CX_SHA256_SIZE);
    used_salt_len = CX_SHA256_SIZE;
  } else {
    used_salt_len = (salt_len <= CX_SHA256_SIZE) ? salt_len : CX_SHA256_SIZE;
    memcpy(key_salt, salt, used_salt_len);
  }

  memset(sk, 0, KEY_LENGTH);

  while (cx_math_is_zero(sk, KEY_LENGTH)) {
    cx_hkdf_extract(CX_SHA256, ikm, ikm_len, key_salt, used_salt_len, prk);
    spec_cx_hkdf_expand(CX_SHA256, prk, CX_SHA256_SIZE, key_info, key_info_len,
                        okm, CX_HKDF_MOD_R_CEIL);
    cx_math_mod(okm, CX_HKDF_MOD_R_CEIL, domain->n, CX_HKDF_MOD_R_CEIL);
    memcpy(sk, okm + (CX_HKDF_MOD_R_CEIL - KEY_LENGTH), KEY_LENGTH);
    memcpy(used_salt, key_salt, used_salt_len);
    sys_cx_hash_sha256(used_salt, used_salt_len, key_salt, CX_SHA256_SIZE);
    used_salt_len = CX_SHA256_SIZE;
  }
}
