
/**
 * @file address_book.c
 * @brief Implementation of Address Book HMAC functions for speculos
 *
 * This module provides the implementation of two functions ADDRESS_BOOK_hmac
 * and ADDRESS_BOOK_hmac_verify required respectively by the syscalls
 * sys_ADDRESS_BOOK_HMAC and sys_ADDRESS_BOOK_HMAC_VERIFY.
 */

/*********************
 *      INCLUDES
 *********************/
#include "cx.h"
#include "emulate.h"
#include "os_address_book.h"
#include "os_bip32.h"
#include <string.h>

/*********************
 *      DEFINES
 *********************/
/** Size of an HMAC-SHA256 output in bytes */
#define ADDRESS_BOOK_HMAC_SIZE CX_SHA256_SIZE

/** Maximum BIP32 derivation depth accepted by the syscall */
#define ADDRESS_BOOK_MAX_BIP32_DEPTH 10U

/** Maximum HMAC message length accepted by the syscall.
 *
 * Worst case:
 *   gid(32) + scope_len(1) + scope(32) + id_len(1) + identifier(80) + family(1)
 * + chain_id(8) = 155 bytes.
 *
 * Rounded up to 256.
 */
#define ADDRESS_BOOK_HMAC_MSG_MAX_LEN 256U

/*********************
 *      TYPEDEFS
 *********************/

/**
 * @brief Salt descriptor: maps a salt_id to its string and length.
 */
typedef struct {
  const uint8_t *str;
  size_t len;
} ADDRESS_BOOK_salt_entry_t;

/*********************
 *  GLOBAL VARIABLES
 *********************/

/*********************
 *  STATIC VARIABLES
 *********************/

/** Salt strings */
static const uint8_t salt_group[] = "AddressBook-Group";
static const uint8_t salt_identity[] = "AddressBook-Identity";
static const uint8_t salt_ledger_account[] = "AddressBook-LedgerAccount";

/** Salt lookup table, indexed by ADDRESS_BOOK_salt_id_t. */
static const ADDRESS_BOOK_salt_entry_t salt_table[ADDRESS_BOOK_SALT_COUNT] = {
  [ADDRESS_BOOK_SALT_GROUP] = { salt_group, sizeof("AddressBook-Group") - 1U },
  [ADDRESS_BOOK_SALT_IDENTITY] = { salt_identity,
                                   sizeof("AddressBook-Identity") - 1U },
  [ADDRESS_BOOK_SALT_LEDGER_ACCOUNT] = { salt_ledger_account,
                                         sizeof("AddressBook-LedgerAccount") -
                                             1U },
};

/*********************
 *  STATIC FUNCTIONS
 *********************/
/**
 * @brief Derives the HMAC key from a BIP32 path and salt.
 *
 * KDF: K = SHA256(salt || privkey.d)
 *
 * @pre  bip32_path_len <= MAX_BIP32_DEPTH
 * @pre  salt != NULL && salt_len > 0
 *
 * @post On success: hmac_key contains the 32-byte derived key.
 * @post On failure: hmac_key is zeroed.
 * @post In all cases: private key material is wiped before return.
 *
 * @param[in]  bip32_path     BIP32 derivation path
 * @param[in]  bip32_path_len Number of elements in bip32_path
 * @param[in]  salt           Salt bytes
 * @param[in]  salt_len       Salt length in bytes
 * @param[out] hmac_key       Output buffer for the 32-byte HMAC key
 *
 * @return true on success, false on failure
 */
static bool
address_book_derive_hmac_key(const uint32_t *bip32_path, size_t bip32_path_len,
                             const uint8_t *salt, size_t salt_len,
                             uint8_t hmac_key[ADDRESS_BOOK_HMAC_SIZE])
{
  cx_ecfp_256_private_key_t privkey = { 0 };
  cx_sha256_t hash_ctx = { 0 };
  uint8_t hash[ADDRESS_BOOK_HMAC_SIZE] = { 0 };
  bool success = false;

  /* Derive secp256k1 private key from BIP32 path, only 32 bytes are needed */
  uint8_t key_data[64] = { 0 };

  sys_os_perso_derive_node_with_seed_key(
      HDW_NORMAL, CX_CURVE_SECP256K1, (const unsigned int *)bip32_path,
      bip32_path_len, key_data, NULL, NULL, 0);

  sys_cx_ecfp_init_private_key(CX_CURVE_SECP256K1, key_data, 32, &privkey);
  explicit_bzero(key_data, sizeof(key_data));

  /* KDF — SHA256(salt || privkey.d) */
  cx_sha256_init(&hash_ctx);
  spec_cx_sha256_update(&hash_ctx, salt, salt_len);
  spec_cx_sha256_update(&hash_ctx, privkey.d, privkey.d_len);
  spec_cx_sha256_final(&hash_ctx, hash);

  memmove(hmac_key, hash, ADDRESS_BOOK_HMAC_SIZE);
  success = true;

  explicit_bzero(&privkey, sizeof(privkey));
  explicit_bzero(hash, sizeof(hash));
  explicit_bzero(&hash_ctx, sizeof(hash_ctx));
  if (!success) {
    explicit_bzero(hmac_key, ADDRESS_BOOK_HMAC_SIZE);
  }
  return success;
}

/**
 * @brief Core HMAC-SHA256 computation.
 *
 * Derives the HMAC key from the BIP32 path + salt, computes HMAC-SHA256(K,
 * message), and writes the result to hmac_out. All key material is wiped before
 * return.
 *
 * @pre  All parameters validated by the calling public function.
 *
 * @post On success: hmac_out contains the 32-byte HMAC.
 * @post On failure: hmac_out is zeroed.
 * @post In all cases: HMAC key material is wiped before return.
 *
 * @param[in]  bip32_path     BIP32 derivation path
 * @param[in]  bip32_path_len Number of elements in bip32_path
 * @param[in]  salt           Salt bytes for the KDF
 * @param[in]  salt_len       Salt length
 * @param[in]  message        HMAC message data
 * @param[in]  message_len    Message length in bytes
 * @param[out] hmac_out       32-byte output buffer
 *
 * @return true on success, false on failure
 */
static bool address_book_compute_hmac(const uint32_t *bip32_path,
                                      size_t bip32_path_len,
                                      const uint8_t *salt, size_t salt_len,
                                      const uint8_t *message,
                                      size_t message_len,
                                      uint8_t hmac_out[ADDRESS_BOOK_HMAC_SIZE])
{
  uint8_t hmac_key[ADDRESS_BOOK_HMAC_SIZE] = { 0 };
  cx_hmac_sha256_t hmac_ctx = { 0 };
  bool success = false;

  if (!address_book_derive_hmac_key(bip32_path, bip32_path_len, salt, salt_len,
                                    hmac_key)) {
    goto cleanup;
  }

  cx_hmac_sha256_init(&hmac_ctx, hmac_key, ADDRESS_BOOK_HMAC_SIZE);
  cx_hmac((cx_hmac_t *)&hmac_ctx, CX_LAST, message, message_len, hmac_out,
          ADDRESS_BOOK_HMAC_SIZE);
  success = true;

cleanup:
  explicit_bzero(hmac_key, sizeof(hmac_key));
  explicit_bzero(&hmac_ctx, sizeof(hmac_ctx));
  if (!success) {
    explicit_bzero(hmac_out, ADDRESS_BOOK_HMAC_SIZE);
  }
  return success;
}

/**
 * @brief Validates common input parameters for both functions.
 *
 * Checks for NULL pointers, path length bounds, salt_id range, and message
 * length.
 *
 * @param[in] bip32_path     BIP32 derivation path
 * @param[in] bip32_path_len Number of elements in bip32_path
 * @param[in] salt_id        Salt identifier
 * @param[in] message        Message buffer (may be NULL only if message_len ==
 * 0)
 * @param[in] message_len    Message length in bytes
 *
 * @return true if all parameters are valid, false otherwise
 */
static bool address_book_validate_params(const uint32_t *bip32_path,
                                         size_t bip32_path_len,
                                         ADDRESS_BOOK_salt_id_t salt_id,
                                         const uint8_t *message,
                                         size_t message_len)
{
  if (bip32_path == NULL) {
    return false;
  }
  if ((bip32_path_len == 0) ||
      (bip32_path_len > ADDRESS_BOOK_MAX_BIP32_DEPTH)) {
    return false;
  }
  if ((unsigned int)salt_id >= (unsigned int)ADDRESS_BOOK_SALT_COUNT) {
    return false;
  }
  if (message_len > ADDRESS_BOOK_HMAC_MSG_MAX_LEN) {
    return false;
  }
  if ((message_len > 0) && (message == NULL)) {
    return false;
  }
  return true;
}

/*********************
 *  GLOBAL FUNCTIONS
 *********************/

bool ADDRESS_BOOK_hmac(const uint32_t *bip32_path, size_t bip32_path_len,
                       ADDRESS_BOOK_salt_id_t salt_id, const uint8_t *message,
                       size_t message_len, uint8_t *hmac_out)
{
  bool success = false;

  /* Validate inputs */
  if (!address_book_validate_params(bip32_path, bip32_path_len, salt_id,
                                    message, message_len)) {
    goto end;
  }
  if (hmac_out == NULL) {
    goto end;
  }

  const ADDRESS_BOOK_salt_entry_t *salt = &salt_table[salt_id];
  success =
      address_book_compute_hmac(bip32_path, bip32_path_len, salt->str,
                                salt->len, message, message_len, hmac_out);

end:
  return success;
}

bool ADDRESS_BOOK_hmac_verify(const uint32_t *bip32_path, size_t bip32_path_len,
                              ADDRESS_BOOK_salt_id_t salt_id,
                              const uint8_t *message, size_t message_len,
                              const uint8_t *hmac_expected)
{
  uint8_t hmac_computed[ADDRESS_BOOK_HMAC_SIZE] = { 0 };
  bool success = false;

  /* Validate inputs */
  if (!address_book_validate_params(bip32_path, bip32_path_len, salt_id,
                                    message, message_len)) {
    goto cleanup;
  }
  if (hmac_expected == NULL) {
    goto cleanup;
  }

  const ADDRESS_BOOK_salt_entry_t *salt = &salt_table[salt_id];
  if (!address_book_compute_hmac(bip32_path, bip32_path_len, salt->str,
                                 salt->len, message, message_len,
                                 hmac_computed)) {
    goto cleanup;
  }

  /* Constant-time comparison */
  unsigned int diff = 0;
  for (size_t i = 0; i < ADDRESS_BOOK_HMAC_SIZE; i++) {
    diff |= hmac_computed[i] ^ hmac_expected[i];
  }
  if (diff != 0) {
    goto cleanup;
  }
  success = true;

cleanup:
  explicit_bzero(hmac_computed, sizeof(hmac_computed));
  return success;
}
