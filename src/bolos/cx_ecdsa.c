#include <err.h>
#include <string.h>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/objects.h>
#include <openssl/sha.h>

#include "bolos/exception.h"
#include "cx.h"
#include "cx_bn.h"
#include "cx_ec.h"
#include "cx_ecdsa.h"
#include "cx_errors.h"
#include "cx_hash.h"
#include "cx_rng_rfc6979.h"
#include "cx_utils.h"
#include "cx_wrap_ossl.h"
#include "emulate.h"

/***************** THROW VERSION *********/
/* the following functions use throw errors instead of error_codes, no_throw
 * versions should be preferably used*/
/*nota: the value returned is the size of the signature*/
int sys_cx_ecdsa_sign(const cx_ecfp_private_key_t *key, int mode,
                      cx_md_t hashID, const uint8_t *hash,
                      unsigned int hash_len, uint8_t *sig, unsigned int sig_len,
                      unsigned int *info)
{
  int nid = 0;
  uint8_t *buf_r, *buf_s;
  const BIGNUM *r, *s;
  const cx_hash_info_t *hash_info;
  bool do_rfc6979_signature;
  BN_CTX *ctx = BN_CTX_new();

  const cx_curve_weierstrass_t *domain =
      (const cx_curve_weierstrass_t *)cx_ecfp_get_domain(key->curve);
  nid = nid_from_curve(key->curve);
  if (nid < 0) {
    return CX_NULLSIZE;
  }

  switch (mode & CX_MASK_RND) {
  case CX_RND_TRNG:
    // Use deterministic signatures with "TRNG" mode in Speculos, in order to
    // avoid signing objects with a weak PRNG, only if the hash is compatible
    // with cx_hmac_init (hash_info->output_size is defined).
    // Otherwise, fall back to OpenSSL signature.
    hash_info = spec_cx_hash_get_info(hashID);
    if (hash_info == NULL || hash_info->output_size == 0) {
      do_rfc6979_signature = false;
      break;
    }
    __attribute__((fallthrough));
  case CX_RND_RFC6979:
    do_rfc6979_signature = true;
    break;
  default:
    THROW(INVALID_PARAMETER);
  }

  cx_rnd_rfc6979_ctx_t rfc_ctx = {};
  uint8_t rnd[CX_RFC6979_MAX_RLEN];
  BIGNUM *q = BN_new();
  BIGNUM *x = BN_new();
  BIGNUM *k = BN_new();
  BIGNUM *kinv = BN_new();
  BIGNUM *rp = BN_new();
  BIGNUM *kg_y = BN_new();

  BN_bin2bn(domain->n, domain->length, q);
  BN_bin2bn(key->d, key->d_len, x);

  EC_GROUP *Stark = NULL;

  if (cx_generic_curve(domain, ctx, &Stark) != 0)
    return CX_NULLSIZE;
  /* get generic curve group from parameters*/
  /* curve must be stored in weierstrass form in C_cx_all_Weierstrass_Curves*/

  // nid = nid_from_curve(key->curve);
  EC_KEY *ec_key = EC_KEY_new();
  EC_KEY_set_group(ec_key, Stark);

  //  EC_KEY *ec_key = EC_KEY_new_by_curve_name(nid);
  // EC_KEY_set_public_key_affine_coordinates(ec_key, x, y);

  //  EC_KEY *ec_key = EC_KEY_new_by_curve_name(nid);
  EC_KEY_set_private_key(ec_key, x);

  const EC_GROUP *group = EC_KEY_get0_group(ec_key);
  EC_POINT *kg = EC_POINT_new(group);
  ECDSA_SIG *ecdsa_sig;

  if (do_rfc6979_signature) {
    if (domain->length >= CX_RFC6979_MAX_RLEN) {
      errx(1, "rfc6979_ecdsa_sign: curve too large");
    }
    spec_cx_rng_rfc6979_init(&rfc_ctx, hashID, key->d, key->d_len, hash,
                             hash_len, domain->n, domain->length);
  }
  do {
    if (do_rfc6979_signature) {
      spec_cx_rng_rfc6979_next(&rfc_ctx, rnd, domain->length);
      BN_bin2bn(rnd, domain->length, k);
    } else {
      BN_rand(k, domain->length, 0, 0);
    }
    if (!EC_POINT_mul(group, kg, k, NULL, NULL, ctx)) {
      errx(1, "ssl: EC_POINT_mul");
    }
    if (!EC_POINT_get_affine_coordinates(group, kg, rp, kg_y, ctx)) {
      errx(1, "ssl: EC_POINT_get_affine_coordinates");
    }
    if (BN_cmp(rp, q) >= 0) {
      if (!BN_nnmod(rp, rp, q, ctx)) {
        errx(1, "ssl: BN_nnmod");
      }
      if (info != NULL) {
        *info |= CX_ECCINFO_xGTn;
      }
    }
    if (BN_is_zero(rp)) {
      continue;
    }
    if (info != NULL && BN_is_odd(kg_y)) {
      *info |= CX_ECCINFO_PARITY_ODD;
    }

    BN_mod_inverse(kinv, k, q, ctx);
    ecdsa_sig = ECDSA_do_sign_ex(hash, hash_len, kinv, rp, ec_key);
  } while (ecdsa_sig == NULL);
  BN_CTX_free(ctx);
  BN_free(q);
  BN_free(x);
  BN_free(k);
  BN_free(kinv);
  BN_free(rp);
  BN_free(kg_y);
  EC_POINT_free(kg);

  // normalize signature (s < n/2) if needed
  BIGNUM *halfn = BN_new();
  const BIGNUM *n = EC_GROUP_get0_order(group);
  BN_rshift1(halfn, n);

  BIGNUM *normalized_s = BN_new();
  ECDSA_SIG_get0(ecdsa_sig, &r, &s);
  if ((mode & CX_NO_CANONICAL) == 0 && BN_cmp(s, halfn) > 0) {
    fprintf(stderr, "cx_ecdsa_sign: normalizing s > n/2\n");
    BN_sub(normalized_s, n, s);

    if (info != NULL) {
      *info ^= CX_ECCINFO_PARITY_ODD; // Inverse the bit
    }
  } else {
    normalized_s = BN_copy(normalized_s, s);
  }

  buf_r = malloc(domain->length);
  buf_s = malloc(domain->length);
  BN_bn2binpad(r, buf_r, domain->length);
  BN_bn2binpad(normalized_s, buf_s, domain->length);
  int ret = spec_cx_ecfp_encode_sig_der(sig, sig_len, buf_r, domain->length,
                                        buf_s, domain->length);
  free(buf_r);
  free(buf_s);
  ECDSA_SIG_free(ecdsa_sig);
  BN_free(normalized_s);
  BN_free(halfn);
  EC_KEY_free(ec_key);
  return ret;
}

int sys_cx_ecdsa_verify(const cx_ecfp_public_key_t *key, int UNUSED(mode),
                        cx_md_t UNUSED(hashID), const uint8_t *hash,
                        unsigned int hash_len, const uint8_t *sig,
                        unsigned int sig_len)
{
  const cx_curve_weierstrass_t *domain;
  unsigned int size;
  const uint8_t *r, *s;
  size_t rlen, slen;
  // int nid = 0;
  BN_CTX *ctx;
  ctx = BN_CTX_new();
  EC_GROUP *Stark = NULL;

  domain = (const cx_curve_weierstrass_t *)cx_ecfp_get_domain(key->curve);
  size = domain->length; // bits  -> bytes

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

  if (cx_generic_curve(domain, ctx, &Stark) != 0)
    return CX_KO;

  // nid = nid_from_curve(key->curve);
  EC_KEY *ec_key = EC_KEY_new();
  EC_KEY_set_group(ec_key, Stark);

  //  EC_KEY *ec_key = EC_KEY_new_by_curve_name(nid);
  EC_KEY_set_public_key_affine_coordinates(ec_key, x, y);

  int ret = ECDSA_do_verify(hash, hash_len, ecdsa_sig, ec_key);
  if (ret != OPEN_SSL_OK) {
    ret = CX_OK;
  }

  ECDSA_SIG_free(ecdsa_sig);

  BN_free(y);
  BN_free(x);
  EC_KEY_free(ec_key);
  return ret;
}

/***************** No HROW VERSION *********/
/* the following functions use errors handling instead of throwing exception,
 * no_throw versions should be preferably used*/
/* the duplication is quite ugly, a macro to only map the throw would be
 * cleaner*/
/* TODO: factorize using a macro for throw*/

cx_err_t cx_ecdsa_sign_no_throw(const cx_ecfp_private_key_t *key, uint32_t mode,
                                cx_md_t hashID, const uint8_t *hash,
                                size_t hash_len, uint8_t *sig, size_t *sig_len,
                                uint32_t *info)
{
  int nid = 0;
  uint8_t *buf_r, *buf_s;
  const BIGNUM *r, *s;
  const cx_hash_info_t *hash_info;
  bool do_rfc6979_signature;
  BN_CTX *ctx = BN_CTX_new();

  const cx_curve_weierstrass_t *domain =
      (const cx_curve_weierstrass_t *)cx_ecfp_get_domain(key->curve);
  nid = nid_from_curve(key->curve);
  if (nid < 0) {
    return CX_NULLSIZE;
  }

  switch (mode & CX_MASK_RND) {
  case CX_RND_TRNG:
    // Use deterministic signatures with "TRNG" mode in Speculos, in order to
    // avoid signing objects with a weak PRNG, only if the hash is compatible
    // with cx_hmac_init (hash_info->output_size is defined).
    // Otherwise, fall back to OpenSSL signature.
    hash_info = spec_cx_hash_get_info(hashID);
    if (hash_info == NULL || hash_info->output_size == 0) {
      do_rfc6979_signature = false;
      break;
    }
    __attribute__((fallthrough));
  case CX_RND_RFC6979:
    do_rfc6979_signature = true;
    break;
  default:
    return CX_INVALID_PARAMETER;
  }

  cx_rnd_rfc6979_ctx_t rfc_ctx = {};
  uint8_t rnd[CX_RFC6979_MAX_RLEN];
  BIGNUM *q = BN_new();
  BIGNUM *x = BN_new();
  BIGNUM *k = BN_new();
  BIGNUM *kinv = BN_new();
  BIGNUM *rp = BN_new();
  BIGNUM *kg_y = BN_new();

  BN_bin2bn(domain->n, domain->length, q);
  BN_bin2bn(key->d, key->d_len, x);

  EC_GROUP *Stark = NULL;

  if (cx_generic_curve(domain, ctx, &Stark) != CX_OK)
    return CX_NULLSIZE;
  /* get generic curve group from parameters*/
  /* curve must be stored in weierstrass form in C_cx_all_Weierstrass_Curves*/

  /* TODO: test OpenSSL return values*/
  // nid = nid_from_curve(key->curve);
  EC_KEY *ec_key = EC_KEY_new();
  EC_KEY_set_group(ec_key, Stark);

  //  EC_KEY *ec_key = EC_KEY_new_by_curve_name(nid);
  // EC_KEY_set_public_key_affine_coordinates(ec_key, x, y);

  //  EC_KEY *ec_key = EC_KEY_new_by_curve_name(nid);
  EC_KEY_set_private_key(ec_key, x);

  const EC_GROUP *group = EC_KEY_get0_group(ec_key);
  EC_POINT *kg = EC_POINT_new(group);
  ECDSA_SIG *ecdsa_sig;

  if (do_rfc6979_signature) {
    if (domain->length >= CX_RFC6979_MAX_RLEN) {
      // errx(1, "rfc6979_ecdsa_sign: curve too large");
      return CX_INVALID_PARAMETER_SIZE;
    }
    spec_cx_rng_rfc6979_init(&rfc_ctx, hashID, key->d, key->d_len, hash,
                             hash_len, domain->n, domain->length);
  }
  do {
    if (do_rfc6979_signature) {
      spec_cx_rng_rfc6979_next(&rfc_ctx, rnd, domain->length);
      BN_bin2bn(rnd, domain->length, k);
    } else {
      BN_rand(k, domain->length, 0, 0);
    }
    if (!EC_POINT_mul(group, kg, k, NULL, NULL, ctx)) {
      // errx(1, "ssl: EC_POINT_mul");
      return OPEN_SSL_UNEXPECTED_ERROR;
    }
    if (!EC_POINT_get_affine_coordinates(group, kg, rp, kg_y, ctx)) {
      // errx(1, "ssl: EC_POINT_get_affine_coordinates");
    }
    if (BN_cmp(rp, q) >= 0) {
      if (!BN_nnmod(rp, rp, q, ctx)) {
        // errx(1, "ssl: BN_nnmod");
        return OPEN_SSL_UNEXPECTED_ERROR;
      }
      if (info != NULL) {
        *info |= CX_ECCINFO_xGTn;
      }
    }
    if (BN_is_zero(rp)) {
      continue;
    }
    if (info != NULL && BN_is_odd(kg_y)) {
      *info |= CX_ECCINFO_PARITY_ODD;
    }

    BN_mod_inverse(kinv, k, q, ctx);
    ecdsa_sig = ECDSA_do_sign_ex(hash, hash_len, kinv, rp, ec_key);
  } while (ecdsa_sig == NULL);
  BN_CTX_free(ctx);
  BN_free(q);
  BN_free(x);
  BN_free(k);
  BN_free(kinv);
  BN_free(rp);
  BN_free(kg_y);
  EC_POINT_free(kg);

  // normalize signature (s < n/2) if needed
  BIGNUM *halfn = BN_new();
  const BIGNUM *n = EC_GROUP_get0_order(group);
  BN_rshift1(halfn, n);

  BIGNUM *normalized_s = BN_new();
  ECDSA_SIG_get0(ecdsa_sig, &r, &s);
  if ((mode & CX_NO_CANONICAL) == 0 && BN_cmp(s, halfn) > 0) {
    // fprintf(stderr, "cx_ecdsa_sign: normalizing s > n/2\n");
    BN_sub(normalized_s, n, s);

    if (info != NULL) {
      *info ^= CX_ECCINFO_PARITY_ODD; // Inverse the bit
    }
  } else {
    normalized_s = BN_copy(normalized_s, s);
  }

  buf_r = malloc(domain->length);
  buf_s = malloc(domain->length);
  BN_bn2binpad(r, buf_r, domain->length);
  BN_bn2binpad(normalized_s, buf_s, domain->length);
  *sig_len = spec_cx_ecfp_encode_sig_der(sig, 2 * domain->length, buf_r,
                                         domain->length, buf_s, domain->length);
  free(buf_r);
  free(buf_s);
  ECDSA_SIG_free(ecdsa_sig);
  BN_free(normalized_s);
  BN_free(halfn);
  EC_KEY_free(ec_key);
  return CX_OK;
}
