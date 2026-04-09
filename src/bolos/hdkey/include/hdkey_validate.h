/**
 * @file hdkey_validate.h
 * @brief This is the public header file for hdkey_validate.c.
 */

#pragma once

/*********************
 *      INCLUDES
 *********************/
#include "hdkey.h"
#include <stddef.h>

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
 * @brief Validates secure context before derivation.
 * @param[in] params Pointer to the HDKEY structure.
 * @return os_result_t
 * @retval OS_SUCCESS Success
 * @retval OS_ERR_BAD_PARAM Invalid parameter
 * @retval OS_ERR_NOT_FOUND Unauthorized curve or unknown caller identity
 * @retval OS_ERR_NOT_IMPLEMENTED Unknown parameter
 * @retval OS_ERR_FORBIDDEN Unauthorized derivation path
 * @retval OS_ERR_ACCESS_DENIED PIN must be validated
 * @retval OS_ERR_BAD_FORMAT Invalid path format
 * @retval OS_ERR_INTEGRITY Integrity check failure
 */
os_result_t HDKEY_VALIDATE_secure_context(HDKEY_params_t *params);
