#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include "bolos/exception.h"
#include "cx_hash.h"
#include "cx_utils.h"

/** RAM overlaid to NES RAM under ST */
union cx_u {
  cx_sha256_t sha256;
};

static union cx_u G_cx;

static const uint32_t primeSqrt[] = {
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,
  0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
  0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,
  0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
  0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
  0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,
  0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,
  0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
  0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

static const uint32_t hzero_224[] = { 0xc1059ed8, 0x367cd507, 0x3070dd17,
                                      0xf70e5939, 0xffc00b31, 0x68581511,
                                      0x64f98fa7, 0xbefa4fa4 };

static const uint32_t hzero[] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372,
                                  0xa54ff53a, 0x510e527f, 0x9b05688c,
                                  0x1f83d9ab, 0x5be0cd19 };

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */

#define sig256(x, a, b, c) (cx_rotr((x), a) ^ cx_rotr((x), b) ^ cx_shr((x), c))
#define sum256(x, a, b, c) (cx_rotr((x), a) ^ cx_rotr((x), b) ^ cx_rotr((x), c))

#define sigma0(x) sig256(x, 7, 18, 3)
#define sigma1(x) sig256(x, 17, 19, 10)
#define sum0(x)   sum256(x, 2, 13, 22)
#define sum1(x)   sum256(x, 6, 11, 25)

// #define ch(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
// #define maj(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define ch(x, y, z)  (z ^ (x & (y ^ z)))
#define maj(x, y, z) ((x & y) | (z & (x | y)))

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int cx_sha224_init(cx_sha256_t *hash)
{
  memset(hash, 0, sizeof(cx_sha256_t));
  hash->header.algo = CX_SHA224;
  memmove(hash->acc, hzero_224, sizeof(hzero));
  return CX_SHA224;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int cx_sha256_init(cx_sha256_t *hash)
{
  memset(hash, 0, sizeof(cx_sha256_t));
  hash->header.algo = CX_SHA256;
  memmove(hash->acc, hzero, sizeof(hzero));
  return CX_SHA256;
}
/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static void cx_sha256_block(cx_sha256_t *hash)
{
  uint32_t t1, t2;

  uint32_t ACC[8];
  uint32_t *accumulator;
  uint32_t *X;

#define A ACC[0]
#define B ACC[1]
#define C ACC[2]
#define D ACC[3]
#define E ACC[4]
#define F ACC[5]
#define G ACC[6]
#define H ACC[7]

  // init
  X = ((uint32_t *)&hash->block[0]);
  accumulator = (uint32_t *)&hash->acc[0];
  memmove(ACC, accumulator, sizeof(ACC));

#ifdef OS_LITTLE_ENDIAN
  spec_cx_swap_buffer32(X, 16);
#endif

  /*
   * T1 = Sum_1_256(e) + Chg(e,f,g) + K_t_256 + Wt
   * T2 = Sum_0_256(a) + Maj(abc)
   * h = g ;
   * g = f;
   * f = e;
   * e = d + T1;
   * d = c;
   * c = b;
   * b = a;
   * a = T1 + T2;
   */
  for (int j = 0; j < 64; j++) {
    /* for j in 16 to 63, Xj <- (Sigma_1_256( Xj-2) + Xj-7 + Sigma_0_256(Xj-15)
     * + Xj-16 ). */
    if (j >= 16) {
      X[j & 0xF] = (sigma1(X[(j - 2) & 0xF]) + X[(j - 7) & 0xF] +
                    sigma0(X[(j - 15) & 0xF]) + X[(j - 16) & 0xF]);
    }

    t1 = H + sum1(E) + ch(E, F, G) + primeSqrt[j] + X[j & 0xF];
    t2 = sum0(A) + maj(A, B, C);
    /*
    H = G ;
    G = F;
    F = E;
    E = D+t1;
    D = C;
    C = B;
    B = A;
    A = t1+t2;
    */
    memmove(&ACC[1], &ACC[0], sizeof(ACC) - sizeof(uint32_t));
    E += t1;
    A = t1 + t2;
  }

  //(update chaining values) (H1 , H2 , H3 , H4 ) <- (H1 + A, H2 + B, H3 + C, H4
  //+ D...)
  accumulator[0] += A;
  accumulator[1] += B;
  accumulator[2] += C;
  accumulator[3] += D;
  accumulator[4] += E;
  accumulator[5] += F;
  accumulator[6] += G;
  accumulator[7] += H;
}

int spec_cx_sha256_update(cx_sha256_t *ctx, const uint8_t *data, size_t len)
{
  unsigned int r;
  unsigned int blen;

  if (ctx == NULL) {
    return 0;
  }
  if (data == NULL && len != 0) {
    return 0;
  }

  // --- init locals ---
  blen = ctx->blen;
  ctx->blen = 0;

  if (blen >= 64) {
    THROW(INVALID_PARAMETER);
  }

  // --- append input data and process all blocks ---
  if (blen + len >= 64) {
    r = 64 - blen;
    do {
      // if (ctx->header.counter == CX_HASH_MAX_BLOCK_COUNT) {
      //   THROW(INVALID_PARAMETER);
      // }
      memcpy(ctx->block + blen, data, r);
      cx_sha256_block(ctx);

      blen = 0;
      ctx->header.counter++;
      data += r;
      len -= r;
      r = 64;
    } while (len >= 64);
  }

  // --- remind rest data---
  memcpy(ctx->block + blen, data, len);
  blen += len;
  ctx->blen = blen;
  return 1;
}

int spec_cx_sha256_final(cx_sha256_t *ctx, uint8_t *digest)
{
  uint64_t bitlen;

  if (ctx == NULL || digest == NULL) {
    return 0;
  }

  // --- init locals ---
  uint8_t *block = ctx->block;

  block[ctx->blen] = 0x80;
  ctx->blen++;

  bitlen =
      (((uint64_t)ctx->header.counter) * 64UL + (uint64_t)ctx->blen - 1UL) *
      8UL;
  // one more block?
  if (64 - ctx->blen < 8) {
    memset(block + ctx->blen, 0, 64 - ctx->blen);
    cx_sha256_block(ctx);
    ctx->blen = 0;
  }
  // last block!
  memset(block + ctx->blen, 0, 64 - ctx->blen);
#ifdef OS_LITTLE_ENDIAN
  *(uint64_t *)&block[64 - 8] = spec_cx_swap_uint64(bitlen);
#else
  (*(uint64_t *)&block[64 - 8]) = bitlen;
#endif
  cx_sha256_block(ctx);
  // provide result
#ifdef OS_LITTLE_ENDIAN
  spec_cx_swap_buffer32((uint32_t *)ctx->acc, 8);
#endif
  if (ctx->header.algo == CX_SHA256) {
    memcpy(digest, ctx->acc, CX_SHA256_SIZE);
  } else {
    memcpy(digest, ctx->acc, CX_SHA224_SIZE);
  }
  return 1;
}

int cx_sha256_validate_context(const cx_sha256_t *ctx)
{
  return ctx->blen < 64;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int sys_cx_hash_sha256(const unsigned char *in, size_t len, unsigned char *out,
                       size_t out_len __attribute__((unused)))
{
  cx_sha256_init(&G_cx.sha256);
  spec_cx_sha256_update(&G_cx.sha256, in, len);
  spec_cx_sha256_final(&G_cx.sha256, out);
  return CX_SHA256_SIZE;
}
