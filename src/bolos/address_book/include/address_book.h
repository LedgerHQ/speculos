
/**
 * @file address_book.h
 * @brief Public header file for address_book.c.
 */

#pragma once

/*********************
 *      INCLUDES
 *********************/
#include "os_address_book.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * STATIC VARIABLES
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * @brief Computes an HMAC-SHA256 using a BIP32-derived key.
 *
 * @pre  @p bip32_path points to an array of at least @p bip32_path_len
 * elements.
 * @pre  @p message points to a buffer of at least @p message_len bytes.
 * @pre  @p hmac_out points to a buffer of at least 32 bytes.
 *
 * @post On success: @p hmac_out contains the 32-byte HMAC.
 * @post On failure: @p hmac_out is unchanged.
 *
 * @param[in]  bip32_path     BIP32 derivation path (array of uint32_t)
 * @param[in]  bip32_path_len Number of elements in @p bip32_path
 * @param[in]  salt_id        Selects which salt string to use for the KDF
 * @param[in]  message        Pre-serialized HMAC message (app-side)
 * @param[in]  message_len    Length of @p message in bytes
 * @param[out] hmac_out       Output buffer for the 32-byte HMAC
 *
 * @return true on success, false on failure
 */
bool ADDRESS_BOOK_hmac(const uint32_t *bip32_path, size_t bip32_path_len,
                       ADDRESS_BOOK_salt_id_t salt_id, const uint8_t *message,
                       size_t message_len, uint8_t *hmac_out);

/**
 * @brief Verifies an HMAC-SHA256 using a BIP32-derived key.
 *
 * @pre  @p bip32_path points to an array of at least @p bip32_path_len
 * elements.
 * @pre  @p message points to a buffer of at least @p message_len bytes.
 * @pre  @p hmac_expected points to a buffer of at least 32 bytes.
 *
 * @param[in] bip32_path     BIP32 derivation path (array of uint32_t)
 * @param[in] bip32_path_len Number of elements in @p bip32_path
 * @param[in] salt_id        Selects which salt string to use for the KDF
 * @param[in] message        Pre-serialized HMAC message (app-side)
 * @param[in] message_len    Length of @p message in bytes
 * @param[in] hmac_expected  32-byte HMAC to verify against
 *
 * @return true if HMAC matches, false otherwise
 */
bool ADDRESS_BOOK_hmac_verify(const uint32_t *bip32_path, size_t bip32_path_len,
                              ADDRESS_BOOK_salt_id_t salt_id,
                              const uint8_t *message, size_t message_len,
                              const uint8_t *hmac_expected);
