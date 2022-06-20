#include <err.h>
#include <string.h>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/objects.h>
#include <openssl/sha.h>

#include "bolos/cx_errors.h"
#include "bolos/exception.h"
#include "cx.h"
#include "cx_curve25519.h"
#include "cx_ec.h"
#include "cx_ed25519.h"
#include "cx_eddsa.h"
#include "cx_errors.h"
#include "cx_hash.h"
#include "cx_rng_rfc6979.h"
#include "cx_utils.h"
#include "cx_wrap_ossl.h"
#include "emulate.h"

#include "cx_const_ecedd25519.c"

/* Unexported functions from OpenSSL, in ec/curve25519.c. Dirty hack... */
int ED25519_sign(uint8_t *out_sig, const uint8_t *message, size_t message_len,
                 const uint8_t public_key[32], const uint8_t private_key[32]);
int ED25519_verify(const uint8_t *message, size_t message_len,
                   const uint8_t signature[64], const uint8_t public_key[32]);
void ED25519_public_from_private(uint8_t out_public_key[32],
                                 const uint8_t private_key[32]);

int sys_cx_eddsa_sign(const cx_ecfp_private_key_t *pvkey,
                      int mode __attribute__((unused)), cx_md_t hashID,
                      const unsigned char *hash, unsigned int hash_len,
                      const unsigned char *ctx __attribute__((unused)),
                      unsigned int ctx_len __attribute__((unused)),
                      unsigned char *sig, unsigned int sig_len,
                      unsigned int *info __attribute__((unused)))
{
  uint8_t public_key[32];

  /* Only SHA-512 is supported in Speculos, as it is the only supported hash
   * in OpenSSL */
  if (hashID != CX_SHA512) {
    return -1;
  }
  if (sig_len < 64) {
    return CX_KO;
  }
  /* Key must be a Ed25519 private key */
  if (pvkey->curve != CX_CURVE_Ed25519 || pvkey->d_len != 32) {
    return CX_KO;
  }
  ED25519_public_from_private(public_key, pvkey->d);
  if (ED25519_sign(sig, hash, hash_len, public_key, pvkey->d) == 1) {
    return _EDD_SIG_T8;
  }
  return CX_KO;
}

int sys_cx_eddsa_verify(const cx_ecfp_public_key_t *pu_key,
                        int mode __attribute__((unused)), cx_md_t hashID,
                        const unsigned char *hash, unsigned int hash_len,
                        const unsigned char *ctx __attribute__((unused)),
                        unsigned int ctx_len __attribute__((unused)),
                        const unsigned char *sig, unsigned int sig_len)
{
  cx_ecfp_public_key_t pub;

  /* Only SHA-512 is supported in Speculos, as it is the only supported hash
   * in OpenSSL */
  if (hashID != CX_SHA512) {
    return 0;
  }
  if (sig_len != 64) {
    return 0;
  }
  if (pu_key->curve != CX_CURVE_Ed25519 || pu_key->W_len != 1 + 2 * 32) {
    return 0;
  }

  /* pass a compressed key to ED25519_verify */
  memcpy(&pub, pu_key, sizeof(pub));
  if (pub.W[0] == 0x04) {
    if (sys_cx_edward_compress_point(pub.curve, pub.W, pub.W_len) != 0) {
      return 0;
    }
  }

  return ED25519_verify(hash, hash_len, sig, pub.W + 1);
}

int sys_cx_eddsa_sign_no_throw(const cx_ecfp_private_key_t *pvkey,
                      cx_md_t hashID,
                      const unsigned char *hash, unsigned int hash_len,
                      unsigned char *sig, unsigned int sig_len)
{
  uint8_t public_key[32];

  /* Only SHA-512 is supported in Speculos, as it was the only supported hash
   * in OpenSSL */
  if (hashID != CX_SHA512) {
    return CX_INVALID_PARAMETER;
  }
  if (sig_len < 64) {
    return CX_KO;
  }
  /* Key must be a Ed25519 private key */
  if (pvkey->curve != CX_CURVE_Ed25519 || pvkey->d_len != 32) {
    return CX_KO;
  }
  ED25519_public_from_private(public_key, pvkey->d);
  if (ED25519_sign(sig, hash, hash_len, public_key, pvkey->d) == 1) {
    return CX_OK;
  }
  return CX_KO;
}
