/**
 * @file hdkey_validate.c
 * @brief This file contains the functions to validate the context and
 * parameters to allow a derivation.
 */
#define _SDK_2_0_

/*********************
 *      INCLUDES
 *********************/
#include "appflags.h"
#include "cx.h"
#include "cx_utils.h"
#include "hdkey.h"
#include "hdkey_bip32.h"
#include "hdkey_validate.h"
#include "launcher.h"
#include "os_bip32.h"
#include "os_hdkey.h"
#include "os_math.h"
#include <stddef.h>

/*********************
 *      DEFINES
 *********************/
#define BOLOS_APP_IDX (0xFF)

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
 * @brief Validates mode, path format and registry integrity.
 * @param[in] mode Derivation mode.
 * @param[in] path Pointer to the path buffer.
 * @return os_result_t
 * @retval OS_SUCCESS Success
 * @retval OS_ERR_BAD_PARAM Invalid parameter
 * @retval OS_ERR_BAD_FORMAT Invalid path format
 * @retval OS_ERR_INTEGRITY Integrity check failure
 */
static os_result_t hdkey_validate_params(uint32_t mode, const uint32_t *path)
{
  os_result_t os_error = OS_ERR_BAD_PARAM;

  if ((mode == HDW_SLIP21) && (path == NULL)) {
    os_error = OS_ERR_BAD_PARAM;
    goto end;
  }

  // ensure the first BYTE of the prefix for SLIP21 is 0.
  if ((mode == HDW_SLIP21) && (*(const char *)path != 0)) {
    os_error = OS_ERR_BAD_FORMAT;
    goto end;
  }

  // can't specify more than one derivation mode
  if (!IS_POW2_OR_ZERO(mode)) {
    os_error = OS_ERR_BAD_PARAM;
    goto end;
  }

  // OS: if invalid registry, returns OS_ERR_INTEGRITY

  os_error = OS_SUCCESS;

end:
  return os_error;
}

/**
 * @brief Checks whether each path index is hardened.
 * @param[in] mode Derivation mode.
 * @param[in] app_idx Application registry index.
 * @param[in] path Pointer to the path buffer of length path_len.
 * @param[in] path_len Path buffer length (number of indices).
 * @return os_result_t
 * @retval OS_SUCCESS Success
 * @retval OS_ERR_BAD_PARAM Invalid parameter
 */
static os_result_t hdkey_validate_hardened(uint32_t mode, uint32_t app_idx,
                                           const uint32_t *path,
                                           size_t path_len)
{
  os_result_t os_error = OS_ERR_BAD_PARAM;

  if ((mode != HDW_SLIP21) && (app_idx != BOLOS_APP_IDX) &&
      (!(app_flags & APPLICATION_FLAG_DERIVE_MASTER))) {
    unsigned int i;
    for (i = 0; i < path_len; i++) {
      if ((path[i] & 0x80000000) != 0) {
        break;
      }
    }
    if (i == path_len) {
      os_error = OS_ERR_BAD_PARAM;
      goto end;
    }
  }

  os_error = OS_SUCCESS;

end:
  return os_error;
}

/**
 * @brief Gets application allowed paths.
 * @param[in] app_idx Application registry index.
 * @param[out] path Pointer to the paths arrays (an application can have
 * multiple allowed paths).
 * @return Output length
 */
static unsigned int hdkey_get_derivation_path(uint8_t app_idx, uint8_t **path)
{
  // No need to check app_idx as it is always 0 here
  (void)app_idx;
  return get_app_derivation_path(path);
}

/**
 * @brief Validates curve.
 * @param[in] mode Derivation mode
 * @param[in] curve Curve identifier
 * @param[in] app_curve_mask Authorized curve.
 * @return os_result_t
 * @retval OS_SUCCESS Success
 * @retval OS_ERR_BAD_PARAM Invalid parameter
 * @retval OS_ERR_NOT_FOUND Unauthorized curve
 * @retval OS_ERR_NOT_IMPLEMENTED Unknown parameter
 */
static os_result_t hdkey_validate_curve(HDKEY_derive_mode_t mode,
                                        cx_curve_t curve,
                                        uint8_t app_curve_mask)
{
  os_result_t os_error = OS_ERR_BAD_PARAM;
  uint8_t curve_mask = 0;

  // Abort if not authorized to operate on this curve
  switch (mode) {
  case HDW_SLIP21:
    curve_mask = HDKEY_DERIVE_AUTH_SLIP21;
    os_error = OS_SUCCESS;
    goto end;

  case HDKEY_DERIVE_MODE_BLS12377_ALEO:
    if (curve == CX_CURVE_BLS12_377_G1) {
      curve_mask = HDKEY_DERIVE_AUTH_BLS12377G1;
      os_error = OS_SUCCESS;
    }
    goto end;
  case HDKEY_DERIVE_MODE_ZIP32_SAPLING:
    if (curve == CX_CURVE_JUBJUB) {
      curve_mask = HDKEY_DERIVE_AUTH_ZIP32;
      os_error = OS_SUCCESS;
    }
    goto end;
  case HDKEY_DERIVE_MODE_ZIP32_ORCHARD:
    // No curve is required
    curve_mask = HDKEY_DERIVE_AUTH_ZIP32;
    os_error = OS_SUCCESS;
    goto end;
  default:
    break;
  }
  // HDW_NORMAL, HDW_ED25519_SLIP10
  switch (curve) {
  case CX_CURVE_SECP256K1:
    curve_mask = HDKEY_DERIVE_AUTH_256K1;
    os_error = OS_SUCCESS;
    break;
  case CX_CURVE_256R1:
    curve_mask = HDKEY_DERIVE_AUTH_256R1;
    os_error = OS_SUCCESS;
    break;
  case CX_CURVE_Ed25519:
    curve_mask = HDKEY_DERIVE_AUTH_ED25519;
    os_error = OS_SUCCESS;
    break;
  default:
    os_error = OS_ERR_NOT_IMPLEMENTED;
    goto end;
  }

end:
  if (os_error == OS_SUCCESS) {
    os_error = ((app_curve_mask & curve_mask) != curve_mask) ? OS_ERR_NOT_FOUND
                                                             : OS_SUCCESS;
  }
  return os_error;
}

/**
 * @brief Checks if a SLIP21 sub-path matches the requested path.
 * @param[in] params Pointer to the HDKEY structure.
 * @param[in] derive_path Pointer to the derivation path buffer.
 * @param[in] check_offset Current offset in derive_path.
 * @param[in] sub_path_length Length of the sub-path to check (in bytes).
 * @return 1 if the sub-path matches, 0 otherwise.
 */
static uint8_t hdkey_match_slip21_subpath(const HDKEY_params_t *params,
                                          const uint8_t *derive_path,
                                          uint32_t check_offset,
                                          uint8_t sub_path_length)
{
  // compare path as bytes as slip21 path may be a string.
  // NOTE: interpret path first byte (len) as a number of bytes in the path
  if (memcmp(params->path, &derive_path[check_offset + 1], sub_path_length) !=
      0) {
    return 0;
  }
  return 1;
}

/**
 * @brief Checks if a BIP32 sub-path matches the requested path.
 * @param[in] params Pointer to the HDKEY structure.
 * @param[in] derive_path Pointer to the derivation path buffer.
 * @param[in] check_offset Current offset in derive_path.
 * @param[in] sub_path_length Length of the sub-path to check (number of
 * indices).
 * @return 1 if the sub-path matches, 0 otherwise.
 */
static uint8_t hdkey_match_bip32_subpath(const HDKEY_params_t *params,
                                         const uint8_t *derive_path,
                                         uint32_t check_offset,
                                         uint8_t sub_path_length)
{
  for (unsigned int i = 0; i < sub_path_length; i++) {
    uint32_t allowed_prefix = U4BE(derive_path, (check_offset + 1 + 4 * i));
    // path is considered to be valid if the allowed prefix contains
    // the 'wildcard' value 0x7fffffff
    if ((params->path[i] != allowed_prefix) &&
        (allowed_prefix != HDKEY_BIP32_WILDCARD_VALUE)) {
      return 0;
    }
  }
  return 1;
}

/**
 * @brief Validates path format. The path prefix must be allowed for the
 * application.
 * @param[in] params Pointer to the HDKEY structure.
 * @param[in] app_idx Application registry index.
 * @return os_result_t
 * @retval OS_SUCCESS Success
 * @retval OS_ERR_BAD_PARAM Invalid parameter
 * @retval OS_ERR_NOT_FOUND Unauthorized curve
 * @retval OS_ERR_NOT_IMPLEMENTED Unknown parameter
 * @retval OS_ERR_FORBIDDEN Unauthorized derivation path
 */
static os_result_t hdkey_validate_path(const HDKEY_params_t *params,
                                       uint32_t app_idx)
{
  uint8_t path_valid = 0;
  uint32_t derive_path_length;
  uint8_t *derive_path;
  uint32_t check_offset = 1;
  os_result_t os_error = OS_ERR_BAD_PARAM;

  // Check the path against the current application path
  derive_path_length = (app_idx != BOLOS_APP_IDX)
                           ? hdkey_get_derivation_path(app_idx, &derive_path)
                           : 0x00;

  // No information specified, everything is valid
  if (derive_path_length == 0) {
    path_valid = 1;
    os_error = OS_SUCCESS;
    goto end;
  }

  // derive_path_length != 0
  // if only the curve was specified, all paths are valid
  if (derive_path_length == 1) {
    path_valid = 1;
    os_error = OS_SUCCESS;
    goto end;
  }

  // Check app's value
  if ((os_error = hdkey_validate_curve(params->mode, params->curve,
                                       derive_path[0])) != OS_SUCCESS) {
    goto end;
  }

  // Check against allowed paths
  while ((check_offset < derive_path_length) && !path_valid) {
    uint8_t sub_path_length = derive_path[check_offset];
    uint8_t slip21_marker = ((sub_path_length & 0x80) != 0);
    uint8_t slip21_request = (params->mode == HDW_SLIP21);
    uint8_t entry_size = slip21_marker ? 1 : 4;
    sub_path_length &= 0x7F;

    // Bounds check: ensure the sub-path data fits within derive_path
    if (check_offset + 1 + (uint32_t)entry_size * sub_path_length >
        derive_path_length) {
      break;
    }

    // Skip if path too short or slip21 mode mismatch
    if ((params->path_len < sub_path_length) ||
        (slip21_marker != slip21_request)) {
      check_offset += 1 + (uint32_t)entry_size * sub_path_length;
      continue;
    }

    // Authorize if a subpath is valid
    if (slip21_request) {
      path_valid = hdkey_match_slip21_subpath(params, derive_path, check_offset,
                                              sub_path_length);
    } else {
      path_valid = hdkey_match_bip32_subpath(params, derive_path, check_offset,
                                             sub_path_length);
    }
    check_offset += 1 + (uint32_t)entry_size * sub_path_length;
  }

end:
  if (!path_valid) {
    os_error = OS_ERR_FORBIDDEN;
  }
  return os_error;
}

/*********************
 *  GLOBAL FUNCTIONS
 *********************/

os_result_t HDKEY_VALIDATE_secure_context(HDKEY_params_t *params)
{
  os_result_t os_error = OS_ERR_BAD_PARAM;
  uint32_t app_idx = 0;

  if (params == NULL) {
    os_error = OS_ERR_BAD_PARAM;
    goto end;
  }

  // Implement partial verification compared to the OS
  // Only application allowed derivation parameters (allowed path, mode, curve)
  // are checked

  if (params->path_len & 0xF0000000) {
    os_error = OS_ERR_OVERFLOW;
    goto end;
  }

  if ((os_error = hdkey_validate_params(params->mode, params->path)) !=
      OS_SUCCESS) {
    goto end;
  }

  if ((os_error = hdkey_validate_hardened(params->mode, app_idx, params->path,
                                          params->path_len)) != OS_SUCCESS) {
    goto end;
  }

  if ((os_error = hdkey_validate_path(params, app_idx)) != OS_SUCCESS) {
    goto end;
  }

  os_error = OS_SUCCESS;

end:
  return os_error;
}
