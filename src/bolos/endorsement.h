/**
 * @file endorsement.h
 * @brief Emulation of endorsement
 */

#ifndef FILE_NAME_H_
#define FILE_NAME_H_

/*********************
 *      INCLUDES
 *********************/

#include "os_types.h"
#include <stddef.h>
#include <stdint.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

// clang-format off
typedef enum {
    ENDORSEMENT_SLOT_1 = 1,
    ENDORSEMENT_SLOT_2
} ENDORSEMENT_slot_t;
// clang-format on

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/// Legacy syscalls (before API_LEVEL_23)

unsigned long sys_os_endorsement_get_public_key(uint8_t index, uint8_t *buffer);

unsigned int sys_os_endorsement_get_public_key_new(uint8_t index,
                                                   uint8_t *buffer,
                                                   uint8_t *length);

unsigned long sys_os_endorsement_key1_sign_data(uint8_t *data,
                                                size_t dataLength,
                                                uint8_t *signature);
unsigned long
sys_os_endorsement_key1_sign_without_code_hash(uint8_t *data, size_t dataLength,
                                               uint8_t *signature);

unsigned int sys_os_endorsement_get_code_hash(uint8_t *buffer);

unsigned int
sys_os_endorsement_get_public_key_certificate(unsigned char index,
                                              unsigned char *buffer);

unsigned int sys_os_endorsement_get_public_key_certificate_new(
    unsigned char index, unsigned char *buffer, unsigned char *length);

/// Refactored syscalls (API_LEVEL_23 and above)

bolos_err_t sys_ENDORSEMENT_get_public_key(ENDORSEMENT_slot_t slot,
                                           uint8_t *out_public_key,
                                           uint8_t *out_public_key_length);

bolos_err_t sys_ENDORSEMENT_key1_sign_data(uint8_t *data, uint32_t data_length,
                                           uint8_t *out_signature,
                                           uint32_t *out_signature_length);

bolos_err_t
sys_ENDORSEMENT_key1_sign_without_code_hash(uint8_t *data, size_t dataLength,
                                            uint8_t *signature,
                                            uint32_t *out_signature_length);

bolos_err_t sys_ENDORSEMENT_get_code_hash(uint8_t *out_hash);

bolos_err_t sys_ENDORSEMENT_get_public_key_certificate(ENDORSEMENT_slot_t slot,
                                                       uint8_t *out_buffer,
                                                       uint8_t *out_length);

/**********************
 *      MACROS
 **********************/

#endif // FILE_NAME_H_
