#pragma once

#include <stdint.h>
#include <string.h>

#define CX_RIPEMD160_SIZE 20
#define CX_SHA224_SIZE    28
#define CX_SHA256_SIZE    32
#define CX_SHA384_SIZE    48
#define CX_SHA512_SIZE    64

#define RIPEMD_BLOCK_SIZE       64
#define SHA256_BLOCK_SIZE       64
#define SHA512_BLOCK_SIZE       128
#define MAX_HASH_SIZE           CX_SHA512_SIZE
#define MAX_HASH_BLOCK_SIZE     SHA512_BLOCK_SIZE
#define CX_HASH_MAX_BLOCK_COUNT 65535

/** Message Digest algorithm identifiers. */
enum cx_md_e {
  /** NONE Digest */
  CX_NONE,
  /** RIPEMD160 Digest */
  CX_RIPEMD160, // 20 bytes
  /** SHA224 Digest */
  CX_SHA224, // 28 bytes
  /** SHA256 Digest */
  CX_SHA256, // 32 bytes
  /** SHA384 Digest */
  CX_SHA384, // 48 bytes
  /** SHA512 Digest */
  CX_SHA512, // 64 bytes
  /** Keccak (pre-SHA3) Digest */
  CX_KECCAK, // 28,32,48,64 bytes
  /** SHA3 Digest */
  CX_SHA3, // 28,32,48,64 bytes
  /** SHA3-XOF  Digest */
  CX_SHA3_XOF, // any bytes
  /** Groestl Digest */
  CX_GROESTL,
  /** Blake Digest */
  CX_BLAKE2B,
  /** SHAKE-128 Digest */
  CX_SHAKE128, // any bytes
  /** SHAKE-128 Digest */
  CX_SHAKE256, // any bytes
};
/** Convenience type. See #cx_md_e. */
typedef enum cx_md_e cx_md_t;

/**
 * Common Message Digest context, used as abstract type.
 */
struct cx_hash_header_s {
  /** Message digest identifier, See cx_md_e. */
  cx_md_t algo;
  /** Number of block already processed */
  uint32_t counter;
};
/** Convenience type. See #cx_hash_header_s. */
typedef struct cx_hash_header_s cx_hash_t;

/**
 * SHA-224 and SHA-256 context
 */
struct cx_sha256_s {
  /** @copydoc cx_ripemd160_s::header */
  struct cx_hash_header_s header;
  /** @internal @copydoc cx_ripemd160_s::blen */
  size_t blen;
  /** @internal @copydoc cx_ripemd160_s::block */
  unsigned char block[64];
  /** @copydoc cx_ripemd160_s::acc */
  unsigned char acc[8 * 4];
};
/** Convenience type. See #cx_sha256_s. */
typedef struct cx_sha256_s cx_sha256_t;

/**
 * SHA-384 and SHA-512 context
 */
struct cx_sha512_s {
  /** @copydoc cx_ripemd160_s::header */
  struct cx_hash_header_s header;
  /** @internal @copydoc cx_ripemd160_s::blen */
  unsigned int blen;
  /** @internal @copydoc cx_ripemd160_s::block */
  unsigned char block[128];
  /** @copydoc cx_ripemd160_s::acc */
  unsigned char acc[8 * 8];
};
/** Convenience type. See #cx_sha512_s. */
typedef struct cx_sha512_s cx_sha512_t;

struct cx_ripemd160_s {
  /** See #cx_hash_header_s */
  struct cx_hash_header_s header;
  /** @internal
   * pending partial block length
   */
  unsigned int blen;
  /** @internal
   * pending partial block
   */
  unsigned char block[64];
  /** Current digest state.
   * After finishing the digest, contains the digest if correct parameters are
   * passed.
   */
  unsigned char acc[5 * 4];
};
/** Convenience type. See #cx_ripemd160_s. */
typedef struct cx_ripemd160_s cx_ripemd160_t;

/**  @private */
enum blake2b_constant {
  BLAKE2B_BLOCKBYTES = 128,
  BLAKE2B_OUTBYTES = 64,
  BLAKE2B_KEYBYTES = 64,
  BLAKE2B_SALTBYTES = 16,
  BLAKE2B_PERSONALBYTES = 16
};

/**  @private */
struct blake2b_state__ {
  uint64_t h[8];
  uint64_t t[2];
  uint64_t f[2];
  uint8_t buf[BLAKE2B_BLOCKBYTES];
  size_t buflen;
  size_t outlen;
  uint8_t last_node;
};
/** @private */
typedef struct blake2b_state__ blake2b_state;

/**
 * Blake2b context
 */
struct cx_blake2b_s {
  /** @copydoc cx_ripemd160_s::header */
  struct cx_hash_header_s header;
  /** @internal output digest size*/
  unsigned int output_size;
  struct blake2b_state__ ctx;
};
/** Convenience type. See #cx_blake2b_s. */
typedef struct cx_blake2b_s cx_blake2b_t;

/**
 * KECCAK, SHA3 and SHA3-XOF context
 */
struct cx_sha3_s {
  /** @copydoc cx_ripemd160_s::header */
  struct cx_hash_header_s header;

  /** @internal output digest size*/
  unsigned int output_size;
  /** @internal input block size*/
  unsigned int block_size;
  /** @internal @copydoc cx_ripemd160_s::blen */
  unsigned int blen;
  /** @internal @copydoc cx_ripemd160_s::block */
  unsigned char block[200];
  /** @copydoc cx_ripemd160_s::acc */
  uint64_t acc[25];
};
/** Convenience type. See #cx_sha3_s. */
typedef struct cx_sha3_s cx_sha3_t;

/* Generic API */
typedef struct {
  cx_md_t md_type;
  size_t output_size;
  size_t block_size;
  int (*init_func)(void *ctx);
  int (*update_func)(void *ctx, const uint8_t *data, size_t len);
  int (*finish_func)(void *ctx, uint8_t *digest);
  int (*init_ex_func)(void *ctx, size_t output_size);
  int (*validate_context)(const void *ctx);
  size_t (*output_size_func)(const void *ctx);
} cx_hash_info_t;

typedef union {
  cx_hash_t header;
  cx_sha256_t sha256;
  cx_sha512_t sha512;
  cx_blake2b_t blake2b;
#if 0
  cx_groestl_t groestl;
#endif
  cx_ripemd160_t ripemd160;
  cx_sha3_t sha3;
} cx_hash_ctx;

int cx_blake2b_init(cx_blake2b_t *hash, unsigned int size);
int cx_blake2b_init2(cx_blake2b_t *hash, unsigned int size, unsigned char *salt,
                     unsigned int salt_len, unsigned char *perso,
                     unsigned int perso_len);
int spec_cx_blake2b_update(cx_blake2b_t *ctx, const uint8_t *data, size_t len);
int spec_cx_blake2b_final(cx_blake2b_t *ctx, uint8_t *digest);
int cx_blake2b_validate_context(const cx_blake2b_t *ctx);
size_t spec_cx_blake2b_get_output_size(const cx_blake2b_t *ctx);

int cx_sha224_init(cx_sha256_t *hash);
int cx_sha256_init(cx_sha256_t *hash);
int spec_cx_sha256_update(cx_sha256_t *ctx, const uint8_t *data, size_t len);
int spec_cx_sha256_final(cx_sha256_t *ctx, uint8_t *digest);
int cx_sha256_validate_context(const cx_sha256_t *ctx);

int cx_sha384_init(cx_sha512_t *hash);
int cx_sha512_init(cx_sha512_t *hash);
int spec_cx_sha512_final(cx_sha512_t *ctx, uint8_t *digest);
int spec_cx_sha512_update(cx_sha512_t *ctx, const uint8_t *data, size_t len);
int cx_sha512_validate_context(const cx_sha512_t *ctx);
int spec_cx_hash_sha512(const uint8_t *data, size_t len, uint8_t *digest,
                        size_t digest_len);

int cx_ripemd160_init(cx_ripemd160_t *hash);
int spec_cx_ripemd160_update(cx_ripemd160_t *ctx, const uint8_t *data,
                             size_t len);
int spec_cx_ripemd160_final(cx_ripemd160_t *ctx, uint8_t *digest);
int cx_ripemd160_validate_context(const cx_ripemd160_t *ctx);

int cx_sha3_init(cx_sha3_t *hash, unsigned int size);
int cx_keccak_init(cx_sha3_t *hash, unsigned int size);
int cx_shake128_init(cx_sha3_t *hash, unsigned int out_size);
int cx_shake256_init(cx_sha3_t *hash, unsigned int out_size);
int cx_sha3_xof_init(cx_sha3_t *hash, unsigned int size,
                     unsigned int out_length);
int spec_cx_sha3_update(cx_sha3_t *ctx, const uint8_t *data, size_t len);
int spec_cx_sha3_final(cx_sha3_t *ctx, uint8_t *digest);
int cx_sha3_validate_context(const cx_sha3_t *ctx);
int cx_shake_validate_context(const cx_sha3_t *ctx);
size_t spec_cx_sha3_get_output_size(const cx_sha3_t *ctx);

const cx_hash_info_t *spec_cx_hash_get_info(cx_md_t md_type);
size_t spec_cx_hash_get_size(const cx_hash_ctx *ctx);
int spec_cx_hash_init(cx_hash_ctx *ctx, cx_md_t md_type);
int spec_cx_hash_init_ex(cx_hash_ctx *ctx, cx_md_t md_type, size_t output_size);
int spec_cx_hash_update(cx_hash_ctx *ctx, const uint8_t *data, size_t len);
int spec_cx_hash_final(cx_hash_ctx *ctx, uint8_t *digest);

int sys_cx_hash_sha256(const uint8_t *in, size_t len, uint8_t *out,
                       size_t out_len);

#define sys_cx_blake2b_init   cx_blake2b_init
#define sys_cx_blake2b_init2  cx_blake2b_init2
#define sys_cx_sha224_init    cx_sha224_init
#define sys_cx_sha256_init    cx_sha256_init
#define sys_cx_sha512_init    cx_sha512_init
#define sys_cx_ripemd160_init cx_ripemd160_init
#define sys_cx_keccak_init    cx_keccak_init
#define sys_cx_sha3_init      cx_sha3_init
#define sys_cx_sha3_xof_init  cx_sha3_xof_init
#define sys_cx_hash_sha512    spec_cx_hash_sha512
