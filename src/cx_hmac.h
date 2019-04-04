#ifndef _CX_HMAC_H
#define _CX_HMAC_H

int cx_hmac_init(cx_hmac_ctx *ctx, cx_md_t hash_id, const uint8_t *key, size_t key_len);
int cx_hmac_update(cx_hmac_ctx *ctx, const uint8_t *data, size_t data_len);
int cx_hmac_final(cx_hmac_ctx *ctx, uint8_t *out, size_t *out_len);

int cx_hmac_sha512_init(cx_hmac_sha512_t *hmac, const unsigned char *key, unsigned int key_len);

int sys_cx_hmac(cx_hmac_t *hmac, int mode, const unsigned char *in, unsigned int len, unsigned char *out, unsigned int out_len);
int sys_cx_hmac_sha256_init(cx_hmac_sha256_t *hmac, const unsigned char *key, unsigned int key_len);

#endif
