#ifndef OS_SIGNATURE_H
#define OS_SIGNATURE_H

#include "cxlib.h"

#define ROOT_CA_V3_KEY_ID (4)

typedef enum cx_sign_algo_e {
  CX_SIGN_ALGO_UNKNOWN = 0x00,
  CX_SIGN_ALGO_EDDSA_SHA512 = 0x05,
  CX_SIGN_ALGO_ECDSA_RND = 0x20,
  CX_SIGN_ALGO_ECDSA_RFC6979_RIPEMD160 = 0x31,
  CX_SIGN_ALGO_ECDSA_RFC6979_SHA224 = 0x32,
  CX_SIGN_ALGO_ECDSA_RFC6979_SHA256 = 0x33,
  CX_SIGN_ALGO_ECDSA_RFC6979_SHA384 = 0x34,
  CX_SIGN_ALGO_ECDSA_RFC6979_SHA512 = 0x35,
  CX_SIGN_ALGO_ECDSA_RFC6979_KECCAK = 0x36,
  CX_SIGN_ALGO_ECDSA_RFC6979_SHA3 = 0x37,
} cx_sign_algo_t;

cx_err_t cx_ecdsa_internal_init_public_key(cx_curve_t curve,
                                           const unsigned char *rawkey,
                                           unsigned int key_len,
                                           cx_ecfp_public_key_t *key);

bool cx_ecdsa_internal_verify(const cx_ecfp_public_key_t *key,
                              const uint8_t *hash, unsigned int hash_len,
                              const uint8_t *sig, unsigned int sig_len);

bool cx_eddsa_internal_verify(const cx_ecfp_public_key_t *pu_key,
                              cx_md_t hashID, const unsigned char *hash,
                              unsigned int hash_len, const unsigned char *sig,
                              unsigned int sig_len);

bool cx_verify(cx_sign_algo_t sign_algo, cx_ecfp_public_key_t *public_key,
               uint8_t *msg_hash, size_t msg_hash_len, uint8_t *signature,
               size_t signature_len);

bool os_ecdsa_verify_with_root_ca(uint8_t key_id, uint8_t *hash,
                                  size_t hash_len, uint8_t *sig,
                                  size_t sig_len);

#endif /* OS_SIGNATURE_H */
