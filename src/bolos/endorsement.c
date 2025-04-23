/**
 * @file
 * @brief
 */

/*********************
 *      INCLUDES
 *********************/

#include "endorsement.h"
#include "bolos/exception.h"
#include "cx.h"
#include "emulate.h"
#include "environment.h"
#include "os_types.h"
#include <string.h>

/*********************
 *      DEFINES
 *********************/

#define cx_ecdsa_init_public_key sys_cx_ecfp_init_public_key

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

static char CODE_HASH[] = "12345678abcdef0000fedcba8765432";

/**********************
 *  STATIC FUNCTIONS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

// Pre API_LEVEL_23 syscalls

unsigned int sys_os_endorsement_get_code_hash(uint8_t *buffer)
{
  memcpy(buffer, CODE_HASH, sizeof(CODE_HASH));
  return 32;
}

unsigned long sys_os_endorsement_get_public_key(uint8_t index, uint8_t *buffer)
{
  cx_ecfp_private_key_t *privateKey;
  cx_ecfp_public_key_t publicKey;

  switch (index) {
  case 1:
    privateKey = env_get_user_private_key(1);
    break;
  case 2:
    privateKey = env_get_user_private_key(2);
    break;
  default:
    THROW(EXCEPTION);
    break;
  }

  cx_ecdsa_init_public_key(CX_CURVE_256K1, NULL, 0, &publicKey);

  sys_cx_ecfp_generate_pair(CX_CURVE_256K1, &publicKey, privateKey, 1);

  memcpy(buffer, publicKey.W, 65);

  return 65;
}

unsigned int sys_os_endorsement_get_public_key_new(uint8_t index,
                                                   uint8_t *buffer,
                                                   uint8_t *length)
{
  *length = sys_os_endorsement_get_public_key(index, buffer);

  return 0;
}

unsigned int
sys_os_endorsement_get_public_key_certificate(unsigned char index,
                                              unsigned char *buffer)
{
  env_user_certificate_t *certificate;

  switch (index) {
  case 1:
    certificate = env_get_user_certificate(1);
    break;
  case 2:
    certificate = env_get_user_certificate(2);
    break;
  default:
    THROW(EXCEPTION);
    break;
  }

  if (certificate->length == 0) {
    THROW(EXCEPTION);
  }

  memcpy(buffer, certificate->buffer, certificate->length);

  return certificate->length;
}

unsigned int sys_os_endorsement_get_public_key_certificate_new(
    unsigned char index, unsigned char *buffer, unsigned char *length)
{
  *length = sys_os_endorsement_get_public_key_certificate(index, buffer);

  return 0;
}

unsigned long sys_os_endorsement_key1_sign_data(uint8_t *data,
                                                size_t dataLength,
                                                uint8_t *signature)
{
  uint8_t hash[32];
  cx_sha256_t sha256;

  sys_os_endorsement_get_code_hash(hash);
  sys_cx_sha256_init(&sha256);
  sys_cx_hash((cx_hash_t *)&sha256, 0, data, dataLength, NULL, 0);
  sys_cx_hash((cx_hash_t *)&sha256, CX_LAST, hash, sizeof(hash), hash, 32);
  /* XXX: CX_RND_TRNG is set but actually ignored by speculos'
   *      sys_cx_ecdsa_sign implementation */
  sys_cx_ecdsa_sign(env_get_user_private_key(1), CX_LAST | CX_RND_TRNG,
                    CX_SHA256, hash,
                    sizeof(hash),          // size of SHA256 hash
                    signature, 6 + 33 * 2, /*3TL+2V*/
                    NULL);
  return signature[1] + 2;
}

unsigned long sys_os_endorsement_key1_sign_without_code_hash(uint8_t *data,
                                                             size_t dataLength,
                                                             uint8_t *signature)
{
  uint8_t hash[32];
  cx_sha256_t sha256;

  sys_cx_sha256_init(&sha256);
  sys_cx_hash((cx_hash_t *)&sha256, CX_LAST, data, dataLength, hash, 32);
  /* XXX: CX_RND_TRNG is set but actually ignored by speculos'
   *      sys_cx_ecdsa_sign implementation */
  sys_cx_ecdsa_sign(env_get_user_private_key(1), CX_LAST | CX_RND_TRNG,
                    CX_SHA256, hash,
                    sizeof(hash),          // size of SHA256 hash
                    signature, 6 + 33 * 2, /*3TL+2V*/
                    NULL);
  return signature[1] + 2;
}

// API_LEVEL_23 and above

bolos_err_t sys_ENDORSEMENT_get_public_key(ENDORSEMENT_slot_t slot,
                                           uint8_t *out_public_key,
                                           uint8_t *out_public_key_length)
{
  cx_ecfp_private_key_t *privateKey;
  cx_ecfp_public_key_t publicKey;

  switch (slot) {
  case ENDORSEMENT_SLOT_1:
    privateKey = env_get_user_private_key(ENDORSEMENT_SLOT_1);
    break;
  case ENDORSEMENT_SLOT_2:
    privateKey = env_get_user_private_key(ENDORSEMENT_SLOT_2);
    break;
  default:
    // Invalid slot
    return 0x4209;
  }

  if (privateKey->d_len == 0) {
    // No private key set
    return 0x4103;
  }

  cx_ecdsa_init_public_key(CX_CURVE_256K1, NULL, 0, &publicKey);
  sys_cx_ecfp_generate_pair(CX_CURVE_256K1, &publicKey, privateKey, 1);
  memcpy(out_public_key, publicKey.W, 65);
  *out_public_key_length = 65;
  return 0;
}

bolos_err_t sys_ENDORSEMENT_key1_sign_data(uint8_t *data, uint32_t data_length,
                                           uint8_t *out_signature,
                                           uint32_t *out_signature_length)
{
  cx_ecfp_private_key_t *private_key =
      env_get_user_private_key(ENDORSEMENT_SLOT_1);

  if (private_key->d_len == 0) {
    // No private key set in slot 1
    return 0x4106;
  }

  // Perform signature
  *out_signature_length =
      sys_os_endorsement_key1_sign_data(data, data_length, out_signature);
  return 0;
}

bolos_err_t
sys_ENDORSEMENT_key1_sign_without_code_hash(uint8_t *data, uint32_t data_length,
                                            uint8_t *out_signature,
                                            uint32_t *out_signature_length)
{
  cx_ecfp_private_key_t *private_key =
      env_get_user_private_key(ENDORSEMENT_SLOT_1);

  if (private_key->d_len == 0) {
    // No private key set in slot 1
    return 0x4117;
  }

  // Perform signature
  *out_signature_length = sys_os_endorsement_key1_sign_without_code_hash(
      data, data_length, out_signature);
  return 0;
}

bolos_err_t sys_ENDORSEMENT_get_code_hash(uint8_t *out_hash)
{
  if (out_hash) {
    memcpy(out_hash, CODE_HASH, sizeof(CODE_HASH));
  }
  return 0;
}

bolos_err_t sys_ENDORSEMENT_get_public_key_certificate(ENDORSEMENT_slot_t slot,
                                                       uint8_t *out_buffer,
                                                       uint8_t *out_length)
{
  env_user_certificate_t *certificate;

  switch (slot) {
  case ENDORSEMENT_SLOT_1:
    certificate = env_get_user_certificate(ENDORSEMENT_SLOT_1);
    break;
  case ENDORSEMENT_SLOT_2:
    certificate = env_get_user_certificate(ENDORSEMENT_SLOT_2);
    break;
  default:
    // Invalid slot
    return 0x420A;
  }

  if (certificate->length == 0) {
    // No certificate loaded
    return 0x4104;
  }

  if (out_buffer) {
    memcpy(out_buffer, certificate->buffer, certificate->length);
  }

  if (out_length) {
    *out_length = certificate->length;
  }

  return 0;
}
