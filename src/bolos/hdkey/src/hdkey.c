/**
 * @file hdkey.c
 * @brief This file contains the functions to perform deterministic key
 * derivation.
 */

/*********************
 *      INCLUDES
 *********************/
#include "hdkey.h"
#include "hdkey_bls12377.h"
#include "hdkey_validate.h"
#include "hdkey_zip32.h"
#include "os_hdkey.h"
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

/*********************
 *  GLOBAL FUNCTIONS
 *********************/

os_result_t HDKEY_derive(HDKEY_params_t *params)
{
  os_result_t os_error = OS_ERR_BAD_PARAM;

  if (params == NULL) {
    return OS_ERR_BAD_PARAM;
  }

  if ((os_error = HDKEY_VALIDATE_secure_context(params)) != OS_SUCCESS) {
    goto end;
  }

  switch (params->mode) {
  case HDKEY_DERIVE_MODE_BLS12377_ALEO:
    os_error = HDKEY_BLS12377_derive(params);
    break;
  case HDKEY_DERIVE_MODE_ZIP32_SAPLING:
    os_error = HDKEY_ZIP32_sapling_derive(params);
    break;
  case HDKEY_DERIVE_MODE_ZIP32_ORCHARD:
    os_error = HDKEY_ZIP32_orchard_derive(params);
    break;
  case HDKEY_DERIVE_MODE_ZIP32_REGISTERED:
    os_error = HDKEY_ZIP32_registered_derive(params);
    break;
  default:
    os_error = OS_ERR_NOT_IMPLEMENTED;
    break;
  }

end:
  return os_error;
}
