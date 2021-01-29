#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <cmocka.h>

#include "bolos/cx.h"
#include "bolos/cx_ec.h"
#include "bolos/endorsement.h"
#include "emulate.h"
#include "utils.h"

#define cx_ecfp_init_public_key      sys_cx_ecfp_init_public_key
#define cx_hash                      sys_cx_hash
#define cx_ecdsa_verify              sys_cx_ecdsa_verify
#define os_endorsement_get_code_hash sys_os_endorsement_get_code_hash
#define os_endorsement_get_public_key_certificate                              \
  sys_os_endorsement_get_public_key_certificate
#define os_endorsement_get_public_key sys_os_endorsement_get_public_key
#define os_endorsement_key1_sign_data sys_os_endorsement_key1_sign_data

void test_endorsement(void **state __attribute__((unused)))
{
  uint8_t raw_endorsement_pubkey[65] = { 0 };
  uint8_t endorsement_sig[80] = { 0 };
  uint8_t endorsement_key1_sig[80] = { 0 };
  size_t pubkey_len, endorsement_sig_len, endorsement_key1_sig_len;
  cx_ecfp_public_key_t endorsement_pubkey, blue_owner_endorsement_pubkey;
  cx_sha256_t sha256;
  uint8_t certif_prefix = 0xFE;
  uint8_t hash[32];
  uint8_t code_hash[32];
  uint8_t nonce[4] = { 0x12, 0x34, 0x56, 0x78 };

  // test key
  uint8_t BLUE_OWNER_PUBLIC_KEY[] = {
    0x04, 0x41, 0x14, 0xd1, 0x6a, 0xec, 0xed, 0xa8, 0xd7, 0x81, 0x31,
    0x1b, 0xd3, 0x4f, 0xd5, 0x7a, 0x44, 0xac, 0xfc, 0xbc, 0xf3, 0x57,
    0xf0, 0x67, 0x88, 0x46, 0x48, 0x07, 0x32, 0x52, 0x69, 0x9c, 0xbd,
    0x46, 0xf6, 0x41, 0xc1, 0x75, 0x54, 0xed, 0x65, 0x3e, 0x6a, 0x42,
    0x7f, 0x4a, 0xea, 0xfa, 0xf7, 0x4e, 0x32, 0x18, 0xe5, 0xdd, 0x0c,
    0x04, 0x20, 0xb6, 0xc8, 0xcf, 0x60, 0x5a, 0x04, 0x96, 0x4e
  };

  pubkey_len = os_endorsement_get_public_key(1, raw_endorsement_pubkey);
  assert_int_equal(pubkey_len, 65);
  assert_int_equal(raw_endorsement_pubkey[0], 0x04);

  assert_int_equal(cx_ecfp_init_public_key(CX_CURVE_256K1,
                                           raw_endorsement_pubkey, pubkey_len,
                                           &endorsement_pubkey),
                   pubkey_len);
  assert_int_equal(cx_ecfp_init_public_key(CX_CURVE_256K1,
                                           BLUE_OWNER_PUBLIC_KEY,
                                           sizeof(BLUE_OWNER_PUBLIC_KEY),
                                           &blue_owner_endorsement_pubkey),
                   sizeof(BLUE_OWNER_PUBLIC_KEY));

  assert_int_equal(cx_sha256_init(&sha256), CX_SHA256);
  assert_int_equal(cx_hash(&sha256.header, 0, &certif_prefix, 1, NULL, 0), 0);
  assert_int_equal(cx_hash(&sha256.header, CX_LAST, raw_endorsement_pubkey,
                           sizeof(raw_endorsement_pubkey), hash, 32),
                   32);

  endorsement_sig_len =
      os_endorsement_get_public_key_certificate(1, endorsement_sig);
  assert_int_equal(cx_ecdsa_verify(&blue_owner_endorsement_pubkey, CX_LAST,
                                   CX_SHA256, hash, sizeof(hash),
                                   endorsement_sig, endorsement_sig_len),
                   1);

  assert_int_equal(os_endorsement_get_code_hash(code_hash), 32);

  assert_int_equal(cx_sha256_init(&sha256), CX_SHA256);
  assert_int_equal(cx_hash(&sha256.header, 0, nonce, sizeof(nonce), NULL, 0),
                   0);
  assert_int_equal(
      cx_hash(&sha256.header, CX_LAST, code_hash, sizeof(code_hash), hash, 32),
      32);

  endorsement_key1_sig_len =
      os_endorsement_key1_sign_data(nonce, sizeof(nonce), endorsement_key1_sig);
  assert_int_equal(cx_ecdsa_verify(&endorsement_pubkey, CX_LAST, CX_SHA256,
                                   hash, sizeof(hash), endorsement_key1_sig,
                                   endorsement_key1_sig_len),
                   1);
};

int main(void)
{
  const struct CMUnitTest tests[] = { cmocka_unit_test(test_endorsement) };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
