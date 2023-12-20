#include <string.h>

#include "bolos/exception.h"
#include "cx.h"
#include "emulate.h"
#include "environment.h"

#define cx_ecdsa_init_public_key sys_cx_ecfp_init_public_key

unsigned int sys_os_endorsement_get_code_hash(uint8_t *buffer)
{
  memcpy(buffer, "12345678abcdef0000fedcba87654321", 32);
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
