#ifndef _ECDSA_H
#define _ECDSA_H


int sys_cx_ecdsa_sign(const cx_ecfp_private_key_t *key, int mode,
                      cx_md_t hashID, const uint8_t *hash,
                      unsigned int hash_len, uint8_t *sig, unsigned int sig_len,
                      unsigned int *info);

int sys_cx_ecdsa_verify(const cx_ecfp_public_key_t *key, int mode,
                        cx_md_t hashID, const uint8_t *hash,
                        unsigned int hash_len, const uint8_t *sig,
                        unsigned int sig_len);


#endif
