#define _SDK_2_0_
#include "cx_bls.h"
#include "cx_hkdf.h"
#include "cx_utils.h"
#include "cxlib.h"
#include <blst.h>

#define DST_SIZE (64)
// ceil((ceil(log2(p)) + k) / 8) where k = 128 is the security parameter
#define L                (64)
#define EXTENSION_DEGREE (2)

cx_err_t sys_cx_bls12381_key_gen(uint8_t mode, const uint8_t *secret,
                                 size_t secret_len, const uint8_t *salt,
                                 size_t salt_len, uint8_t *key_info,
                                 size_t key_info_len,
                                 cx_ecfp_384_private_key_t *private_key,
                                 uint8_t *public_key, size_t public_key_len)
{
  uint8_t out_key[CX_BLS_BLS12381_PARAM_LEN] = { 0 };
  uint8_t key_info_padded[DST_SIZE] = { 0 };
  cx_err_t error;
  blst_scalar blst_out;

  if (secret_len > CX_BLS_BLS12381_KEY_LEN) {
    return CX_INVALID_PARAMETER_SIZE;
  }
  if (key_info_len > sizeof(key_info_padded) - 2) {
    return CX_INVALID_PARAMETER_SIZE;
  }
  CX_CHECK(cx_mpi_lock(CX_BLS_BLS12381_PARAM_LEN, 0));

  // Derive secret
  if (mode & 0x80) {
    blst_keygen_v5(&blst_out, secret, secret_len, salt, salt_len, key_info,
                   key_info_len);
    blst_bendian_from_scalar(out_key + 16, &blst_out);
  } else {
    memcpy(out_key + 16, secret, secret_len);
  }

  sys_cx_ecfp_init_private_key(
      CX_CURVE_BLS12_381_G1, (const unsigned char *)out_key,
      CX_BLS_BLS12381_PARAM_LEN, (cx_ecfp_private_key_t *)private_key);

  // Compute public key from private key (only for augmented scheme)
  if ((CX_BLS_SIGN_AUG == (mode & 0xF)) || (CX_BLS_PROVE == (mode & 0xF))) {
    cx_ecpoint_t public_point;
    cx_bn_t y_op;
    cx_bn_t p;
    uint32_t sign;
    int diff;

    CX_CHECK(sys_cx_ecpoint_alloc(&public_point, CX_CURVE_BLS12_381_G1));
    CX_CHECK(sys_cx_ecdomain_generator_bn(private_key->curve, &public_point));
    CX_CHECK(sys_cx_ecpoint_scalarmul(&public_point, private_key->d + 16, 32));
    CX_CHECK(sys_cx_ecpoint_compress(&public_point, public_key, public_key_len,
                                     &sign));
    // Compare y and p-y
    CX_CHECK(sys_cx_bn_alloc(&y_op, CX_BLS_BLS12381_PARAM_LEN));
    CX_CHECK(sys_cx_bn_alloc(&p, CX_BLS_BLS12381_PARAM_LEN));
    CX_CHECK(sys_cx_ecdomain_parameter_bn(CX_CURVE_BLS12_381_G1,
                                          CX_CURVE_PARAM_Field, p));
    CX_CHECK(sys_cx_bn_mod_sub(y_op, p, public_point.y, p));
    CX_CHECK(sys_cx_bn_cmp(public_point.y, y_op, &diff));
    if (diff > 0) {
      public_key[0] |= 0xa0;
    } else {
      public_key[0] |= 0x80;
    }
  }

end:
  cx_mpi_unlock();
  return error;
}

static cx_err_t cx_expand_message_xmd(const uint8_t *msg, size_t msg_len,
                                      uint8_t *dst, size_t dst_len,
                                      uint8_t *out, size_t *out_len)
{

  uint32_t ell = *out_len / CX_SHA256_SIZE;
  uint8_t lib_str[3];
  uint8_t b0[32];
  uint8_t tmp[33];
  uint32_t bi_len = CX_SHA256_SIZE;
  uint32_t offset = 0;
  cx_sha256_t sha256_hash;
  cx_hash_ctx *hash = (cx_hash_ctx *)&sha256_hash;

  if (ell > 255) {
    return CX_INVALID_PARAMETER;
  }
  if (*out_len < SHA256_BLOCK_SIZE) {
    return CX_INVALID_PARAMETER;
  }
  memset(out, 0, SHA256_BLOCK_SIZE);
  spec_cx_hash_init(hash, CX_SHA256);
  spec_cx_hash_update(hash, out, SHA256_BLOCK_SIZE);
  spec_cx_hash_update(hash, msg, msg_len);
  U2BE_ENCODE(lib_str, 0, *out_len);
  lib_str[2] = 0x0;
  spec_cx_hash_update(hash, lib_str, sizeof(lib_str));
  spec_cx_hash_update(hash, dst,
                      dst_len); // dst = DST || I2OSP(dst_len, 1)
  spec_cx_hash_final(hash, b0);
  memcpy(tmp, b0, bi_len);
  tmp[bi_len] = 0x1;
  spec_cx_hash_init(hash, CX_SHA256);
  spec_cx_hash_update(hash, tmp, bi_len + 1);
  spec_cx_hash_update(hash, dst, dst_len);
  spec_cx_hash_final(hash, out);
  for (uint32_t i = 2; i <= ell; i++) {
    memcpy(tmp, out + offset, bi_len);
    cx_memxor(tmp, b0, bi_len);
    tmp[bi_len] = (uint8_t)i;
    spec_cx_hash_init(hash, CX_SHA256);
    spec_cx_hash_update(hash, tmp, bi_len + 1);
    spec_cx_hash_update(hash, dst, dst_len);
    offset += bi_len;
    spec_cx_hash_final(hash, out + offset);
  }

  return CX_OK;
}

cx_err_t sys_cx_hash_to_field(const uint8_t *msg, size_t msg_len,
                              const uint8_t *dst, size_t dst_len, uint8_t *hash,
                              size_t hash_len)
{
  size_t len = 2 * EXTENSION_DEGREE * L;
  uint8_t msg_exp[512];
  int elm_offset;
  uint8_t dst_prime[DST_SIZE];
  size_t p_len = 0;
  cx_mpi_t *tv_mpi = NULL;
  cx_mpi_t *p_mpi = NULL;
  cx_mpi_t *ej_mpi = NULL;
  cx_bn_t bn_p, bn_tv, bn_ej;
  cx_err_t error = CX_INTERNAL_ERROR;
  const cx_curve_weierstrass_t *domain;

  if (dst_len > sizeof(dst_prime)) {
    return CX_INVALID_PARAMETER;
  }

  domain = (const cx_curve_weierstrass_t *)cx_ecdomain(CX_CURVE_BLS12_381_G1);
  p_len = domain->length;

  if (hash_len != 2 * EXTENSION_DEGREE * p_len) {
    return CX_INVALID_PARAMETER;
  }

  CX_CHECK(cx_mpi_lock(p_len, 0));

  if (((p_mpi = cx_mpi_alloc_init(&bn_p, p_len, domain->p, p_len)) == NULL) ||
      ((tv_mpi = cx_mpi_alloc(&bn_tv, 2 * p_len)) == NULL) ||
      ((ej_mpi = cx_mpi_alloc(&bn_ej, p_len)) == NULL)) {
    error = CX_MEMORY_FULL;
    goto end;
  }
  memcpy(dst_prime, dst, dst_len);
  dst_prime[dst_len] =
      (uint8_t)dst_len; // I2OSP(dst_len, 1) encode dst_len with one byte in BE
  cx_expand_message_xmd(msg, msg_len, dst_prime, dst_len + 1, msg_exp, &len);
  len = 0;
  for (int i = 0; i < 2; i++) {
    for (int j = 0; j < EXTENSION_DEGREE; j++) {
      elm_offset = L * (j + i * EXTENSION_DEGREE);
      CX_CHECK(cx_mpi_init(tv_mpi, msg_exp + elm_offset, L));
      CX_CHECK(cx_mpi_rem(ej_mpi, tv_mpi, p_mpi));
      CX_CHECK(cx_mpi_export(ej_mpi, hash + len, p_len));
      len += p_len;
    }
  }

end:
  cx_mpi_destroy(&bn_p);
  cx_mpi_destroy(&bn_tv);
  cx_mpi_destroy(&bn_ej);
  cx_mpi_unlock();
  return error;
}

cx_err_t sys_ox_bls12381_sign(const cx_ecfp_384_private_key_t *key,
                              uint8_t *message, size_t message_len,
                              uint8_t *signature, size_t signature_len)
{
  blst_p2_affine point;
  blst_p2 in_point, out_point;
  uint8_t le_key[CX_BLS_BLS12381_KEY_LEN];

  if (!(BLS_FIELD_EXT_LEN * 2 == message_len) &&
      (BLS_COMPRESSED_SIG_LEN == signature_len)) {
    return CX_INVALID_PARAMETER;
  }
  if ((CX_CURVE_BLS12_381_G1 != key->curve) &&
      (CX_BLS_BLS12381_PARAM_LEN != key->d_len)) {
    return CX_INVALID_PARAMETER_SIZE;
  }

  cx_bls_g2_hash_field_to_curve(&point, message, message_len);
  blst_p2_from_affine(&in_point, &point);
  memcpy(le_key, key->d + 16, sizeof(le_key));
  be2le(le_key, sizeof(le_key));
  blst_p2_mult(&out_point, &in_point, le_key, sizeof(le_key) * 8);
  blst_p2_to_affine(&point, &out_point);
  blst_p2_compress(signature, &out_point);

  return CX_OK;
}

cx_err_t sys_cx_bls12381_aggregate(const uint8_t *in, size_t in_len, bool first,
                                   uint8_t *aggregated_signature,
                                   size_t signature_len)
{
  blst_p2_affine in_point, previous_point;
  blst_p2 p2_in, p2_previous, p2_out;

  if (0 == in_len) {
    return CX_INVALID_PARAMETER;
  }
  if (BLS_COMPRESSED_SIG_LEN != signature_len) {
    return CX_INVALID_PARAMETER;
  }

  if (BLST_SUCCESS != blst_p2_uncompress(&in_point, in)) {
    return CX_INTERNAL_ERROR;
  }
  if (first) {
    blst_p2_affine_compress(aggregated_signature, &in_point);
    return CX_OK;
  }
  blst_p2_from_affine(&p2_in, &in_point);
  if (BLST_SUCCESS !=
      blst_p2_uncompress(&previous_point, aggregated_signature)) {
    return CX_INTERNAL_ERROR;
  }
  blst_p2_from_affine(&p2_previous, &previous_point);
  blst_p2_add_or_double(&p2_out, &p2_in, &p2_previous);
  blst_p2_compress(aggregated_signature, &p2_out);

  return CX_OK;
}
