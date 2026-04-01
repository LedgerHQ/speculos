/**
 * @file hdkey_bip32.c
 * @brief Implementation of BIP32 derivation
 * https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki
 */

#define _SDK_2_0_

/*********************
 *      INCLUDES
 *********************/
#include <stddef.h>
#include <stdint.h>
#include "cxlib.h"
#include "environment.h"
#include "hdkey.h"
#include "hdkey_bip32.h"
#include "os_bip32.h"
#include "os_errors.h"
#include "os_seed.h"
#include "os_types.h"

/*********************
 *      DEFINES
 *********************/

/*********************
 *      TYPEDEFS
 *********************/

/*********************
 *  GLOBAL VARIABLES
 *********************/

/*********************
 *  STATIC VARIABLES
 *********************/

/*********************
 *  STATIC FUNCTIONS
 *********************/

/**
 * @brief Gets a predefined key string to expand the seed value, either
 * with HMAC or BLAKE2.
 * @param[in] params Pointer to the HDKEY structure.
 * @return os_result_t
 * @retval OS_SUCCESS Success
 * @retval OS_ERR_BAD_PARAM Invalid parameter
 * @retval OS_ERR_NOT_IMPLEMENTED Unknown curve parameter
 */
static os_result_t hdkey_bip32_get_default_seed(HDKEY_params_t *params)
{
    os_result_t os_error = OS_ERR_BAD_PARAM;

    if (params == NULL) {
        goto end;
    }
    if (params->seed_key_len == 0 || params->seed_key == NULL) {
        if (params->mode == HDW_SLIP21) {
            params->seed_key     = SLIP21_SEED;
            params->seed_key_len = SLIP21_SEED_LENGTH;
        }
        else {
            switch (params->curve) {
                case CX_CURVE_SECP256K1:
                    params->seed_key     = BIP32_SECP_SEED;
                    params->seed_key_len = BIP32_SECP_SEED_LENGTH;
                    break;
                case CX_CURVE_256R1:
                    params->seed_key     = BIP32_NIST_SEED;
                    params->seed_key_len = BIP32_NIST_SEED_LENGTH;
                    break;
                case CX_CURVE_Ed25519:
                    params->seed_key     = BIP32_ED_SEED;
                    params->seed_key_len = BIP32_ED_SEED_LENGTH;
                    break;
                case CX_CURVE_BLS12_377_G1:
                    params->seed_key     = BLS12_377_SEED;
                    params->seed_key_len = BLS12377_SEED_LENGTH;
                    break;
                default:
                    os_error = OS_ERR_NOT_IMPLEMENTED;
                    goto end;
            }
        }
    }

    os_error = OS_SUCCESS;

end:
    return os_error;
}

/**
 * @brief Checks the result buffer length.
 * @param[in] curve Curve identifier.
 * @param[in] result_len Length of the result buffer. It must be of size HDKEY_BIP32_ED25519_KEY_LEN
 * if the curve is Ed25519, otherwise CX_SHA512_SIZE.
 * @return os_result_t
 * @retval OS_SUCCESS Success
 * @retval OS_ERR_BAD_PARAM Invalid parameter
 */
static os_result_t hdkey_bip32_check_result_len(cx_curve_t curve, size_t result_len)
{
    if (curve == CX_CURVE_Ed25519) {
        if (result_len != HDKEY_BIP32_ED25519_KEY_LEN) {
            return OS_ERR_BAD_PARAM;
        }
    }
    else {
        if (result_len != CX_SHA512_SIZE) {
            return OS_ERR_BAD_PARAM;
        }
    }

    return OS_SUCCESS;
}

/**
 * @brief Computes a HMAC of a seed stored in NVM using either a given key string or the default key
 * string.
 * @param[in] params Pointer to the HDKEY structure.
 * @param[out] result Pointer to the buffer to write the HMAC value, of length result_len
 * @param[in] result_len Result buffer length, must be either 64 bytes or 96 bytes depending on the
 * curve parameter.
 * @return os_result_t
 * @retval OS_SUCCESS Success
 * @retval OS_ERR_BAD_PARAM Invalid parameter
 */
static os_result_t
hdkey_bip32_compute_seed_hmac_nvm(const HDKEY_params_t *params, uint8_t *result,
                                  size_t result_len)
{
    os_result_t os_error = OS_ERR_BAD_PARAM;

    if ((params == NULL) || (result == NULL)) {
        goto end;
    }

    if ((os_error = hdkey_bip32_check_result_len(params->curve, result_len)) != OS_SUCCESS) {
        goto end;
    }

    if (params->curve == CX_CURVE_Ed25519 && params->mode == HDW_NORMAL) {
        // compute chain code
        result[0] = 0x01;
        env_get_seed(&result[1], 64);
        sys_cx_hmac_sha256(params->seed_key,
                       params->seed_key_len,
                       result,
                       65,
                       result + CX_SHA512_SIZE,
                       CX_SHA256_SIZE);
    }

    env_get_seed(result, 64);
    sys_cx_hmac_sha512(
        params->seed_key, params->seed_key_len, result, CX_SHA512_SIZE, result, CX_SHA512_SIZE);

    os_error = OS_SUCCESS;

end:
    return os_error;
}

/**
 * @brief Recomputes seed HMAC for Ed25519 BIP32 derivation if the result doesn't have the expected
 * format.
 * @param[in] seed_key Pointer to the HMAC default key of length seed_key_len.
 * @param[in] seed_key_len Length of the HMAC default key.
 * @param[in,out] result Pointer to the result buffer of length #HDKEY_BIP32_ED25519_KEY_LEN. Result
 * length must be checked before calling this function.
 * @return os_result_t
 * @retval OS_SUCCESS Success
 * @retval OS_ERR_CRYPTO_INTERNAL Cryptographic computation failure
 */
static os_result_t hdkey_bip32_ed25519_hdw_normal_seed_hmac_final(const uint8_t *seed_key,
                                                                  size_t         seed_key_len,
                                                                  uint8_t       *result)
{
    while (result[31] & 0x20) {
        if (sys_cx_hmac_sha512(seed_key, seed_key_len, result, CX_SHA512_SIZE, result, CX_SHA512_SIZE)
            != CX_SHA512_SIZE) {
            return OS_ERR_CRYPTO_INTERNAL;
        }
    }
    result[0] &= 0xF8;
    result[31] = (result[31] & 0x7F) | 0x40;

    return OS_SUCCESS;
}

/**
 * @brief Recomputes seed HMAC for BIP32 derivation (except Ed25519) if the result doesn't have the
 * expected format.
 * @param[in] curve Curve identifier. Must not be Ed25519.
 * @param[in] seed_key Pointer to the HMAC default key of length seed_key_len.
 * @param[in] seed_key_len Length of the HMAC default key.
 * @param[in] seed_len Seed length.
 * @param[in,out] result Pointer to the result buffer of length #CX_SHA512_SIZE. Result length must
 * be checked before calling this function.
 * @return os_result_t
 * @retval OS_SUCCESS Success
 * @retval OS_ERR_BAD_PARAM Invalid parameter
 * @retval OS_ERR_CRYPTO_INTERNAL Cryptographic computation failure
 */
static os_result_t hdkey_bip32_hdw_normal_seed_hmac_final(cx_curve_t     curve,
                                                          const uint8_t *seed_key,
                                                          size_t         seed_key_len,
                                                          size_t         seed_len,
                                                          uint8_t       *result)
{
  int diff = 0;
  cx_err_t error = CX_OK;
  os_result_t os_error = OS_ERR_BAD_PARAM;
  cx_bn_t bn_n = { 0 };
  cx_bn_t bn_result = { 0 };

  CX_CHECK(sys_cx_bn_lock(HDKEY_BIP32_KEY_LEN, 0));
  CX_CHECK(sys_cx_bn_alloc(&bn_n, HDKEY_BIP32_KEY_LEN));
  CX_CHECK(sys_cx_ecdomain_parameter_bn(curve, CX_CURVE_PARAM_Order, bn_n));
  CX_CHECK(sys_cx_bn_alloc_init(&bn_result, HDKEY_BIP32_KEY_LEN, result,
                                HDKEY_BIP32_KEY_LEN));
  CX_CHECK(sys_cx_bn_cmp_u32(bn_result, 0, &diff));
  while (diff == 0) {
    CX_CHECK(sys_cx_bn_cmp(bn_result, bn_n, &diff));
    if (diff < 0) {
      break;
    }
    sys_cx_hmac_sha512(seed_key, seed_key_len, result, seed_len, result,
                       CX_SHA512_SIZE);
    CX_CHECK(sys_cx_bn_init(bn_result, result, HDKEY_BIP32_KEY_LEN));
    CX_CHECK(sys_cx_bn_cmp_u32(bn_result, 0, &diff));
  }
  os_error = OS_SUCCESS;

end:
    sys_cx_bn_unlock();
    if (error) {
        os_error = OS_ERR_CRYPTO_INTERNAL;
    }
    return os_error;
}

/*********************
 *  GLOBAL FUNCTIONS
 *********************/

os_result_t HDKEY_BIP32_compute_seed_hmac_final(HDKEY_params_t *params,
                                                uint8_t        *result,
                                                size_t          result_len,
                                                size_t          seed_len)
{
    os_result_t os_error = OS_ERR_BAD_PARAM;

    if ((params == NULL) || (result == NULL)) {
        return os_error;
    }

    if ((os_error = hdkey_bip32_check_result_len(params->curve, result_len)) != OS_SUCCESS) {
        goto end;
    }

  if ((params->curve == CX_CURVE_Ed25519) && (params->mode == HDW_NORMAL)) {
    os_error = hdkey_bip32_ed25519_hdw_normal_seed_hmac_final(
        params->seed_key, params->seed_key_len, result);
  } else if ((params->curve != CX_CURVE_Ed25519 &&
              params->mode != HDW_SLIP21)) {
    os_error = hdkey_bip32_hdw_normal_seed_hmac_final(
        params->curve, params->seed_key, params->seed_key_len, seed_len,
        result);
  }

end:
    if (os_error != OS_SUCCESS) {
        explicit_bzero(params, sizeof(HDKEY_params_t));
    }
    return os_error;
}

os_result_t HDKEY_BIP32_compute_seed_hmac(HDKEY_params_t         *params,
                                          hdkey_bip32_seed_type_t seed,
                                          uint8_t                *result,
                                          size_t                  result_len)
{
    os_result_t os_error = OS_ERR_BAD_PARAM;
    // Seed type is unused
    (void) seed;

    if ((params == NULL) || (result == NULL)) {
        return OS_ERR_BAD_PARAM;
    }

    if ((os_error = hdkey_bip32_check_result_len(params->curve, result_len)) != OS_SUCCESS) {
        goto end;
    }

    if ((os_error = hdkey_bip32_get_default_seed(params)) != OS_SUCCESS) {
        goto end;
    }

    os_error = hdkey_bip32_compute_seed_hmac_nvm(params, result, result_len);

end:
    if (os_error != OS_SUCCESS) {
        explicit_bzero(params, sizeof(HDKEY_params_t));
    }
    return os_error;
}

os_result_t HDKEY_BIP32_get_seed_hmac(HDKEY_params_t *params,
                                      uint8_t        *seed_hmac,
                                      size_t          seed_hmac_len)
{
    os_result_t os_error = OS_ERR_BAD_PARAM;

    if ((params == NULL) || (seed_hmac == NULL)) {
        return OS_ERR_BAD_PARAM;
    }

    if ((os_error = hdkey_bip32_check_result_len(params->curve, seed_hmac_len)) != OS_SUCCESS) {
        goto end;
    }

    // OS checks not required here
    os_error = HDKEY_BIP32_compute_seed_hmac(
                params, HDKEY_BIP32_NVM_SEED, seed_hmac, seed_hmac_len);

end:
    if (os_error != OS_SUCCESS) {
        explicit_bzero(params, sizeof(HDKEY_params_t));
    }
    return os_error;
}
