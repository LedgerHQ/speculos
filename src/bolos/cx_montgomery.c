#define _SDK_2_0_
#include "bolos/cxlib.h"

//-----------------------------------------------------------------------------

cx_err_t cx_montgomery_recover_y(cx_mpi_ecpoint_t *P, uint32_t sign)
{
  cx_err_t error;
  cx_mpi_t *p, *sqy, *t1, *t2, *t3;
  cx_bn_t bn_p, bn_sqy, bn_t1, bn_t2, bn_t3;
  uint32_t sz;
  const cx_curve_montgomery_t *domain;

  domain = (const cx_curve_montgomery_t *)cx_ecdomain(P->curve);
  if (domain == NULL) {
    return CX_INVALID_PARAMETER;
  }

  sz = domain->length;
  bn_p = bn_sqy = bn_t1 = bn_t2 = bn_t3 = -1;
  if (((p = cx_mpi_alloc_init(&bn_p, sz, domain->p, sz)) == NULL) ||
      ((sqy = cx_mpi_alloc(&bn_sqy, sz)) == NULL) ||
      ((t1 = cx_mpi_alloc(&bn_t1, sz)) == NULL) ||
      ((t2 = cx_mpi_alloc(&bn_t2, sz)) == NULL) ||
      ((t3 = cx_mpi_alloc(&bn_t3, sz)) == NULL)) {
    error = CX_MEMORY_FULL;
    goto end;
  }
  // y²=(x³+a*x²+x)*b^-1
  cx_mpi_init(sqy, domain->b, sz);
  cx_mpi_mod_invert_nprime(t3, sqy, p);

  cx_mpi_mod_mul(t2, P->x, P->x, p); // x²
  cx_mpi_init(t1, domain->a, sz);
  cx_mpi_mod_mul(sqy, t1, t2, p);    // ax²
  cx_mpi_mod_add(sqy, sqy, P->x, p); // ax²+ x
  cx_mpi_mod_mul(t1, t2, P->x, p);   // x³
  cx_mpi_mod_add(t2, sqy, t1, p);    // x³ + ax²+ x

  cx_mpi_mod_mul(sqy, t2, t3, p); // y²

  // y²==0 case
  if (cx_mpi_cmp_u32(sqy, 0) == 0) {
    cx_mpi_set_u32(P->y, 0);

    if (sign) {
      error = CX_INVALID_PARAMETER;
      goto end;
    }
  }
  // root
  error = cx_mpi_mod_sqrt(P->y, sqy, p, sign);
end:
  cx_mpi_destroy(&bn_t3);
  cx_mpi_destroy(&bn_t2);
  cx_mpi_destroy(&bn_t1);
  cx_mpi_destroy(&bn_sqy);
  cx_mpi_destroy(&bn_p);

  return error;
}
