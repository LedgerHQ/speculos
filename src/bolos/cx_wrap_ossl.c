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
#include "cx_hash.h"
#include "cx_rng_rfc6979.h"
#include "cx_wrap_ossl.h"

/* get generic curve group from parameters*/
/* curve must be stored in weierstrass form in C_cx_all_Weierstrass_Curves*/
int cx_generic_curve(const cx_curve_weierstrass_t *i_weier, BN_CTX *ctx,
                     EC_GROUP **o_group)
{
  BIGNUM *p, *a, *b, *x, *y, *order;
  EC_POINT *P = NULL;

  /* Allocate a BN for each parameter*/
  p = BN_new();
  a = BN_new();
  b = BN_new();
  x = BN_new();
  y = BN_new();
  order = BN_new();

  BN_bin2bn(i_weier->p, i_weier->length, p);
  BN_bin2bn(i_weier->a, i_weier->length, a);
  BN_bin2bn(i_weier->b, i_weier->length, b);
  BN_bin2bn(i_weier->Gx, i_weier->length, x);
  BN_bin2bn(i_weier->Gy, i_weier->length, y);
  BN_bin2bn(i_weier->n, i_weier->length, order);

  *o_group = EC_GROUP_new_curve_GFp(p, a, b, ctx);

  P = EC_POINT_new(*o_group);
  // if(!EC_POINT_set_compressed_coordinates_GFp(Stark, P, x, 1, ctx))

  if (!EC_POINT_set_affine_coordinates_GFp(*o_group, P, x, y, ctx)) {
    errx(1, "Set affine coordinates failed");
    return -1;
  }
  if (!EC_POINT_is_on_curve(*o_group, P, ctx)) {
    errx(1, "generator not on curve: check curve parameters p,a,b,x,y,q");
    return -1;
  }
  if (!EC_GROUP_set_generator(*o_group, P, order, BN_value_one())) {
    errx(1, "\n generator set failed); //ici pour openSSL OK=1 !!!!");
    return -1;
  }

  if (!EC_GROUP_check(*o_group, ctx)) {
    errx(1, "EC_GROUP_check() failed");
    return -1;
  }

  /* free BN parameters*/
  BN_free(p);
  BN_free(a);
  BN_free(b);
  BN_free(x);
  BN_free(y);
  BN_free(order);
  /* free generator*/
  EC_POINT_free(P);

  return 0;
}
#include "cx_utils.h"
#include "emulate.h"

#include "cx_wrap_ossl.h"

int nid_from_curve(cx_curve_t curve)
{
  int nid;

  switch (curve) {
  case CX_CURVE_SECP256K1:
    nid = NID_secp256k1;
    break;
  case CX_CURVE_SECP256R1:
    nid = NID_X9_62_prime256v1;
    break;
  case CX_CURVE_SECP384R1:
    nid = NID_secp384r1;
    break;
  case CX_CURVE_BrainPoolP256R1:
    nid = NID_brainpoolP256r1;
    break;
  case CX_CURVE_BrainPoolP256T1:
    nid = NID_brainpoolP256t1;
    break;
  case CX_CURVE_BrainPoolP320R1:
    nid = NID_brainpoolP320r1;
    break;
  case CX_CURVE_BrainPoolP320T1:
    nid = NID_brainpoolP320t1;
    break;
  case CX_CURVE_BrainPoolP384R1:
    nid = NID_brainpoolP384r1;
    break;
  case CX_CURVE_BrainPoolP384T1:
    nid = NID_brainpoolP384t1;
    break;
  case CX_CURVE_BrainPoolP512R1:
    nid = NID_brainpoolP512r1;
    break;
  case CX_CURVE_BrainPoolP512T1:
    nid = NID_brainpoolP512t1;
    break;
  case CX_CURVE_Stark256:
    nid = NID_Stark256;
    break;
#if 0
  case CX_CURVE_SECP521R1:
    nid = NID_secp521r1;
    break;
#endif
  default:
    nid = -1;
    errx(1, "cx_ecfp_generate_pair unsupported curve");
    break;
  }
  return nid;
}

int spec_cx_ecfp_encode_sig_der(unsigned char *sig, unsigned int sig_len,
                                unsigned char *r, unsigned int r_len,
                                unsigned char *s, unsigned int s_len)
{
  unsigned int offset;

  while ((*r == 0) && r_len) {
    r++;
    r_len--;
  }
  while ((*s == 0) && s_len) {
    s++;
    s_len--;
  }
  if (!r_len || !s_len) {
    return 0;
  }

  // check sig_len
  offset = 3 * 2 + r_len + s_len;
  if (*r & 0x80) {
    offset++;
  }
  if (*s & 0x80) {
    offset++;
  }
  if (sig_len < offset) {
    return 0;
  }

  // r
  offset = 2;
  if (*r & 0x80) {
    sig[offset + 2] = 0;
    memmove(sig + offset + 3, r, r_len);
    r_len++;
  } else {
    memmove(sig + offset + 2, r, r_len);
  }
  sig[offset] = 0x02;
  sig[offset + 1] = r_len;

  // s
  offset += 2 + r_len;
  if (*s & 0x80) {
    sig[offset + 2] = 0;
    memmove(sig + offset + 3, s, s_len);
    s_len++;
  } else {
    memmove(sig + offset + 2, s, s_len);
  }
  sig[offset] = 0x02;
  sig[offset + 1] = s_len;

  // head
  sig[0] = 0x30;
  sig[1] = 2 + r_len + 2 + s_len;

  return 2 + sig[1];
}
