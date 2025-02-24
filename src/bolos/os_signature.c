#include "os_signature.h"
#include "exception.h"
#include <openssl/ec.h>

cx_ecfp_public_key_t const speculos_root_ca_public_key = {
  .curve = CX_CURVE_SECP256K1,
  .W_len = 65,
  .W = { 0x04, 0x37, 0x70, 0x13, 0xa7, 0x8f, 0xde, 0xeb, 0xae, 0xb4, 0x4c,
         0x26, 0x25, 0x60, 0xc0, 0x39, 0xb4, 0x94, 0xbc, 0xfd, 0x89, 0xdc,
         0x59, 0xfd, 0x23, 0xcc, 0xca, 0xce, 0x1b, 0x29, 0x52, 0xca, 0xa6,
         0x61, 0x6a, 0x0c, 0xfe, 0x66, 0x83, 0xab, 0x96, 0x39, 0xc6, 0xfc,
         0xe4, 0x44, 0xcd, 0xcd, 0x7c, 0x69, 0x86, 0x8d, 0x27, 0xdc, 0x77,
         0xea, 0x66, 0xd8, 0x62, 0x28, 0xae, 0xe5, 0x93, 0x31, 0x72 }
};

cx_ecfp_public_key_t const root_ca_public_key = {
  .curve = CX_CURVE_SECP256K1,
  .W_len = 65,
  .W = { 0x04, 0xe6, 0xb3, 0x51, 0x48, 0xbf, 0x55, 0x6c, 0xa9, 0x0b, 0xac,
         0x3a, 0x4e, 0x85, 0xc0, 0x08, 0x1e, 0x55, 0x81, 0x24, 0xce, 0xeb,
         0x3c, 0x7d, 0x72, 0xf9, 0x14, 0x10, 0x0f, 0x8b, 0x2a, 0xd6, 0x50,
         0x9a, 0x55, 0x22, 0x36, 0xa3, 0x48, 0x6e, 0xf0, 0xaf, 0x3a, 0x1e,
         0xf8, 0x7d, 0x86, 0x55, 0x83, 0xa9, 0xa7, 0x4d, 0x8e, 0x5d, 0xb8,
         0x1e, 0xdd, 0xdc, 0x87, 0x76, 0x21, 0x1e, 0x6c, 0x70, 0xff }
};

bool pki_prod = false;

cx_err_t cx_ecdsa_internal_init_public_key(cx_curve_t curve,
                                           const unsigned char *rawkey,
                                           unsigned int key_len,
                                           cx_ecfp_public_key_t *key)
{
  const cx_curve_domain_t *domain;
  unsigned int expected_key_len;
  unsigned int size;

  domain = cx_ecdomain(curve);
  size = domain->length;

  memset(key, 0, sizeof(cx_ecfp_public_key_t));

  if (rawkey != NULL) {
    expected_key_len = 0;
    if (rawkey[0] == 0x04) {
      expected_key_len = 1 + size * 2;
    }
    if (expected_key_len == 0 || key_len != expected_key_len) {
      return INVALID_PARAMETER;
    }
  } else {
    key_len = 0;
  }
  // init key
  key->curve = curve;
  key->W_len = key_len;
  memcpy(key->W, rawkey, key_len);

  return key_len;
}

bool cx_ecdsa_internal_verify(const cx_ecfp_public_key_t *key,
                              const uint8_t *hash, unsigned int hash_len,
                              const uint8_t *sig, unsigned int sig_len)
{
  const cx_curve_weierstrass_t *domain;
  unsigned int size;
  const uint8_t *r, *s;
  size_t rlen, slen;
  int nid = 0;
  bool result = false;

  domain = (const cx_curve_weierstrass_t *)cx_ecdomain(key->curve);
  size = domain->length;

  if (!spec_cx_ecfp_decode_sig_der(sig, sig_len, size, &r, &rlen, &s, &slen)) {
    return 0;
  }
  // Load ECDSA signature
  BIGNUM *sig_r = BN_new();
  BIGNUM *sig_s = BN_new();
  BN_bin2bn(r, rlen, sig_r);
  BN_bin2bn(s, slen, sig_s);
  ECDSA_SIG *ecdsa_sig = ECDSA_SIG_new();
  ECDSA_SIG_set0(ecdsa_sig, sig_r, sig_s);

  // Set public key
  BIGNUM *x = BN_new();
  BIGNUM *y = BN_new();
  BN_bin2bn(key->W + 1, domain->length, x);
  BN_bin2bn(key->W + domain->length + 1, domain->length, y);

  nid = cx_nid_from_curve(key->curve);
  EC_KEY *ec_key = EC_KEY_new_by_curve_name(nid);
  EC_KEY_set_public_key_affine_coordinates(ec_key, x, y);

  if (ECDSA_do_verify(hash, hash_len, ecdsa_sig, ec_key) == 1) {
    result = true;
  }

  ECDSA_SIG_free(ecdsa_sig);

  BN_free(y);
  BN_free(x);
  EC_KEY_free(ec_key);
  return result;
}

int ED25519_verify(const uint8_t *message, size_t message_len,
                   const uint8_t signature[64], const uint8_t public_key[32]);

bool cx_eddsa_internal_verify(const cx_ecfp_public_key_t *pu_key,
                              cx_md_t hashID, const unsigned char *hash,
                              unsigned int hash_len, const unsigned char *sig,
                              unsigned int sig_len)
{
  cx_ecfp_public_key_t pub;

  /* Only SHA-512 is supported in Speculos, as it is the only supported hash
   * in OpenSSL */
  if (hashID != CX_SHA512) {
    return 0;
  }
  if (sig_len != 64) {
    return 0;
  }
  if (pu_key->curve != CX_CURVE_Ed25519 || pu_key->W_len != 1 + 2 * 32) {
    return 0;
  }

  /* pass a compressed key to ED25519_verify */
  memcpy(&pub, pu_key, sizeof(pub));
  if (pub.W[0] == 0x04) {
    if (sys_cx_edward_compress_point(pub.curve, pub.W, pub.W_len) != 0) {
      return 0;
    }
  }

  return ED25519_verify(hash, hash_len, sig, pub.W + 1);
}

bool cx_verify(cx_sign_algo_t sign_algo, cx_ecfp_public_key_t *public_key,
               uint8_t *msg_hash, size_t msg_hash_len, uint8_t *signature,
               size_t signature_len)
{
  switch (sign_algo) {
  case CX_SIGN_ALGO_ECDSA_RND:
  case CX_SIGN_ALGO_ECDSA_RFC6979_RIPEMD160:
  case CX_SIGN_ALGO_ECDSA_RFC6979_SHA224:
  case CX_SIGN_ALGO_ECDSA_RFC6979_SHA256:
  case CX_SIGN_ALGO_ECDSA_RFC6979_SHA384:
  case CX_SIGN_ALGO_ECDSA_RFC6979_SHA512:
  case CX_SIGN_ALGO_ECDSA_RFC6979_KECCAK:
  case CX_SIGN_ALGO_ECDSA_RFC6979_SHA3:
    return cx_ecdsa_internal_verify(public_key, msg_hash, msg_hash_len,
                                    signature, signature_len);
    break;
  case CX_SIGN_ALGO_EDDSA_SHA512:
    return cx_eddsa_internal_verify(public_key, CX_SHA512, msg_hash,
                                    msg_hash_len, signature, signature_len);
  default:
    return false;
  }
  return false;
}

bool os_ecdsa_verify_with_root_ca(uint8_t key_id, uint8_t *hash,
                                  size_t hash_len, uint8_t *sig, size_t sig_len)
{
  bool result = false;
  if (ROOT_CA_V3_KEY_ID == key_id) {
    if (pki_prod) {
      result = cx_ecdsa_internal_verify(&root_ca_public_key, hash, hash_len,
                                        sig, sig_len);
    } else {
      // Verification with test key
      result = cx_ecdsa_internal_verify(&speculos_root_ca_public_key, hash,
                                        hash_len, sig, sig_len);
    }
  }
  return result;
}
