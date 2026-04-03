/**
 * @file hdkey_bls12377.c
 * @brief Implementation of BLS12-377 key derivation
 * https://github.com/ProvableHQ/aleo-hd-key
 */

/*********************
 *      INCLUDES
 *********************/
#include "hdkey_bls12377.h"
#include "cx_utils.h"
#include "cxlib.h"
#include "hdkey.h"
#include "hdkey_bip32.h"
#include "os_errors.h"
#include "os_result.h"
#include "os_seed.h"
#include "os_types.h"
#include <stddef.h>
#include <stdint.h>

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
 * @brief BLS12-377 child key derivation.
 * @param[in] index Child index.
 * @param[out] seed_chain Pointer to buffer to hold the seed HMAC value.
 * @param[in] seed_chain_len Length of the seed HMAC buffer, must be 64 bytes.
 * @return os_result_t
 * @retval OS_SUCCESS Success
 */
static os_result_t hdkey_bls12377_derive_child(uint32_t index,
                                               uint8_t *seed_chain,
                                               size_t seed_chain_len)
{
  // data always contains 0x00 || seed (32 bytes) || index (4 bytes)
  uint8_t data[HDKEY_BLS12377_KEY_LEN + 5] = { 0 };
  size_t data_len = HDKEY_BLS12377_KEY_LEN;

  memcpy(&data[1], seed_chain, data_len);
  data_len += 1;
  U4BE_ENCODE(data, data_len, index);
  data_len += 4;

  if (sys_cx_hmac_sha512(&seed_chain[HDKEY_BLS12377_KEY_LEN],
                         HDKEY_BLS12377_KEY_LEN, data, data_len, seed_chain,
                         seed_chain_len) != CX_SHA512_SIZE) {
    return OS_ERR_CRYPTO_INTERNAL;
  }

  return OS_SUCCESS;
}

/**
 * @brief BLS12-377 child key derivation given a path of indices.
 * @param[in] params Pointer to the HDKEY structure.
 * @param[out] seed_chain Pointer to buffer to hold the seed HMAC value.
 * @param[in] seed_chain_len Length of the seed HMAC buffer, must be 64 bytes.
 * @return os_result_t
 * @retval OS_SUCCESS Success
 */
static os_result_t hdkey_bls12377_derive_path(const HDKEY_params_t *params,
                                              uint8_t *seed_chain,
                                              size_t seed_chain_len)
{
  os_result_t os_error = OS_SUCCESS;

  for (size_t i = 0; i < params->path_len; i++) {
    if ((os_error = hdkey_bls12377_derive_child(
             params->path[i], seed_chain, seed_chain_len)) != OS_SUCCESS) {
      return os_error;
    }
  }

  return os_error;
}

/*********************
 *  GLOBAL FUNCTIONS
 *********************/
os_result_t HDKEY_BLS12377_derive(HDKEY_params_t *params)
{
  os_result_t os_error = OS_ERR_BAD_PARAM;
  uint8_t seed_chain[CX_SHA512_SIZE] = { 0 };

  if ((params == NULL) || (params->private_key == NULL) ||
      (params->path == NULL)) {
    return os_error;
  }

  if (params->private_key_len < HDKEY_BLS12377_KEY_LEN) {
    os_error = OS_ERR_SHORT_BUFFER;
    goto end;
  }

  if ((params->chain != NULL) &&
      (params->chain_len != HDKEY_BLS12377_KEY_LEN)) {
    os_error = OS_ERR_SHORT_BUFFER;
    goto end;
  }

  // get_master_seed

  if ((os_error = HDKEY_BIP32_get_seed_hmac(
           params, seed_chain, sizeof(seed_chain))) != OS_SUCCESS) {
    goto end;
  }

  // child derivation

  if ((os_error = hdkey_bls12377_derive_path(
           params, seed_chain, sizeof(seed_chain))) != OS_SUCCESS) {
    goto end;
  }

  memcpy(params->private_key, seed_chain, HDKEY_BLS12377_KEY_LEN);

  if (params->chain != NULL) {
    memcpy(params->chain, &seed_chain[HDKEY_BLS12377_KEY_LEN],
           HDKEY_BLS12377_KEY_LEN);
  }

end:
  explicit_bzero(seed_chain, sizeof(seed_chain));
  if (os_error != OS_SUCCESS) {
    explicit_bzero(params, sizeof(HDKEY_params_t));
  }
  return os_error;
}
