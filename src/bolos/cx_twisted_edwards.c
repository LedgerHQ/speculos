#define _SDK_2_0_
#include "bolos/cxlib.h"

//-----------------------------------------------------------------------------

enum {
  N = 0, // modulus
  A,     // a coefficient
  B,     // b coefficient
  TA,
  TB,
  TC,
  TD,
  TE,
  TF,
  TG,
  TH,
  X1,
  X2,
  X3,
  Y1,
  Y2,
  Y3,
  Z1,
  Z2,
  Z3,
  T1,
  T2,
  MAX_ID
};

static void cx_twisted_edwards_dbl(cx_mpi_ecpoint_t *p, cx_mpi_t *p_t,
                                   cx_mpi_t *mpi[])
{
  mpi[X1] = p->x;
  mpi[Y1] = p->y;
  if (p_t != NULL) {
    mpi[T1] = p_t;
  }
  mpi[Z1] = p->z;
  // A = (X1*X1)
  cx_mpi_mod_mul(mpi[TA], mpi[X1], mpi[X1], mpi[N]);
  // B = (Y1*Y1)
  cx_mpi_mod_mul(mpi[TB], mpi[Y1], mpi[Y1], mpi[N]);
  // D = (Z1*Z1)
  cx_mpi_mod_mul(mpi[TD], mpi[Z1], mpi[Z1], mpi[N]);
  // C = (D+D)
  cx_mpi_mod_add(mpi[TC], mpi[TD], mpi[TD], mpi[N]);
  // D = (X1+Y1)
  cx_mpi_mod_add(mpi[TD], mpi[X1], mpi[Y1], mpi[N]);
  // F = (D*D)
  cx_mpi_mod_mul(mpi[TF], mpi[TD], mpi[TD], mpi[N]);
  // E = (F-A)
  cx_mpi_mod_sub(mpi[TE], mpi[TF], mpi[TA], mpi[N]);
  // E = (E-B)
  cx_mpi_mod_sub(mpi[TE], mpi[TE], mpi[TB], mpi[N]);
  // D = (a*A)
  cx_mpi_mod_mul(mpi[TD], mpi[A], mpi[TA], mpi[N]);
  // G = (D+B)
  cx_mpi_mod_add(mpi[TG], mpi[TD], mpi[TB], mpi[N]);
  // F = (G-C)
  cx_mpi_mod_sub(mpi[TF], mpi[TG], mpi[TC], mpi[N]);
  // H = (D-B)
  cx_mpi_mod_sub(mpi[TH], mpi[TD], mpi[TB], mpi[N]);
  // X1 = (E*F)
  cx_mpi_mod_mul(mpi[X1], mpi[TE], mpi[TF], mpi[N]);
  // Y1 = (G*H)
  cx_mpi_mod_mul(mpi[Y1], mpi[TG], mpi[TH], mpi[N]);
  // T1 = (E*H)
  cx_mpi_mod_mul(mpi[T1], mpi[TE], mpi[TH], mpi[N]);
  // Z1 = (F*G)
  cx_mpi_mod_mul(mpi[Z1], mpi[TF], mpi[TG], mpi[N]);
}

static void cx_twisted_edwards_add_simple(cx_mpi_ecpoint_t *p,
                                          cx_mpi_ecpoint_t *q, cx_mpi_t *mpi[])
{
  mpi[X1] = p->x;
  mpi[Y1] = p->y;
  mpi[Z1] = p->z;
  mpi[X2] = q->x;
  mpi[Y2] = q->y;
  mpi[Z2] = q->z;

  // A = z1 * z2
  cx_mpi_mod_mul(mpi[TA], mpi[Z1], mpi[Z2], mpi[N]);
  // B = A * A
  cx_mpi_mod_mul(mpi[TB], mpi[TA], mpi[TA], mpi[N]);
  // C = x1 * x2
  cx_mpi_mod_mul(mpi[TC], mpi[X1], mpi[X2], mpi[N]);
  // D = y1 * y2
  cx_mpi_mod_mul(mpi[TD], mpi[Y1], mpi[Y2], mpi[N]);
  // F = d * C * D
  cx_mpi_mod_mul(mpi[TE], mpi[B], mpi[TC], mpi[N]);
  cx_mpi_mod_mul(mpi[TF], mpi[TE], mpi[TD], mpi[N]);
  // E = B - F
  cx_mpi_mod_sub(mpi[TE], mpi[TB], mpi[TF], mpi[N]);
  // G = B + F
  cx_mpi_mod_add(mpi[TG], mpi[TB], mpi[TF], mpi[N]);
  // X2 = A * E * ((x1 + y1)(x2 + y2) - C - D)
  cx_mpi_mod_add(mpi[T1], mpi[X1], mpi[Y1], mpi[N]);
  cx_mpi_mod_add(mpi[T2], mpi[X2], mpi[Y2], mpi[N]);
  cx_mpi_mod_mul(mpi[TF], mpi[T1], mpi[T2], mpi[N]);
  cx_mpi_mod_sub(mpi[TF], mpi[TF], mpi[TC], mpi[N]);
  cx_mpi_mod_sub(mpi[TF], mpi[TF], mpi[TD], mpi[N]);
  cx_mpi_mod_mul(mpi[TB], mpi[TF], mpi[TE], mpi[N]);
  cx_mpi_mod_mul(mpi[X2], mpi[TA], mpi[TB], mpi[N]);
  // Y2 = A * G * (D - a*C)
  cx_mpi_mod_mul(mpi[TF], mpi[A], mpi[TC], mpi[N]);
  cx_mpi_mod_sub(mpi[TF], mpi[TD], mpi[TF], mpi[N]);
  cx_mpi_mod_mul(mpi[TD], mpi[TG], mpi[TF], mpi[N]);
  cx_mpi_mod_mul(mpi[Y2], mpi[TD], mpi[TA], mpi[N]);
  // Z2 = E *G
  cx_mpi_mod_mul(mpi[Z2], mpi[TE], mpi[TG], mpi[N]);
}

static void cx_twisted_edwards_add(cx_mpi_ecpoint_t *p, cx_mpi_t *p_t,
                                   cx_mpi_ecpoint_t *q, cx_mpi_t *q_t,
                                   cx_mpi_t *mpi[])
{
  mpi[X1] = p->x;
  mpi[Y1] = p->y;
  mpi[T1] = p_t;
  mpi[Z1] = p->z;
  mpi[X2] = q->x;
  mpi[Y2] = q->y;
  mpi[T2] = q_t;
  mpi[Z2] = q->z;

  cx_mpi_mod_mul(mpi[TA], mpi[X1], mpi[X2], mpi[N]);
  cx_mpi_mod_mul(mpi[TB], mpi[Y1], mpi[Y2], mpi[N]);
  cx_mpi_mod_mul(mpi[TC], mpi[Z1], mpi[T2], mpi[N]);
  cx_mpi_mod_mul(mpi[TD], mpi[T1], mpi[Z2], mpi[N]);
  cx_mpi_mod_sub(mpi[TE], mpi[X1], mpi[Y1], mpi[N]);
  cx_mpi_mod_add(mpi[TF], mpi[X2], mpi[Y2], mpi[N]);
  cx_mpi_mod_mul(mpi[TG], mpi[TE], mpi[TF], mpi[N]);
  cx_mpi_mod_sub(mpi[TF], mpi[TB], mpi[TA], mpi[N]);
  cx_mpi_mod_add(mpi[TF], mpi[TF], mpi[TG], mpi[N]);
  cx_mpi_mod_mul(mpi[TE], mpi[A], mpi[TA], mpi[N]);
  cx_mpi_mod_add(mpi[TG], mpi[TB], mpi[TE], mpi[N]);
  cx_mpi_mod_add(mpi[TE], mpi[TD], mpi[TC], mpi[N]);
  cx_mpi_mod_sub(mpi[TH], mpi[TD], mpi[TC], mpi[N]);
  cx_mpi_mod_mul(mpi[X2], mpi[TE], mpi[TF], mpi[N]);
  cx_mpi_mod_mul(mpi[Y2], mpi[TG], mpi[TH], mpi[N]);
  cx_mpi_mod_mul(mpi[T2], mpi[TE], mpi[TH], mpi[N]);
  cx_mpi_mod_mul(mpi[Z2], mpi[TF], mpi[TG], mpi[N]);
}

static uint32_t cx_internal_get_bit(const uint8_t *k, uint32_t k_len,
                                    uint32_t pos)
{
  uint8_t c = k[k_len - 1 - pos / 8];
  return (uint8_t)(c >> (pos % 8u)) & 1u;
}

static bool cx_internal_is_buffer_zero(const uint8_t *k, uint32_t k_len)
{
  bool i = 0;
  while (k_len--) {
    i |= k[k_len];
  }
  return i == 0;
}

cx_err_t cx_twisted_edwards_recover_x(cx_mpi_ecpoint_t *P, uint32_t sign)
{
  cx_err_t error;
  cx_mpi_t *p, *t1, *t2, *t3;
  cx_bn_t bn_p, bn_t1, bn_t2, bn_t3;
  uint32_t sz;
  const cx_curve_twisted_edwards_t *domain;

  domain = (const cx_curve_twisted_edwards_t *)cx_ecdomain(P->curve);
  if (domain == NULL) {
    return CX_INVALID_PARAMETER;
  }

  sz = domain->length;
  bn_p = bn_t1 = bn_t2 = bn_t3 = -1;
  if (((p = cx_mpi_alloc_init(&bn_p, sz, domain->p, sz)) == NULL) ||
      ((t1 = cx_mpi_alloc(&bn_t1, sz)) == NULL) ||
      ((t2 = cx_mpi_alloc(&bn_t2, sz)) == NULL) ||
      ((t3 = cx_mpi_alloc(&bn_t3, sz)) == NULL)) {
    error = CX_MEMORY_FULL;
    goto end;
  }
  // x² = (y²-1) * (d*y²-a)^-1
  // y²
  cx_mpi_mod_mul(t3, P->y, P->y, p);
  // y²*d
  cx_mpi_init(t1, domain->b, sz);
  cx_mpi_mod_mul(t2, t3, t1, p);
  // d*y²-a
  cx_mpi_init(t1, domain->a, sz);
  cx_mpi_mod_sub(t2, t2, t1, p);
  //(d*y²-a)^-1
  cx_mpi_mod_invert_nprime(t1, t2, p);

  //(y^2-1)
  cx_mpi_mod_mul(t3, P->y, P->y, p);
  cx_mpi_set_u32(t2, 1);
  cx_mpi_mod_sub(t2, t3, t2, p);

  // x²
  cx_mpi_mod_mul(t3, t2, t1, p);

  // x²==0 case
  if (cx_mpi_cmp_u32(t3, 0) == 0) {
    cx_mpi_set_u32(P->x, 0);

    if (sign) {
      error = CX_INVALID_PARAMETER;
      goto end;
    }
  }
  // root
  error = cx_mpi_mod_sqrt(P->x, t3, p, sign);
end:
  cx_mpi_destroy(&bn_t3);
  cx_mpi_destroy(&bn_t2);
  cx_mpi_destroy(&bn_t1);
  cx_mpi_destroy(&bn_p);

  return error;
}

cx_err_t cx_twisted_edwards_add_point(cx_mpi_ecpoint_t *R,
                                      const cx_mpi_ecpoint_t *P,
                                      const cx_mpi_ecpoint_t *Q)
{
  cx_err_t error;
  cx_ecpoint_t ec_q2;
  cx_mpi_ecpoint_t Q2;
  cx_mpi_t *mpi[MAX_ID];
  cx_bn_t bn[MAX_ID];

  uint32_t sz;
  const cx_curve_twisted_edwards_t *domain;

  domain = (const cx_curve_twisted_edwards_t *)cx_ecdomain(P->curve);
  if (domain == NULL) {
    return CX_INVALID_PARAMETER;
  }
  sz = domain->length;

  for (int i = 0; i < MAX_ID; i++) {
    if ((mpi[i] = cx_mpi_alloc(&bn[i], sz)) == NULL) {
      error = CX_MEMORY_FULL;
      goto end;
    }
  }

  CX_CHECK(cx_mpi_init(mpi[N], domain->p, sz));
  CX_CHECK(cx_mpi_init(mpi[A], domain->a, sz));
  CX_CHECK(cx_mpi_init(mpi[B], domain->b, sz));
  CX_CHECK(sys_cx_ecpoint_alloc(&ec_q2, domain->curve));
  CX_CHECK(cx_mpi_ecpoint_from_ecpoint(&Q2, &ec_q2));

  // R = P
  cx_mpi_ecpoint_copy(R, P);
  // Q2 = Q
  cx_mpi_ecpoint_copy(&Q2, Q);

  if (cx_mpi_cmp(P->y, Q->y) == 0) {
    if (cx_mpi_cmp(P->x, Q->x) != 0) {
      cx_mpi_set_u32(R->x, 0);
      cx_mpi_set_u32(R->y, 1);
      cx_mpi_set_u32(R->z, 0);
      error = CX_EC_INFINITE_POINT;
      goto end;
    } else {
      cx_twisted_edwards_dbl(R, NULL, mpi);
    }
  } else {
    cx_twisted_edwards_add_simple(&Q2, R, mpi);
  }
  error = CX_OK;

end:
  for (int i = 0; i < MAX_ID; i++) {
    cx_mpi_destroy(&bn[i]);
  }
  error = sys_cx_ecpoint_destroy(&ec_q2);

  if (error == CX_OK) {
    error = cx_mpi_ecpoint_normalize(R);
  }
  return error;
}

cx_err_t cx_twisted_edwards_mul_point(cx_mpi_ecpoint_t *P, const uint8_t *k,
                                      uint32_t k_len)
{
  cx_err_t error;
  cx_mpi_t *mpi[MAX_ID];
  cx_bn_t bn[MAX_ID];
  cx_mpi_ecpoint_t R1;
  cx_mpi_ecpoint_t *RR[2];
  cx_ecpoint_t ec_R1;
  cx_mpi_t *XY[2]; // T coordinate
  cx_bn_t bn_xy[2];
  uint32_t sz;
  const cx_curve_twisted_edwards_t *domain;

  if (cx_internal_is_buffer_zero(k, k_len)) {
    cx_mpi_set_u32(P->x, 0);
    cx_mpi_set_u32(P->y, 1);
    cx_mpi_set_u32(P->z, 0);
    return CX_EC_INFINITE_POINT;
  }

  domain = (const cx_curve_twisted_edwards_t *)cx_ecdomain(P->curve);
  if (domain == NULL) {
    return CX_INVALID_PARAMETER;
  }
  sz = domain->length;

  for (int i = 0; i < MAX_ID; i++) {
    if ((mpi[i] = cx_mpi_alloc(&bn[i], sz)) == NULL) {
      error = CX_MEMORY_FULL;
      goto end;
    }
  }

  for (int i = 0; i < 2; i++) {
    if ((XY[i] = cx_mpi_alloc(&bn_xy[i], sz)) == NULL) {
      error = CX_MEMORY_FULL;
      goto end;
    }
  }

  CX_CHECK(sys_cx_ecpoint_alloc(&ec_R1, domain->curve));
  CX_CHECK(cx_mpi_ecpoint_from_ecpoint(&R1, &ec_R1));

  CX_CHECK(cx_mpi_init(mpi[A], domain->a, sz));
  CX_CHECK(cx_mpi_init(mpi[N], domain->p, sz));

  CX_CHECK(cx_mpi_mod_mul(XY[0], P->x, P->y, mpi[N]));

  // Initialize ladder, assume k != 0
  // Set R0 = P and R1 = 2P
  cx_mpi_ecpoint_copy(&R1, P);
  CX_CHECK(cx_mpi_copy(XY[1], XY[0]));

  RR[0] = P;
  RR[1] = &R1;

  cx_twisted_edwards_dbl(RR[1], XY[1], mpi);

  // Find first bit at 1 and discard it due to the initial doubling
  size_t bit_pos = k_len * 8 - 1;
  while (cx_internal_get_bit(k, k_len, bit_pos) == 0) {
    bit_pos -= 1;
  }

  while (bit_pos-- > 0) {
    int bit = cx_internal_get_bit(k, k_len, bit_pos);
    cx_twisted_edwards_add(RR[bit], XY[bit], RR[1 - bit], XY[1 - bit], mpi);
    cx_twisted_edwards_dbl(RR[bit], XY[bit], mpi);
  }

  error = CX_OK;

end:
  for (int i = 0; i < MAX_ID; i++) {
    cx_mpi_destroy(&bn[i]);
  }
  for (int i = 0; i < 2; i++) {
    cx_mpi_destroy(&bn_xy[i]);
  }

  error = sys_cx_ecpoint_destroy(&ec_R1);

  if (error == CX_OK) {
    error = cx_mpi_ecpoint_normalize(P);
  }
  return error;
}