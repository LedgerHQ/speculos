/**
 * @file os_address_book.h
 * @brief Address Book HMAC syscalls
 *
 * Two generic syscalls for the Address Book
 * feature.
 */
#pragma once

#include "decorators.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Salt identifiers for Address Book HMAC key derivation.
 *
 * Fixed set of allowed salt strings, selected by ID.
 */
typedef enum ADDRESS_BOOK_salt_id_e {
  ADDRESS_BOOK_SALT_GROUP = 0,          ///< "AddressBook-Group"
  ADDRESS_BOOK_SALT_IDENTITY = 1,       ///< "AddressBook-Identity"
  ADDRESS_BOOK_SALT_LEDGER_ACCOUNT = 2, ///< "AddressBook-LedgerAccount"
  ADDRESS_BOOK_SALT_COUNT               ///< Last salt ID
} ADDRESS_BOOK_salt_id_t;

/**
 * @brief Computes an HMAC-SHA256 using a BIP32-derived key.
 *
 * Internally:
 *  1. Derive secp256k1 private key from @p bip32_path.
 *  2. Compute HMAC key: K = SHA256(salt || privkey.d) where salt is looked up
 *     from @p salt_id.
 *  3. Compute HMAC-SHA256(K, message) and return in @p hmac_out.
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
bool sys_ADDRESS_BOOK_HMAC(const uint32_t *bip32_path, size_t bip32_path_len,
                           ADDRESS_BOOK_salt_id_t salt_id,
                           const uint8_t *message, size_t message_len,
                           uint8_t *hmac_out);

/**
 * @brief Verifies an HMAC-SHA256 using a BIP32-derived key.
 *
 * Internally:
 *  1. Same key derivation as sys_address_book_hmac.
 *  2. Compute HMAC-SHA256(K, message).
 *  3. Constant-time comparison (os_secure_memcmp) against @p hmac_expected.
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
bool sys_ADDRESS_BOOK_HMAC_VERIFY(const uint32_t *bip32_path,
                                  size_t bip32_path_len,
                                  ADDRESS_BOOK_salt_id_t salt_id,
                                  const uint8_t *message, size_t message_len,
                                  const uint8_t *hmac_expected);
