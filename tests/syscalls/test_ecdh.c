#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdbool.h>

#include <setjmp.h>
#include <cmocka.h>

#include "nist_cavp.h"
#include "utils.h"

#include "cx.h"
#include "cx_ec.h"
#include "emulate.h"

#define cx_ecfp_init_private_key  sys_cx_ecfp_init_private_key
#define cx_ecdh sys_cx_ecdh

enum wycheproof_result { kValid, kInvalid, kAcceptable };

static bool GetWycheproofCurve(const char *curve, cx_curve_t *out) {
  if (strcmp(curve, "secp256k1") == 0) {
    *out = CX_CURVE_SECP256K1;
  } else if (strcmp(curve, "secp256r1") == 0) {
    *out = CX_CURVE_SECP256R1;
  } else if (strcmp(curve, "secp384r1") == 0) {
    *out = CX_CURVE_SECP384R1;
  } else if (strcmp(curve, "secp521r1") == 0) {
    *out = CX_CURVE_SECP521R1;
  } else if (strcmp(curve, "brainpoolP256r1") == 0) {
    *out = CX_CURVE_BrainPoolP256R1;
  } else if (strcmp(curve, "brainpoolP320r1") == 0) {
    *out = CX_CURVE_BrainPoolP320R1;
  } else if (strcmp(curve, "brainpoolP384r1") == 0) {
    *out = CX_CURVE_BrainPoolP384R1;
  } else if (strcmp(curve, "brainpoolP512r1") == 0) {
    *out = CX_CURVE_BrainPoolP512R1;
  } else if (strcmp(curve, "edwards25519") == 0) {
    *out = CX_CURVE_Ed25519;
  } else {
    printf("%s\n", curve);
    return false;
  }
  return true;
}

static bool GetWycheproofResult(const char *result,
                                enum wycheproof_result *out) {
  if (strcmp(result, "valid") == 0) {
    *out = kValid;
  } else if (strcmp(result, "invalid") == 0) {
    *out = kInvalid;
  } else if (strcmp(result, "acceptable") == 0) {
    *out = kAcceptable;
  } else {
    return false;
  }
  return true;
}

#define BUF_SIZE 1024
#define MAX_LINE_LENGTH 1024

static void test_wycheproof_vectors(const char *filename) {
  cx_ecfp_private_key_t priv = {0};
  char *line = malloc(MAX_LINE_LENGTH);
  assert_non_null(line);

  FILE *f = fopen(filename, "r");
  assert_non_null(f);

  enum wycheproof_result res;

  // Format:
  // curve : public_key : private key : shared : result
  while (fgets(line, MAX_LINE_LENGTH, f) != NULL) {
    char *pos1 = strchr(line, ':');
    assert_non_null(pos1);
    char *pos2 = strchr(pos1 + 1, ':');
    assert_non_null(pos2);
    char *pos3 = strchr(pos2 + 1, ':');
    assert_non_null(pos3);
    char *pos4 = strchr(pos3 + 1, ':');
    assert_non_null(pos4);
    char *pos5 = strchr(pos4 + 1, '\n');

    *pos1 = *pos2 = *pos3 = *pos4 = 0;
    if (pos5) {
      *pos5 = 0;
      if (*(pos5 - 1) == '\r') {
        *(pos5 - 1) = 0;
      }
    }

    size_t public_key_len = strlen(pos1 + 1) / 2;
    size_t private_key_len = strlen(pos2 + 1) / 2;
    size_t shared_len = strlen(pos3 + 1) / 2;
    cx_curve_t curve;

    uint8_t *public_key = malloc(public_key_len);
    assert_non_null(public_key);
    uint8_t *private_key = malloc(private_key_len);
    assert_non_null(private_key);
    uint8_t *shared = malloc(shared_len);
    assert_non_null(shared);
    uint8_t *computed_share = malloc(shared_len);
    assert_non_null(computed_share);

    assert_int_equal(hexstr2bin(pos1 + 1, public_key, public_key_len),
                     public_key_len);
    assert_int_equal(hexstr2bin(pos2 + 1, private_key, private_key_len), private_key_len);
    assert_int_equal(hexstr2bin(pos3 + 1, shared, shared_len), shared_len);

    assert_true(GetWycheproofCurve(line, &curve));
    assert_true(GetWycheproofResult(pos4 + 1, &res));

    assert_int_equal(cx_ecfp_init_private_key(curve, private_key, private_key_len, &priv), private_key_len);

    if (res == kValid) {
      assert_int_equal(cx_ecdh(&priv, CX_ECDH_X, public_key, public_key_len, computed_share, shared_len), shared_len);
      assert_memory_equal(computed_share, shared, shared_len);
    } else if (res == kInvalid) {
      assert_int_equal(cx_ecdh(&priv, CX_ECDH_X, public_key, public_key_len, computed_share, shared_len), 0);
    }

    free(computed_share);
    free(shared);
    free(private_key);
    free(public_key);
  }
  fclose(f);
  free(line);
}

void test_ecdh_overflow(void **state) {
  cx_ecfp_640_private_key_t private_key;
  (void)state;
  uint8_t raw_private_key[66] = {
      0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
      0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
      0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
      0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
      0x11, 0x11
  };
  const uint8_t p[1 + 66 + 66] = {
      0x04,
      0x00, 0xC6,
      0x85, 0x8E, 0x06, 0xB7, 0x04, 0x04, 0xE9, 0xCD, 0x9E, 0x3E, 0xCB, 0x66, 0x23, 0x95, 0xB4, 0x42, // gx
      0x9C, 0x64, 0x81, 0x39, 0x05, 0x3F, 0xB5, 0x21, 0xF8, 0x28, 0xAF, 0x60, 0x6B, 0x4D, 0x3D, 0xBA,
      0xA1, 0x4B, 0x5E, 0x77, 0xEF, 0xE7, 0x59, 0x28, 0xFE, 0x1D, 0xC1, 0x27, 0xA2, 0xFF, 0xA8, 0xDE,
      0x33, 0x48, 0xB3, 0xC1, 0x85, 0x6A, 0x42, 0x9B, 0xF9, 0x7E, 0x7E, 0x31, 0xC2, 0xE5, 0xBD, 0x66,
      0x01, 0x18,
      0x39, 0x29, 0x6A, 0x78, 0x9A, 0x3B, 0xC0, 0x04, 0x5C, 0x8A, 0x5F, 0xB4, 0x2C, 0x7D, 0x1B, 0xD9, // gy
      0x98, 0xF5, 0x44, 0x49, 0x57, 0x9B, 0x44, 0x68, 0x17, 0xAF, 0xBD, 0x17, 0x27, 0x3E, 0x66, 0x2C,
      0x97, 0xEE, 0x72, 0x99, 0x5E, 0xF4, 0x26, 0x40, 0xC5, 0x50, 0xB9, 0x01, 0x3F, 0xAD, 0x07, 0x61,
      0x35, 0x3C, 0x70, 0x86, 0xA2, 0x72, 0xC2, 0x40, 0x88, 0xBE, 0x94, 0x76, 0x9F, 0xD1, 0x66, 0x50
  };
  size_t key_len = sizeof(raw_private_key);
  uint8_t secret[1 + 66 + 66];

  assert_int_equal(cx_ecfp_init_private_key(CX_CURVE_SECP521R1,
                                            raw_private_key,
                                            key_len,
                                            (cx_ecfp_private_key_t *) &private_key), key_len);
  cx_ecdh((cx_ecfp_private_key_t *) &private_key, CX_ECDH_POINT, p, sizeof(p), secret, sizeof(secret));
}

void test_wycheproof_ecdh_secp256k1(void **state) {
  (void)state;
  test_wycheproof_vectors(TESTS_PATH "wycheproof/ecdh_secp256k1.data");
}

static void test_ecdh_secp256k1(void **state) {
  (void)state;
  uint8_t raw_public_key[65];
  uint8_t raw_private_key[32];
  uint8_t secret[32];
  cx_ecfp_private_key_t private_key;

  assert_int_equal(hexstr2bin("04d8096af8a11e0b80037e1ee68246b5dcbb0aeb1cf1244fd767db80f3fa27da2b396812ea1686e7472e9692eaf3e958e50e9500d3b4c77243db1f2acd67ba9cc4", raw_public_key, 65), 65);
  assert_int_equal(hexstr2bin("f4b7ff7cccc98813a69fae3df222bfe3f4e28f764bf91b4a10d8096ce446b254", raw_private_key, 32), 32);
  assert_int_equal(cx_ecfp_init_private_key(CX_CURVE_SECP256K1, raw_private_key, 32, &private_key), 32);
  assert_int_equal(cx_ecdh(&private_key, CX_ECDH_X, raw_public_key, sizeof(raw_public_key), secret, sizeof(secret)), 32);

  hexstr2bin("0432BDD978EB62B1F369A56D0949AB8551A7AD527D9602E891CE457586C2A8569E981E67FAE053B03FC33E1A291F0A3BEB58FCEB2E85BB1205DACEE1232DFD316B", raw_public_key, 64 + 1);
  hexstr2bin("fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd03640c3", raw_private_key, 32);
  assert_int_equal(cx_ecfp_init_private_key(CX_CURVE_SECP256K1, raw_private_key, 32, &private_key), 32);
  assert_int_equal(cx_ecdh(&private_key, CX_ECDH_X, raw_public_key, 64 + 1, secret, sizeof(secret)), 32);
}

int main(void) {
  //cx_init();

  const struct CMUnitTest tests[] = {
      // cmocka_unit_test(test_ecdh_overflow),
      cmocka_unit_test(test_ecdh_secp256k1),
      cmocka_unit_test(test_wycheproof_ecdh_secp256k1)
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
