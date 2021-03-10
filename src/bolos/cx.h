#pragma once

#include "cx_hash.h"

#define CX_LAST      (1u << 0u)
#define CX_NO_REINIT (1 << 15)

#define CX_MASK_ECC_VARIANT (7 << 6)
#define CX_NO_CANONICAL     (1 << 6)

#define CX_MASK_RND     (7 << 9)
#define CX_RND_PRNG     (1 << 9)
#define CX_RND_TRNG     (2 << 9)
#define CX_RND_RFC6979  (3 << 9)
#define CX_RND_PROVIDED (4 << 9)

typedef union {
  cx_hash_t header;
  cx_sha256_t sha256;
  cx_sha512_t sha512;
  cx_ripemd160_t ripemd160;
} cx_hash_for_hmac_ctx;
