/**
 * @file os_hdkey.h
 * @brief Header file containing prototypes for hierarchical deterministic key
 * derivation
 */

#pragma once

/*********************
 *      INCLUDES
 *********************/
#include "cxlib.h"
#include "os_types.h"
#include <stddef.h>
#include <stdint.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**
 * @brief Derivation authorization to set in application parameters.
 */
typedef enum HDKEY_derive_auth_e {
  HDKEY_DERIVE_AUTH_256K1 = 0x01u, // Authorizes BIP32 derivation with SECP256K1
  HDKEY_DERIVE_AUTH_256R1 = 0x02u, // Authorizes BIP32 derivation with SECP256R1
  HDKEY_DERIVE_AUTH_ED25519 =
      0x04u, // Authorizes BIP32-like derivation with ED25519
  HDKEY_DERIVE_AUTH_SLIP21 = 0x08u,     // Authorizes SLIP21 derivation
  HDKEY_DERIVE_AUTH_BLS12381G1 = 0x10u, // Authorizes EIP2333 derivation
  HDKEY_DERIVE_AUTH_BLS12377G1 =
      0x20u, // Authorizes BIP32-live derivation with BLS12-377
  HDKEY_DERIVE_AUTH_ZIP32 = 0x40u, // Authorizes ZIP32 Sapling derivation with
                                   // Jubjub and Orchard derivation
} HDKEY_derive_auth_t;

/**
 * @brief Derivation modes. Values must be powers of two.
 */
typedef enum HDKEY_derive_mode_e {
  HDKEY_DERIVE_MODE_NORMAL = 0x00u,         // BIP32 derivation
  HDKEY_DERIVE_MODE_ED25519_SLIP10 = 0x01u, // SLIP10 derivation with ED25519
  HDKEY_DERIVE_MODE_SLIP21 = 0x02u,         // SLIP21 derivation
  HDKEY_DERIVE_MODE_RESERVED = 0x04u, // Derivation mode reserved to the OS
  HDKEY_DERIVE_MODE_BLS12377_ALEO =
      0x08u, // BIP32-like derivation with BLS12-377
  HDKEY_DERIVE_MODE_ZIP32_SAPLING = 0x10u, // ZIP32 Sapling hardened derivation
  HDKEY_DERIVE_MODE_ZIP32_ORCHARD = 0x20u, // ZIP32 Orchard derivation
  HDKEY_DERIVE_MODE_ZIP32_REGISTERED = 0x40u, // ZIP32 registered derivation
} HDKEY_derive_mode_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Derives a private key and a chain code with regard to a standard
 * derivation algorithm.
 * @param[in] derivation_mode Derivation mode of type HDKEY_derive_mode_t.
 * Application must have the authorization to use that derivation mode.
 * @param[in] curve Curve required by the derivation algorithm.
 * @param[in] path Pointer to the derivation path buffer. Each index is a 32-bit
 * value. path can be NULL
 * @param[in] path_len Path buffer length.
 * @param[out] private_key Pointer to the private key buffer of length
 * private_key_len. private_key must not be NULL.
 * @param[in] private_key_len Private key buffer length.
 * @param[out] chain_code Pointer to the chain code buffer of length
 * chain_code_len. chain_code can be NULL.
 * @param[in] chain_code_len Chain code buffer length.
 * @param[in] seed (Optional) Pointer to a seed buffer used to expand the BIP39
 * seed. Seed can be NULL.
 * @param[in] seed_len Seed buffer length.
 * @return bolos_err_t
 * @retval 0x0000 Success
 * @retval 0x550B PIN must be validated
 * @retval 0x4214 Unknown caller identity
 * @retval 0x521D Integrity check failure
 * @retval 0x420E Unsupported path length
 * @retval 0x4210 Invalid path format
 * @retval 0x4213 Unknown parameter
 * @retval 0x4215 Forbidden derivation
 * @retval 0x3308 Cryptographic computation failure
 * @retval 0x4212 Buffer overflow
 * @retval 0x420E Invalid parameter
 */
bolos_err_t sys_hdkey_derive(HDKEY_derive_mode_t derivation_mode,
                             cx_curve_t curve, const uint32_t *path,
                             size_t path_len, uint8_t *private_key,
                             size_t private_key_len, uint8_t *chain_code,
                             size_t chain_code_len, uint8_t *seed,
                             size_t seed_len);
