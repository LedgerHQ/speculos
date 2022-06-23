#define _SDK_2_0_
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <openssl/objects.h>
#include <openssl/rand.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "bolos/cx_ed25519.h"
#include "bolos/cxlib.h"
#include "emulate.h"

//-----------------------------------------------------------------------------
// Almost duplicated from cx_ec.c one because some curves id have changed!
static int cx_weierstrass_mult(cx_curve_t curve, cx_mpi_t *qx, cx_mpi_t *qy,
                               cx_mpi_t *px, cx_mpi_t *py, cx_mpi_t *k)
{
  EC_POINT *p, *q;
  EC_GROUP *group = NULL;
  BN_CTX *ctx;
  int nid;
  int ret = 0;

  if ((nid = cx_nid_from_curve(curve)) >= 0) {
    group = cx_group_from_nid_and_curve(nid, curve);
  }
  if (group != NULL) {
    p = EC_POINT_new(group);
    q = EC_POINT_new(group);

    if (p != NULL && q != NULL) {
      ctx = cx_get_bn_ctx();

      if (ctx != NULL) {
        if (EC_POINT_set_affine_coordinates(group, p, px, py, ctx) == 1) {
          if (EC_POINT_mul(group, q, NULL, p, k, ctx) == 1) {
            if (EC_POINT_get_affine_coordinates(group, q, qx, qy, ctx) == 1) {
              ret = 1;
            }
          }
        }
      }
    }
    EC_GROUP_free(group);
    EC_POINT_free(p); // No need to check for NULL, OpenSSL handle it.
    EC_POINT_free(q);
  }
  return ret;
}

//-----------------------------------------------------------------------------
static cx_err_t cx_mpi_ecpoint_normalize(cx_mpi_ecpoint_t *P)
{
  const cx_curve_domain_t *domain;

  domain = cx_ecdomain(P->curve);
  if (domain == NULL) {
    return CX_EC_INVALID_CURVE;
  }
  if (cx_mpi_cmp_u32(P->z, 0) == 0) {
    cx_mpi_set_u32(P->x, 0);
    cx_mpi_set_u32(P->y, 1);
    cx_mpi_set_u32(P->z, 0);
    return CX_EC_INFINITE_POINT;
  }
  if (cx_mpi_cmp_u32(P->z, 1) != 0) {
    errx(1, "cx_mpi_ecpoint_normalize: TODO: unsupported curve (P->z=%u)",
         cx_mpi_get_u32(P->z));
    return CX_INTERNAL_ERROR;
  }
  return CX_OK;
}

//-----------------------------------------------------------------------------
cx_err_t cx_mpi_ecpoint_from_ecpoint(cx_mpi_ecpoint_t *P, const cx_ecpoint_t *Q)
{
  cx_err_t error;
  const cx_curve_domain_t *domain;

  domain = cx_ecdomain(Q->curve);
  if (domain == NULL) {
    return CX_EC_INVALID_CURVE;
  }
  CX_CHECK(cx_bn_rab_to_mpi(Q->x, &P->x, Q->y, &P->y, Q->z, &P->z));
  P->curve = Q->curve;
end:
  return error;
}

static EC_POINT *EC_POINT_from_ecpoint(const EC_GROUP *group,
                                       const cx_ecpoint_t *ec_P, bool set_coord)
{
  EC_POINT *p;
  cx_mpi_ecpoint_t bP;
  BN_CTX *ctx;

  p = EC_POINT_new(group);
  if (p != NULL) {
    ctx = cx_get_bn_ctx();

    if (cx_mpi_ecpoint_from_ecpoint(&bP, ec_P) != CX_OK ||
        (set_coord &&
         EC_POINT_set_affine_coordinates(group, p, bP.x, bP.y, ctx) != 1)) {
      EC_POINT_clear_free(p);
      p = NULL;
    }
  }
  return p;
}

static cx_err_t cx_ecpoint_from_EC_POINT(const EC_GROUP *group,
                                         const cx_ecpoint_t *ec_P,
                                         const EC_POINT *p)
{
  cx_mpi_ecpoint_t bP;
  BN_CTX *ctx;
  cx_err_t error;

  ctx = cx_get_bn_ctx();

  CX_CHECK(cx_mpi_ecpoint_from_ecpoint(&bP, ec_P));
  if (EC_POINT_get_affine_coordinates(group, p, bP.x, bP.y, ctx) != 1) {
    error = CX_INTERNAL_ERROR;
  }
end:
  return error;
}

//-----------------------------------------------------------------------------

cx_err_t sys_cx_ecpoint_alloc(cx_ecpoint_t *P, cx_curve_t curve)
{
  cx_err_t error;
  uint32_t size;
  const cx_curve_domain_t *domain;

  P->x = P->y = P->z = -1;
  CX_CHECK(cx_bn_locked());

  domain = cx_ecdomain(curve);
  if (domain == NULL) {
    return CX_EC_INVALID_CURVE;
  }
  size = size_to_mpi_word_bytes(domain->length);

  CX_CHECK(sys_cx_bn_alloc(&P->x, size));
  CX_CHECK(sys_cx_bn_alloc(&P->y, size));
  CX_CHECK(sys_cx_bn_alloc(&P->z, size));
  P->curve = curve;
end:
  if (error != CX_OK) {
    sys_cx_bn_destroy(&P->z);
    sys_cx_bn_destroy(&P->y);
    sys_cx_bn_destroy(&P->x);
  }
  return error;
}

cx_err_t sys_cx_ecpoint_destroy(cx_ecpoint_t *P)
{
  cx_err_t error;

  if (P == NULL) {
    return CX_OK;
  }
  CX_CHECK(cx_bn_locked());
  CX_CHECK(sys_cx_bn_destroy(&P->x));
  CX_CHECK(sys_cx_bn_destroy(&P->y));
  CX_CHECK(sys_cx_bn_destroy(&P->z));
end:
  memset(P, 0, sizeof(cx_ecpoint_t));
  return error;
}

cx_err_t sys_cx_ecpoint_init(cx_ecpoint_t *ec_P, const uint8_t *x, size_t x_len,
                             const uint8_t *y, size_t y_len)
{
  cx_err_t error;
  cx_mpi_ecpoint_t P;

  CX_CHECK(cx_mpi_ecpoint_from_ecpoint(&P, ec_P));
  BN_bin2bn(x, x_len, P.x);
  BN_bin2bn(y, y_len, P.y);
  cx_mpi_set_u32(P.z, 1);
end:
  return error;
}

cx_err_t sys_cx_ecpoint_init_bn(cx_ecpoint_t *ec_P, const cx_bn_t bn_x,
                                const cx_bn_t bn_y)
{
  cx_err_t error;
  cx_mpi_t *xy;
  cx_mpi_ecpoint_t P;

  CX_CHECK(cx_mpi_ecpoint_from_ecpoint(&P, ec_P));

  CX_CHECK(cx_bn_to_mpi(bn_x, &xy));
  CX_CHECK(cx_mpi_copy(P.x, xy));

  CX_CHECK(cx_bn_to_mpi(bn_y, &xy));
  CX_CHECK(cx_mpi_copy(P.y, xy));

  cx_mpi_set_u32(P.z, 1);
end:
  return error;
}

static cx_err_t cx_ecpoint_normalize(cx_mpi_ecpoint_t *P,
                                     const cx_ecpoint_t *ec_P)
{
  cx_err_t error;

  CX_CHECK(cx_mpi_ecpoint_from_ecpoint(P, ec_P));
  CX_CHECK(cx_mpi_ecpoint_normalize(P));
end:
  return error;
}

cx_err_t sys_cx_ecpoint_export_bn(const cx_ecpoint_t *ec_P, cx_bn_t *bn_x,
                                  cx_bn_t *bn_y)
{
  cx_err_t error;
  cx_mpi_t *xy;
  cx_mpi_ecpoint_t P;

  CX_CHECK(cx_ecpoint_normalize(&P, ec_P));
  if (bn_x != NULL) {
    CX_CHECK(cx_bn_to_mpi(*bn_x, &xy));
    CX_CHECK(cx_mpi_copy(xy, P.x));
  }
  if (bn_y != NULL) {
    CX_CHECK(cx_bn_to_mpi(*bn_y, &xy));
    CX_CHECK(cx_mpi_copy(xy, P.y));
  }
end:
  return error;
}

cx_err_t sys_cx_ecpoint_export(const cx_ecpoint_t *ec_P, uint8_t *x,
                               size_t x_len, uint8_t *y, size_t y_len)
{
  cx_err_t error;
  cx_mpi_ecpoint_t P;

  CX_CHECK(cx_ecpoint_normalize(&P, ec_P));
  if (x != NULL) {
    CX_CHECK(cx_mpi_export(P.x, x, x_len));
  }
  if (y != NULL) {
    CX_CHECK(cx_mpi_export(P.y, y, y_len));
  }
end:
  return error;
}

cx_err_t sys_cx_ecpoint_scalarmul(cx_ecpoint_t *ec_P, const uint8_t *k,
                                  size_t k_len)
{
  cx_err_t error;
  cx_mpi_ecpoint_t P;
  cx_mpi_t *Qx, *Qy, *e;

  CX_CHECK(cx_mpi_ecpoint_from_ecpoint(&P, ec_P));
  // TODO Check if ec_P point is on curve.
  // TODO (?) use internal alloc, to update/check count & total memory.
  Qx = BN_new();
  Qy = BN_new();
  e = BN_new();

  if (Qx == NULL || Qy == NULL || e == NULL) {
    error = CX_MEMORY_FULL;
    goto cleanup;
  }
  if (BN_bin2bn(k, k_len, e) == NULL) {
    error = CX_INTERNAL_ERROR;
    goto cleanup;
  }
  switch (ec_P->curve) {
  case CX_CURVE_Ed25519: {
    if (scalarmult_ed25519(Qx, Qy, P.x, P.y, e) != 0) {
      error = CX_INTERNAL_ERROR;
      goto cleanup;
    }
    break;
  }
  case CX_CURVE_SECP256K1:
  case CX_CURVE_SECP256R1:
  case CX_CURVE_SECP384R1:
  case CX_CURVE_Stark256:
  case CX_CURVE_BrainPoolP256R1:
  case CX_CURVE_BrainPoolP256T1:
  case CX_CURVE_BrainPoolP320R1:
  case CX_CURVE_BrainPoolP320T1:
  case CX_CURVE_BrainPoolP384R1:
  case CX_CURVE_BrainPoolP384T1:
  case CX_CURVE_BrainPoolP512R1:
  case CX_CURVE_BrainPoolP512T1: {
    if (cx_weierstrass_mult(ec_P->curve, Qx, Qy, P.x, P.y, e) != 1) {
      error = CX_INTERNAL_ERROR;
      goto cleanup;
    }
    break;
  }
  default: {
    error = CX_EC_INVALID_CURVE;
    goto cleanup;
  }
  }
  // Copy Qx & Qy into P.x, P.y:
  BN_copy(P.x, Qx);
  BN_copy(P.y, Qy);
  // At this point, error value is still CX_OK.

cleanup:
  // No need to check for NULL, OpenSSL does it already.
  BN_clear_free(e);
  BN_clear_free(Qy);
  BN_clear_free(Qx);
end:
  return error;
}

cx_err_t sys_cx_ecpoint_scalarmul_bn(cx_ecpoint_t *ec_P, const cx_bn_t bn_k)
{
  cx_err_t error;
  cx_mpi_t *k;
  uint8_t *bytes;
  int num_bytes;

  CX_CHECK(cx_bn_to_mpi(bn_k, &k));
  // Convert bn_k into big-endian form and store it in 'bytes':
  num_bytes = cx_mpi_nbytes(k);
  bytes = malloc(num_bytes);
  if (bytes == NULL) {
    return CX_MEMORY_FULL;
  }
  if (BN_bn2binpad(k, bytes, num_bytes) != -1) {
    error = sys_cx_ecpoint_scalarmul(ec_P, bytes, num_bytes);
  } else {
    error = CX_INTERNAL_ERROR;
  }
  free(bytes);
end:
  return error;
}

cx_err_t sys_cx_ecpoint_rnd_scalarmul(cx_ecpoint_t *ec_P, const uint8_t *k,
                                      size_t k_len)
{
  // rnd is 'only' for security, which we don't care about in speculos:
  return sys_cx_ecpoint_scalarmul(ec_P, k, k_len);
}

cx_err_t sys_cx_ecpoint_rnd_scalarmul_bn(cx_ecpoint_t *ec_P, const cx_bn_t bn_k)
{
  // rnd is 'only' for security, which we don't care about in speculos:
  return sys_cx_ecpoint_scalarmul_bn(ec_P, bn_k);
}

cx_err_t sys_cx_ecpoint_rnd_fixed_scalarmul(cx_ecpoint_t *ec_P,
                                            const uint8_t *k, size_t k_len)
{
  return sys_cx_ecpoint_rnd_scalarmul(ec_P, k, k_len);
}

cx_err_t sys_cx_ecpoint_double_scalarmul(cx_ecpoint_t *ec_R, cx_ecpoint_t *ec_P,
                                         cx_ecpoint_t *ec_Q, const uint8_t *k,
                                         size_t k_len, const uint8_t *r,
                                         size_t r_len)
{
  cx_err_t error;

  CX_CHECK(sys_cx_ecpoint_scalarmul(ec_P, k, k_len));
  CX_CHECK(sys_cx_ecpoint_scalarmul(ec_Q, r, r_len));
  CX_CHECK(sys_cx_ecpoint_add(ec_R, ec_P, ec_Q));
end:
  return error;
}

cx_err_t sys_cx_ecpoint_double_scalarmul_bn(cx_ecpoint_t *ec_R,
                                            cx_ecpoint_t *ec_P,
                                            cx_ecpoint_t *ec_Q,
                                            const cx_bn_t bn_k,
                                            const cx_bn_t bn_r)
{
  cx_mpi_t *k, *r;
  uint8_t *kbytes, *rbytes;
  int k_len, r_len;
  cx_err_t error;

  kbytes = rbytes = NULL;
  CX_CHECK(cx_bn_ab_to_mpi(bn_k, &k, bn_r, &r));
  // Convert bn_k/bn_r into big-endian form and store it in 'kbytes/rbytes':
  k_len = cx_mpi_nbytes(k);
  r_len = cx_mpi_nbytes(r);
  kbytes = malloc(k_len);
  rbytes = malloc(r_len);
  if (kbytes == NULL || rbytes == NULL) {
    error = CX_MEMORY_FULL;
    goto end;
  }
  if (BN_bn2binpad(k, kbytes, k_len) != -1 &&
      BN_bn2binpad(r, rbytes, r_len) != -1) {
    error = sys_cx_ecpoint_double_scalarmul(ec_R, ec_P, ec_Q, kbytes, k_len,
                                            rbytes, r_len);
  } else {
    error = CX_INTERNAL_ERROR;
  }
end:
  free(rbytes);
  free(kbytes);
  return error;
}

cx_err_t sys_cx_ecpoint_add(cx_ecpoint_t *ec_R, const cx_ecpoint_t *ec_P,
                            const cx_ecpoint_t *ec_Q)
{
  cx_err_t error;
  POINT R, P, Q;
  int nid;
  EC_GROUP *group;
  EC_POINT *p, *q, *r;

  if (ec_P->curve != ec_Q->curve) {
    return CX_EC_INVALID_POINT;
  }
  CX_CHECK(cx_bn_locked());
  if (ec_P->curve == CX_CURVE_Ed25519) {
    CX_CHECK(cx_bn_ab_to_mpi(ec_R->x, &R.x, ec_R->y, &R.y));
    CX_CHECK(cx_bn_ab_to_mpi(ec_P->x, &P.x, ec_P->y, &P.y));
    CX_CHECK(cx_bn_ab_to_mpi(ec_Q->x, &Q.x, ec_Q->y, &Q.y));
    // TODO: check that P, Q & R points are on the curve
    if (edwards_add(&R, &P, &Q) != 0) {
      error = CX_INTERNAL_ERROR;
    }
  } else {
    // Try to use EC_POINT_add:
    if ((nid = cx_nid_from_curve(ec_P->curve)) < 0 ||
        (group = cx_group_from_nid_and_curve(nid, ec_P->curve)) == NULL) {
      return CX_EC_INVALID_CURVE;
    }
    p = EC_POINT_from_ecpoint(group, ec_P, true);
    q = EC_POINT_from_ecpoint(group, ec_Q, true);
    r = EC_POINT_from_ecpoint(group, ec_R, false);

    if (p == NULL || q == NULL || r == NULL) {
      error = CX_MEMORY_FULL;
    } else if (EC_POINT_add(group, r, p, q, cx_get_bn_ctx()) == 0) {
      error = CX_INTERNAL_ERROR;
    } else {
      error = cx_ecpoint_from_EC_POINT(group, ec_R, r);
    }
    EC_POINT_clear_free(r);
    EC_POINT_clear_free(q);
    EC_POINT_clear_free(p);
    EC_GROUP_free(group);
  }
  // Don't forget z member!
  if (error == CX_OK) {
    error = sys_cx_bn_copy(ec_R->z, ec_P->z);
  }
end:
  return error;
}

cx_err_t sys_cx_ecpoint_cmp(const cx_ecpoint_t *ec_P, const cx_ecpoint_t *ec_Q,
                            bool *is_equal)
{
  cx_err_t error;
  cx_mpi_ecpoint_t P, Q;

  CX_CHECK(cx_ecpoint_normalize(&P, ec_P));
  CX_CHECK(cx_ecpoint_normalize(&Q, ec_Q));
  *is_equal = (P.curve == Q.curve) && (BN_cmp(P.x, Q.x) == 0) &&
              (BN_cmp(P.y, Q.y) == 0);
end:
  return error;
}

cx_err_t sys_cx_ecpoint_neg(cx_ecpoint_t *ec_P)
{
  cx_mpi_ecpoint_t P;
  const cx_curve_domain_t *domain;
  cx_mpi_t *n, *coord;
  cx_bn_t bn_n;
  cx_err_t error;

  CX_CHECK(cx_mpi_ecpoint_from_ecpoint(&P, ec_P));

  if ((domain = cx_ecdomain(P.curve)) == NULL) {
    return CX_EC_INVALID_CURVE;
  }
  // TODO check ecpoint is on curve
  if ((n = cx_mpi_alloc_init(&bn_n, domain->length, domain->p,
                             domain->length)) == NULL) {
    return CX_MEMORY_FULL;
  }
  coord = NULL;

  if (CX_CURVE_RANGE(P.curve, WEIERSTRASS)) {
    coord = P.y;
  } else if (CX_CURVE_RANGE(P.curve, TWISTED_EDWARDS)) {
    coord = P.x;
  } else if (CX_CURVE_RANGE(P.curve, MONTGOMERY)) {
    coord = P.y;
  }

  if (coord == NULL) {
    error = CX_EC_INVALID_POINT;
  } else {
    cx_mpi_sub(coord, n, coord);
  }
  cx_mpi_destroy(&bn_n);
end:
  return error;
}

cx_err_t sys_cx_ecpoint_compress(const cx_ecpoint_t *ec_P,
                                 uint8_t *xy_compressed,
                                 size_t xy_compressed_len, uint32_t *sign)
{
  cx_err_t error;
  cx_mpi_ecpoint_t P;
  bool set = false;

  CX_CHECK(cx_ecpoint_normalize(&P, ec_P));
  error = CX_EC_INVALID_CURVE; // Now by default

  if (CX_CURVE_RANGE(P.curve, WEIERSTRASS)) {
    CX_CHECK(cx_mpi_export(P.x, xy_compressed, xy_compressed_len));
    CX_CHECK(cx_mpi_tst_bit(P.y, 0, &set));
  } else if (CX_CURVE_RANGE(P.curve, TWISTED_EDWARDS)) {
    CX_CHECK(cx_mpi_export(P.y, xy_compressed, xy_compressed_len));
    CX_CHECK(cx_mpi_tst_bit(P.x, 0, &set));
  } else if (CX_CURVE_RANGE(P.curve, MONTGOMERY)) {
    CX_CHECK(cx_mpi_export(P.x, xy_compressed, xy_compressed_len));
    CX_CHECK(cx_mpi_tst_bit(P.y, 0, &set));
  }

  *sign = set ? 1 : 0;
end:
  return error;
}

cx_err_t sys_cx_ecpoint_decompress(cx_ecpoint_t *ec_P,
                                   const uint8_t *xy_compressed,
                                   size_t xy_compressed_len, uint32_t sign)
{
  cx_mpi_ecpoint_t P;
  cx_err_t error;

  CX_CHECK(cx_mpi_ecpoint_from_ecpoint(&P, ec_P));
  error = CX_EC_INVALID_CURVE; // Now by default

  if (CX_CURVE_RANGE(P.curve, WEIERSTRASS)) {
    cx_mpi_init(P.x, xy_compressed, xy_compressed_len);
    cx_mpi_set_u32(P.y, 0);
    cx_mpi_set_u32(P.z, 1);
    CX_CHECK(cx_weierstrass_recover_y(&P, sign));
  } else if (CX_CURVE_RANGE(P.curve, TWISTED_EDWARDS)) {
    cx_mpi_set_u32(P.x, 0);
    cx_mpi_init(P.y, xy_compressed, xy_compressed_len);
    cx_mpi_set_u32(P.z, 1);
    CX_CHECK(cx_twisted_edwards_recover_x(&P, sign));
  } else if (CX_CURVE_RANGE(P.curve, MONTGOMERY)) {
    cx_mpi_init(P.x, xy_compressed, xy_compressed_len);
    cx_mpi_set_u32(P.y, 0);
    cx_mpi_set_u32(P.z, 1);
    CX_CHECK(cx_montgomery_recover_y(&P, sign));
  }
end:
  return error;
}

cx_err_t sys_cx_ecpoint_is_at_infinity(const cx_ecpoint_t *ec_P,
                                       bool *is_infinite)
{
  cx_mpi_ecpoint_t P;
  cx_err_t error;

  // Instead of calling EC_POINT_is_at_infinity, let's do it like BOLOS:
  CX_CHECK(cx_mpi_ecpoint_from_ecpoint(&P, ec_P));

  if ((cx_mpi_cmp_u32(P.x, 0) == 0) && (cx_mpi_cmp_u32(P.y, 1) == 0) &&
      (cx_mpi_cmp_u32(P.z, 0) == 0)) {
    *is_infinite = 1;
  } else {
    *is_infinite = 0;
  }
end:
  return error;
}

// This only supports SECP256K1, SECP256R1 and SECP384R1 for now due to openssl
cx_err_t sys_cx_ecpoint_is_on_curve(const cx_ecpoint_t *ec_P, bool *is_on_curve)
{
  cx_err_t error = CX_INTERNAL_ERROR;
  cx_mpi_ecpoint_t P;
  int nid;
  EC_POINT *point;
  BN_CTX *ctx;
  int res = 0;

  EC_GROUP *group = NULL;
  *is_on_curve = false;

  CX_CHECK(cx_mpi_ecpoint_from_ecpoint(&P, ec_P));

  if ((nid = cx_nid_from_curve(ec_P->curve)) >= 0) {
    group = cx_group_from_nid_and_curve(nid, ec_P->curve);
  }
  if (group != NULL) {
    point = EC_POINT_new(group);

    if (point != NULL) {
      ctx = cx_get_bn_ctx();

      if (ctx != NULL) {
        if (EC_POINT_set_affine_coordinates(group, point, P.x, P.y, ctx) == 1) {
          res = EC_POINT_is_on_curve(group, point, ctx);
        }
      }
    }
  }
  if (res == -1) {
    return CX_EC_INVALID_POINT;
  }
  if (res) {
    *is_on_curve = true;
  }

end:
  return error;
}
