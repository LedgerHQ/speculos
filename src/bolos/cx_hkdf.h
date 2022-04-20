#ifndef CX_HKDF_H
#define CX_HKDF_H

#include "cx.h"

void cx_hkdf_extract(const cx_md_t hash_id, const unsigned char *ikm,
                     unsigned int ikm_len, unsigned char *salt,
                     unsigned int salt_len, unsigned char *prk);
void spec_cx_hkdf_expand(const cx_md_t hash_id, const unsigned char *prk,
                         unsigned int prk_len, unsigned char *info,
                         unsigned int info_len, unsigned char *okm,
                         unsigned int okm_len);

#endif // CX_HKDF_H
