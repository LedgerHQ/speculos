#ifndef CX_HKDF_H
#define CX_HKDF_H

#include "cx_hash.h"

#define CX_HKDF_MOD_R_CEIL (48)
#define KEY_LENGTH         (32)
#define LAMPORT_KEY_LENGTH (KEY_LENGTH + 1)

void cx_hkdf_extract(const cx_md_t hash_id, const unsigned char *ikm,
                     unsigned int ikm_len, unsigned char *salt,
                     unsigned int salt_len, unsigned char *prk);
void spec_cx_hkdf_expand(const cx_md_t hash_id, const unsigned char *prk,
                         unsigned int prk_len, unsigned char *info,
                         unsigned int info_len, unsigned char *okm,
                         unsigned int okm_len);
void cx_hkdf_mod_r(const unsigned char *ikm, unsigned int ikm_len,
                   const uint8_t *salt, unsigned int salt_len,
                   unsigned char *key_info, unsigned int key_info_len,
                   unsigned char *sk);

#endif // CX_HKDF_H
