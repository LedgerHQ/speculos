/**
 * @file hdkey_zip32.h
 * @brief This is the public header file for hdkey_zip32.c.
 */

#pragma once

/*********************
 *      INCLUDES
 *********************/
#include "hdkey.h"
#include "os_result.h"
#include <stddef.h>
#include <stdint.h>

/*********************
 *      DEFINES
 *********************/
#define HDKEY_ZIP32_KEY_LEN  (32u)
#define HDKEY_ZIP32_HASH_LEN (64u)

/**********************
 *      TYPEDEFS
 **********************/
/**
 * Sapling key structure to hold extended spending key
 */
typedef struct {
  uint8_t ask[HDKEY_ZIP32_KEY_LEN];        // Spending Key
  uint8_t nsk[HDKEY_ZIP32_KEY_LEN];        // Nullifier Key
  uint8_t ovk[HDKEY_ZIP32_KEY_LEN];        // Outgoing Viewing Key
  uint8_t dk[HDKEY_ZIP32_KEY_LEN];         // Diversifier Key
  uint8_t chain_code[HDKEY_ZIP32_KEY_LEN]; // Chain Code
} HDKEY_ZIP32_sapling_xsk_t;

/**
 * Orchard key structure to hold extended spending key
 */
typedef struct {
  uint8_t sk[HDKEY_ZIP32_KEY_LEN];         // Spending key
  uint8_t chain_code[HDKEY_ZIP32_KEY_LEN]; // Chain code
} HDKEY_ZIP32_hard_sk_t;

/**
 * Sapling key derivation tags
 */
enum HDKEY_ZIP32_tag_e {
  HDKEY_ZIP32_TAG_MASTER_ASK = 0x00, // Tag for master spending key
  HDKEY_ZIP32_TAG_MASTER_NSK = 0x01, // Tag for master nullifier key
  HDKEY_ZIP32_TAG_MASTER_OVK = 0x02, // Tag for master viewing key
  HDKEY_ZIP32_TAG_MASTER_DK = 0x10,  // tag for master diversifier key
  HDKEY_ZIP32_TAG_CHILD_HARDENED =
      0x11, // Tag for deriving hardened key (>= 2**31)
  HDKEY_ZIP32_TAG_CHILD_NORMAL = 0x12, // Tag for deriving normal key
  HDKEY_ZIP32_TAG_CHILD_ASK = 0x13,    // Tag for child spending key
  HDKEY_ZIP32_TAG_CHILD_NSK = 0x14,    // Tag for child nullifier key
  HDKEY_ZIP32_TAG_CHILD_OVK = 0x15,    // Tag for child viewing key
  HDKEY_ZIP32_TAG_CHILD_DK = 0x16,     // Tag for child diversifier key
  HDKEY_ZIP32_TAG_ORCHARD_DOMAIN =
      0x81, // Tag for Orchard Child Key Derivation Domain
  HDKEY_ZIP32_TAG_REGISTERED_DOMAIN =
      0xAC, // Tag for Registered derivation domain
};

/**********************
 * STATIC VARIABLES
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
/**
 * @brief ZIP32 Sapling master key generation.
 * @param[out] out_key Pointer to the sapling key structure containing ask, nsk,
 * ovk, dk and c. (ask, nsk, ovk, dk, c) is the master extended spending key.
 * @param[in] seed Pointer to the buffer containing a seed of length seed_len.
 * @param[in] seed_len Seed length, must be at least 32 bytes.
 * @return os_result_t
 */
os_result_t HDKEY_ZIP32_sapling_master_key(HDKEY_ZIP32_sapling_xsk_t *out_key,
                                           const uint8_t *seed,
                                           size_t seed_len);

/**
 * @brief ZIP32 Sapling child key derivation from a hardened index (>= 2**31)
 * @param[out] out_child Pointer to the sapling key structure containing ask,
 * nsk, ovk, dk and c corresponding to the index.
 * @param[in] parent Pointer to the parent sapling key structure from which the
 * next spending key is derived.
 * @param[in] index Index of the spending key to derive.
 * @return os_result_t
 */
os_result_t
HDKEY_ZIP32_sapling_derive_child(HDKEY_ZIP32_sapling_xsk_t *out_child,
                                 const HDKEY_ZIP32_sapling_xsk_t *parent,
                                 uint32_t index);

/**
 * @brief ZIP32 Sapling full derivation given a path of hardened indices.
 * @param[in, out] params Pointer to the HDKEY structure.
 * @return os_result_t
 */
os_result_t HDKEY_ZIP32_sapling_derive(HDKEY_params_t *params);

/**
 * @brief ZIP32 Orchard master key generation.
 * @param[out] out_key Pointer to the orchard key structure containing sk and c.
 *                     (sk, c) is the master extended spending key.
 * @param[in] seed Pointer to the buffer containing a seed of length seed_len.
 * @param[in] seed_len Seed length, must be at least 32 bytes.
 * @return os_result_t
 */
os_result_t HDKEY_ZIP32_orchard_master_key(HDKEY_ZIP32_hard_sk_t *out_key,
                                           uint8_t *seed, size_t seed_len);

/**
 * @brief ZIP32 Orchard child key derivation from a hardened index (>= 2**31)
 * @param[out] out_child Pointer to the orchard key structure containing sk and
 * c corresponding to the index.
 * @param[in] parent Pointer to the parent orchard key structure from which the
 * next spending key is derived.
 * @param[in] index Index of the spending key to derive.
 * @return os_result_t
 */
os_result_t
HDKEY_ZIP32_orchard_derive_child(HDKEY_ZIP32_hard_sk_t *out_child,
                                 const HDKEY_ZIP32_hard_sk_t *parent,
                                 uint32_t index);

/**
 * @brief ZIP32 orchard full derivation given a path of hardened indices.
 * @param[in, out] params Pointer to the HDKEY structure.
 * @return os_result_t
 */
os_result_t HDKEY_ZIP32_orchard_derive(HDKEY_params_t *params);

/**
 * @brief ZIP32 registered subtree root key derivation.
 * @param[out] out_key         Pointer to the structure that will receive the
 * derived subtree root key.
 * @param[in]  zipnumber       The ZIP number (or derivation index) for the
 * target subtree. This must be >= 2**31.
 * @param[in]  seed            Pointer to the seed bytes.
 * @param[in]  seed_len        Length of the \p seed in bytes.
 * @param[in]  context_str     Pointer to the context string identifying the
 * derivation purpose (can be NULL if length is 0).
 * @param[in]  context_str_len Length of the \p context_str in bytes.
 * @return os_result_t
 * @retval OS_SUCCESS Success
 * @retval OS_ERR_BAD_PARAM Invalid parameter
 * @retval OS_ERR_CRYPTO_INTERNAL Cryptographic computation failure
 */
os_result_t HDKEY_ZIP32_hardened_subtree_root_key(
    HDKEY_ZIP32_hard_sk_t *out_key, uint32_t zipnumber, uint8_t *seed,
    size_t seed_len, const uint8_t *context_str, size_t context_str_len);

/**
 * @brief ZIP32 hardened child key derivation.
 * @param[out] out_child   Pointer to the structure that will receive the
 * derived key (sk, c).
 * @param[in]  parent      Pointer to the parent key structure.
 * @param[in]  index       The derivation index, should be >= 2**31
 * @param[in]  ckd_domain  The domain byte (tag) identifying the derivation type
 * @param[in]  lead        Optional leading byte, hashed after the index if
 * `lead != 0` or if \p tag_seq is provided.
 * @param[in]  tag_seq     Optional pointer to a sequence of additional bytes to
 * include in the hash (can be NULL).
 * @param[in]  tag_seq_len Length of the \p tag_seq sequence in bytes.
 * @return os_result_t
 * @retval OS_SUCCESS Success
 * @retval OS_ERR_BAD_PARAM Invalid parameter
 * @retval OS_ERR_CRYPTO_INTERNAL Cryptographic computation failure
 */
os_result_t HDKEY_ZIP32_hardened_derive_child(HDKEY_ZIP32_hard_sk_t *out_child,
                                              HDKEY_ZIP32_hard_sk_t *parent,
                                              uint32_t index,
                                              uint8_t ckd_domain, uint8_t lead,
                                              const uint8_t *tag_seq,
                                              size_t tag_seq_len);

/**
 * @brief ZIP32 registered full derivation given a path of hardened indices.
 * @param[in, out] params Pointer to the HDKEY structure.
 * @return os_result_t
 * @retval OS_SUCCESS Success
 * @retval OS_ERR_BAD_PARAM Invalid parameter
 * @retval OS_ERR_CRYPTO_INTERNAL Cryptographic computation failure
 * @retval OS_ERR_OVERFLOW Buffer overflow
 * @retval OS_ERR_SHORT_BUFFER Buffer too short
 */
os_result_t HDKEY_ZIP32_registered_derive(HDKEY_params_t *params);
