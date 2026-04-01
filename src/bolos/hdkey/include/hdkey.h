/**
 * @file hdkey.h
 * @brief This is the public header file for hdkey.c.
 */

#pragma once

/*********************
 *      INCLUDES
 *********************/
#include "cx.h"
#include "os_hdkey.h"
#include "os_result.h"
#include "os_types.h"
#include <stddef.h>

/*********************
 *      DEFINES
 *********************/
#define HDKEY_BIP32_WILDCARD_VALUE (0x7fffffff)

/**********************
 *      TYPEDEFS
 **********************/
/**
 * @brief HDKEY structure.
 */
typedef struct HDKEY_params_s {
  HDKEY_derive_mode_t mode; // Derivation mode
  bolos_bool_t
      from_app;     // True if the caller is an application, false otherwise
  cx_curve_t curve; // Curve identifier
  size_t path_len;  // Path length (number of indices)
  size_t private_key_len;  // Private key length
  size_t chain_len;        // Chain code length
  size_t seed_key_len;     // Expanding seed key length
  const uint32_t *path;    // Pointer to the derivation path buffer
  uint8_t *private_key;    // Pointer to the private key buffer
  uint8_t *chain;          // Pointer to the chain buffer
  const uint8_t *seed_key; // Pointer to the expanding key buffer
} HDKEY_params_t;

/**********************
 * STATIC VARIABLES
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
 * @brief Internal function for hierarchical deterministic derivation.
 * @param[in] params Pointer to the HDKEY structure.
 * @return os_result_t
 * @retval 0x0000 Success
 * @retval OS_ERR_ACCESS_DENIED PIN must be validated
 * @retval OS_ERR_NOT_FOUND Unknown caller identity
 * @retval OS_ERR_INTEGRITY Integrity check failure
 * @retval OS_ERR_OVERFLOW Unsupported path length
 * @retval OS_ERR_BAD_FORMAT Invalid path format
 * @retval OS_ERR_NOT_IMPLEMENTED Unknown parameter
 * @retval OS_ERR_FORBIDDEN Forbidden derivation
 * @retval OS_ERR_CRYPTO_INTERNAL Cryptographic computation failure
 * @retval OS_ERR_SHORT_BUFFER Buffer overflow
 * @retval OS_ERR_BAD_PARAM Invalid parameter
 */
os_result_t HDKEY_derive(HDKEY_params_t *params);
