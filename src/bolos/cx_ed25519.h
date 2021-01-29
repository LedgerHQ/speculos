#pragma once

typedef struct {
  BIGNUM *x;
  BIGNUM *y;
} POINT;

int scalarmult_ed25519(BIGNUM *Qx, BIGNUM *Qy, BIGNUM *Px, BIGNUM *Py,
                       BIGNUM *e);
int edwards_add(POINT *R, POINT *P, POINT *Q);
