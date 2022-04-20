#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include "cx.h"
#include "cx_hmac.h"
//#include "cx_utils.h"
#include "bolos/exception.h"

#define IPAD 0x36u
#define OPAD 0x5cu

union cx_u {
  cx_hmac_ctx hmac;
};

static union cx_u G_cx;

static size_t get_block_size(cx_md_t md)
{
  const cx_hash_info_t *info = spec_cx_hash_get_info(md);
  if (info == NULL || info->block_size == 0) {
    return 0;
  }
  return info->block_size;
}

static cx_md_t get_algorithm(cx_hmac_ctx *ctx)
{
  return ctx->hash_ctx.header.algo;
}

static int is_allowed_digest(cx_md_t md_type)
{
  const cx_md_t allowed_algorithms[] = { CX_SHA256, CX_SHA224, CX_SHA384,
                                         CX_SHA512, CX_RIPEMD160 };
  for (unsigned int i = 0;
       i < sizeof(allowed_algorithms) / sizeof(allowed_algorithms[0]); i++) {
    if (allowed_algorithms[i] == md_type) {
      return 1;
    }
  }
  return 0;
}

static cx_hash_ctx *get_hash_ctx(cx_hmac_ctx *ctx)
{
  if (is_allowed_digest(get_algorithm(ctx))) {
    return (cx_hash_ctx *)ctx;
  }
  return NULL;
}

int spec_cx_hmac_init(cx_hmac_ctx *ctx, cx_md_t hash_id, const uint8_t *key,
                      size_t key_len)
{
  if (ctx == NULL) {
    return 0;
  }
  if (key == NULL && key_len != 0) {
    return 0;
  }
  if (!is_allowed_digest(hash_id)) {
    return 0;
  }
  cx_hash_ctx *hash_ctx = (cx_hash_ctx *)&ctx->hash_ctx;
  if (hash_ctx == NULL) {
    return 0;
  }

  memset(ctx, 0, sizeof(*ctx));
  size_t block_size = get_block_size(hash_id);

  if (key) {
    if (key_len > block_size) {
      spec_cx_hash_init(hash_ctx, hash_id);
      spec_cx_hash_update(hash_ctx, key, key_len);
      spec_cx_hash_final(hash_ctx, ctx->key);
    } else {
      memcpy(ctx->key, key, key_len);
    }

    for (unsigned int i = 0; i < block_size; i++) {
      ctx->key[i] ^= IPAD;
    }
  }

  spec_cx_hash_init(hash_ctx, hash_id);
  spec_cx_hash_update(hash_ctx, ctx->key, block_size);
  return 1;
}

int spec_cx_hmac_update(cx_hmac_ctx *ctx, const uint8_t *data, size_t data_len)
{
  if (ctx == NULL || data == NULL) {
    return 0;
  }
  cx_hash_ctx *hash_ctx = get_hash_ctx(ctx);
  if (hash_ctx == NULL) {
    return 0;
  }
  return spec_cx_hash_update(hash_ctx, data, data_len);
}

int spec_cx_hmac_final(cx_hmac_ctx *ctx, uint8_t *out, size_t *out_len)
{
  uint8_t inner_hash[MAX_HASH_SIZE];
  uint8_t hkey[MAX_HASH_BLOCK_SIZE];

  if (ctx == NULL || out == NULL || out_len == 0) {
    return 0;
  }

  cx_hash_ctx *hash_ctx = get_hash_ctx(ctx);
  if (hash_ctx == NULL) {
    return 0;
  }

  cx_md_t hash_algorithm = get_algorithm(ctx);
  size_t block_size = get_block_size(hash_algorithm);
  size_t hash_output_size = spec_cx_hash_get_size(hash_ctx);

  spec_cx_hash_final(hash_ctx, inner_hash);

  // hash key xor 5c (and 36 to remove prepadding at init)
  memcpy(hkey, ctx->key, block_size);
  for (unsigned int i = 0; i < block_size; i++) {
    hkey[i] ^= OPAD ^ IPAD;
  }

  spec_cx_hash_init(hash_ctx, hash_algorithm);
  spec_cx_hash_update(hash_ctx, hkey, block_size);
  spec_cx_hash_update(hash_ctx, inner_hash, hash_output_size);
  spec_cx_hash_final(hash_ctx, hkey);

  // length result
  if (*out_len >= hash_output_size) {
    *out_len = hash_output_size;
  }
  memcpy(out, hkey, *out_len);
  return 1;
}

// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
int cx_hmac_sha256_init(cx_hmac_sha256_t *hmac, const unsigned char *key,
                        unsigned int key_len)
{
  if (!spec_cx_hmac_init(hmac, CX_SHA256, key, key_len)) {
    THROW(INVALID_PARAMETER);
  }
  return CX_SHA256;
}

// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
int cx_hmac_ripemd160_init(cx_hmac_ripemd160_t *hmac, const unsigned char *key,
                           unsigned int key_len)
{
  if (!spec_cx_hmac_init(hmac, CX_RIPEMD160, key, key_len)) {
    THROW(INVALID_PARAMETER);
  }
  return CX_RIPEMD160;
}

// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
int cx_hmac_sha512_init(cx_hmac_sha512_t *hmac, const unsigned char *key,
                        unsigned int key_len)
{
  if (!spec_cx_hmac_init(hmac, CX_SHA512, key, key_len)) {
    THROW(INVALID_PARAMETER);
  }
  return CX_SHA512;
}

// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
int cx_hmac(cx_hmac_t *hmac, int mode, const unsigned char *in,
            unsigned int len, unsigned char *out, unsigned int out_len)
{
  int ret = 0;
  size_t output_size = 0;

  if (in == NULL && len != 0) {
    goto err;
  }
  if (out == NULL && out_len != 0) {
    goto err;
  }
  cx_scc_struct_check_hashmac(hmac);

  spec_cx_hmac_update(hmac, in, len);
  if (mode & CX_LAST) {
    output_size = out_len;
    spec_cx_hmac_final(hmac, out, &output_size);
    ret = output_size;

    if (!(mode & CX_NO_REINIT)) {
      spec_cx_hmac_init(hmac, get_algorithm(hmac), NULL, 0);
    }
  }
  return ret;

err:
  THROW(INVALID_PARAMETER);
}

// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
int spec_cx_hmac_sha256(const unsigned char *key, unsigned int key_len,
                        const unsigned char *in, unsigned int len,
                        unsigned char *out, unsigned int out_len)
{
  size_t mac_len = out_len;
  spec_cx_hmac_init(&G_cx.hmac, CX_SHA256, key, key_len);
  spec_cx_hmac_update(&G_cx.hmac, in, len);
  spec_cx_hmac_final(&G_cx.hmac, out, &mac_len);
  return (int)mac_len;
}

// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
int spec_cx_hmac_sha512(const unsigned char *key, unsigned int key_len,
                        const unsigned char *in, unsigned int len,
                        unsigned char *out, unsigned int out_len)
{
  size_t mac_len = out_len;
  spec_cx_hmac_init(&G_cx.hmac, CX_SHA512, key, key_len);
  spec_cx_hmac_update(&G_cx.hmac, in, len);
  spec_cx_hmac_final(&G_cx.hmac, out, &mac_len);
  return (int)mac_len;
}
