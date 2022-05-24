/* @BANNER@ */

#ifndef BOLOS_OS_UPGRADER

#include "bolos/exception.h"
#include "cx.h"
#include "cx_utils.h"
#include <string.h>

#ifdef OS_BIG_ENDIAN
#error "OS_BIG_ENDIAN not supported. Plz abort your project"
#endif

#ifndef NATIVE_64BITS // NO 64BITS natively supported by the compiler
#error sha3 require 64 bits support at compiler level (NATIVE_64BITS option)
#endif

/* ----------------------------------------------------------------------- */
/* --    DEBUG -- DEBUG -- DEBUG -- DEBUG -- DEBUG -- DEBUG -- DEBUG    -- */
/* ----------------------------------------------------------------------- */
#if 0
#undef PRINTF
#define PRINTF noprintf

int noprintf(const char *format, ...) {
  return 0;
}


void printhex64(char* head, const uint64_t* hex, int size) {
  int i;
  PRINTF("%s: ",head);
  for (i =0; i<size; i++) {
    PRINTF("%.016llx.", hex[i]);
  }
  PRINTF("\n");
}

void printstate(char* head, const uint64_t* state) {
  PRINTF("%s state\n",head);
  printhex64(" state line y=0", state+ 0 ,5);
  printhex64(" state line y=1", state+ 5 ,5);
  printhex64(" state line y=2", state+10 ,5);
  printhex64(" state line y=3", state+15 ,5);
  printhex64(" state line y=4", state+20 ,5);
}
#else
#undef PRINTF
#define PRINTF(...)
#define printhex64(...)
#define printstate(...)
#endif

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */

// Assume state is a uint64_t array
#define S64(x, y)    state[x + 5 * y]
#define ROTL64(x, n) cx_rotl64(x, n)

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static void cx_sha3_theta(uint64_t state[])
{
  uint64_t C[5];
  uint64_t D[5];
  int i, j;

  PRINTF("\nEntering Theta\n");
  printstate("  Input", state);

  for (i = 0; i < 5; i++) {
    C[i] = S64(i, 0) ^ S64(i, 1) ^ S64(i, 2) ^ S64(i, 3) ^ S64(i, 4);
  }
  printstate("  After C ", state);
  for (i = 0; i < 5; i++) {
    D[i] = C[(i + 4) % 5] ^ ROTL64(C[(i + 1) % 5], 1);
    for (j = 0; j < 5; j++) {
      S64(i, j) ^= D[i];
    }
  }
  printstate("  After D ", state);
  printstate("  After Merge ", state);
  PRINTF("\nLeaving Theta\n");
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static const int C_cx_pi_table[] = { 10, 7,  11, 17, 18, 3,  5,  16,
                                     8,  21, 24, 4,  15, 23, 19, 13,
                                     12, 2,  20, 14, 22, 9,  6,  1 };

static const int C_cx_rho_table[] = { 1,  3,  6,  10, 15, 21, 28, 36,
                                      45, 55, 2,  14, 27, 41, 56, 8,
                                      25, 43, 62, 18, 39, 61, 20, 44 };

static void cx_sha3_rho_pi(uint64_t state[])
{
  int i, j;
  uint64_t A;
  uint64_t tmp;

  PRINTF("\nEntering Rho Pi\n");
  printstate("  Input", state);

  A = state[1];
  for (i = 0; i < 24; i++) {
    j = C_cx_pi_table[i];
    tmp = state[j];
    state[j] = ROTL64(A, C_cx_rho_table[i]);
    A = tmp;
  }
  printstate("  Output", state);
  PRINTF("\nLeaving Rho Pi\n");
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static void cx_sha3_chi(uint64_t state[])
{
  uint64_t C[5];

  PRINTF("\nEntering Chi\n");
  printstate("  Input", state);

  int i, j;
  for (j = 0; j < 5; j++) {
    for (i = 0; i < 5; i++) {
      C[i] = S64(i, j);
    }
    for (i = 0; i < 5; i++) {
      S64(i, j) ^= (~C[(i + 1) % 5]) & C[(i + 2) % 5];
    }
    PRINTF("  Output %d", 5 * j);
    printstate("", state);
  }

  printstate("  Output", state);
  PRINTF("\nLeaving Chi\n");
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
static const uint64_t C_cx_iota_RC[24] = {
  _64BITS(0x00000000, 0x00000001), _64BITS(0x00000000, 0x00008082),
  _64BITS(0x80000000, 0x0000808A), _64BITS(0x80000000, 0x80008000),
  _64BITS(0x00000000, 0x0000808B), _64BITS(0x00000000, 0x80000001),
  _64BITS(0x80000000, 0x80008081), _64BITS(0x80000000, 0x00008009),
  _64BITS(0x00000000, 0x0000008A), _64BITS(0x00000000, 0x00000088),
  _64BITS(0x00000000, 0x80008009), _64BITS(0x00000000, 0x8000000A),
  _64BITS(0x00000000, 0x8000808B), _64BITS(0x80000000, 0x0000008B),
  _64BITS(0x80000000, 0x00008089), _64BITS(0x80000000, 0x00008003),
  _64BITS(0x80000000, 0x00008002), _64BITS(0x80000000, 0x00000080),
  _64BITS(0x00000000, 0x0000800A), _64BITS(0x80000000, 0x8000000A),
  _64BITS(0x80000000, 0x80008081), _64BITS(0x80000000, 0x00008080),
  _64BITS(0x00000000, 0x80000001), _64BITS(0x80000000, 0x80008008)
};

static void cx_sha3_iota(uint64_t state[], int round)
{
  PRINTF("\nEntering Iota\n");
  printstate("  Input", state);

  S64(0, 0) ^= C_cx_iota_RC[round];

  printstate("  Output", state);
  PRINTF("\nLeaving Iota\n");
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
void check_hash_out_size(int size)
{
  switch (size) {
  case 128:
  case 224:
  case 256:
  case 384:
  case 512:
    return;
  default:
    break;
  }
  THROW(INVALID_PARAMETER);
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int cx_sha3_init(cx_sha3_t *hash, unsigned int size)
{
  check_hash_out_size(size);
  memset(hash, 0, sizeof(cx_sha3_t));
  hash->header.algo = CX_SHA3;
  hash->output_size = size >> 3;
  hash->block_size = (1600 - 2 * size) >> 3;
  return CX_SHA3;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int cx_keccak_init(cx_sha3_t *hash, unsigned int size)
{
  check_hash_out_size(size);
  cx_sha3_init(hash, size);
  hash->header.algo = CX_KECCAK;
  return CX_KECCAK;
}

int cx_shake128_init(cx_sha3_t *hash, unsigned int size)
{
  memset(hash, 0, sizeof(cx_sha3_t));
  if (size % 8 != 0) {
    return 0;
  }
  hash->header.algo = CX_SHAKE128;
  hash->output_size = size / 8;
  hash->block_size = (1600 - 2 * 128) >> 3;
  return CX_SHAKE128;
}

int cx_shake256_init(cx_sha3_t *hash, unsigned int size)
{
  memset(hash, 0, sizeof(cx_sha3_t));
  if (size % 8 != 0) {
    return 0;
  }
  hash->header.algo = CX_SHAKE256;
  hash->output_size = size / 8;
  hash->block_size = (1600 - 2 * 256) >> 3;
  return CX_SHAKE256;
}

int cx_sha3_xof_init(cx_sha3_t *hash, unsigned int size,
                     unsigned int out_length)
{
  if ((size != 128) && (size != 256)) {
    THROW(INVALID_PARAMETER);
  }
  memset(hash, 0, sizeof(cx_sha3_t));
  if (size == 128) {
    hash->header.algo = CX_SHAKE128;
  } else {
    hash->header.algo = CX_SHAKE256;
  }
  hash->output_size = out_length;
  hash->block_size = (1600 - 2 * size) >> 3;
  return hash->header.algo;
}

void spec_cx_sha3_block(cx_sha3_t *hash)
{
  uint64_t *block;
  uint64_t *acc;
  int r, i, n;

  block = (uint64_t *)hash->block;
  acc = (uint64_t *)hash->acc;

  PRINTF("sha3 block");
  printhex64(">Input block", block, hash->block_size / 8);
  printstate("Input", acc);

  if (hash->block_size > 144) {
    n = 21;
  } else if (hash->block_size > 136) {
    n = 18;
  } else if (hash->block_size > 104) {
    n = 17;
  } else if (hash->block_size > 72) {
    n = 13;
  } else {
    n = 9;
  }
  for (i = 0; i < n; i++) {
    acc[i] ^= block[i];
  }

  printstate("BlockMerged", acc);

  for (r = 0; r < 24; r++) {
    PRINTF("\nRound %d\n", r);
    cx_sha3_theta(acc);
    cx_sha3_rho_pi(acc);
    cx_sha3_chi(acc);
    cx_sha3_iota(acc, r);
  }
}

int spec_cx_sha3_update(cx_sha3_t *ctx, const uint8_t *data, size_t len)
{
  unsigned int r;
  unsigned int block_size;
  unsigned char *block;
  unsigned int blen;

  if (ctx == NULL) {
    return 0;
  }
  if (data == NULL && len != 0) {
    return 0;
  }

  block_size = ctx->block_size;
  if (block_size > 200) {
    return 0;
  }

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
      spec_cx_sha3_block(ctx);

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

int spec_cx_sha3_final(cx_sha3_t *hash, uint8_t *digest)
{
  unsigned int block_size;
  unsigned char *block;
  unsigned int blen;
  unsigned int len;

  if (hash == NULL || digest == NULL) {
    return 0;
  }
  block = hash->block;
  block_size = hash->block_size;
  blen = hash->blen;

  // one more block?
  if (hash->header.algo == CX_KECCAK || hash->header.algo == CX_SHA3) {
    // last block!
    memset(block + blen, 0, (200 - blen));

    if (hash->header.algo == CX_KECCAK) {
      block[blen] |= 01;
    } else {
      block[blen] |= 06;
    }
    block[block_size - 1] |= 0x80;
    spec_cx_sha3_block(hash);

    // provide result
    len = (hash)->output_size;
    memcpy(digest, hash->acc, len);
  } else {
    // CX_SHA3_XOF
    memset(block + blen, 0, (200 - blen));
    block[blen] |= 0x1F;
    block[block_size - 1] |= 0x80;
    spec_cx_sha3_block(hash);
    // provide result
    len = hash->output_size;
    blen = len;

    memset(block, 0, 200);

    while (blen > block_size) {
      memcpy(digest, hash->acc, block_size);
      blen -= block_size;
      digest += block_size;
      spec_cx_sha3_block(hash);
    }
    memcpy(digest, hash->acc, blen);
  }
  return 1;
}

size_t spec_cx_sha3_get_output_size(const cx_sha3_t *ctx)
{
  return ctx->output_size;
}

int cx_sha3_validate_context(const cx_sha3_t *ctx)
{
  size_t output_size = ctx->output_size;
  size_t block_size = ctx->block_size;
  if ((output_size != 128 / 8) && (output_size != 224 / 8) &&
      (output_size != 256 / 8) && (output_size != 384 / 8) &&
      (output_size != 512 / 8)) {
    return 0;
  }

  if (block_size != (1600 >> 3) - 2 * output_size) {
    return 0;
  }

  if (ctx->blen >= 200) {
    return 0;
  }
  return 1;
}

int cx_shake_validate_context(const cx_sha3_t *ctx)
{
  size_t block_size = ctx->block_size;

  if ((block_size != (1600 - 2 * 256) >> 3) &&
      (block_size != (1600 - 2 * 128) >> 3)) {
    return 0;
  }
  if (ctx->blen >= 200) {
    return 0;
  }
  return 1;
}
#endif // BOLOS_OS_UPGRADER
