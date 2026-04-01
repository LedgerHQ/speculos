/**
 * @file hdkey_bip32.h
 * @brief This is the public header file for hdkey_bip32.c.
 */

#pragma once

/*********************
 *      INCLUDES
 *********************/
#include "hdkey.h"
#include "os_result.h"

/*********************
 *      DEFINES
 *********************/
#define HDKEY_BIP32_KEY_LEN         (CX_SHA512_SIZE / 2)
#define HDKEY_BIP32_ED25519_KEY_LEN (96u)
#define HDKEY_BIP32_WILDCARD_VALUE  (0x7fffffff)

/**********************
 *      TYPEDEFS
 **********************/
/**
 * @brief Seed type.
 */
typedef enum {
    HDKEY_BIP32_NVM_SEED,  // Seed stored in NVM (default and passphrase seeds: 64 bytes)
} hdkey_bip32_seed_type_t;

/**********************
 * STATIC VARIABLES
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Gets the default HMAC key and computes the seed HMAC value.
 * @param[in] params Pointer to the HDKEY structure.
 * @param[out] result Pointer to the buffer to write the HMAC value, of length result_len
 * @param[in] result_len Result buffer length, must be either 64 bytes or 96 bytes depending on the
 * curve parameter.
 * @return os_result_t
 * @retval OS_SUCCESS Success
 * @retval OS_ERR_BAD_PARAM Invalid parameter
 */
os_result_t HDKEY_BIP32_compute_seed_hmac(HDKEY_params_t         *params,
                                          hdkey_bip32_seed_type_t seed,
                                          uint8_t                *result,
                                          size_t                  result_len);

/**
 * @brief Gets the seed HMAC value.
 * @param[in] params Pointer to the HDKEY structure.
 * @param[out] seed_hmac Pointer to the buffer to write the HMAC value, of length result_len
 * @param[in] seed_hmac_len Result buffer length, must be either 64 bytes or 96 bytes depending on
 * the curve parameter.
 * @return os_result_t
 * @retval OS_SUCCESS Success
 * @retval OS_ERR_BAD_PARAM Invalid parameter
 * @retval OS_ERR_ACCESS_DENIED Unknown caller identity
 */
os_result_t HDKEY_BIP32_get_seed_hmac(HDKEY_params_t *params,
                                      uint8_t        *seed_hmac,
                                      size_t          seed_hmac_len);

/**
 * @brief Recomputes a HMAC value to make sure that the result has the right format and within curve
 * range. Must be called after HDKEY_BIP32_compute_seed_hmac for BIP32 derivation.
 * @param[in] params Pointer to the HDKEY structure.
 * @param[out] result Pointer to the buffer to write the HMAC value, of length result_len
 * @param[in] result_len Result buffer length, must be either 64 bytes or 96 bytes depending on the
 * curve parameter.
 * @param[in] seed_len Previously computed HMAC value length.
 * @return os_result_t
 * @retval OS_SUCCESS Success
 * @retval OS_ERR_BAD_PARAM Invalid parameter
 * @retval OS_ERR_CRYPTO_INTERNAL Cryptographic computation failure
 */
os_result_t HDKEY_BIP32_compute_seed_hmac_final(HDKEY_params_t *params,
                                                uint8_t        *result,
                                                size_t          result_len,
                                                size_t          seed_len);
