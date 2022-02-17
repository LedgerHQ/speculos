#define _SDK_2_0_
#include "bolos/cxlib.h"

//-----------------------------------------------------------------------------

cx_err_t cx_weierstrass_recover_y(cx_mpi_ecpoint_t *P, uint32_t sign)
{
  cx_err_t error;
  cx_mpi_t *p, *sqy, *t1, *t2;
  cx_bn_t bn_p, bn_sqy, bn_t1, bn_t2;
  uint32_t sz;
  const cx_curve_weierstrass_t *domain;

  domain = (const cx_curve_weierstrass_t *)cx_ecdomain(P->curve);
  if (domain == NULL) {
    return CX_INVALID_PARAMETER;
  }

  sz = domain->length;
  p = NULL;
  sqy = NULL;
  t1 = NULL;
  t2 = NULL;
  bn_p = bn_sqy = bn_t1 = bn_t2 = -1;
  if (((p = cx_mpi_alloc_init(&bn_p, sz, domain->p, sz)) == NULL) ||
      ((sqy = cx_mpi_alloc(&bn_sqy, sz)) == NULL) ||
      ((t1 = cx_mpi_alloc(&bn_t1, sz)) == NULL) ||
      ((t2 = cx_mpi_alloc(&bn_t2, sz)) == NULL)) {

    error = CX_MEMORY_FULL;
    goto end;
  }
  // y²=x³+a*x+b
  cx_mpi_init(sqy, domain->b, sz); // sqy = b

  cx_mpi_init(t1, domain->a, sz);
  cx_mpi_mod_mul(t2, t1, P->x, p); // ax
  cx_mpi_mod_add(sqy, sqy, t2, p); // sqy =  ax + b

  cx_mpi_mod_mul(t1, P->x, P->x, p); // x²
  cx_mpi_mod_mul(t2, t1, P->x, p);   // x³
  cx_mpi_mod_add(sqy, sqy, t2, p);   // sqy = x³ + ax + b

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
  cx_mpi_destroy(&bn_t2);
  cx_mpi_destroy(&bn_t1);
  cx_mpi_destroy(&bn_sqy);
  cx_mpi_destroy(&bn_p);

  return error;
}
