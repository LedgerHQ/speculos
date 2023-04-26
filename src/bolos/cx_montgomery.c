#define _SDK_2_0_
#include "bolos/cxlib.h"

#define MAX_MONT_BYTE_LEN (56)

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

static uint32_t cx_internal_get_bit(const uint8_t *k, uint32_t k_len,
                                    uint32_t pos)
{
  uint8_t c = k[k_len - 1 - pos / 8];
  return (uint8_t)(c >> (pos % 8u)) & 1u;
}

static bool cx_is_buffer_zero(const uint8_t *buffer, uint32_t buffer_len)
{
  bool i = 0;
  while (buffer_len--) {
    i |= buffer[buffer_len];
  }
  return i == 0;
}

static void cx_reverse_buffer(uint8_t *buf, uint8_t *rev, size_t buf_len)
{
  size_t i;
  memset(rev, 0, buf_len);
  for (i = 0; i < buf_len; i++) {
    rev[i] = buf[buf_len - 1 - i];
  }
}

static void cx_decode_coordinate(cx_curve_t curve, cx_mpi_t *u, uint8_t nbytes)
{
  uint8_t buf[MAX_MONT_BYTE_LEN];

  if (curve == CX_CURVE_Curve25519) {
    cx_mpi_export(u, buf, nbytes);
    // Mask the msb of the final byte of u
    buf[31] &= 0x7F;
    cx_mpi_init(u, buf, nbytes);
  }
  cx_mpi_reverse(u, nbytes);
}

static void cx_decode_scalar(cx_curve_t curve, const uint8_t *k, uint8_t *k_enc,
                             size_t k_len)
{
  uint8_t buf[MAX_MONT_BYTE_LEN];

  memcpy(buf, k, k_len);
  if (curve == CX_CURVE_Curve25519) {
    // Set the three lsb of the first byte of k to 0
    buf[0] &= 0xF8;
    // Set the msb of the last byte of k to 0
    buf[31] &= 0x7F;
    // Set the second msb of the last byteof k to 1
    buf[31] |= 0x40;
  } else {
    // Set the two lsb of the first byte of k to 0
    buf[0] &= 0xFC;
    // Set the msb of the last byte of k to 1
    buf[55] |= 0x80;
  }
  cx_reverse_buffer(buf, k_enc, k_len);
}

/**
 * @brief Scalar multiplication on a Montgomery curve (Curve25519 or Curve448)
 *        using only the u-coordinate of the point.
 *
 * @param[in]      curve Curve identifier:
 *                       - CX_CURVE_Curve25519
 *                       - CX_CURVE_Curve448
 * @param[in, out] x_coordinate U-coordinate of the point to multiply
 * @param[in]      scalar       Scalar
 * @param[in]      scalar_len   Length of the scalar
 * @return Error code
 */
cx_err_t cx_montgomery_mul_coordinate(cx_curve_t curve, cx_mpi_t *u_coordinate,
                                      const uint8_t *scalar, size_t scalar_len)
{
  uint8_t nbytes;
  uint8_t scalar_copy[MAX_MONT_BYTE_LEN];
  int i, bit;
  cx_err_t error;
  const cx_curve_montgomery_t *domain;
  cx_mpi_t *n, *a, *v0, *v1, *v2, *v3, *x;
  cx_bn_t bn_n, bn_a, bn_v0, bn_v1, bn_v2, bn_v3, bn_x;
  cx_ecpoint_t ec_P1, ec_P2;
  cx_mpi_ecpoint_t P1, P2;

  if (cx_is_buffer_zero(scalar, scalar_len)) {
    return CX_EC_INFINITE_POINT;
  }

  domain = (const cx_curve_montgomery_t *)cx_ecdomain(curve);
  if ((domain == NULL) ||
      ((curve != CX_CURVE_Curve448) && (curve != CX_CURVE_Curve25519))) {
    return CX_INVALID_PARAMETER;
  }

  nbytes = domain->length;
  error = CX_MEMORY_FULL;
  if (((n = cx_mpi_alloc_init(&bn_n, nbytes, domain->p, nbytes)) == NULL) ||
      ((a = cx_mpi_alloc(&bn_a, nbytes)) == NULL) ||
      ((v0 = cx_mpi_alloc(&bn_v0, nbytes)) == NULL) ||
      ((v1 = cx_mpi_alloc(&bn_v1, nbytes)) == NULL) ||
      ((v2 = cx_mpi_alloc(&bn_v2, nbytes)) == NULL) ||
      ((v3 = cx_mpi_alloc(&bn_v3, nbytes)) == NULL) ||
      ((x = cx_mpi_alloc(&bn_x, nbytes)) == NULL) ||
      ((error = sys_cx_ecpoint_alloc(&ec_P1, curve)) != CX_OK) ||
      ((error = sys_cx_ecpoint_alloc(&ec_P2, curve)) != CX_OK)) {
    goto end;
  }

  CX_CHECK(cx_mpi_ecpoint_from_ecpoint(&P1, &ec_P1));
  CX_CHECK(cx_mpi_ecpoint_from_ecpoint(&P2, &ec_P2));

  // Decode the u-coordinate
  cx_decode_coordinate(curve, u_coordinate, nbytes);

  // Decode the scalar
  cx_decode_scalar(curve, scalar, scalar_copy, scalar_len);

  // Process non-canonical values
  cx_mpi_rem(P2.x, u_coordinate, n);

  cx_mpi_set_u32(P1.x, 1);
  cx_mpi_set_u32(P1.z, 0);
  cx_mpi_set_u32(P2.z, 1);

  // A24 = (A - 2)/4
  cx_mpi_init(v1, domain->a, nbytes);
  cx_mpi_set_u32(v2, 2);
  cx_mpi_sub(v1, v1, v2);
  cx_mpi_shr(v1, 2);

  cx_mpi_copy(x, P2.x);
  cx_mpi_copy(a, v1);

  int bit_pos = domain->bit_size - 1;
  if (curve == CX_CURVE_Curve25519) {
    bit_pos = bit_pos - 1;
  }

  // Initial swap bit
  int swap = 0;
  // Used for Montgomery ladder
  cx_mpi_set_u32(v0, 0);

  for (i = bit_pos; i >= 0; i--) {
    bit = cx_internal_get_bit(scalar_copy, scalar_len, i);
    cx_mpi_swap(P1.x, P2.x, swap ^ bit);
    cx_mpi_swap(P1.z, P2.z, swap ^ bit);
    swap = bit;
    cx_mpi_mod_add(v1, P1.x, P1.z, n);
    cx_mpi_mod_sub(v2, P1.x, P1.z, n);
    cx_mpi_mod_sub(P1.x, P2.x, P2.z, n);
    cx_mpi_mod_mul(P1.z, P1.x, v1, n);
    cx_mpi_mod_add(P1.x, P2.x, P2.z, n);
    cx_mpi_mod_mul(P2.x, P1.x, v2, n);
    cx_mpi_mod_sub(P1.x, P1.z, P2.x, n);
    cx_mpi_mod_mul(v3, P1.x, P1.x, n);
    cx_mpi_mod_mul(P2.z, x, v3, n);
    cx_mpi_mod_add(P1.z, P1.z, P2.x, n);
    cx_mpi_mod_mul(P2.x, P1.z, P1.z, n);
    cx_mpi_mod_mul(v3, v1, v1, n);
    cx_mpi_mod_add(v3, v3, v0, n);
    cx_mpi_mod_mul(v1, v2, v2, n);
    cx_mpi_mod_mul(P1.x, v3, v1, n);
    cx_mpi_mod_sub(v1, v3, v1, n);
    cx_mpi_mod_mul(v2, v1, a, n);
    cx_mpi_mod_add(v2, v2, v3, n);
    cx_mpi_mod_mul(P1.z, v1, v2, n);
  }

  cx_mpi_swap(P1.x, P2.x, swap);
  cx_mpi_swap(P1.z, P2.z, swap);

  // Calculate the affine coordinate u = X/Z
  CX_CHECK(cx_mpi_mod_invert_nprime(v1, P1.z, n));
  CX_CHECK(cx_mpi_mod_mul(u_coordinate, v1, P1.x, n));

end:
  cx_mpi_destroy(&bn_n);
  cx_mpi_destroy(&bn_a);
  cx_mpi_destroy(&bn_v0);
  cx_mpi_destroy(&bn_v1);
  cx_mpi_destroy(&bn_v2);
  cx_mpi_destroy(&bn_v3);
  cx_mpi_destroy(&bn_x);
  sys_cx_ecpoint_destroy(&ec_P1);
  sys_cx_ecpoint_destroy(&ec_P2);

  return error;
}
