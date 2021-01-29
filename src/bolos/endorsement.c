#include <string.h>

#include "bolos/exception.h"
#include "cx.h"
#include "cx_ec.h"
#include "emulate.h"

#define cx_ecdsa_init_public_key sys_cx_ecfp_init_public_key

// TODO: all keys are currently hardcoded

static cx_ecfp_private_key_t user_private_key_1 = {
  CX_CURVE_256K1,
  32,
  { 0xe1, 0x5e, 0x01, 0xd4, 0x70, 0x82, 0xf0, 0xea, 0x47, 0x71, 0xc9,
    0x9f, 0xe3, 0x12, 0xf9, 0xd7, 0x00, 0x93, 0xc8, 0x9a, 0xf4, 0x77,
    0x87, 0xfd, 0xf8, 0x2e, 0x03, 0x1f, 0x67, 0x28, 0xb7, 0x10 },
};

static cx_ecfp_private_key_t user_private_key_2 = {
  CX_CURVE_256K1,
  32,
  { 0xe1, 0x5e, 0x01, 0xd4, 0x70, 0x82, 0xf0, 0xea, 0x47, 0x71, 0xc9,
    0x9f, 0xe3, 0x12, 0xf9, 0xd7, 0x00, 0x93, 0xc8, 0x9a, 0xf4, 0x77,
    0x87, 0xfd, 0xf8, 0x2e, 0x03, 0x1f, 0x67, 0x28, 0xb7, 0x10 },
};

// user_private_key_1 signed by test owner private key
// "138fb9b91da745f12977a2b46f0bce2f0418b50fcb76631baf0f08ceefdb5d57"
static uint8_t user_certificate_1[] = {
  0x30, 0x45, 0x02, 0x21, 0x00, 0xbf, 0x23, 0x7e, 0x5b, 0x40, 0x06, 0x14,
  0x17, 0xf6, 0x62, 0xa6, 0xd0, 0x8a, 0x4b, 0xde, 0x1f, 0xe3, 0x34, 0x3b,
  0xd8, 0x70, 0x8c, 0xed, 0x04, 0x6c, 0x84, 0x17, 0x49, 0x5a, 0xd3, 0x6c,
  0xcf, 0x02, 0x20, 0x3d, 0x39, 0xa5, 0x32, 0xee, 0xca, 0xdf, 0xf6, 0xdf,
  0x20, 0x53, 0xe4, 0xab, 0x98, 0x96, 0xaa, 0x00, 0xf3, 0xbe, 0xf1, 0x5c,
  0x4b, 0xd1, 0x1c, 0x53, 0x66, 0x1e, 0x54, 0xfe, 0x5e, 0x2f, 0xf4
};
static const uint8_t user_certificate_1_length = sizeof(user_certificate_1);

// user_private_key_2 signed by test owner private key
// "138fb9b91da745f12977a2b46f0bce2f0418b50fcb76631baf0f08ceefdb5d57"
static uint8_t user_certificate_2[] = {
  0x30, 0x45, 0x02, 0x21, 0x00, 0xbf, 0x23, 0x7e, 0x5b, 0x40, 0x06, 0x14,
  0x17, 0xf6, 0x62, 0xa6, 0xd0, 0x8a, 0x4b, 0xde, 0x1f, 0xe3, 0x34, 0x3b,
  0xd8, 0x70, 0x8c, 0xed, 0x04, 0x6c, 0x84, 0x17, 0x49, 0x5a, 0xd3, 0x6c,
  0xcf, 0x02, 0x20, 0x3d, 0x39, 0xa5, 0x32, 0xee, 0xca, 0xdf, 0xf6, 0xdf,
  0x20, 0x53, 0xe4, 0xab, 0x98, 0x96, 0xaa, 0x00, 0xf3, 0xbe, 0xf1, 0x5c,
  0x4b, 0xd1, 0x1c, 0x53, 0x66, 0x1e, 0x54, 0xfe, 0x5e, 0x2f, 0xf4
};
static uint8_t user_certificate_2_length;

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
    privateKey = &user_private_key_1;
    break;
  case 2:
    privateKey = &user_private_key_2;
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

unsigned int
sys_os_endorsement_get_public_key_certificate(unsigned char index,
                                              unsigned char *buffer)
{
  unsigned char *certificate;
  unsigned char length;

  switch (index) {
  case 1:
    length = user_certificate_1_length;
    certificate = user_certificate_1;
    break;
  case 2:
    length = user_certificate_2_length;
    certificate = user_certificate_2;
    break;
  default:
    THROW(EXCEPTION);
    break;
  }

  if (length == 0) {
    THROW(EXCEPTION);
  }

  memcpy(buffer, certificate, length);

  return length;
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
  sys_cx_ecdsa_sign(&user_private_key_1, CX_LAST | CX_RND_TRNG, CX_SHA256, hash,
                    sizeof(hash),          // size of SHA256 hash
                    signature, 6 + 33 * 2, /*3TL+2V*/
                    NULL);
  return signature[1] + 2;
}
