
//#define _SDK_2_0_
#include <err.h>
#include <errno.h>
#include <openssl/bn.h>
#include <openssl/objects.h>
#include <openssl/rand.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "bolos/cxlib.h"
#include "cx_ec.h"
#include "cx_ecfp.h"
#include "cx_wrap_ossl.h"

static void const *sys_cx_ecdomain_param_ptr(const cx_curve_domain_t *domain,
                                             cx_curve_dom_param_t id)
{
  switch (id) {
  case CX_CURVE_PARAM_A:
    return domain->a;
  case CX_CURVE_PARAM_B:
    return domain->b;
  case CX_CURVE_PARAM_Field:
    return domain->p;
  case CX_CURVE_PARAM_Gx:
    return domain->Gx;
  case CX_CURVE_PARAM_Gy:
    return domain->Gy;
  case CX_CURVE_PARAM_Order:
    return domain->n;
  case CX_CURVE_PARAM_Cofactor:
    return domain->h;
  default:
    break;
  }
  return NULL;
}

cx_err_t sys_cx_ecdomain_parameter(cx_curve_t curve, cx_curve_dom_param_t id,
                                   uint8_t *p, uint32_t p_len)
{
  void const *ptr;
  const cx_curve_domain_t *domain;

  domain = cx_ecdomain(curve);
  if (domain == NULL) {
    return CX_EC_INVALID_CURVE;
  }

  ptr = sys_cx_ecdomain_param_ptr(domain, id);
  if ((ptr == NULL) || (domain->length > p_len)) {
    return CX_INVALID_PARAMETER;
  }

  if (id == CX_CURVE_PARAM_Cofactor) {
    memmove(p, ptr, 4);
  } else {
    memmove(p, ptr, domain->length);
  }
  return CX_OK;
}

cx_err_t sys_cx_ecdomain_parameter_bn(cx_curve_t curve, cx_curve_dom_param_t id,
                                      cx_bn_t bn_p)
{
  cx_err_t error;
  cx_mpi_t *p;
  void const *ptr;
  const cx_curve_domain_t *domain;

  domain = cx_ecdomain(curve);
  if (domain == NULL) {
    return CX_EC_INVALID_CURVE;
  }
  CX_CHECK(cx_bn_to_mpi(bn_p, &p));

  ptr = sys_cx_ecdomain_param_ptr(domain, id);
  if (ptr == NULL) {
    return CX_INVALID_PARAMETER;
  }
  // Copy the content of ptr into BN:
  if (id == CX_CURVE_PARAM_Cofactor) {
    BN_bin2bn(ptr, 4, p);
  } else {
    BN_bin2bn(ptr, domain->length, p);
  }
end:
  return error;
}

cx_err_t sys_cx_ecdomain_generator(cx_curve_t curve, uint8_t *Gx, uint8_t *Gy,
                                   size_t len)
{
  const cx_curve_domain_t *domain;

  domain = cx_ecdomain(curve);
  if (domain == NULL) {
    return CX_EC_INVALID_CURVE;
  }
  if (domain->length > len) {
    return CX_INVALID_PARAMETER;
  }
  memmove(Gx, domain->Gx, domain->length);
  memmove(Gy, domain->Gy, domain->length);

  return CX_OK;
}

cx_err_t sys_cx_ecdomain_generator_bn(cx_curve_t curve, cx_ecpoint_t *ec_P)
{
  cx_err_t error;
  cx_mpi_ecpoint_t P;
  const cx_curve_domain_t *domain;

  CX_CHECK(cx_bn_locked());

  domain = cx_ecdomain(curve);
  if (domain == NULL) {
    return CX_EC_INVALID_CURVE;
  }
  if ((curve != ec_P->curve) ||
      (cx_mpi_ecpoint_from_ecpoint(&P, ec_P) != CX_OK)) {
    return CX_EC_INVALID_POINT;
  }
  BN_bin2bn(domain->Gx, domain->length, P.x);
  BN_bin2bn(domain->Gy, domain->length, P.y);
  cx_mpi_set_u32(P.z, 1);
end:
  return error;
}

cx_err_t sys_cx_ecdomain_parameters_length(cx_curve_t curve, size_t *length)
{
  const cx_curve_domain_t *domain;

  domain = cx_ecdomain(curve);
  if (domain == NULL) {
    return CX_EC_INVALID_CURVE;
  }

  *length = domain->length;
  return CX_OK;
}

cx_err_t sys_cx_ecdomain_size(cx_curve_t curve, size_t *length)
{
  const cx_curve_domain_t *domain;

  domain = cx_ecdomain(curve);
  if (domain == NULL) {
    return CX_EC_INVALID_CURVE;
  }

  *length = domain->bit_size;
  return CX_OK;
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
      ret=CX_INVALID_PARAMETER_SIZE;
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
        //errx(1, "ssl: invalid bn");
     	  return OPEN_SSL_UNEXPECTED_ERROR;
      }
      private_key->curve = curve;
      private_key->d_len = BN_bn2bin(priv, private_key->d);
    } else {
      BIGNUM *priv;

      priv = BN_new();

      if (BN_bin2bn(private_key->d, private_key->d_len, priv) == NULL) {
        //errx(1, "ssl : BN_bin2bn of private key");
    	  return OPEN_SSL_UNEXPECTED_ERROR;
      }

      if (EC_KEY_set_private_key(key, priv) == 0) {
        //errx(1, "ssl : EC_KEY_set_private_key");
        return OPEN_SSL_UNEXPECTED_ERROR;
      }

      if (EC_POINT_mul(Stark, pub, priv, NULL, NULL, ctx) == 0) {
        //errx(1, "ssl: EC_POINT_mul");
        return OPEN_SSL_UNEXPECTED_ERROR;
      }

      if (EC_KEY_set_public_key(key, pub) == OPEN_SSL_KO) {
        //errx(1, "ssl: EC_KEY_set_public_key");
    	  return OPEN_SSL_UNEXPECTED_ERROR;
      }

      BN_free(priv);
      EC_POINT_free(pub);
    }

    bn = BN_new();
    EC_POINT_point2bn(Stark, EC_KEY_get0_public_key(key),
                      POINT_CONVERSION_UNCOMPRESSED, bn, ctx);

    if (BN_num_bytes(bn) > 2 * (int)weier->length + 1) {
      //errx(1, "ssl: invalid bn");
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

/*
cx_edwards_compress_point_no_throw() added in cx_ed25519.c
os_perso_derive_node_with_seed_key() already present, exported in .h
cx_ecfp_init_private_key_no_throw() added in cx_ecfp.c
cx_ecfp_generate_pair_no_throw()    added in cx_ecfp.c
cx_ecdsa_sign_no_throw() added in cx_ecdsa.c
cx_eddsa_sign_no_throw()  added in cx_ecdsa.c
cx_ecdomain_parameters_length() already present, exported in .h
*/



