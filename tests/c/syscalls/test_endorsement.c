#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <cmocka.h>

#include "../utils.h"

#include "bolos/cx.h"
#include "bolos/endorsement.h"
#include "emulate.h"
#include "environment.h"

typedef struct {
  uint8_t raw_endorsement_pubkey[65];
  uint8_t endorsement_sig[80];
  uint8_t endorsement_key1_sig[80];
  size_t endorsement_key1_sig_len;
  uint8_t pubkey_len;
  uint8_t endorsement_sig_len;
  cx_ecfp_public_key_t endorsement_pubkey;
  cx_ecfp_public_key_t blue_owner_endorsement_pubkey;
  cx_sha256_t sha256;
  uint8_t hash[32];
  uint8_t code_hash[32];

} TestCtx_t;

// test key
static uint8_t BLUE_OWNER_PUBLIC_KEY[] = {
  0x04, 0x41, 0x14, 0xd1, 0x6a, 0xec, 0xed, 0xa8, 0xd7, 0x81, 0x31, 0x1b, 0xd3,
  0x4f, 0xd5, 0x7a, 0x44, 0xac, 0xfc, 0xbc, 0xf3, 0x57, 0xf0, 0x67, 0x88, 0x46,
  0x48, 0x07, 0x32, 0x52, 0x69, 0x9c, 0xbd, 0x46, 0xf6, 0x41, 0xc1, 0x75, 0x54,
  0xed, 0x65, 0x3e, 0x6a, 0x42, 0x7f, 0x4a, 0xea, 0xfa, 0xf7, 0x4e, 0x32, 0x18,
  0xe5, 0xdd, 0x0c, 0x04, 0x20, 0xb6, 0xc8, 0xcf, 0x60, 0x5a, 0x04, 0x96, 0x4e
};

static uint8_t certif_prefix = 0xFE;
static uint8_t nonce[4] = { 0x12, 0x34, 0x56, 0x78 };

static void check_1(TestCtx_t *ctx)
{

  assert_int_equal(ctx->pubkey_len, 65);
  assert_int_equal(ctx->raw_endorsement_pubkey[0], 0x04);

  assert_int_equal(
      sys_cx_ecfp_init_public_key(CX_CURVE_256K1, ctx->raw_endorsement_pubkey,
                                  ctx->pubkey_len, &ctx->endorsement_pubkey),
      ctx->pubkey_len);
  assert_int_equal(
      sys_cx_ecfp_init_public_key(CX_CURVE_256K1, BLUE_OWNER_PUBLIC_KEY,
                                  sizeof(BLUE_OWNER_PUBLIC_KEY),
                                  &ctx->blue_owner_endorsement_pubkey),
      sizeof(BLUE_OWNER_PUBLIC_KEY));

  assert_int_equal(cx_sha256_init(&ctx->sha256), CX_SHA256);
  assert_int_equal(
      sys_cx_hash(&ctx->sha256.header, 0, &certif_prefix, 1, NULL, 0), 0);
  assert_int_equal(
      sys_cx_hash(&ctx->sha256.header, CX_LAST, ctx->raw_endorsement_pubkey,
                  sizeof(ctx->raw_endorsement_pubkey), ctx->hash, 32),
      32);
}

static void check_2(TestCtx_t *ctx)
{

  assert_int_equal(cx_sha256_init(&ctx->sha256), CX_SHA256);
  assert_int_equal(
      sys_cx_hash(&ctx->sha256.header, 0, nonce, sizeof(nonce), NULL, 0), 0);
  assert_int_equal(sys_cx_hash(&ctx->sha256.header, CX_LAST, ctx->code_hash,
                               sizeof(ctx->code_hash), ctx->hash, 32),
                   32);
}

static void test_endorsement_api_level_22(void **state __attribute__((unused)))
{
  TestCtx_t ctx = { 0 };
  init_environment();

  assert_int_equal(0, sys_os_endorsement_get_public_key_new(
                          1, ctx.raw_endorsement_pubkey, &ctx.pubkey_len));
  check_1(&ctx);
  assert_int_equal(0, sys_os_endorsement_get_public_key_certificate_new(
                          1, ctx.endorsement_sig, &ctx.endorsement_sig_len));
  assert_int_equal(sys_cx_ecdsa_verify(&ctx.blue_owner_endorsement_pubkey,
                                       CX_LAST, CX_SHA256, ctx.hash,
                                       sizeof(ctx.hash), ctx.endorsement_sig,
                                       ctx.endorsement_sig_len),
                   1);

  assert_int_equal(sys_os_endorsement_get_code_hash(ctx.code_hash), 32);

  check_2(&ctx);

  ctx.endorsement_key1_sig_len = sys_os_endorsement_key1_sign_data(
      nonce, sizeof(nonce), ctx.endorsement_key1_sig);
  assert_int_equal(sys_cx_ecdsa_verify(&ctx.endorsement_pubkey, CX_LAST,
                                       CX_SHA256, ctx.hash, sizeof(ctx.hash),
                                       ctx.endorsement_key1_sig,
                                       ctx.endorsement_key1_sig_len),
                   1);
};

static void test_endorsement_after_api_level_22(void **state
                                                __attribute__((unused)))
{
  TestCtx_t ctx = { 0 };

  init_environment();

  // -- Check get public key
  assert_int_equal(0, sys_ENDORSEMENT_get_public_key(ENDORSEMENT_SLOT_1,
                                                     ctx.raw_endorsement_pubkey,
                                                     &ctx.pubkey_len));
  check_1(&ctx);

  assert_int_equal(0, sys_ENDORSEMENT_get_public_key_certificate(
                          ENDORSEMENT_SLOT_1, ctx.endorsement_sig,
                          &ctx.endorsement_sig_len));
  assert_int_equal(sys_cx_ecdsa_verify(&ctx.blue_owner_endorsement_pubkey,
                                       CX_LAST, CX_SHA256, ctx.hash,
                                       sizeof(ctx.hash), ctx.endorsement_sig,
                                       ctx.endorsement_sig_len),
                   1);

  // -- Check get code hash
  assert_int_equal(sys_ENDORSEMENT_get_code_hash(ctx.code_hash), 0);

  // -- Check key1 signature
  check_2(&ctx);

  sys_ENDORSEMENT_key1_sign_data(nonce, sizeof(nonce), ctx.endorsement_key1_sig,
                                 &ctx.endorsement_key1_sig_len);
  assert_int_equal(sys_cx_ecdsa_verify(&ctx.endorsement_pubkey, CX_LAST,
                                       CX_SHA256, ctx.hash, sizeof(ctx.hash),
                                       ctx.endorsement_key1_sig,
                                       ctx.endorsement_key1_sig_len),
                   1);
};

static void test_endorsement_after_api_level_25(void **state
                                                __attribute__((unused)))
{
  TestCtx_t ctx = { 0 };
  size_t pubkey_len = 0;
  size_t endorsement_sig_len = 0;

  init_environment();

  // -- Check get public key
  assert_int_equal(0, sys_ENDORSEMENT_GET_PUB_KEY(ENDORSEMENT_SLOT_1,
                                                  ctx.raw_endorsement_pubkey,
                                                  &pubkey_len));
  ctx.pubkey_len = pubkey_len;
  check_1(&ctx);

  assert_int_equal(0, sys_ENDORSEMENT_GET_PUB_KEY_SIG(ENDORSEMENT_SLOT_1,
                                                      ctx.endorsement_sig,
                                                      &endorsement_sig_len));
  ctx.endorsement_sig_len = endorsement_sig_len;
  assert_int_equal(sys_cx_ecdsa_verify(&ctx.blue_owner_endorsement_pubkey,
                                       CX_LAST, CX_SHA256, ctx.hash,
                                       sizeof(ctx.hash), ctx.endorsement_sig,
                                       ctx.endorsement_sig_len),
                   1);

  // -- Check get code hash
  assert_int_equal(sys_ENDORSEMENT_GET_CODE_HASH(ctx.code_hash, 32), 0);
  assert_int_equal(sys_ENDORSEMENT_GET_CODE_HASH(ctx.code_hash, 31), 1);

  // -- Check key1 signature
  check_2(&ctx);

  sys_ENDORSEMENT_KEY1_SIGN_DATA(nonce, sizeof(nonce), ctx.endorsement_key1_sig,
                                 &ctx.endorsement_key1_sig_len);
  assert_int_equal(sys_cx_ecdsa_verify(&ctx.endorsement_pubkey, CX_LAST,
                                       CX_SHA256, ctx.hash, sizeof(ctx.hash),
                                       ctx.endorsement_key1_sig,
                                       ctx.endorsement_key1_sig_len),
                   1);
};

int main(void)
{
  // clang-format off
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_endorsement_api_level_22),
    cmocka_unit_test(test_endorsement_after_api_level_22),
    cmocka_unit_test(test_endorsement_after_api_level_25)
  };
  // clang-format on
  return cmocka_run_group_tests(tests, NULL, NULL);
}
