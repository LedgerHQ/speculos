/* https://ed25519.cr.yp.to/python/ed25519.py */
#include <stdbool.h>

#include <openssl/bn.h>
#include <openssl/ec.h>

#include "cx_ed25519.h"

static const char *constant_q = "57896044618658097711785492504343953926634992332820282019728792003956564819949";
static bool initialized;
static BIGNUM *d1, *d2, *q, *two;
static const BIGNUM *one;
static BN_CTX *ctx;

typedef struct {
  BIGNUM *x;
  BIGNUM *y;
} POINT;

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

static int edwards(POINT *R, POINT *P, POINT *Q)
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

  if (edwards(Q, Q, Q) != 0) {
    return -1;
  }

  if (odd && edwards(Q, Q, P) != 0) {
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

  if (ctx == NULL || a == NULL || two == NULL || d1 == NULL || d2 == NULL || q == NULL) {
    BN_CTX_free(ctx);
    BN_free(a);
    BN_free(two);
    BN_free(d1);
    BN_free(d2);
    BN_free(q);
    return -1;
  }

  one = BN_value_one();
  BN_dec2bn(&two, "2");
  BN_dec2bn(&q, constant_q);

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
