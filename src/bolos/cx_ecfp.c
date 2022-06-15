
#define _SDK_2_0_
#include <err.h>
#include <errno.h>
#include <openssl/bn.h>
#include <openssl/objects.h>
#include <openssl/rand.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "bolos/cxlib.h"
#include "cx_ecfp.h"

static void const *sys_cx_ecdomain_param_ptr(const cx_curve_domain_t *domain,
                                             cx_curve_dom_param_t id)
{
  switch (id) {
  case CX_CURVE_PARAM_A:
    return domain->a;
  case CX_CURVE_PARAM_B:
    return domain->b;
  case CX_CURVE_PARAM_Field:
    return domain->p;
  case CX_CURVE_PARAM_Gx:
    return domain->Gx;
  case CX_CURVE_PARAM_Gy:
    return domain->Gy;
  case CX_CURVE_PARAM_Order:
    return domain->n;
  case CX_CURVE_PARAM_Cofactor:
    return domain->h;
  default:
    break;
  }
  return NULL;
}

cx_err_t sys_cx_ecdomain_parameter(cx_curve_t curve, cx_curve_dom_param_t id,
                                   uint8_t *p, uint32_t p_len)
{
  void const *ptr;
  const cx_curve_domain_t *domain;

  domain = cx_ecdomain(curve);
  if (domain == NULL) {
    return CX_EC_INVALID_CURVE;
  }

  ptr = sys_cx_ecdomain_param_ptr(domain, id);
  if ((ptr == NULL) || (domain->length > p_len)) {
    return CX_INVALID_PARAMETER;
  }

  if (id == CX_CURVE_PARAM_Cofactor) {
    memmove(p, ptr, 4);
  } else {
    memmove(p, ptr, domain->length);
  }
  return CX_OK;
}

cx_err_t sys_cx_ecdomain_parameter_bn(cx_curve_t curve, cx_curve_dom_param_t id,
                                      cx_bn_t bn_p)
{
  cx_err_t error;
  cx_mpi_t *p;
  void const *ptr;
  const cx_curve_domain_t *domain;

  domain = cx_ecdomain(curve);
  if (domain == NULL) {
    return CX_EC_INVALID_CURVE;
  }
  CX_CHECK(cx_bn_to_mpi(bn_p, &p));

  ptr = sys_cx_ecdomain_param_ptr(domain, id);
  if (ptr == NULL) {
    return CX_INVALID_PARAMETER;
  }
  // Copy the content of ptr into BN:
  if (id == CX_CURVE_PARAM_Cofactor) {
    BN_bin2bn(ptr, 4, p);
  } else {
    BN_bin2bn(ptr, domain->length, p);
  }
end:
  return error;
}

cx_err_t sys_cx_ecdomain_generator(cx_curve_t curve, uint8_t *Gx, uint8_t *Gy,
                                   size_t len)
{
  const cx_curve_domain_t *domain;

  domain = cx_ecdomain(curve);
  if (domain == NULL) {
    return CX_EC_INVALID_CURVE;
  }
  if (domain->length > len) {
    return CX_INVALID_PARAMETER;
  }
  memmove(Gx, domain->Gx, domain->length);
  memmove(Gy, domain->Gy, domain->length);

  return CX_OK;
}

cx_err_t sys_cx_ecdomain_generator_bn(cx_curve_t curve, cx_ecpoint_t *ec_P)
{
  cx_err_t error;
  cx_mpi_ecpoint_t P;
  const cx_curve_domain_t *domain;

  CX_CHECK(cx_bn_locked());

  domain = cx_ecdomain(curve);
  if (domain == NULL) {
    return CX_EC_INVALID_CURVE;
  }
  if ((curve != ec_P->curve) ||
      (cx_mpi_ecpoint_from_ecpoint(&P, ec_P) != CX_OK)) {
    return CX_EC_INVALID_POINT;
  }
  BN_bin2bn(domain->Gx, domain->length, P.x);
  BN_bin2bn(domain->Gy, domain->length, P.y);
  cx_mpi_set_u32(P.z, 1);
end:
  return error;
}

cx_err_t sys_cx_ecdomain_parameters_length(cx_curve_t curve, size_t *length)
{
  const cx_curve_domain_t *domain;

  domain = cx_ecdomain(curve);
  if (domain == NULL) {
    return CX_EC_INVALID_CURVE;
  }

  *length = domain->length;
  return CX_OK;
}

cx_err_t sys_cx_ecdomain_size(cx_curve_t curve, size_t *length)
{
  const cx_curve_domain_t *domain;

  domain = cx_ecdomain(curve);
  if (domain == NULL) {
    return CX_EC_INVALID_CURVE;
  }

  *length = domain->bit_size;
  return CX_OK;
}

#ifdef no_thorw
cx_err_t cx_ecfp_init_private_key_no_throw(cx_curve_t curve,
                                           const uint8_t *rawkey,
                                           size_t key_len,
                                           cx_ecfp_private_key_t *pvkey)
{
}
#endif
