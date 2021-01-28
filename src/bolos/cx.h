#ifndef _CX_H
#define _CX_H

#include "cx_hash.h"

#define CX_LAST                (1u<<0u)
#define CX_NO_REINIT           (1<<15)

#define CX_MASK_ECC_VARIANT (7 << 6)
#define CX_NO_CANONICAL (1 << 6)

#define CX_MASK_RND                 (7<<9)
#define CX_RND_PRNG                 (1<<9)
#define CX_RND_TRNG                 (2<<9)
#define CX_RND_RFC6979              (3<<9)
#define CX_RND_PROVIDED             (4<<9)

typedef union {
  cx_hash_t header;
  cx_sha256_t sha256;
  cx_sha512_t sha512;
  cx_ripemd160_t ripemd160;
} cx_hash_for_hmac_ctx;

/**
 * HMAC context.
 */
typedef struct {
  cx_hash_for_hmac_ctx hash_ctx;
  uint8_t key[128];
} cx_hmac_ctx;

/* Aliases for compatibility with the old SDK */
typedef cx_hmac_ctx cx_hmac_t;
typedef cx_hmac_ctx cx_hmac_ripemd160_t;
typedef cx_hmac_ctx cx_hmac_sha256_t;
typedef cx_hmac_ctx cx_hmac_sha512_t;

void cx_scc_struct_check_hashmac(const cx_hmac_t *hmac);
int cx_hmac_sha256(const unsigned char *key , unsigned int key_len,
                   const unsigned char *in, unsigned int len,
                   unsigned char *out, unsigned int out_len);
int cx_hmac_sha512(const unsigned char *key , unsigned int key_len,
                   const unsigned char *in, unsigned int len,
                   unsigned char *out, unsigned int out_len);

#endif
