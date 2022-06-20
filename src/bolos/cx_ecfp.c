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

int sys_cx_ecfp_generate_pair_no_throw(cx_curve_t curve,
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
      /*errx(1, "nid not supported, refer to manual 'how to add a curve to "
              "speculos.txt' ");*/
      return CX_EC_INVALID_CURVE;
    }

    key = EC_KEY_new();

    if (cx_generic_curve(weier, ctx, &Stark) != 0)
      return CX_KO;

    if (Stark == NULL)
      return CX_EC_INVALID_CURVE;

    ret = EC_KEY_set_group(key, Stark); // here for openSSL OK=1 !!!!
    if (ret != OPEN_SSL_OK) {
      return OPEN_SSL_UNEXPECTED_ERROR;
    }

    pub = EC_POINT_new(Stark);
    if (pub == NULL)
      return OPEN_SSL_UNEXPECTED_ERROR;

    if (!keep_private) {
      if (EC_KEY_generate_key(key) == 0) {
        return OPEN_SSL_UNEXPECTED_ERROR;
      }

      const BIGNUM *priv = EC_KEY_get0_private_key(key);
      if (BN_num_bytes(priv) > (int)weier->length) {
        // errx(1, "ssl: invalid bn");
        return OPEN_SSL_UNEXPECTED_ERROR;
      }
      private_key->curve = curve;
      private_key->d_len = BN_bn2bin(priv, private_key->d);
    } else {
      BIGNUM *priv;

      priv = BN_new();

      if (BN_bin2bn(private_key->d, private_key->d_len, priv) == NULL) {
        // errx(1, "ssl : BN_bin2bn of private key");
        return OPEN_SSL_UNEXPECTED_ERROR;
      }

      if (EC_KEY_set_private_key(key, priv) == 0) {
        // errx(1, "ssl : EC_KEY_set_private_key");
        return OPEN_SSL_UNEXPECTED_ERROR;
      }

      if (EC_POINT_mul(Stark, pub, priv, NULL, NULL, ctx) == 0) {
        // errx(1, "ssl: EC_POINT_mul");
        return OPEN_SSL_UNEXPECTED_ERROR;
      }

      if (EC_KEY_set_public_key(key, pub) == OPEN_SSL_KO) {
        // errx(1, "ssl: EC_KEY_set_public_key");
        return OPEN_SSL_UNEXPECTED_ERROR;
      }

      BN_free(priv);
      EC_POINT_free(pub);
    }

    bn = BN_new();
    EC_POINT_point2bn(Stark, EC_KEY_get0_public_key(key),
                      POINT_CONVERSION_UNCOMPRESSED, bn, ctx);

    if (BN_num_bytes(bn) > 2 * (int)weier->length + 1) {
      // errx(1, "ssl: invalid bn");
      return OPEN_SSL_UNEXPECTED_ERROR;
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

int cx_ecfp_init_private_key_no_throw(cx_curve_t curve,
                                      const unsigned char *rawkey,
                                      unsigned int key_len,
                                      cx_ecfp_public_key_t *key)
{
  const cx_curve_domain_t *domain;
  unsigned int expected_key_len;
  unsigned int size;
  int ret;

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
      ret = CX_INVALID_PARAMETER_SIZE;
      goto end;
    }
  } else {
    key_len = 0;
  }
  // init key
  key->curve = curve;
  key->W_len = key_len;
  memcpy(key->W, rawkey, key_len);

end:
  return ret;
}
