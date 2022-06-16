#ifndef _CX_WRAP_OSSL_H
#define _CX_WRAP_OSSL_H

#define Stark_SIZE_u8    32
#define NID_Stark256     73
#define NID_BLS12_381_G1 666

/* UGLY return codes of openSSL, when OK=1 and KO=0 :( */
#define OPEN_SSL_KO               0
#define OPEN_SSL_OK               1
#define OPEN_SSL_UNEXPECTED_ERROR 0xcaca055L
/* openssl wrappers*/
int cx_generic_curve(const cx_curve_weierstrass_t *i_weier, BN_CTX *ctx,
                     EC_GROUP **o_group);

int nid_from_curve(cx_curve_t curve);
#define cx_nid_from_curve(a) nid_from_curve(a)

int spec_cx_ecfp_encode_sig_der(unsigned char *sig, unsigned int sig_len,
                                unsigned char *r, unsigned int r_len,
                                unsigned char *s, unsigned int s_len);

int spec_cx_ecfp_decode_sig_der(const uint8_t *input, size_t input_len,
                                size_t max_size, const uint8_t **r,
                                size_t *r_len, const uint8_t **s,
                                size_t *s_len);

#endif
