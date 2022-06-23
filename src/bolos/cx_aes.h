#pragma once

#include <stdint.h>

#define CX_AES_BLOCK_SIZE 16

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
