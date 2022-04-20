#pragma once

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

/* speculos helpers */
void cx_scc_struct_check_hashmac(const cx_hmac_t *hmac);
int spec_cx_hmac_init(cx_hmac_ctx *ctx, cx_md_t hash_id, const uint8_t *key,
                      size_t key_len);
int spec_cx_hmac_update(cx_hmac_ctx *ctx, const uint8_t *data, size_t data_len);
int spec_cx_hmac_final(cx_hmac_ctx *ctx, uint8_t *out, size_t *out_len);

/* bolos syscalls */
int spec_cx_hmac_sha256(const unsigned char *key, unsigned int key_len,
                        const unsigned char *in, unsigned int len,
                        unsigned char *out, unsigned int out_len);
int cx_hmac_sha256_init(cx_hmac_sha256_t *hmac, const unsigned char *key,
                        unsigned int key_len);

int spec_cx_hmac_sha512(const unsigned char *key, unsigned int key_len,
                        const unsigned char *in, unsigned int len,
                        unsigned char *out, unsigned int out_len);
int cx_hmac_sha512_init(cx_hmac_sha512_t *hmac, const unsigned char *key,
                        unsigned int key_len);

int cx_hmac(cx_hmac_t *hmac, int mode, const unsigned char *in,
            unsigned int len, unsigned char *out, unsigned int out_len);

#define sys_cx_hmac_sha256      spec_cx_hmac_sha256
#define sys_cx_hmac_sha256_init cx_hmac_sha256_init
#define sys_cx_hmac_sha512      spec_cx_hmac_sha512
#define sys_cx_hmac_sha512_init cx_hmac_sha512_init
#define sys_cx_hmac             cx_hmac
