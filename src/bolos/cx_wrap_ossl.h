#ifndef _CX_WRAP_OSSL_H
#define _CX_WRAP_OSSL_H

#define Stark_SIZE_u8 32
#define NID_Stark256     73
#define NID_BLS12_381_G1 666

/* openssl wrappers*/
 int cx_generic_curve(const cx_curve_weierstrass_t *i_weier, BN_CTX *ctx,
                     EC_GROUP **o_group);

 int nid_from_curve(cx_curve_t curve);


 int spec_cx_ecfp_encode_sig_der(unsigned char *sig, unsigned int sig_len,
                                 unsigned char *r, unsigned int r_len,
                                 unsigned char *s, unsigned int s_len);


#endif
