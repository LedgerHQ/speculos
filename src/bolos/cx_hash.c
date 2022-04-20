#include <stdio.h>

#include "bolos/exception.h"
#include "cx.h"
#include "emulate.h"

static const cx_hash_info_t cx_sha256_info = {
  CX_SHA256,
  CX_SHA256_SIZE,
  SHA256_BLOCK_SIZE,
  (int (*)(void *ctx))cx_sha256_init,
  (int (*)(void *ctx, const uint8_t *data, size_t len))spec_cx_sha256_update,
  (int (*)(void *ctx, uint8_t *digest))spec_cx_sha256_final,
  NULL,
  (int (*)(const void *ctx))cx_sha256_validate_context,
  NULL
};

static const cx_hash_info_t cx_sha224_info = {
  CX_SHA224,
  CX_SHA224_SIZE,
  SHA256_BLOCK_SIZE,
  (int (*)(void *ctx))cx_sha224_init,
  (int (*)(void *ctx, const uint8_t *data, size_t len))spec_cx_sha256_update,
  (int (*)(void *ctx, uint8_t *digest))spec_cx_sha256_final,
  NULL,
  (int (*)(const void *ctx))cx_sha256_validate_context,
  NULL
};

static const cx_hash_info_t cx_sha384_info = {
  CX_SHA384,
  CX_SHA384_SIZE,
  SHA512_BLOCK_SIZE,
  (int (*)(void *ctx))cx_sha384_init,
  (int (*)(void *ctx, const uint8_t *data, size_t len))spec_cx_sha512_update,
  (int (*)(void *ctx, uint8_t *digest))spec_cx_sha512_final,
  NULL,
  (int (*)(const void *ctx))cx_sha512_validate_context,
  NULL
};

static const cx_hash_info_t cx_sha512_info = {
  CX_SHA512,
  CX_SHA512_SIZE,
  SHA512_BLOCK_SIZE,
  (int (*)(void *ctx))cx_sha512_init,
  (int (*)(void *ctx, const uint8_t *data, size_t len))spec_cx_sha512_update,
  (int (*)(void *ctx, uint8_t *digest))spec_cx_sha512_final,
  NULL,
  (int (*)(const void *ctx))cx_sha512_validate_context,
  NULL
};

static const cx_hash_info_t cx_sha3_info = {
  CX_SHA3,
  0,
  0,
  NULL,
  (int (*)(void *ctx, const uint8_t *data, size_t len))spec_cx_sha3_update,
  (int (*)(void *ctx, uint8_t *digest))spec_cx_sha3_final,
  (int (*)(void *ctx, size_t output_size))cx_sha3_init,
  (int (*)(const void *ctx))cx_sha3_validate_context,
  (size_t(*)(const void *ctx))spec_cx_sha3_get_output_size
};

static const cx_hash_info_t cx_keccak_info = {
  CX_KECCAK,
  0,
  0,
  NULL,
  (int (*)(void *ctx, const uint8_t *data, size_t len))spec_cx_sha3_update,
  (int (*)(void *ctx, uint8_t *digest))spec_cx_sha3_final,
  (int (*)(void *ctx, size_t output_size))cx_keccak_init,
  (int (*)(const void *ctx))cx_sha3_validate_context,
  (size_t(*)(const void *ctx))spec_cx_sha3_get_output_size
};

static const cx_hash_info_t cx_shake128_info = {
  CX_SHAKE128,
  0,
  0,
  NULL,
  (int (*)(void *ctx, const uint8_t *data, size_t len))spec_cx_sha3_update,
  (int (*)(void *ctx, uint8_t *digest))spec_cx_sha3_final,
  (int (*)(void *ctx, size_t output_size))cx_shake128_init,
  (int (*)(const void *ctx))cx_shake_validate_context,
  (size_t(*)(const void *ctx))spec_cx_sha3_get_output_size
};

static const cx_hash_info_t cx_shake256_info = {
  CX_SHAKE256,
  0,
  0,
  NULL,
  (int (*)(void *ctx, const uint8_t *data, size_t len))spec_cx_sha3_update,
  (int (*)(void *ctx, uint8_t *digest))spec_cx_sha3_final,
  (int (*)(void *ctx, size_t output_size))cx_shake256_init,
  (int (*)(const void *ctx))cx_shake_validate_context,
  (size_t(*)(const void *ctx))spec_cx_sha3_get_output_size
};

static const cx_hash_info_t cx_ripemd160_info = {
  CX_RIPEMD160,
  CX_RIPEMD160_SIZE,
  RIPEMD_BLOCK_SIZE,
  (int (*)(void *ctx))cx_ripemd160_init,
  (int (*)(void *ctx, const uint8_t *data, size_t len))spec_cx_ripemd160_update,
  (int (*)(void *ctx, uint8_t *digest))spec_cx_ripemd160_final,
  NULL,
  (int (*)(const void *ctx))cx_ripemd160_validate_context,
  NULL
};

static const cx_hash_info_t cx_blake2b_info = {
  CX_BLAKE2B,
  0,
  BLAKE2B_BLOCKBYTES,
  NULL,
  (int (*)(void *ctx, const uint8_t *data, size_t len))spec_cx_blake2b_update,
  (int (*)(void *ctx, uint8_t *digest))spec_cx_blake2b_final,
  (int (*)(void *ctx, size_t output_size))cx_blake2b_init,
  (int (*)(const void *ctx))cx_blake2b_validate_context,
  (size_t(*)(const void *ctx))spec_cx_blake2b_get_output_size
};

#if 0
static const cx_hash_info_t cx_groestl_info = {
    CX_GROESTL,
    0,
    SIZE1024,
    NULL,
    (int (*)(void *ctx, const uint8_t *data, size_t len))spec_cx_groestl_update,
    (int (*)(void *ctx, uint8_t *digest))spec_cx_groestl_final,
    (int (*)(void *ctx, size_t output_size))cx_groestl_init,
    (int (*)(const void *ctx))cx_groestl_validate_context,
    (size_t(*)(const void *ctx))spec_cx_groestl_get_output_size};
#endif

const cx_hash_info_t *spec_cx_hash_get_info(cx_md_t md_type)
{
  switch (md_type) {
  case CX_SHA256:
    return &cx_sha256_info;
  case CX_RIPEMD160:
    return &cx_ripemd160_info;
  case CX_SHA224:
    return &cx_sha224_info;
  case CX_SHA384:
    return &cx_sha384_info;
  case CX_SHA512:
    return &cx_sha512_info;
  case CX_SHA3:
    return &cx_sha3_info;
  case CX_KECCAK:
    return &cx_keccak_info;
  case CX_SHAKE128:
    return &cx_shake128_info;
  case CX_SHAKE256:
    return &cx_shake256_info;
  case CX_BLAKE2B:
    return &cx_blake2b_info;
#if 0
  case CX_GROESTL:
    return &cx_groestl_info;
#endif
  default:
    return NULL;
  }
}

size_t spec_cx_hash_get_size(const cx_hash_ctx *ctx)
{
  const cx_hash_info_t *info = spec_cx_hash_get_info(ctx->header.algo);
  if (info == NULL) {
    return 0;
  }
  if (info->output_size) {
    return info->output_size;
  }
  return info->output_size_func(ctx);
}

int spec_cx_hash_init(cx_hash_ctx *ctx, cx_md_t md_type)
{
  const cx_hash_info_t *info = spec_cx_hash_get_info(md_type);
  if (info == NULL) {
    return 0;
  }
  if (info->output_size ==
      0) { /* variable output size, must use spec_cx_hash_init_ex */
    return 0;
  }
  ctx->header.algo = md_type;
  info->init_func(ctx);
  return 1;
}

int spec_cx_hash_init_ex(cx_hash_ctx *ctx, cx_md_t md_type, size_t output_size)
{
  const cx_hash_info_t *info = spec_cx_hash_get_info(md_type);
  if (info == NULL) {
    return 0;
  }
  if (info->output_size != 0) {
    if (info->output_size != output_size) {
      return 0;
    }
    return spec_cx_hash_init(ctx, md_type);
  }
  ctx->header.algo = md_type;
  info->init_ex_func(ctx, output_size * 8);
  return 1;
}

int spec_cx_hash_update(cx_hash_ctx *ctx, const uint8_t *data, size_t len)
{
  if (ctx == NULL) {
    return 0;
  }
  const cx_hash_info_t *info = spec_cx_hash_get_info(ctx->header.algo);
  if (info == NULL) {
    return 0;
  }
  if (!info->validate_context(ctx)) {
    return 0;
  }
  return info->update_func(ctx, data, len);
}

int spec_cx_hash_final(cx_hash_ctx *ctx, uint8_t *digest)
{
  if (ctx == NULL || digest == NULL) {
    return 0;
  }
  const cx_hash_info_t *info = spec_cx_hash_get_info(ctx->header.algo);
  if (info == NULL) {
    return 0;
  }
  if (!info->validate_context(ctx)) {
    return 0;
  }
  return info->finish_func(ctx, digest);
}

static int cx_hash_validate_context(const cx_hash_t *ctx)
{
  if (ctx == NULL) {
    return 0;
  }
  const cx_hash_info_t *info = spec_cx_hash_get_info(ctx->algo);
  if (info == NULL) {
    return 0;
  }
  return info->validate_context(ctx);
}

unsigned long sys_cx_hash(cx_hash_t *hash, int mode, const uint8_t *in,
                          size_t len, uint8_t *out, size_t out_len)
{
  unsigned int digest_len;

  // --- init locals ---
  if (!cx_hash_validate_context(hash)) {
    goto err;
  }
  digest_len = (unsigned int)spec_cx_hash_get_size((cx_hash_ctx *)hash);
  if (!spec_cx_hash_update((cx_hash_ctx *)hash, in, len)) {
    goto err;
  }

  if (mode & CX_LAST) {
    if (out_len < digest_len) {
      goto err;
    }
    if (!spec_cx_hash_final((cx_hash_ctx *)hash, out)) {
      goto err;
    }
    return digest_len;
  }
  return 0;

err:
  THROW(INVALID_PARAMETER);

  return 0;
}
