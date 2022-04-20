#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include "bolos/exception.h"
#include "cx_hash.h"
#include "cx_utils.h"

/* clang-format off */
// Message Selection L: left, R: right
static const unsigned char zL[] = {
  /*zL[16..31]*/   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 
  /*zL[16..31]*/   7,  4, 13,  1, 10,  6, 15,  3, 12,  0,  9,  5,  2, 14, 11,  8, 
  /*zL[32..47]*/   3, 10, 14,  4,  9, 15,  8,  1,  2,  7,  0,  6, 13, 11,  5, 12, 
  /*zL[48..63]*/   1,  9, 11, 10,  0,  8, 12,  4, 13,  3,  7, 15, 14,  5,  6,  2, 
  /*zL[64..79]*/   4,  0,  5,  9,  7, 12,  2, 10, 14,  1,  3,  8, 11,  6, 15, 13, 
};

static const unsigned char zR[] = {
  /*zR[ 0..15]*/   5, 14,  7,  0,  9,  2, 11,  4, 13,  6, 15,  8,  1, 10,  3, 12,
  /*zR[16..31]*/   6, 11,  3,  7,  0, 13,  5, 10, 14, 15,  8, 12,  4,  9,  1,  2,
  /*zR[32..47]*/  15,  5,  1,  3,  7, 14,  6,  9, 11,  8, 12,  2, 10,  0,  4, 13,
  /*zR[48..63]*/   8,  6,  4,  1,  3, 11, 15,  0,  5, 12,  2, 13,  9,  7, 10, 14,
  /*zR[64..79]*/  12, 15, 10,  4,  1,  5,  8,  7,  6,  2, 13, 14,  0,  3,  9, 11,
};

//  Rotate left
static const unsigned char sL[] = {
  /*sL[ 0..15],*/  11, 14, 15, 12,  5,  8,  7,  9, 11, 13, 14, 15,  6,  7,  9,  8, 
  /*sL[16..31]*/    7, 6,   8, 13, 11,  9,  7, 15,  7, 12, 15,  9, 11,  7, 13, 12, 
  /*sL[32..47]*/   11, 13,  6,  7, 14,  9, 13, 15, 14,  8, 13,  6,  5, 12,  7,  5, 
  /*sL[48..63]*/   11, 12, 14, 15, 14, 15,  9,  8,  9, 14,  5,  6,  8,  6,  5, 12, 
  /*sL[64..79]*/    9, 15,  5, 11,  6,  8, 13, 12,  5, 12, 13, 14, 11,  8,  5,  6, 
};

static const unsigned char sR[] = {
  /*sR[ 0..15]*/   8,  9,  9, 11, 13, 15, 15,  5,  7,  7,  8, 11, 14, 14, 12,  6, 
  /*sR[16..31]*/   9, 13, 15,  7, 12,  8,  9, 11,  7,  7, 12,  7,  6, 15, 13, 11, 
  /*sR[32..47]*/   9,  7, 15, 11,  8,  6,  6, 14, 12, 13,  5, 14, 13, 13,  7,  5, 
  /*sR[48..63]*/  15,  5,  8, 11, 14, 14,  6, 14,  6,  9, 12,  9, 12,  5, 15,  8, 
  /*sR[64..79]*/   8,  5, 12,  9, 12,  5, 14,  6,  8, 13,  6,  5, 15, 13, 11, 11, 
};
/* clang-format on */

static const uint32_t hzero[] = { 0x67452301UL, 0xEFCDAB89UL, 0x98BADCFEUL,
                                  0x10325476UL, 0xC3D2E1F0UL };

// additive constante
#define kL_0_15  0x00000000UL
#define kL_16_31 0x5A827999UL
#define kL_32_47 0x6ED9EBA1UL
#define kL_48_63 0x8F1BBCDCUL
#define kL_64_78 0xA953FD4EUL

#define kR_0_15  0x50A28BE6UL
#define kR_16_31 0x5C4DD124UL
#define kR_32_47 0x6D703EF3UL
#define kR_48_63 0x7A6D76E9UL
#define kR_64_78 0x00000000UL

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
#define x BCD[0]
#define y BCD[1]
#define z BCD[2]
static unsigned long int f1(uint32_t *BCD)
{
  return ((x) ^ (y) ^ (z));
}
static unsigned long int f2(uint32_t *BCD)
{
  return (((x) & (y)) | ((~x) & (z)));
}
static unsigned long int f3(uint32_t *BCD)
{
  return (((x) | (~(y))) ^ (z));
}
static unsigned long int f4(uint32_t *BCD)
{
  return (((x) & (z)) | ((y) & (~(z))));
}
static unsigned long int f5(uint32_t *BCD)
{
  return ((x) ^ ((y) | (~(z))));
}
#undef x
#undef y
#undef z

#define rotL(x, n) cx_rotl(x, n)

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int cx_ripemd160_init(cx_ripemd160_t *hash)
{
  memset(hash, 0, sizeof(cx_ripemd160_t));
  hash->header.algo = CX_RIPEMD160;
  memmove(hash->acc, hzero, sizeof(hzero));
  return CX_RIPEMD160;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */

static void cx_ripemd160_block(cx_ripemd160_t *hash)
{

  unsigned char j;
  uint32_t Tl;
  uint32_t Tr;

  uint32_t ACCl[5];
  uint32_t ACCr[5];
  uint32_t *accumulator;
  uint32_t *X;

#define Al ACCl[0]
#define Bl ACCl[1]
#define Cl ACCl[2]
#define Dl ACCl[3]
#define El ACCl[4]

#define Ar ACCr[0]
#define Br ACCr[1]
#define Cr ACCr[2]
#define Dr ACCr[3]
#define Er ACCr[4]

  // init
  X = (uint32_t *)&hash->block[0];
  accumulator = (uint32_t *)&hash->acc[0];
  memmove(ACCl, accumulator, sizeof(ACCl));
  memmove(ACCr, accumulator, sizeof(ACCr));

#ifdef OS_BIG_ENDIAN
  spec_cx_swap_buffer32(X, 16);
#endif

  for (j = 0; j < 80; j++) {
    Tl = Al + X[zL[j]];
    Tr = Ar + X[zR[j]];
    switch (j >> 4) {
    case 0:
      Tl += f1(&Bl) + kL_0_15;
      Tr += f5(&Br) + kR_0_15;
      break;
    case 1:
      Tl += f2(&Bl) + kL_16_31;
      Tr += f4(&Br) + kR_16_31;
      break;
    case 2:
      Tl += f3(&Bl) + kL_32_47;
      Tr += f3(&Br) + kR_32_47;
      break;
    case 3:
      Tl += f4(&Bl) + kL_48_63;
      Tr += f2(&Br) + kR_48_63;
      break;
    case 4:
      Tl += f5(&Bl) + kL_64_78;
      Tr += f1(&Br) + kR_64_78;
      break;
    default:
      break;
    }

    Tl = rotL(Tl, sL[j]);
    Tl += El;
    Al = El;
    El = Dl;
    Dl = rotL(Cl, 10);
    Cl = Bl;
    Bl = Tl;

    Tr = rotL(Tr, sR[j]);
    Tr += Er;
    Ar = Er;
    Er = Dr;
    Dr = rotL(Cr, 10);
    Cr = Br;
    Br = Tr;
  }

  //(update chaining values)
  Tl = accumulator[1] + Cl + Dr;
  accumulator[1] = accumulator[2] + Dl + Er;
  accumulator[2] = accumulator[3] + El + Ar;
  accumulator[3] = accumulator[4] + Al + Br;
  accumulator[4] = accumulator[0] + Bl + Cr;
  accumulator[0] = Tl;
}

int spec_cx_ripemd160_update(cx_ripemd160_t *ctx, const uint8_t *data,
                             size_t len)
{
  if (ctx == NULL) {
    return 0;
  }
  if (data == NULL && len != 0) {
    return 0;
  }

  unsigned int r;
  unsigned char block_size;
  unsigned char *block;
  unsigned int blen;

  block_size = 64;
  block = ctx->block;
  blen = ctx->blen;
  ctx->blen = 0;

  if (blen >= block_size) {
    return 0;
  }

  // --- append input data and process all blocks ---
  if ((blen + len) >= block_size) {
    r = block_size - blen;
    do {
      if (ctx->header.counter == CX_HASH_MAX_BLOCK_COUNT) {
        THROW(INVALID_PARAMETER);
      }
      memcpy(block + blen, data, r);
      cx_ripemd160_block(ctx);
      blen = 0;
      ctx->header.counter++;
      data += r;
      len -= r;
      r = block_size;
    } while (len >= block_size);
  }

  // --- remind rest data---
  memcpy(block + blen, data, len);
  blen += len;
  ctx->blen = blen;
  return 1;
}

int spec_cx_ripemd160_final(cx_ripemd160_t *ctx, uint8_t *digest)
{
  unsigned char *block;
  unsigned int blen;
  unsigned long int bitlen;

  if (ctx == NULL || digest == NULL) {
    return 0;
  }
  block = ctx->block;
  blen = ctx->blen;

  // one more block?
  block[blen] = 0x80;
  blen++;
  bitlen = (((unsigned long int)ctx->header.counter) * 64UL +
            (unsigned long int)blen - 1UL) *
           8UL;

  // one more block?
  if (64 - blen < 8) {
    memset(block + blen, 0, (64 - blen));
    cx_ripemd160_block(ctx);
    blen = 0;
  }
  // last block!
  memset(block + blen, 0, (64 - blen));
#ifdef OS_BIG_ENDIAN
  (*(unsigned long int *)(&block[64 - 8])) = spec_cx_swap_uint32(bitlen);
#else
  (*(uint64_t *)(&block[64 - 8])) = bitlen;
#endif
  cx_ripemd160_block(ctx);
  // provide result
#ifdef OS_BIG_ENDIAN
  spec_cx_swap_buffer32((unsigned long int *)acc, 5);
#endif
  memcpy(digest, ctx->acc, CX_RIPEMD160_SIZE);
  return 1;
}

int cx_ripemd160_validate_context(const cx_ripemd160_t *ctx)
{
  return ctx->blen < 64;
}
