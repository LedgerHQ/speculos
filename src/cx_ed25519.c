/* https://ed25519.cr.yp.to/python/ed25519.py */
#include <err.h>
#include <string.h>
#include <stdbool.h>

#include <openssl/bn.h>
#include <openssl/ec.h>

#include "cx_ec.h"
#include "cx_utils.h"
#include "cx_ed25519.h"

static const char *constant_q = "57896044618658097711785492504343953926634992332820282019728792003956564819949";
static const char *constant_I = "19681161376707505956807079304988542015446066515923890162744021073123829784752";
static bool initialized;
static BIGNUM *d1, *d2, *I, *q, *two;
static const BIGNUM *one;
static BN_CTX *ctx;

static int edwards_helper(BIGNUM *r, BIGNUM *x1, BIGNUM *x2, BIGNUM *y1, BIGNUM *y2, BIGNUM *d)
{
  BIGNUM *a, *b;
  int ret;

  ret = 0;

  a = BN_new();
  b = BN_new();
  if (a == NULL || b == NULL) {
    ret = -1;
    goto free_bn;
  }

  BN_mul(a, x1, y2, ctx);
  BN_mul(b, x2, y1, ctx);
  BN_add(a, a, b);

  BN_mul(b, d, x1, ctx);
  BN_mul(b, b, x2, ctx);
  BN_mul(b, b, y1, ctx);
  BN_mul(b, b, y2, ctx);
  BN_add(b, one, b);

  BN_mod_inverse(b, b, q, ctx);

  BN_mul(r, a, b, ctx);

 free_bn:
  BN_free(b);
  BN_free(a);

  return ret;
}

int edwards_add(POINT *R, POINT *P, POINT *Q)
{
  BIGNUM *x1, *y1, *x2, *y2, *x3, *y3;
  int ret;

  ret = 0;

  x3 = BN_new();
  y3 = BN_new();
  if (x3 == NULL || y3 == NULL) {
    ret = -1;
    goto free_bn;
  }

  x1 = P->x;
  y1 = P->y;
  x2 = Q->x;
  y2 = Q->y;

  if (edwards_helper(x3, x1, x2, y1, y2, d1) != 0) {
    ret = -1;
    goto free_bn;
  }

  if (edwards_helper(y3, y1, x1, x2, y2, d2) != 0) {
    ret = -1;
    goto free_bn;
  }

  BN_mod(R->x, x3, q, ctx);
  BN_mod(R->y, y3, q, ctx);

 free_bn:
  BN_free(y3);
  BN_free(x3);

  return ret;
}

static int scalarmult_helper(POINT *Q, POINT *P, BIGNUM *e)
{
  int odd;

  if (BN_is_zero(e)) {
    BN_zero(Q->x);
    BN_copy(Q->y, one);
    return 0;
  }

  odd = BN_is_odd(e);

  BN_rshift1(e, e);

  if (scalarmult_helper(Q, P, e) != 0) {
    return -1;
  }

  if (edwards_add(Q, Q, Q) != 0) {
    return -1;
  }

  if (odd && edwards_add(Q, Q, P) != 0) {
    return -1;
  }

  return 0;
}

static int initialize(void)
{
  BIGNUM *a;

  ctx = BN_CTX_new();
  a = BN_new();
  two = BN_new();
  d1 = BN_new();
  d2 = BN_new();
  q = BN_new();
  I = BN_new();

  if (ctx == NULL || a == NULL || two == NULL || d1 == NULL || d2 == NULL || q == NULL || I == NULL) {
    BN_CTX_free(ctx);
    BN_free(a);
    BN_free(two);
    BN_free(d1);
    BN_free(d2);
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

  BN_dec2bn(&a, "-1");
  BN_mul(d2, d1, a, ctx);

  BN_free(a);

  initialized = true;

  return 0;
}

static int scalarmult(POINT *Q, POINT *P, BIGNUM *e)
{
  if (!initialized && initialize() != 0) {
    return -1;
  }

  return scalarmult_helper(Q, P, e);
}

int scalarmult_ed25519(BIGNUM *Qx, BIGNUM *Qy, BIGNUM *Px, BIGNUM *Py, BIGNUM *e)
{
  POINT P, Q;

  P.x = Px;
  P.y = Py;
  Q.x = Qx;
  Q.y = Qy;

  return scalarmult(&Q, &P, e);
}

static void cx_compress(uint8_t *x, uint8_t *y, size_t size)
{
  if (x[size - 1] & 1) {
    y[0] |= 0x80;
  }

  be2le(y, size);
}

int sys_cx_edward_compress_point(cx_curve_t curve, uint8_t *P, size_t P_len) {
  cx_curve_twisted_edward_t *domain;
  uint8_t *x, *y;
  size_t size;

  domain = (cx_curve_twisted_edward_t *)cx_ecfp_get_domain(curve);
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
  cx_curve_twisted_edward_t *domain;
  unsigned int size;
  uint8_t *x, *y;

  domain = (cx_curve_twisted_edward_t *)cx_ecfp_get_domain(curve);
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
