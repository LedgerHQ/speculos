#define _SDK_2_0_
#include <openssl/aes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bolos/cxlib.h"

//-----------------------------------------------------------------------------
static AES_KEY local_aes_key;
static bool local_aes_ready;
static uint32_t local_aes_mode;

//-----------------------------------------------------------------------------
// AES related functions:
//-----------------------------------------------------------------------------
cx_err_t sys_cx_aes_set_key_hw(const cx_aes_key_t *key, uint32_t mode)
{
  switch (mode) {
  case CX_ENCRYPT:
  case CX_SIGN:
  case CX_VERIFY:
    AES_set_encrypt_key(key->keys, (int)key->size * 8, &local_aes_key);
    break;
  case CX_DECRYPT:
    AES_set_decrypt_key(key->keys, (int)key->size * 8, &local_aes_key);
    break;
  default:
    local_aes_ready = false;
    return CX_INVALID_PARAMETER;
  }
  local_aes_mode = mode;
  local_aes_ready = true;

  return CX_OK;
}

cx_err_t sys_cx_aes_block_hw(const unsigned char *inblock,
                             unsigned char *outblock)
{

  if (!local_aes_ready) {
    return CX_INTERNAL_ERROR;
  }
  if ((local_aes_mode & CX_MASK_SIGCRYPT) == CX_DECRYPT) {
    AES_decrypt(inblock, outblock, &local_aes_key);
  } else { // CX_SIGN, CX_VERIFY, CX_ENCRYPT:
    AES_encrypt(inblock, outblock, &local_aes_key);
  }

  return CX_OK;
}

void sys_cx_aes_reset_hw(void)
{
  OPENSSL_cleanse(&local_aes_key, sizeof(local_aes_key));
  local_aes_ready = false;
}
