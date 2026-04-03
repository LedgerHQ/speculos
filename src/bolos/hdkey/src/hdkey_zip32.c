/**
 * @file hdkey_zip32.c
 * @brief Implementation of ZIP32 key derivation
 */

#define _SDK_2_0_

/*********************
 *      INCLUDES
 *********************/
#include "hdkey_zip32.h"
#include "cx_utils.h"
#include "cxlib.h"
#include "environment.h"
#include "hdkey.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*********************
 *      DEFINES
 *********************/
/**
 * Tag value indicating a ZIP32 registered tag sequence.
 */
#define HDKEY_ZIP32_TAG_SEQUENCE_STR (0xA0u)
/**
 * Tag value indicating a ZIP32 registered context string.
 */
#define HDKEY_ZIP32_TAG_CONTEXT_STR (0x16u)

/*********************
 *      TYPEDEFS
 *********************/
/**
 * ZIP32 registered structure for tag sequence
 */
typedef struct {
  const uint8_t *value; // Tag sequence bytes value
  size_t length;        // Tag sequence length
} hdkey_zip32_tag_t;

/*********************
 *  GLOBAL VARIABLES
 *********************/

/*********************
 *  STATIC VARIABLES
 *********************/
/**
 * BLAKE2B personalization string for ZIP32 expanding
 */
static char *PERSON_Z_EXPAND = "Zcash_ExpandSeed";
/**
 * BLAKE2B personalization string for ZIP32 master key
 */
static char *PERSON_Z_SAPLING = "ZcashIP32Sapling";
/**
 * BLAKE2B personalization string for ZIP32 Orchard master key
 */
static char *PERSON_ORCHARD = "ZcashIP32Orchard";
/**
 * ZIP32 registered domain constant
 */
static char *DOMAIN_REGISTERED = "ZIPRegistered_KD";

/*********************
 *  STATIC FUNCTIONS
 *********************/

/**
 * @brief ZIP32 expanding function based on BLAKE2B.
 * @param[out] out Pointer to the output buffer of length out_len.
 * @param[in] out_len Output buffer length, must be 64 bytes.
 * @param[in] key Pointer to the key buffer of length key_len.
 * @param[in] key_len Key buffer length, should be 32 bytes.
 * @param[in] input Pointer to the input buffer to digest of length input_len.
 * @param[in] input_len Input buffer length.
 * @return cx_err_t
 */
static cx_err_t hdkey_zip32_prf_expand(uint8_t *out, size_t out_len,
                                       const uint8_t *key, size_t key_len,
                                       const uint8_t *input, size_t input_len)
{
  cx_blake2b_t blake2_context;
  cx_err_t error = CX_INTERNAL_ERROR;

  if ((out == NULL) || (key == NULL) || (input == NULL)) {
    goto end;
  }
  ERROR_CHECK(cx_blake2b_init2(&blake2_context, out_len * 8, NULL, 0,
                               (uint8_t *)PERSON_Z_EXPAND,
                               strlen(PERSON_Z_EXPAND)),
              CX_BLAKE2B, CX_INVALID_PARAMETER);
  ERROR_CHECK(spec_cx_blake2b_update(&blake2_context, key, key_len), 1,
              CX_INVALID_PARAMETER);

  ERROR_CHECK(spec_cx_blake2b_update(&blake2_context, input, input_len), 1,
              CX_INVALID_PARAMETER);
  ERROR_CHECK(spec_cx_blake2b_final(&blake2_context, out), 1,
              CX_INVALID_PARAMETER);

end:
  return error;
}

/**
 * @brief ZIP32 hash to scalar. Reduces a hash value modulo a curve order.
 * @param[in] curve Curve identifier.
 * @param[in] hash Pointer to the hash buffer of length hash_len.
 * @param[in] hash_len Hash buffer length, must be 64 bytes.
 * @param[out] scalar Pointer to th scalar buffer of length scalar_len.
 * @param[in] scalar_len Scalar buffer length, must be 32 bytes.
 * @return cx_err_t
 */
static cx_err_t hdkey_zip32_to_scalar(cx_curve_t curve, const uint8_t *hash,
                                      size_t hash_len, uint8_t *scalar,
                                      size_t scalar_len)
{
  uint8_t tmp[HDKEY_ZIP32_HASH_LEN] = { 0 };
  cx_bn_t bn_v = { 0 };
  cx_bn_t bn_m = { 0 };
  cx_bn_t bn_r = { 0 };
  cx_err_t error = CX_INTERNAL_ERROR;
  const cx_curve_domain_t *curve_domain = cx_ecdomain(curve);

  if ((hash == NULL) || (scalar == NULL) ||
      (hash_len != HDKEY_ZIP32_HASH_LEN) ||
      (scalar_len != HDKEY_ZIP32_KEY_LEN)) {
    goto end;
  }
  if (curve_domain != NULL) {
    memcpy(tmp, hash, hash_len);
    cx_swap_bytes(tmp, sizeof(tmp));

    CX_CHECK(sys_cx_bn_lock(sizeof(tmp), 0));
    CX_CHECK(sys_cx_bn_alloc(&bn_r, sizeof(tmp)));
    CX_CHECK(sys_cx_bn_alloc_init(&bn_v, sizeof(tmp), tmp, sizeof(tmp)));
    CX_CHECK(sys_cx_bn_alloc_init(&bn_m, sizeof(tmp), curve_domain->n,
                                  curve_domain->length));
    CX_CHECK(sys_cx_bn_reduce(bn_r, bn_v, bn_m));
    CX_CHECK(sys_cx_bn_export(bn_r, tmp, sizeof(tmp)));

    memcpy(scalar, &tmp[hash_len - scalar_len], scalar_len);
    cx_swap_bytes(scalar, scalar_len);
  }
end:
  if (sys_cx_bn_is_locked()) {
    sys_cx_bn_unlock();
  }
  return error;
}

/**
 * @brief ZIP32 scalar addition. The addition is done modulo a curve order.
 * @param[in] curve Curve identifier.
 * @param[out] out Pointer to the output buffer of length out_len.
 * @param[in] out_len Output buffer length, must be 32 bytes.
 * @param[in] a Pointer to the first operand buffer of length a_len.
 * @param[in] a_len First operand buffer length, must be 32 bytes.
 * @param[in] b Pointer to the first operand buffer of length b_len.
 * @param[in] b_len First operand buffer length, must be 32 bytes.
 * @return cx_err_t
 */
static cx_err_t hdkey_zip32_add_scalar(cx_curve_t curve, uint8_t *out,
                                       size_t out_len, const uint8_t *a,
                                       size_t a_len, const uint8_t *b,
                                       size_t b_len)
{
  uint8_t le_a[HDKEY_ZIP32_KEY_LEN] = { 0 };
  uint8_t le_b[HDKEY_ZIP32_KEY_LEN] = { 0 };
  cx_bn_t bn_a = { 0 };
  cx_bn_t bn_b = { 0 };
  cx_bn_t bn_m = { 0 };
  cx_bn_t bn_r = { 0 };
  cx_err_t error = CX_INTERNAL_ERROR;
  const cx_curve_domain_t *curve_domain = cx_ecdomain(curve);

  if ((out == NULL) || (a == NULL) || (b == NULL)) {
    goto end;
  }

  if ((a_len != out_len) || (b_len != out_len)) {
    error = CX_INVALID_PARAMETER_SIZE;
    goto end;
  }
  if (out_len != HDKEY_ZIP32_KEY_LEN) {
    error = CX_INVALID_PARAMETER_SIZE;
    goto end;
  }
  if (curve_domain != NULL) {
    memcpy(le_a, a, a_len);
    memcpy(le_b, b, b_len);
    cx_swap_bytes(le_a, sizeof(le_a));
    cx_swap_bytes(le_b, sizeof(le_b));

    CX_CHECK(sys_cx_bn_lock(out_len, 0));
    CX_CHECK(sys_cx_bn_alloc(&bn_r, out_len));
    CX_CHECK(sys_cx_bn_alloc_init(&bn_a, out_len, le_a, out_len));
    CX_CHECK(sys_cx_bn_alloc_init(&bn_b, out_len, le_b, out_len));
    CX_CHECK(sys_cx_bn_alloc_init(&bn_m, out_len, curve_domain->n, out_len));
    CX_CHECK(sys_cx_bn_mod_add(bn_r, bn_a, bn_b, bn_m));
    CX_CHECK(sys_cx_bn_set_u32(bn_a, 0));
    CX_CHECK(sys_cx_bn_mod_sub(bn_r, bn_r, bn_a, bn_m));
    CX_CHECK(sys_cx_bn_export(bn_r, out, out_len));
    cx_swap_bytes(out, out_len);
  }
end:
  if (sys_cx_bn_is_locked()) {
    sys_cx_bn_unlock();
  }
  return error;
}

/**
 * @brief Get seed for ZIP32 derivation.
 * @param[out] seed Pointer to seed buffer of length seed_len.
 * @param[in] seed_len Seed buffer length, must be 64 bytes
 * @return os_result_t
 */
static os_result_t hdkey_zip32_get_seed(uint8_t *seed, size_t seed_len)
{
  os_result_t os_error = OS_ERR_BAD_PARAM;

  if ((seed == NULL) || (seed_len != 64)) {
    return os_error;
  }

  // Different from OS implementation
  env_get_seed(seed, seed_len);

  return OS_SUCCESS;
}

/**
 * @brief Parses a specific TLV (Tag-Length-Value) element from a buffer.
 *
 * @param[in]     expected_tag The exact tag byte value that is expected to be
 * found at the current offset.
 * @param[in]     buffer       Pointer to the byte buffer containing the TLV
 * sequence. Can be `NULL` (treated as an empty sequence).
 * @param[in]     buffer_len   The total size of the `buffer` in bytes.
 * @param[out]    out_tag      Pointer to the structure where the parsed payload
 * pointer and its length will be stored.
 * @param[in,out] offset       Pointer to the current reading position index
 * within the buffer. If parsing is successful, this value is incremented by `2
 * + length` to point to the start of the next potential TLV element.
 *
 * @retval OS_SUCCESS          If the TLV element was successfully parsed, or if
 * `buffer` is `NULL`.
 * @retval OS_ERR_SHORT_BUFFER If `out_tag` or `offset` are `NULL`, or if there
 * are fewer than 2 bytes remaining in the buffer to read the tag and length.
 * @retval OS_ERR_OVERFLOW     If the current `*offset` is out of bounds, or if
 * the parsed `length` implies reading past the end of `buffer_len`.
 * @retval OS_ERR_NOT_FOUND    If the tag read from the buffer does not match
 * the `expected_tag`.
 */
static os_result_t hdkey_zip32_parse_tlv_tag(uint8_t expected_tag,
                                             const uint8_t *buffer,
                                             size_t buffer_len,
                                             hdkey_zip32_tag_t *out_tag,
                                             size_t *offset)
{
  uint8_t tag = 0;
  uint8_t length = 0;

  if ((out_tag == NULL) || (offset == NULL)) {
    return OS_ERR_SHORT_BUFFER;
  }

  // tag sequence = NULL is allowed
  if (buffer == NULL) {
    out_tag->value = NULL;
    out_tag->length = 0;
    return OS_SUCCESS;
  }

  if (*offset >= buffer_len) {
    return OS_ERR_OVERFLOW;
  }

  // Check that least tag and length can be parsed
  if (*offset + 2 > buffer_len) {
    return OS_ERR_SHORT_BUFFER;
  }

  tag = buffer[(*offset)++];
  length = buffer[(*offset)++];

  if (tag != expected_tag) {
    return OS_ERR_NOT_FOUND;
  }

  if (*offset + length > buffer_len) {
    return OS_ERR_OVERFLOW;
  }

  if (length == 0) {
    out_tag->value = NULL;
  } else {
    out_tag->value = &buffer[*offset];
  }
  out_tag->length = length;
  *offset += length;

  return OS_SUCCESS;
}

/*********************
 *  GLOBAL FUNCTIONS
 *********************/

os_result_t HDKEY_ZIP32_sapling_master_key(HDKEY_ZIP32_sapling_xsk_t *out_key,
                                           const uint8_t *seed, size_t seed_len)
{
  uint8_t I[HDKEY_ZIP32_HASH_LEN] = { 0 };
  cx_blake2b_t blake2_context = { 0 };
  uint8_t tmp_out[HDKEY_ZIP32_HASH_LEN] = { 0 };
  uint8_t tag_byte;
  cx_err_t error = CX_INTERNAL_ERROR;
  os_result_t os_error = OS_ERR_BAD_PARAM;

  if ((out_key == NULL) || (seed == NULL)) {
    error = CX_OK;
    goto end;
  }

  // I = BLAKE2b-512("ZcashIP32Sapling", S)
  ERROR_CHECK(cx_blake2b_init2(&blake2_context, HDKEY_ZIP32_HASH_LEN * 8, NULL,
                               0, (uint8_t *)PERSON_Z_SAPLING,
                               strlen(PERSON_Z_SAPLING)),
              CX_BLAKE2B, CX_INVALID_PARAMETER);
  ERROR_CHECK(spec_cx_blake2b_update(&blake2_context, seed, seed_len), 1,
              CX_INVALID_PARAMETER);
  ERROR_CHECK(spec_cx_blake2b_final(&blake2_context, I), 1,
              CX_INVALID_PARAMETER);

  // IR = I[32..64]: Master chain code
  memcpy(out_key->chain_code, &I[HDKEY_ZIP32_KEY_LEN], HDKEY_ZIP32_KEY_LEN);

  // ask
  tag_byte = HDKEY_ZIP32_TAG_MASTER_ASK;
  CX_CHECK(hdkey_zip32_prf_expand(tmp_out, sizeof(tmp_out), I,
                                  HDKEY_ZIP32_KEY_LEN, &tag_byte, 1));
  CX_CHECK(hdkey_zip32_to_scalar(CX_CURVE_JUBJUB, tmp_out, sizeof(tmp_out),
                                 out_key->ask, sizeof(out_key->ask)));

  // nsk
  tag_byte = HDKEY_ZIP32_TAG_MASTER_NSK;
  CX_CHECK(hdkey_zip32_prf_expand(tmp_out, sizeof(tmp_out), I,
                                  HDKEY_ZIP32_KEY_LEN, &tag_byte, 1));
  CX_CHECK(hdkey_zip32_to_scalar(CX_CURVE_JUBJUB, tmp_out, sizeof(tmp_out),
                                 out_key->nsk, sizeof(out_key->nsk)));

  // ovk
  tag_byte = HDKEY_ZIP32_TAG_MASTER_OVK;
  CX_CHECK(hdkey_zip32_prf_expand(tmp_out, sizeof(tmp_out), I,
                                  HDKEY_ZIP32_KEY_LEN, &tag_byte, 1));
  memcpy(out_key->ovk, tmp_out, HDKEY_ZIP32_KEY_LEN);

  // dk
  tag_byte = HDKEY_ZIP32_TAG_MASTER_DK;
  CX_CHECK(hdkey_zip32_prf_expand(tmp_out, sizeof(tmp_out), I,
                                  HDKEY_ZIP32_KEY_LEN, &tag_byte, 1));
  memcpy(out_key->dk, tmp_out, HDKEY_ZIP32_KEY_LEN);

  os_error = OS_SUCCESS;

end:
  if (error != CX_OK) {
    os_error = OS_ERR_CRYPTO_INTERNAL;
  }
  return os_error;
}

os_result_t
HDKEY_ZIP32_sapling_derive_child(HDKEY_ZIP32_sapling_xsk_t *out_child,
                                 const HDKEY_ZIP32_sapling_xsk_t *parent,
                                 uint32_t index)
{
  uint8_t I[HDKEY_ZIP32_HASH_LEN] = { 0 };
  uint8_t intermediate_hash[HDKEY_ZIP32_HASH_LEN] = { 0 };
  uint8_t tag_byte;
  size_t offset = 1;
  uint8_t scalar[HDKEY_ZIP32_KEY_LEN] = { 0 };
  // Input: [0x11] || ask || nsk || ovk || dk || I2LEOSP(index)
  uint8_t input_data[1 + 128 + 4] = { 0 };
  uint8_t k_input[HDKEY_ZIP32_KEY_LEN + 1];
  cx_err_t error = CX_INTERNAL_ERROR;
  os_result_t os_error = OS_ERR_BAD_PARAM;

  if ((out_child == NULL) || (parent == NULL)) {
    error = CX_OK;
    goto end;
  }

  input_data[0] = HDKEY_ZIP32_TAG_CHILD_HARDENED;
  memcpy(input_data + offset, parent->ask, HDKEY_ZIP32_KEY_LEN);
  offset += HDKEY_ZIP32_KEY_LEN;
  memcpy(input_data + offset, parent->nsk, HDKEY_ZIP32_KEY_LEN);
  offset += HDKEY_ZIP32_KEY_LEN;
  memcpy(input_data + offset, parent->ovk, HDKEY_ZIP32_KEY_LEN);
  offset += HDKEY_ZIP32_KEY_LEN;
  memcpy(input_data + offset, parent->dk, HDKEY_ZIP32_KEY_LEN);
  offset += HDKEY_ZIP32_KEY_LEN;

  U4LE_ENCODE(input_data, offset, index);

  CX_CHECK(hdkey_zip32_prf_expand(I, sizeof(I), parent->chain_code,
                                  HDKEY_ZIP32_KEY_LEN, input_data,
                                  sizeof(input_data)));

  // Split I into IL and IR
  // IR = I[32..64]: chain code
  memcpy(out_child->chain_code, &I[HDKEY_ZIP32_KEY_LEN], HDKEY_ZIP32_KEY_LEN);

  // I_ask = ToScalar(PRF_expand(IL, [0x13]))
  tag_byte = HDKEY_ZIP32_TAG_CHILD_ASK;
  CX_CHECK(hdkey_zip32_prf_expand(intermediate_hash, sizeof(intermediate_hash),
                                  I, HDKEY_ZIP32_KEY_LEN, &tag_byte, 1));

  CX_CHECK(hdkey_zip32_to_scalar(CX_CURVE_JUBJUB, intermediate_hash,
                                 sizeof(intermediate_hash), scalar,
                                 sizeof(scalar)));

  // ask_child = (I_ask + ask_parent) mod n
  CX_CHECK(hdkey_zip32_add_scalar(
      CX_CURVE_JUBJUB, out_child->ask, sizeof(out_child->ask), scalar,
      sizeof(scalar), parent->ask, sizeof(parent->ask)));

  // I_nsk = ToScalar(PRF_expand(IL, [0x14]))
  tag_byte = HDKEY_ZIP32_TAG_CHILD_NSK;
  CX_CHECK(hdkey_zip32_prf_expand(intermediate_hash, sizeof(intermediate_hash),
                                  I, HDKEY_ZIP32_KEY_LEN, &tag_byte, 1));
  memset(scalar, 0, sizeof(scalar));
  CX_CHECK(hdkey_zip32_to_scalar(CX_CURVE_JUBJUB, intermediate_hash,
                                 sizeof(intermediate_hash), scalar,
                                 sizeof(scalar)));

  // nsk_child = (I_nsk + nsk_parent) mod r
  CX_CHECK(hdkey_zip32_add_scalar(
      CX_CURVE_JUBJUB, out_child->nsk, sizeof(out_child->nsk), scalar,
      sizeof(scalar), parent->nsk, sizeof(parent->nsk)));

  // Compute ovk_child = truncate(PRF_expand(IL, [0x15] || ovk_parent))
  k_input[0] = HDKEY_ZIP32_TAG_CHILD_OVK;
  memcpy(k_input + 1, parent->ovk, HDKEY_ZIP32_KEY_LEN);

  CX_CHECK(hdkey_zip32_prf_expand(intermediate_hash, sizeof(intermediate_hash),
                                  I, HDKEY_ZIP32_KEY_LEN, k_input,
                                  sizeof(k_input)));
  memcpy(out_child->ovk, intermediate_hash, HDKEY_ZIP32_KEY_LEN);

  memset(k_input, 0, sizeof(k_input));
  // Compute dk_child = truncate(PRF_expand(IL, [0x16] || dk_parent))
  k_input[0] = HDKEY_ZIP32_TAG_CHILD_DK;
  memcpy(k_input + 1, parent->dk, HDKEY_ZIP32_KEY_LEN);

  CX_CHECK(hdkey_zip32_prf_expand(intermediate_hash, sizeof(intermediate_hash),
                                  I, HDKEY_ZIP32_KEY_LEN, k_input,
                                  sizeof(k_input)));
  memcpy(out_child->dk, intermediate_hash, HDKEY_ZIP32_KEY_LEN);

  os_error = OS_SUCCESS;

end:
  if (error != CX_OK) {
    os_error = OS_ERR_CRYPTO_INTERNAL;
  }

  return os_error;
}

os_result_t HDKEY_ZIP32_sapling_derive(HDKEY_params_t *params)
{
  uint8_t tmp_seed[64];
  size_t offset = 0;
  os_result_t os_error = OS_ERR_BAD_PARAM;
  HDKEY_ZIP32_sapling_xsk_t out_child = { 0 };
  HDKEY_ZIP32_sapling_xsk_t parent = { 0 };
  size_t dest_max_size;
  size_t total_size;

  if ((params == NULL) || (params->private_key == NULL) ||
      (params->path == NULL)) {
    return os_error;
  }

  if ((params->chain != NULL) &&
      (params->chain_len != sizeof(out_child.chain_code))) {
    os_error = OS_ERR_BAD_LEN;
    goto end;
  }

  dest_max_size = params->private_key_len;
  total_size = sizeof(out_child.ask) + sizeof(out_child.nsk) +
               sizeof(out_child.ovk) + sizeof(out_child.dk);

  // Make sure that dst is large enough before memcpy
  if (dest_max_size < offset + total_size) {
    os_error = OS_ERR_SHORT_BUFFER;
    goto end;
  }

  if ((os_error = hdkey_zip32_get_seed(tmp_seed, sizeof(tmp_seed))) !=
      OS_SUCCESS) {
    goto end;
  }

  if ((os_error = HDKEY_ZIP32_sapling_master_key(
           &parent, tmp_seed, sizeof(tmp_seed))) != OS_SUCCESS) {
    goto end;
  }
  // Clear as it's not needed anymore
  explicit_bzero(tmp_seed, sizeof(tmp_seed));

  for (size_t i = 0; i < params->path_len; i++) {
    if ((os_error = HDKEY_ZIP32_sapling_derive_child(
             &out_child, &parent, params->path[i])) != OS_SUCCESS) {
      goto end;
    }
    // Child becomes parent
    memcpy(&parent, &out_child, sizeof(HDKEY_ZIP32_sapling_xsk_t));
  }

  // Copy keys
  memcpy(params->private_key, out_child.ask, sizeof(out_child.ask));
  offset += HDKEY_ZIP32_KEY_LEN;
  memcpy(&params->private_key[offset], out_child.nsk, sizeof(out_child.nsk));
  offset += HDKEY_ZIP32_KEY_LEN;
  memcpy(&params->private_key[offset], out_child.ovk, sizeof(out_child.ovk));
  offset += HDKEY_ZIP32_KEY_LEN;
  memcpy(&params->private_key[offset], out_child.dk, sizeof(out_child.dk));
  // Copy chain code
  if (params->chain != NULL) {
    memcpy(params->chain, out_child.chain_code, sizeof(out_child.chain_code));
  }

end:
  explicit_bzero(tmp_seed, sizeof(tmp_seed));
  explicit_bzero(&out_child, sizeof(HDKEY_ZIP32_sapling_xsk_t));
  explicit_bzero(&parent, sizeof(HDKEY_ZIP32_sapling_xsk_t));
  if (os_error != OS_SUCCESS) {
    explicit_bzero(params, sizeof(HDKEY_params_t));
  }
  return os_error;
}

os_result_t HDKEY_ZIP32_orchard_master_key(HDKEY_ZIP32_hard_sk_t *out_key,
                                           uint8_t *seed, size_t seed_len)
{
  uint8_t I[HDKEY_ZIP32_HASH_LEN] = { 0 };
  cx_blake2b_t blake2_context = { 0 };
  cx_err_t error = CX_INTERNAL_ERROR;
  os_result_t os_error = OS_ERR_BAD_PARAM;

  if ((out_key == NULL) || (seed == NULL)) {
    error = CX_OK;
    goto end;
  }

  ERROR_CHECK(cx_blake2b_init2(&blake2_context, HDKEY_ZIP32_HASH_LEN * 8, NULL,
                               0, (uint8_t *)PERSON_ORCHARD,
                               strlen(PERSON_ORCHARD)),
              CX_BLAKE2B, CX_INVALID_PARAMETER);
  ERROR_CHECK(spec_cx_blake2b_update(&blake2_context, seed, seed_len), 1,
              CX_INVALID_PARAMETER);
  ERROR_CHECK(spec_cx_blake2b_final(&blake2_context, I), 1,
              CX_INVALID_PARAMETER);

  memcpy(out_key->sk, I, HDKEY_ZIP32_KEY_LEN);
  memcpy(out_key->chain_code, &I[HDKEY_ZIP32_KEY_LEN], HDKEY_ZIP32_KEY_LEN);

  os_error = OS_SUCCESS;

end:
  if (error != CX_OK) {
    os_error = OS_ERR_CRYPTO_INTERNAL;
  }
  return os_error;
}

os_result_t
HDKEY_ZIP32_orchard_derive_child(HDKEY_ZIP32_hard_sk_t *out_child,
                                 const HDKEY_ZIP32_hard_sk_t *parent,
                                 uint32_t index)
{
  // Tag + SK_parent (32 bytes) + Index (4 bytes)
  uint8_t data[1 + HDKEY_ZIP32_KEY_LEN + 4] = { 0 };
  uint8_t I[HDKEY_ZIP32_HASH_LEN] = { 0 };
  cx_err_t error = CX_INTERNAL_ERROR;
  os_result_t os_error = OS_ERR_BAD_PARAM;

  if ((out_child == NULL) || (parent == NULL)) {
    error = CX_OK;
    goto end;
  }

  // data = tag || sk || i2leosp(index)
  data[0] = HDKEY_ZIP32_TAG_ORCHARD_DOMAIN;
  memcpy(&data[1], parent->sk, HDKEY_ZIP32_KEY_LEN);
  U4LE_ENCODE(data, HDKEY_ZIP32_KEY_LEN + 1, index);

  CX_CHECK(hdkey_zip32_prf_expand(I, sizeof(I), parent->chain_code,
                                  sizeof(parent->chain_code), data,
                                  sizeof(data)));

  memcpy(out_child->sk, I, HDKEY_ZIP32_KEY_LEN);
  memcpy(out_child->chain_code, &I[HDKEY_ZIP32_KEY_LEN], HDKEY_ZIP32_KEY_LEN);
  os_error = OS_SUCCESS;

end:
  if (error != CX_OK) {
    os_error = OS_ERR_CRYPTO_INTERNAL;
  }
  return os_error;
}

os_result_t HDKEY_ZIP32_orchard_derive(HDKEY_params_t *params)
{
  uint8_t tmp_seed[64] = { 0 };
  os_result_t os_error = OS_ERR_BAD_PARAM;
  HDKEY_ZIP32_hard_sk_t out_child = { 0 };
  HDKEY_ZIP32_hard_sk_t parent = { 0 };

  if ((params == NULL) || (params->private_key == NULL) ||
      (params->path == NULL)) {
    return os_error;
  }

  if (params->private_key_len < HDKEY_ZIP32_KEY_LEN) {
    os_error = OS_ERR_SHORT_BUFFER;
    goto end;
  }

  if ((params->chain != NULL) && (params->chain_len != HDKEY_ZIP32_KEY_LEN)) {
    os_error = OS_ERR_BAD_LEN;
    goto end;
  }

  if ((os_error = hdkey_zip32_get_seed(tmp_seed, sizeof(tmp_seed))) !=
      OS_SUCCESS) {
    goto end;
  }

  if ((os_error = HDKEY_ZIP32_orchard_master_key(
           &parent, tmp_seed, sizeof(tmp_seed))) != OS_SUCCESS) {
    goto end;
  }
  // Clear as it's not needed anymore
  explicit_bzero(tmp_seed, sizeof(tmp_seed));

  for (size_t i = 0; i < params->path_len; i++) {
    if ((os_error = HDKEY_ZIP32_orchard_derive_child(
             &out_child, &parent, params->path[i])) != OS_SUCCESS) {
      goto end;
    }
    // Child becomes parent
    memcpy(&parent, &out_child, sizeof(HDKEY_ZIP32_hard_sk_t));
  }

  // Copy key
  memcpy(params->private_key, out_child.sk, sizeof(out_child.sk));
  // Copy chain code
  if (params->chain != NULL) {
    memcpy(params->chain, out_child.chain_code, sizeof(out_child.chain_code));
  }

end:
  explicit_bzero(tmp_seed, sizeof(tmp_seed));
  explicit_bzero(&out_child, sizeof(HDKEY_ZIP32_hard_sk_t));
  explicit_bzero(&parent, sizeof(HDKEY_ZIP32_hard_sk_t));
  if (os_error != OS_SUCCESS) {
    explicit_bzero(params, sizeof(HDKEY_params_t));
  }
  return os_error;
}

os_result_t HDKEY_ZIP32_hardened_derive_child(HDKEY_ZIP32_hard_sk_t *out_child,
                                              HDKEY_ZIP32_hard_sk_t *parent,
                                              uint32_t index,
                                              uint8_t ckd_domain, uint8_t lead,
                                              const uint8_t *tag_seq,
                                              size_t tag_seq_len)
{
  // Tag + SK_parent (32 bytes) + Index (4 bytes)
  uint8_t data[1 + HDKEY_ZIP32_KEY_LEN + 4] = { 0 };
  uint8_t I[HDKEY_ZIP32_HASH_LEN] = { 0 };
  cx_blake2b_t blake2_context = { 0 };
  cx_err_t error = CX_INTERNAL_ERROR;
  os_result_t os_error = OS_ERR_BAD_PARAM;

  if ((out_child == NULL) || (parent == NULL)) {
    error = CX_OK;
    goto end;
  }

  // Concatenate data = tag || sk || i2leosp(index)
  data[0] = ckd_domain;
  memcpy(&data[1], parent->sk, HDKEY_ZIP32_KEY_LEN);
  U4LE_ENCODE(data, HDKEY_ZIP32_KEY_LEN + 1, index);

  ERROR_CHECK(cx_blake2b_init2(&blake2_context, sizeof(I) * 8, NULL, 0,
                               (uint8_t *)PERSON_Z_EXPAND,
                               strlen(PERSON_Z_EXPAND)),
              CX_BLAKE2B, CX_INVALID_PARAMETER);
  ERROR_CHECK(spec_cx_blake2b_update(&blake2_context, parent->chain_code,
                                     sizeof(parent->chain_code)),
              1, CX_INVALID_PARAMETER);
  ERROR_CHECK(spec_cx_blake2b_update(&blake2_context, data, sizeof(data)), 1,
              CX_INVALID_PARAMETER);
  // Update lead
  if ((lead != 0) || (tag_seq != NULL)) {
    ERROR_CHECK(spec_cx_blake2b_update(&blake2_context, &lead, 1), 1,
                CX_INVALID_PARAMETER);
  }
  // Update tag sequence
  if (tag_seq != NULL) {
    ERROR_CHECK(spec_cx_blake2b_update(&blake2_context, tag_seq, tag_seq_len),
                1, CX_INVALID_PARAMETER);
  }
  ERROR_CHECK(spec_cx_blake2b_final(&blake2_context, I), 1,
              CX_INVALID_PARAMETER);

  memcpy(out_child->sk, I, HDKEY_ZIP32_KEY_LEN);
  memcpy(out_child->chain_code, &I[HDKEY_ZIP32_KEY_LEN], HDKEY_ZIP32_KEY_LEN);
  os_error = OS_SUCCESS;

end:
  if (error != CX_OK) {
    os_error = OS_ERR_CRYPTO_INTERNAL;
  }
  return os_error;
}

os_result_t HDKEY_ZIP32_hardened_subtree_root_key(
    HDKEY_ZIP32_hard_sk_t *out_key, uint32_t zipnumber, uint8_t *seed,
    size_t seed_len, const uint8_t *context_str, size_t context_str_len)
{
  uint8_t I[HDKEY_ZIP32_HASH_LEN] = { 0 };
  cx_blake2b_t blake2_context = { 0 };
  HDKEY_ZIP32_hard_sk_t parent = { 0 };
  cx_err_t error = CX_INTERNAL_ERROR;
  os_result_t os_error = OS_ERR_BAD_PARAM;

  if (seed == NULL) {
    error = CX_OK;
    goto end;
  }

  ERROR_CHECK(cx_blake2b_init2(&blake2_context, HDKEY_ZIP32_HASH_LEN * 8, NULL,
                               0, (uint8_t *)DOMAIN_REGISTERED,
                               strlen(DOMAIN_REGISTERED)),
              CX_BLAKE2B, CX_INVALID_PARAMETER);
  ERROR_CHECK(
      spec_cx_blake2b_update(&blake2_context, (uint8_t *)&context_str_len, 1),
      1, CX_INVALID_PARAMETER);
  if (context_str != NULL) {
    spec_cx_blake2b_update(&blake2_context, context_str, context_str_len);
  }
  ERROR_CHECK(spec_cx_blake2b_update(&blake2_context, (uint8_t *)&seed_len, 1),
              1, CX_INVALID_PARAMETER);
  ERROR_CHECK(spec_cx_blake2b_update(&blake2_context, seed, seed_len), 1,
              CX_INVALID_PARAMETER);
  ERROR_CHECK(spec_cx_blake2b_final(&blake2_context, I), 1,
              CX_INVALID_PARAMETER);

  memcpy(parent.sk, I, HDKEY_ZIP32_KEY_LEN);
  memcpy(parent.chain_code, &I[HDKEY_ZIP32_KEY_LEN], HDKEY_ZIP32_KEY_LEN);

  if ((os_error = HDKEY_ZIP32_hardened_derive_child(
           out_key, &parent, zipnumber, HDKEY_ZIP32_TAG_REGISTERED_DOMAIN, 0,
           NULL, 0)) != OS_SUCCESS) {
    goto end;
  }

  os_error = OS_SUCCESS;

end:
  if (error != CX_OK) {
    os_error = OS_ERR_CRYPTO_INTERNAL;
  }
  return os_error;
}

os_result_t HDKEY_ZIP32_registered_derive(HDKEY_params_t *params)
{
  uint8_t tmp_seed[64] = { 0 };
  size_t tag_offset = 0;
  os_result_t os_error = OS_ERR_BAD_PARAM;
  HDKEY_ZIP32_hard_sk_t out_child = { 0 };
  HDKEY_ZIP32_hard_sk_t parent = { 0 };
  hdkey_zip32_tag_t tag = { 0 };

  if ((params == NULL) || (params->private_key == NULL) ||
      (params->path == NULL)) {
    return os_error;
  }

  // Check that private_key buffer is large enough for the derived key
  if (params->private_key_len < sizeof(out_child.sk)) {
    os_error = OS_ERR_SHORT_BUFFER;
    goto end;
  }

  if ((params->chain != NULL) &&
      (params->chain_len != sizeof(out_child.chain_code))) {
    os_error = OS_ERR_BAD_LEN;
    goto end;
  }

  if ((os_error = hdkey_zip32_get_seed(tmp_seed, sizeof(tmp_seed))) !=
      OS_SUCCESS) {
    goto end;
  }

  // Context_str is passed to params->seed
  if ((os_error = hdkey_zip32_parse_tlv_tag(
           HDKEY_ZIP32_TAG_CONTEXT_STR, params->seed_key, params->seed_key_len,
           &tag, &tag_offset)) != OS_SUCCESS) {
    goto end;
  }
  // path[0] contains ZipNumber + 2**31
  if ((os_error = HDKEY_ZIP32_hardened_subtree_root_key(
           &parent, params->path[0], tmp_seed, sizeof(tmp_seed), tag.value,
           tag.length)) != OS_SUCCESS) {
    goto end;
  }
  // Clear as it's not needed anymore
  explicit_bzero(tmp_seed, sizeof(tmp_seed));

  // Tag sequence is passed to params->seed
  // Parse params->seed to get the tag sequence corresponding to each path index
  // If only one tag sequence, use it for all path indices
  if ((os_error = hdkey_zip32_parse_tlv_tag(
           HDKEY_ZIP32_TAG_SEQUENCE_STR, params->seed_key, params->seed_key_len,
           &tag, &tag_offset)) != OS_SUCCESS) {
    goto end;
  }

  for (size_t i = 1; i < params->path_len; i++) {
    if ((os_error = HDKEY_ZIP32_hardened_derive_child(
             &out_child, &parent, params->path[i],
             HDKEY_ZIP32_TAG_REGISTERED_DOMAIN, 0, tag.value, tag.length)) !=
        OS_SUCCESS) {
      goto end;
    }
    // Child becomes parent
    memcpy(&parent, &out_child, sizeof(HDKEY_ZIP32_hard_sk_t));

    if (tag_offset >= params->seed_key_len) {
      continue;
    }
    // If tag sequence not found, use the same sequence
    os_error = hdkey_zip32_parse_tlv_tag(HDKEY_ZIP32_TAG_SEQUENCE_STR,
                                         params->seed_key, params->seed_key_len,
                                         &tag, &tag_offset);
    if ((os_error != OS_SUCCESS) && (os_error != OS_ERR_NOT_FOUND)) {
      goto end;
    }
  }

  // Copy key
  memcpy(params->private_key, out_child.sk, sizeof(out_child.sk));
  // Copy chain code
  if (params->chain != NULL) {
    memcpy(params->chain, out_child.chain_code, sizeof(out_child.chain_code));
  }

end:
  explicit_bzero(tmp_seed, sizeof(tmp_seed));
  explicit_bzero(&out_child, sizeof(HDKEY_ZIP32_hard_sk_t));
  explicit_bzero(&parent, sizeof(HDKEY_ZIP32_hard_sk_t));
  if (os_error != OS_SUCCESS) {
    explicit_bzero(params, sizeof(HDKEY_params_t));
  }
  return os_error;
}
