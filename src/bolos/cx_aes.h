#pragma once

#include <stdint.h>

#define CX_AES_BLOCK_SIZE 16
#define CX_MASK_SIGCRYPT  (3u << 1u)
#define CX_ENCRYPT        (2u << 1u)
#define CX_DECRYPT        (0u << 1u)
#define CX_PAD_NONE       (0u << 3u)
#define CX_MASK_CHAIN     (7u << 6u)
#define CX_CHAIN_ECB      (0u << 6u)
#define CX_CHAIN_CBC      (1u << 6u)

struct cx_aes_key_s {
  unsigned int size;
  unsigned char keys[32];
};

typedef struct cx_aes_key_s cx_aes_key_t;

int sys_cx_aes_init_key(const uint8_t *raw_key, unsigned int key_len,
                        cx_aes_key_t *key);
int sys_cx_aes(const cx_aes_key_t *key, int mode, const unsigned char *in,
               unsigned int len, unsigned char *out, unsigned int out_len);
int sys_cx_aes_iv(const cx_aes_key_t *key, int mode, const unsigned char *IV,
                  unsigned int iv_len, const unsigned char *in,
                  unsigned int len, unsigned char *out, unsigned int out_len);
