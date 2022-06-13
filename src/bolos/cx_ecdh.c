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
#include "cx_hash.h"
#include "cx_rng_rfc6979.h"
#include "cx_utils.h"
#include "emulate.h"
#include "cx_wrap_ossl.h"
#include "cx_ecdh.h"


static int ecdh_simple_compute_key_hack(unsigned char *pout, size_t *poutlen,
                                        const EC_POINT *pub_key,
                                        const EC_KEY *ecdh)
{
  BN_CTX *ctx;
  EC_POINT *tmp = NULL;
  BIGNUM *x = NULL, *y = NULL;
  int xLen, yLen;
  const BIGNUM *priv_key;
  const EC_GROUP *group;
  size_t group_size;
  int ret = 0;
  unsigned char *buf = NULL;

  if ((ctx = BN_CTX_new()) == NULL) {
    goto err;
  }
  BN_CTX_start(ctx);
  x = BN_CTX_get(ctx);
  if (x == NULL) {
    ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, ERR_R_MALLOC_FAILURE);
    goto err;
  }

  y = BN_CTX_get(ctx);
  if (y == NULL) {
    ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, ERR_R_MALLOC_FAILURE);
    goto err;
  }

  priv_key = EC_KEY_get0_private_key(ecdh);
  if (priv_key == NULL) {
    ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, EC_R_NO_PRIVATE_VALUE);
    goto err;
  }

  group = EC_KEY_get0_group(ecdh);
  group_size = (EC_GROUP_get_degree(group) + 7) / 8;

  if (EC_KEY_get_flags(ecdh) & EC_FLAG_COFACTOR_ECDH) {
    if (!EC_GROUP_get_cofactor(group, x, NULL) ||
        !BN_mul(x, x, priv_key, ctx)) {
      ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, ERR_R_MALLOC_FAILURE);
      goto err;
    }
    priv_key = x;
  }

  if ((tmp = EC_POINT_new(group)) == NULL) {
    ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, ERR_R_MALLOC_FAILURE);
    goto err;
  }

  if (!EC_POINT_mul(group, tmp, NULL, pub_key, priv_key, ctx)) {
    ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, EC_R_POINT_ARITHMETIC_FAILURE);
    goto err;
  }

  if (!EC_POINT_get_affine_coordinates(group, tmp, x, y, ctx)) {
    ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, EC_R_POINT_ARITHMETIC_FAILURE);
    goto err;
  }

  if (*poutlen < (1 + 32 + 32)) {
    ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, EC_R_INVALID_ARGUMENT);
    goto err;
  }

  pout[0] = 0x04;
  xLen = BN_bn2binpad(x, pout + 1, group_size);
  if (xLen < 0) {
    ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, ERR_R_BN_LIB);
    goto err;
  }
  yLen = BN_bn2binpad(y, pout + 1 + xLen, group_size);
  if (yLen < 0) {
    ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, ERR_R_BN_LIB);
    goto err;
  }
  *poutlen = 1 + xLen + yLen;
  buf = NULL;

  ret = 1;

err:
  EC_POINT_clear_free(tmp);
  BN_CTX_end(ctx);
  BN_CTX_free(ctx);
  OPENSSL_free(buf);
  return ret;
}

int sys_cx_ecdh(const cx_ecfp_private_key_t *key, int mode,
                const uint8_t *public_point, size_t UNUSED(P_len),
                uint8_t *secret, size_t secret_len)
{
  const cx_curve_domain_t *domain;
  EC_KEY *privkey, *peerkey;
  // uint8_t point[65];
  BIGNUM *x, *y;
  int nid;

  domain = cx_ecfp_get_domain(key->curve);

  x = BN_new();
  BN_bin2bn(key->d, key->d_len, x);
  nid = nid_from_curve(key->curve);
  privkey = EC_KEY_new_by_curve_name(nid);
  EC_KEY_set_private_key(privkey, x);
  BN_free(x);

  x = BN_new();
  y = BN_new();
  BN_bin2bn(public_point + 1, domain->length, x);
  BN_bin2bn(public_point + domain->length + 1, domain->length, y);

  nid = nid_from_curve(key->curve);
  peerkey = EC_KEY_new_by_curve_name(nid);
  EC_KEY_set_public_key_affine_coordinates(peerkey, x, y);

  BN_free(y);
  BN_free(x);

  switch (mode & CX_MASK_EC) {
  case CX_ECDH_POINT:
    if (!ecdh_simple_compute_key_hack(
            secret, &secret_len, EC_KEY_get0_public_key(peerkey), privkey)) {
      THROW(INVALID_PARAMETER);
    }
    break;
  case CX_ECDH_X:
    secret_len = ECDH_compute_key(
        secret, secret_len, EC_KEY_get0_public_key(peerkey), privkey, NULL);
    break;
  }

  EC_KEY_free(privkey);
  EC_KEY_free(peerkey);

  /* XXX: not used in the result */
  /*switch (mode & CX_MASK_EC) {
  case CX_ECDH_POINT:
    if (public_point[0] == 4) {
      len = 1 + 2 * domain->length;
    } else {
      len = 1 + domain->length;
    }
    break;

  case CX_ECDH_X:
    len = domain->length;
    break;

  default:
    THROW(INVALID_PARAMETER);
    break;
  }*/
  return secret_len;
}
