#ifndef _CX_EDDSA_H
#define _CX_EDDSA_H

#define _EDD_SIG_T8 64

int sys_cx_eddsa_sign(const cx_ecfp_private_key_t *pvkey, int mode,
                      cx_md_t hashID, const unsigned char *hash,
                      unsigned int hash_len, const unsigned char *ctx,
                      unsigned int ctx_len, unsigned char *sig,
                      unsigned int sig_len, unsigned int *info);

int sys_cx_eddsa_verify(const cx_ecfp_public_key_t *pu_key, int mode,
                        cx_md_t hashID, const unsigned char *hash,
                        unsigned int hash_len, const unsigned char *ctx,
                        unsigned int ctx_len, const unsigned char *sig,
                        unsigned int sig_len);

#endif
