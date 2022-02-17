/* https://ed25519.cr.yp.to/python/ed25519.py */
#include <err.h>
#include <stdbool.h>
#include <string.h>

#include <openssl/bn.h>
#include <openssl/ec.h>

#include "cx_ec.h"
#include "cx_ed25519.h"
#include "cx_utils.h"

static const char *constant_q = "5789604461865809771178549250434395392663499233"
                                "2820282019728792003956564819949";
static const char *constant_I = "1968116137670750595680707930498854201544606651"
                                "5923890162744021073123829784752";
static bool initialized;
static BIGNUM *d1, *I, *q, *two;
static const BIGNUM *one;
static BN_CTX *ctx;

static int initialize(void)
{
  BIGNUM *a;

  ctx = BN_CTX_new();
  a = BN_new();
  two = BN_new();
  d1 = BN_new();
  q = BN_new();
  I = BN_new();

  if (ctx == NULL || a == NULL || two == NULL || d1 == NULL || q == NULL ||
      I == NULL) {
    BN_CTX_free(ctx);
    BN_free(a);
    BN_free(two);
    BN_free(d1);
    BN_free(q);
    BN_free(I);
    return -1;
  }

  one = BN_value_one();
  BN_dec2bn(&two, "2");
  BN_dec2bn(&q, constant_q);
  BN_dec2bn(&I, constant_I);

  BN_dec2bn(&a, "121666");
  BN_mod_inverse(a, a, q, ctx);
  BN_dec2bn(&d1, "-121665");
  BN_mul(d1, d1, a, ctx);

  BN_free(a);

  initialized = true;

  return 0;
}

int edwards_add(POINT *R, POINT *P, POINT *Q)
{
  BIGNUM *x1, *y1, *x2, *y2, *a, *b, *rx, *dx1x2y1y2;
  int ret;

  if (!initialized && initialize() != 0) {
    return -1;
  }
  ret = 0;

  a = BN_new();
  b = BN_new();
  rx = BN_new();
  dx1x2y1y2 = BN_new();
  if (rx == NULL || dx1x2y1y2 == NULL || a == NULL || b == NULL) {
    ret = -1;
    goto free_bn;
  }

  x1 = P->x;
  y1 = P->y;
  x2 = Q->x;
  y2 = Q->y;

  BN_mod_mul(a, x1, y2, q, ctx);
  BN_mod_mul(b, x2, y1, q, ctx);
  BN_mod_mul(dx1x2y1y2, a, b, q, ctx);
  BN_mod_mul(dx1x2y1y2, d1, dx1x2y1y2, q, ctx);

  BN_mod_add(a, a, b, q, ctx);
  BN_add(b, one, dx1x2y1y2);
  BN_mod_inverse(b, b, q, ctx);
  BN_mod_mul(rx, a, b, q, ctx);

  BN_mod_mul(a, y1, y2, q, ctx);
  BN_mod_mul(b, x1, x2, q, ctx);
  BN_mod_add(a, a, b, q, ctx);

  BN_sub(b, one, dx1x2y1y2);
  BN_mod_inverse(b, b, q, ctx);

  BN_mod_mul(R->y, a, b, q, ctx);
  BN_copy(R->x, rx);

free_bn:
  BN_free(b);
  BN_free(a);
  BN_free(dx1x2y1y2);
  BN_free(rx);

  return ret;
}

int scalarmult_ed25519(BIGNUM *Qx, BIGNUM *Qy, BIGNUM *Px, BIGNUM *Py,
                       BIGNUM *e)
{
  POINT P, Q;
  int bit;

  if (!initialized && initialize() != 0) {
    return -1;
  }

  BN_zero(Qx);
  BN_one(Qy);
  P.x = Px;
  P.y = Py;
  Q.x = Qx;
  Q.y = Qy;

  for (bit = BN_num_bits(e) - 1; bit >= 0; bit--) {
    if (edwards_add(&Q, &Q, &Q) != 0) {
      return -1;
    }
    if (BN_is_bit_set(e, bit)) {
      if (edwards_add(&Q, &Q, &P) != 0) {
        return -1;
      }
    }
  }
  return 0;
}

static void cx_compress(uint8_t *x, uint8_t *y, size_t size)
{
  if (x[size - 1] & 1) {
    y[0] |= 0x80;
  }

  be2le(y, size);
}

int sys_cx_edward_compress_point(cx_curve_t curve, uint8_t *P, size_t P_len)
{
  const cx_curve_twisted_edward_t *domain;
  uint8_t *x, *y;
  size_t size;

  domain = (const cx_curve_twisted_edward_t *)cx_ecfp_get_domain(curve);
  size = domain->length;
  if (curve != CX_CURVE_Ed25519 || P_len < (1 + 2 * size)) {
    errx(1, "cx_edward_compress_point: invalid parameters");
    return -1;
  }

  x = P + 1;
  y = P + 1 + size;

  cx_compress(x, y, size);
  memcpy(x, y, size);

  P[0] = 0x02;

  return 0;
}

static int xrecover(BIGNUM *x, BIGNUM *y)
{
  BIGNUM *a, *xx;
  int ret;

  if (!initialized && initialize() != 0) {
    return -1;
  }

  ret = 0;

  a = BN_new();
  xx = BN_new();
  if (a == NULL || xx == NULL) {
    ret = -1;
    goto free_bn;
  }

  BN_mul(xx, y, y, ctx);
  BN_mul(a, d1, xx, ctx);
  BN_add(a, a, one);
  BN_mod_inverse(a, a, q, ctx);
  BN_sub(xx, xx, one);
  BN_mul(xx, xx, a, ctx);

  BN_add(a, q, two);
  BN_add(a, a, one);
  BN_rshift(a, a, 3);
  BN_mod_exp(x, xx, a, q, ctx);

  BN_mul(a, x, x, ctx);
  BN_mod_sub(a, a, xx, q, ctx);
  if (!BN_is_zero(a)) {
    BN_mod_mul(x, x, I, q, ctx);
  }

  if (BN_is_odd(x)) {
    BN_sub(x, q, x);
  }

free_bn:
  BN_free(a);
  BN_free(xx);
  return ret;
}

static int cx_decompress(uint8_t *x, uint8_t *y, size_t size)
{
  BIGNUM *xx, *yy;
  bool negative;
  int ret;

  ret = 0;

  xx = BN_new();
  yy = BN_new();
  if (xx == NULL || yy == NULL) {
    ret = -1;
    goto free_bn;
  }

  le2be(y, size);
  negative = (y[0] & 0x80) != 0;
  y[0] &= 0x7f;

  BN_bin2bn(y, size, yy);

  ret = xrecover(xx, yy);
  if (ret != 0) {
    goto free_bn;
  }

  if (negative != BN_is_negative(xx)) {
    BN_mod_sub(xx, q, xx, q, ctx);
  }

  BN_bn2binpad(xx, x, size);

free_bn:
  BN_free(yy);
  BN_free(xx);
  return ret;
}

int sys_cx_edward_decompress_point(cx_curve_t curve, uint8_t *P, size_t P_len)
{
  const cx_curve_twisted_edward_t *domain;
  unsigned int size;
  uint8_t *x, *y;

  domain = (const cx_curve_twisted_edward_t *)cx_ecfp_get_domain(curve);
  size = domain->length;
  if (curve != CX_CURVE_Ed25519 || P_len < (1 + 2 * size)) {
    errx(1, "cx_edward_compress_point: invalid parameters");
    return -1;
  }

  x = P + 1;
  y = P + 1 + size;

  memcpy(y, x, size);
  if (cx_decompress(x, y, size) != 0) {
    errx(1, "cx_decompress failed");
    return -1;
  }
  P[0] = 0x04;

  return 0;
}
