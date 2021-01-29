#include <assert.h>
#include <err.h>
#include <string.h>

#include <openssl/aes.h>
#include <openssl/crypto.h>

#include "bolos/exception.h"
#include "cx.h"
#include "cx_aes.h"

static void aes_encrypt_block(const cx_aes_key_t *key, const uint8_t *inblock,
                              uint8_t *outblock)
{
  AES_KEY aes_key;

  AES_set_encrypt_key(key->keys, (int)key->size * 8, &aes_key);
  AES_encrypt(inblock, outblock, &aes_key);
  OPENSSL_cleanse(&aes_key, sizeof(aes_key));
}

static void aes_decrypt_block(const cx_aes_key_t *key, const uint8_t *inblock,
                              uint8_t *outblock)
{
  AES_KEY aes_key;

  AES_set_decrypt_key(key->keys, (int)key->size * 8, &aes_key);
  AES_decrypt(inblock, outblock, &aes_key);
  OPENSSL_cleanse(&aes_key, sizeof(aes_key));
}

static int aes_cbc_encrypt(const cx_aes_key_t *key, const uint8_t *input,
                           uint8_t *output, uint8_t *iv, size_t len)
{
  assert(len % CX_AES_BLOCK_SIZE == 0);
  size_t out_len = len;
  uint8_t block[CX_AES_BLOCK_SIZE];

  while (len > 0) {
    for (int i = 0; i < CX_AES_BLOCK_SIZE; i++) {
      block[i] = input[i] ^ iv[i];
    }
    aes_encrypt_block(key, block, output);
    memcpy(iv, output, CX_AES_BLOCK_SIZE);

    input += CX_AES_BLOCK_SIZE;
    output += CX_AES_BLOCK_SIZE;
    len -= CX_AES_BLOCK_SIZE;
  }
  return out_len;
}

static int aes_cbc_decrypt(const cx_aes_key_t *key, const uint8_t *input,
                           uint8_t *output, uint8_t *iv, size_t len)
{
  assert(len % CX_AES_BLOCK_SIZE == 0);
  size_t out_len = len;

  uint8_t block[CX_AES_BLOCK_SIZE];

  while (len > 0) {
    memcpy(block, input, CX_AES_BLOCK_SIZE);
    aes_decrypt_block(key, input, output);
    for (int i = 0; i < CX_AES_BLOCK_SIZE; i++) {
      output[i] ^= iv[i];
    }
    memcpy(iv, block, CX_AES_BLOCK_SIZE);

    input += CX_AES_BLOCK_SIZE;
    output += CX_AES_BLOCK_SIZE;
    len -= CX_AES_BLOCK_SIZE;
  }
  return out_len;
}

int sys_cx_aes_init_key(const unsigned char *raw_key, unsigned int key_len,
                        cx_aes_key_t *key)
{
  memset(key, 0, sizeof(cx_aes_key_t));
  if (key_len != 16 && key_len != 24 && key_len != 32) {
    THROW(INVALID_PARAMETER);
  }
  key->size = key_len;
  memcpy(key->keys, raw_key, key_len);
  return (int)key_len;
}

int sys_cx_aes(const cx_aes_key_t *key, int mode, const unsigned char *in,
               unsigned int len, unsigned char *out, unsigned int out_len)
{
  return sys_cx_aes_iv(key, mode, NULL, 0, in, len, out, out_len);
}

int sys_cx_aes_iv(const cx_aes_key_t *key, int mode, const unsigned char *IV,
                  unsigned int iv_len, const unsigned char *in,
                  unsigned int len, unsigned char *out, unsigned int out_len)
{
  size_t len_out;
  uint32_t umode = mode;
  uint8_t running_iv[CX_AES_BLOCK_SIZE];

  if (len % CX_AES_BLOCK_SIZE != 0 || out_len < len) {
    err(1,
        "cx_aes: unsupported size %u (the vault app is the only app currently "
        "supported), please open an issue",
        len);
  }

  if (IV && iv_len != CX_AES_BLOCK_SIZE) {
    err(1,
        "cx_aes: unsupported iv_len %u (the vault app is the only app "
        "currently "
        "supported), please open an issue",
        iv_len);
  }

  if (!(umode & (CX_LAST | CX_PAD_NONE))) {
    err(1, "cx_aes: unsupported mode (the vault app is the only app currently "
           "supported), please open an issue");
  }

  umode &= ~(CX_LAST | CX_PAD_NONE);
  if (umode & ~(CX_ENCRYPT | CX_DECRYPT | CX_CHAIN_CBC | CX_CHAIN_ECB)) {
    err(1, "cx_aes: unsupported mode (the vault app is the only app currently "
           "supported), please open an issue");
  }

  if ((umode & CX_MASK_CHAIN) == CX_CHAIN_CBC) {
    if (IV == NULL) {
      memset(running_iv, 0, CX_AES_BLOCK_SIZE);
    } else {
      memcpy(running_iv, IV, CX_AES_BLOCK_SIZE);
    }

    if ((umode & CX_MASK_SIGCRYPT) == CX_ENCRYPT) {
      len_out = aes_cbc_encrypt(key, in, out, running_iv, len);
    } else if ((umode & CX_MASK_SIGCRYPT) == CX_DECRYPT) {
      len_out = aes_cbc_decrypt(key, in, out, running_iv, len);
    } else {
      err(1, "cx_aes: unsupported mode (the vault app is the only app "
             "currently supported), please open an issue");
    }
  } else if ((umode & CX_MASK_CHAIN) == CX_CHAIN_ECB) {
    len_out = len;

    if ((umode & CX_MASK_SIGCRYPT) == CX_ENCRYPT) {
      while (len > 0) {
        aes_encrypt_block(key, in, out);

        len -= CX_AES_BLOCK_SIZE;
        in += CX_AES_BLOCK_SIZE;
        out += CX_AES_BLOCK_SIZE;
      }
    } else if ((umode & CX_MASK_SIGCRYPT) == CX_DECRYPT) {
      while (len > 0) {
        aes_decrypt_block(key, in, out);

        len -= CX_AES_BLOCK_SIZE;
        in += CX_AES_BLOCK_SIZE;
        out += CX_AES_BLOCK_SIZE;
      }
    } else {
      err(1, "cx_aes: unsupported mode (the vault app is the only app "
             "currently supported), please open an issue");
    }
  } else {
    err(1, "acx_aes: unsupported mode (the vault app is the only app currently "
           "supported), please open an issue");
  }
  return (int)len_out;
}
