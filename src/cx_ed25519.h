#ifndef _CX_25519_H
#define _CX_25519_H

typedef struct {
  BIGNUM *x;
  BIGNUM *y;
} POINT;

int scalarmult_ed25519(BIGNUM *Qx, BIGNUM *Qy, BIGNUM *Px, BIGNUM *Py, BIGNUM *e);
int edwards_add(POINT *R, POINT *P, POINT *Q);

#endif
