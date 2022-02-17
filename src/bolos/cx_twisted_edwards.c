#define _SDK_2_0_
#include "bolos/cxlib.h"

//-----------------------------------------------------------------------------

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
