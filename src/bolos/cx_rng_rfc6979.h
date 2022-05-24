#pragma once

#include "cx_hmac.h"
#include <stddef.h>
#include <stdint.h>

// In the supported elliptic curves, the order has at most 521 bits (with NIST
// P-521 a.k.a. secp521r1). The next multiple of 8 is 528 = 66*8.
#define CX_RFC6979_MAX_RLEN 66
typedef struct {
  uint8_t v[CX_SHA512_SIZE + 1];
  uint8_t k[CX_SHA512_SIZE];
  uint8_t q[CX_RFC6979_MAX_RLEN];
  uint32_t q_len;
  uint32_t r_len;
  uint8_t tmp[CX_RFC6979_MAX_RLEN];
  cx_md_t hash_id;
  size_t md_len;
  cx_hmac_t hmac;
} cx_rnd_rfc6979_ctx_t;

void spec_cx_rng_rfc6979_init(cx_rnd_rfc6979_ctx_t *rfc_ctx, cx_md_t hash_id,
                              const uint8_t *x, size_t x_len, const uint8_t *h1,
                              size_t h1_len, const uint8_t *q, size_t q_len);

void spec_cx_rng_rfc6979_next(cx_rnd_rfc6979_ctx_t *rfc_ctx, uint8_t *out,
                              size_t out_len);
