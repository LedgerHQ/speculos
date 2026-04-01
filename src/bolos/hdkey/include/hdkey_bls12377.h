/**
 * @file hdkey_bls12377.h
 * @brief This is the public header file for hdkey_bls12377.c.
 */

#pragma once

/*********************
 *      INCLUDES
 *********************/
#include "hdkey.h"
#include "os_result.h"

/*********************
 *      DEFINES
 *********************/
/**
 * @brief BLS12377 key length in bytes. Child key length and chain code length are equal
 */
#define HDKEY_BLS12377_KEY_LEN (CX_SHA512_SIZE / 2)

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
 * @brief BLS12-377 BIP32-like derivation.
 * @param[in, out] params Pointer to the HDKEY structure.
 * @return os_result_t
 * @retval OS_SUCCESS Success
 * @retval OS_ERR_BAD_PARAM Invalid parameter
 * @retval OS_ERR_ACCESS_DENIED Unknown caller identity
 */
os_result_t HDKEY_BLS12377_derive(HDKEY_params_t *params);
