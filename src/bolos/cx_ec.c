#include <err.h>
#include <string.h>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/objects.h>
#include <openssl/sha.h>

#include "bolos/exception.h"
#include "cx.h"
#include "cx_curve25519.h"
#include "cx_ec.h"
#include "cx_ed25519.h"
#include "cx_errors.h"
#include "cx_hash.h"
#include "cx_rng_rfc6979.h"
#include "cx_utils.h"
#include "cx_wrap_ossl.h"
#include "emulate.h"

#include "cx_const_brainpool.c"
#include "cx_const_curves_zoo.c"
#include "cx_const_ecbls12381.c"
#include "cx_const_ecedd25519.c"
#include "cx_const_sec256k1.c"
#include "cx_const_secp.c"
#include "cx_const_stark.c"

cx_curve_domain_t const *const C_cx_allCurves[] = {
  (const cx_curve_domain_t *)&C_cx_secp256k1,
  (const cx_curve_domain_t *)&C_cx_secp256r1,
  (const cx_curve_domain_t *)&C_cx_secp384r1,
  (const cx_curve_domain_t *)&C_cx_Ed25519,
  (const cx_curve_domain_t *)&C_cx_BLS12_381_G1,
  (const cx_curve_domain_t *)&C_cx_BrainpoolP256r1,
  (const cx_curve_domain_t *)&C_cx_BrainpoolP256t1,
  (const cx_curve_domain_t *)&C_cx_BrainpoolP320r1,
  (const cx_curve_domain_t *)&C_cx_BrainpoolP320t1,
  (const cx_curve_domain_t *)&C_cx_BrainpoolP384r1,
  (const cx_curve_domain_t *)&C_cx_BrainpoolP384t1,
  (const cx_curve_domain_t *)&C_cx_BrainpoolP512r1,
  (const cx_curve_domain_t *)&C_cx_BrainpoolP512t1,
  (const cx_curve_domain_t *)&C_cx_Stark256
};

cx_curve_weierstrass_t const *const C_cx_all_Weierstrass_Curves[] = {
  (const cx_curve_weierstrass_t *)&C_cx_secp256k1,
  (const cx_curve_weierstrass_t *)&C_cx_secp256r1,
  (const cx_curve_weierstrass_t *)&C_cx_secp384r1,
  (const cx_curve_weierstrass_t *)&C_cx_Ed25519,
  (const cx_curve_weierstrass_t *)&C_cx_BLS12_381_G1,
  (const cx_curve_weierstrass_t *)&C_cx_BrainpoolP256r1,
  (const cx_curve_weierstrass_t *)&C_cx_BrainpoolP256t1,
  (const cx_curve_weierstrass_t *)&C_cx_BrainpoolP320r1,
  (const cx_curve_weierstrass_t *)&C_cx_BrainpoolP320t1,
  (const cx_curve_weierstrass_t *)&C_cx_BrainpoolP384r1,
  (const cx_curve_weierstrass_t *)&C_cx_BrainpoolP384t1,
  (const cx_curve_weierstrass_t *)&C_cx_BrainpoolP512r1,
  (const cx_curve_weierstrass_t *)&C_cx_BrainpoolP512t1,
  (const cx_curve_weierstrass_t *)&C_cx_Stark256
};

/* Unexported functions from OpenSSL, in ec/curve25519.c. Dirty hack... */
int ED25519_sign(uint8_t *out_sig, const uint8_t *message, size_t message_len,
                 const uint8_t public_key[32], const uint8_t private_key[32]);
int ED25519_verify(const uint8_t *message, size_t message_len,
                   const uint8_t signature[64], const uint8_t public_key[32]);
void ED25519_public_from_private(uint8_t out_public_key[32],
                                 const uint8_t private_key[32]);

int sys_cx_eddsa_get_public_key(const cx_ecfp_private_key_t *pv_key,
                                cx_md_t hashID, cx_ecfp_public_key_t *pu_key)
{
  uint8_t digest[SHA512_DIGEST_LENGTH];

  if (hashID != CX_SHA512) {
    errx(1, "cx_eddsa_get_public_key: invalid hashId (0x%x)", hashID);
    return -1;
  }

  if (pv_key->d_len != 32) {
    errx(1, "cx_eddsa_get_public_key: invalid key size (0x%zx)", pv_key->d_len);
    return -1;
  }

  SHA512(pv_key->d, pv_key->d_len, digest);

  digest[0] &= 0xf8;
  digest[31] &= 0x7f;
  digest[31] |= 0x40;

  le2be(digest, 32);

  pu_key->curve = CX_CURVE_Ed25519;
  pu_key->W_len = 1 + 2 * 32;
  pu_key->W[0] = 0x04;
  memcpy(pu_key->W + 1, C_cx_Ed25519_Bx, sizeof(C_cx_Ed25519_Bx));
  memcpy(pu_key->W + 1 + 32, C_cx_Ed25519_By, sizeof(C_cx_Ed25519_By));

  return sys_cx_ecfp_scalar_mult(CX_CURVE_Ed25519, pu_key->W, pu_key->W_len,
                                 digest, 32);
}

const cx_curve_weierstrass_t *cx_ecfp_get_weierstrass(cx_curve_t curve)
{
  unsigned int i;
  for (i = 0; i < sizeof(C_cx_allCurves) / sizeof(cx_curve_domain_t *); i++) {
    if (C_cx_all_Weierstrass_Curves[i]->curve == curve) {
      return C_cx_all_Weierstrass_Curves[i];
    }
  }
  THROW(INVALID_PARAMETER);
  return NULL;
}

int sys_cx_ecfp_generate_pair2(cx_curve_t curve,
                               cx_ecfp_public_key_t *public_key,
                               cx_ecfp_private_key_t *private_key,
                               int keep_private, cx_md_t hashID)
{
  EC_GROUP *Stark = NULL;

  EC_POINT *pub;
  BN_CTX *ctx;
  EC_KEY *key;
  BIGNUM *bn;
  int nid, ret;
  ctx = BN_CTX_new();

  const cx_curve_weierstrass_t *weier = cx_ecfp_get_weierstrass(curve);

  if (curve == CX_CURVE_Ed25519) {
    return sys_cx_eddsa_get_public_key(private_key, hashID, public_key);
  } else {

    nid = nid_from_curve(curve);
    if (nid == -1) {
      errx(1, "nid not supported, refer to manual 'how to add a curve to "
              "speculos.txt' ");
      return CX_KO;
    }

    key = EC_KEY_new();

    if (cx_generic_curve(weier, ctx, &Stark) != 0)
      return CX_KO;

    if (Stark == NULL)
      errx(1, "setting ecc group failed");

    ret = EC_KEY_set_group(key, Stark); // here for openSSL OK=1 !!!!
    if (ret != 1) {
      errx(1, "ssl: EC_KEY_set_group");
    }

    pub = EC_POINT_new(Stark);
    if (pub == NULL)
      return CX_KO;

    if (!keep_private) {
      if (EC_KEY_generate_key(key) == 0) {
        errx(1, "ssl: EC_KEY_generate_key");
      }

      const BIGNUM *priv = EC_KEY_get0_private_key(key);
      if (BN_num_bytes(priv) > (int)weier->length) {
        errx(1, "ssl: invalid bn");
      }
      private_key->curve = curve;
      private_key->d_len = BN_bn2bin(priv, private_key->d);
    } else {
      BIGNUM *priv;

      priv = BN_new();

      if (BN_bin2bn(private_key->d, private_key->d_len, priv) == NULL) {
        errx(1, "ssl : BN_bin2bn of private key");
      }

      if (EC_KEY_set_private_key(key, priv) == 0) {
        errx(1, "ssl : EC_KEY_set_private_key");
      }

      if (EC_POINT_mul(Stark, pub, priv, NULL, NULL, ctx) == 0) {
        errx(1, "ssl: EC_POINT_mul");
      }

      if (EC_KEY_set_public_key(key, pub) == OPEN_SSL_KO) {
        errx(1, "ssl: EC_KEY_set_public_key");
      }

      BN_free(priv);
      EC_POINT_free(pub);
    }

    bn = BN_new();
    EC_POINT_point2bn(Stark, EC_KEY_get0_public_key(key),
                      POINT_CONVERSION_UNCOMPRESSED, bn, ctx);

    if (BN_num_bytes(bn) > 2 * (int)weier->length + 1) {
      errx(1, "ssl: invalid bn");
    }
    public_key->curve = curve;
    public_key->W_len = BN_bn2bin(bn, public_key->W);
    EC_POINT_point2oct(Stark, EC_KEY_get0_public_key(key),
                       POINT_CONVERSION_UNCOMPRESSED, public_key->W,
                       public_key->W_len, ctx);

    BN_free(bn);
    EC_KEY_free(key);
    BN_CTX_free(ctx);

    return CX_OK;
  }
}

int sys_cx_ecfp_generate_pair(cx_curve_t curve,
                              cx_ecfp_public_key_t *public_key,
                              cx_ecfp_private_key_t *private_key,
                              int keep_private)
{
  return sys_cx_ecfp_generate_pair2(curve, public_key, private_key,
                                    keep_private, CX_SHA512);
}

int sys_cx_ecfp_init_private_key(cx_curve_t curve, const uint8_t *raw_key,
                                 unsigned int key_len,
                                 cx_ecfp_private_key_t *key)
{
  /* TODO: missing checks */

  if (raw_key != NULL) {
    key->d_len = key_len;
    memmove(key->d, raw_key, key_len);
  } else {
    key_len = 0;
  }

  key->curve = curve;

  /* TODO: modify bolos API to return an errcode instead*/
  return key_len;
}

const cx_curve_domain_t *cx_ecfp_get_domain(cx_curve_t curve)
{
  unsigned int i;
  for (i = 0; i < sizeof(C_cx_allCurves) / sizeof(cx_curve_domain_t *); i++) {
    if (C_cx_allCurves[i]->curve == curve) {
      return C_cx_allCurves[i];
    }
  }
  THROW(INVALID_PARAMETER);
}

int cx_ecdomain_parameters_length(cx_curve_t i_curve, size_t *o_t8curve)
{

  const cx_curve_domain_t *ps_curve = cx_ecfp_get_domain(i_curve);
  if ((ps_curve) == NULL)
    return CX_EC_INVALID_CURVE;

  *o_t8curve = (size_t)ps_curve->length;
  return CX_OK;
}

int cx_ecdomain_parameters_size(cx_curve_t i_curve, size_t *o_t8curve)
{

  const cx_curve_domain_t *ps_curve = cx_ecfp_get_domain(i_curve);
  if ((ps_curve) == NULL)
    return CX_EC_INVALID_CURVE;

  *o_t8curve = ((size_t)ps_curve->length) << 3;
  return CX_OK;
}

unsigned long sys_cx_ecfp_init_public_key(cx_curve_t curve,
                                          const unsigned char *rawkey,
                                          unsigned int key_len,
                                          cx_ecfp_public_key_t *key)
{
  const cx_curve_domain_t *domain;
  unsigned int expected_key_len;
  unsigned int size;

  domain = cx_ecfp_get_domain(curve);
  size = domain->length;

  memset(key, 0, sizeof(cx_ecfp_public_key_t));

  if (rawkey != NULL) {
    expected_key_len = 0;
    if (rawkey[0] == 0x02) {
      if (CX_CURVE_RANGE(curve, TWISTED_EDWARDS) ||
          CX_CURVE_RANGE(curve, MONTGOMERY)) {
        expected_key_len = 1 + size;
      }
    } else if (rawkey[0] == 0x04) {
      if (CX_CURVE_RANGE(curve, WEIERSTRASS) ||
          CX_CURVE_RANGE(curve, TWISTED_EDWARDS)) {
        expected_key_len = 1 + size * 2;
      }
    }
    if (expected_key_len == 0 || key_len != expected_key_len) {
      THROW(INVALID_PARAMETER);
    }
  } else {
    key_len = 0;
  }
  // init key
  key->curve = curve;
  key->W_len = key_len;
  memcpy(key->W, rawkey, key_len);

  /* TODO: modify bolos API to return an errcode instead*/
  return key_len;
}

static int cx_weierstrass_mult(cx_curve_t curve, BIGNUM *qx, BIGNUM *qy,
                               BIGNUM *px, BIGNUM *py, BIGNUM *k)
{
  EC_POINT *p, *q;
  EC_GROUP *group = NULL;
  BN_CTX *ctx;
  int ret = CX_KO;

  if (curve == CX_CURVE_SECP256K1) {
    group = EC_GROUP_new_by_curve_name(NID_secp256k1);
  } else if (curve == CX_CURVE_256R1) {
    group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
  } else {
    return 0;
  }
  if (group == NULL) {
    errx(1, "cx_weierstrass_mult: EC_GROUP_new_by_curve_name() failed");
  }

  p = EC_POINT_new(group);
  q = EC_POINT_new(group);
  if (p == NULL || q == NULL) {
    errx(1, "cx_weierstrass_mult: EC_POINT_new() failed");
  }

  ctx = BN_CTX_new();
  if (ctx == NULL) {
    errx(1, "cx_weierstrass_mult: BN_CTX_new() failed");
  }

  if (EC_POINT_set_affine_coordinates(group, p, px, py, ctx) != 1) {
    goto err;
  }
  if (EC_POINT_mul(group, q, NULL, p, k, ctx) != 1) {
    goto err;
  }
  if (EC_POINT_get_affine_coordinates(group, q, qx, qy, ctx) != 1) {
    goto err;
  }

  ret = CX_OK;

err:
  BN_CTX_free(ctx);
  EC_GROUP_free(group);
  EC_POINT_free(p);
  EC_POINT_free(q);
  return ret;
}

int sys_cx_ecfp_scalar_mult(cx_curve_t curve, unsigned char *P,
                            unsigned int P_len, const unsigned char *k,
                            unsigned int k_len)
{
  BIGNUM *Px, *Py, *Qx, *Qy, *e;

  /* TODO: ensure that the point is valid */

  const uint8_t PTy = P[0];

  if (curve == CX_CURVE_Curve25519) {
    if (PTy != 0x02) {
      errx(1, "cx_ecfp_scalar_mult: only compressed points for Curve25519 are "
              "supported.");
    }
    if (P_len != (32 + 1) || k_len != 32) {
      errx(1, "cx_ecfp_scalar_mult: expected P_len == 33 and k_len == 32");
    }
    if (scalarmult_curve25519(&P[1], k, &P[1]) != CX_OK) {
      errx(1, "cx_ecfp_scalar_mult: scalarmult_x25519 failed");
    }
    return P_len;
  }

  if (P_len != 65) {
    errx(1, "cx_ecfp_scalar_mult: invalid P_len (%u)", P_len);
  }

  Px = BN_new();
  Py = BN_new();
  Qx = BN_new();
  Qy = BN_new();
  e = BN_new();

  if (Px == NULL || Py == NULL || Qx == NULL || Qy == NULL || e == NULL) {
    errx(1, "cx_ecfp_scalar_mult: BN_new() failed");
  }

  BN_bin2bn(P + 1, 32, Px);
  BN_bin2bn(P + 33, 32, Py);
  BN_bin2bn(k, k_len, e);

  switch (curve) {
  case CX_CURVE_Ed25519: {
    if (scalarmult_ed25519(Qx, Qy, Px, Py, e) != CX_OK) {
      errx(1, "cx_ecfp_scalar_mult: scalarmult_ed25519 failed");
    }
  } break;
  case CX_CURVE_SECP256K1:
  case CX_CURVE_SECP256R1:
    if (PTy != 0x04) {
      errx(1, "cx_ecfp_scalar_mult: compressed points for Weierstrass curves "
              "are not supported yet");
    }
    if (cx_weierstrass_mult(curve, Qx, Qy, Px, Py, e) != CX_OK) {
      errx(1, "cx_ecfp_scalar_mult: cx_weierstrass_mult failed");
    }
    break;
  default:
    errx(1, "cx_ecfp_scalar_mult: TODO: unsupported curve (0x%x)", curve);
    break;
  }

  BN_bn2binpad(Qx, P + 1, 32);
  BN_bn2binpad(Qy, P + 33, 32);

  BN_free(Qy);
  BN_free(Qx);
  BN_free(Py);
  BN_free(Px);
  BN_free(e);

  return CX_OK;
}

int sys_cx_ecfp_add_point(cx_curve_t curve, uint8_t *R, const uint8_t *P,
                          const uint8_t *Q, size_t X_len)
{
  POINT PP, QQ, RR;
  int ret;

  /* TODO: ensure that points are valid */

  PP.x = BN_new();
  PP.y = BN_new();
  QQ.x = BN_new();
  QQ.y = BN_new();
  RR.x = BN_new();
  RR.y = BN_new();
  if (PP.x == NULL || PP.y == NULL || QQ.x == NULL || QQ.y == NULL ||
      RR.x == NULL || RR.y == NULL) {
    errx(1, "cx_ecfp_scalar_mult: BN_new() failed");
    return -1;
  }

  switch (curve) {
  case CX_CURVE_Ed25519:
    if (X_len != 65) {
      errx(1, "cx_ecfp_add_point: invalid X_len (%zu)", X_len);
      ret = CX_KO;
      break;
    }

    if (P[0] != 0x04 || Q[0] != 0x4) {
      errx(1, "cx_ecfp_add_point: points must be uncompressed");
      ret = CX_KO;
      break;
    }

    BN_bin2bn(P + 1, 32, PP.x);
    BN_bin2bn(P + 33, 32, PP.y);
    BN_bin2bn(Q + 1, 32, QQ.x);
    BN_bin2bn(Q + 33, 32, QQ.y);

    if (edwards_add(&RR, &PP, &QQ) != 0) {
      errx(1, "cx_ecfp_add_point: edwards_add failed");
      ret = CX_KO;
      break;
    }

    R[0] = 0x04;
    BN_bn2binpad(RR.x, R + 1, 32);
    BN_bn2binpad(RR.y, R + 33, 32);
    ret = CX_OK;
    break;

  default:
    errx(1, "cx_ecfp_add_point: TODO: unsupported curve (0x%x)", curve);
    ret = CX_KO;
    break;
  }

  BN_free(RR.y);
  BN_free(RR.x);
  BN_free(QQ.y);
  BN_free(QQ.x);
  BN_free(PP.y);
  BN_free(PP.x);

  return ret;
}
