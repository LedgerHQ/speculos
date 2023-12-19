#define _SDK_2_0_
#include <openssl/aes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bolos/cx_utils.h"
#include "bolos/cxlib.h"

//-----------------------------------------------------------------------------
static AES_KEY local_aes_key;
static bool local_aes_ready;
static uint32_t local_aes_op;
static uint32_t local_aes_chain_mode;
bool hdw_cbc = false;
bool set_aes_iv;
static uint8_t aes_current_block[AES_BLOCK_SIZE] = { 0 };

//-----------------------------------------------------------------------------
// AES related functions:
//-----------------------------------------------------------------------------
cx_err_t sys_cx_aes_set_key_hw(const cx_aes_key_t *key, uint32_t mode)
{
  switch (mode & CX_MASK_SIGCRYPT) {
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
  local_aes_op = mode & CX_MASK_SIGCRYPT;
  local_aes_chain_mode = mode & CX_MASK_CHAIN;
  set_aes_iv = hdw_cbc;
  local_aes_ready = true;

  return CX_OK;
}

// This function aims at reproducing a CBC mode performed in hardware
static cx_err_t cx_aes_block_hw_cbc(const unsigned char *inblock,
                                    unsigned char *outblock)
{
  if (local_aes_op == CX_DECRYPT) {
    uint8_t inblock_prev_value[AES_BLOCK_SIZE] = { 0 };
    // If the same buffer is used for inblock and outblock
    // save inblock value for next block encryption
    memcpy(inblock_prev_value, inblock, AES_BLOCK_SIZE);

    AES_decrypt(inblock, outblock, &local_aes_key);
    // XOR the decryption result with aes_current_block
    cx_memxor(outblock, aes_current_block, AES_BLOCK_SIZE);

    // Store the input block for next block decryption
    memcpy(aes_current_block, inblock_prev_value, AES_BLOCK_SIZE);
  } else { // CX_SIGN, CX_VERIFY, CX_ENCRYPT:

    // Before the encryption, XOR the input block with the
    // previous value of aes_current_block which is either
    // the IV or the previous ciphertext block
    cx_memxor(aes_current_block, inblock, AES_BLOCK_SIZE);

    AES_encrypt(aes_current_block, outblock, &local_aes_key);

    // Store the ciphertext block for next block encryption
    memcpy(aes_current_block, outblock, AES_BLOCK_SIZE);
  }

  return CX_OK;
}

cx_err_t sys_cx_aes_block_hw(const unsigned char *inblock,
                             unsigned char *outblock)
{

  if (!local_aes_ready) {
    return CX_INTERNAL_ERROR;
  }
  // Stores the IV
  if (set_aes_iv && (local_aes_chain_mode == CX_CHAIN_CBC)) {
    memcpy(aes_current_block, inblock, AES_BLOCK_SIZE);
    set_aes_iv = false;
    return CX_OK;
  }
  if (hdw_cbc && (local_aes_chain_mode == CX_CHAIN_CBC)) {
    return cx_aes_block_hw_cbc(inblock, outblock);
  }
  if (local_aes_op == CX_DECRYPT) {
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
