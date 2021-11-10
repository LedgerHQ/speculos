#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <setjmp.h>
// must come after setjmp.h
#include <cmocka.h>

#include "nist_cavp.h"
#include "utils.h"

#include "bolos/cx.h"
#include "bolos/cx_ec.h"
#include "emulate.h"

#define cx_ecfp_init_private_key sys_cx_ecfp_init_private_key
#define cx_ecdh                  sys_cx_ecdh

enum wycheproof_result { kValid, kInvalid, kAcceptable };

static bool GetWycheproofCurve(const char *curve, cx_curve_t *out)
{
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

static bool GetWycheproofResult(const char *result, enum wycheproof_result *out)
{
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

#define BUF_SIZE        1024
#define MAX_LINE_LENGTH 1024

static void test_wycheproof_vectors(const char *filename)
{
  cx_ecfp_private_key_t priv = { 0 };
  char *line = malloc(MAX_LINE_LENGTH);
  assert_non_null(line);

  FILE *f = fopen(filename, "r");
  assert_non_null(f);

  enum wycheproof_result res = 0;

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
    cx_curve_t curve = 0;

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
    assert_int_equal(hexstr2bin(pos2 + 1, private_key, private_key_len),
                     private_key_len);
    assert_int_equal(hexstr2bin(pos3 + 1, shared, shared_len), shared_len);

    assert_true(GetWycheproofCurve(line, &curve));
    assert_true(GetWycheproofResult(pos4 + 1, &res));

    assert_int_equal(
        cx_ecfp_init_private_key(curve, private_key, private_key_len, &priv),
        private_key_len);

    if (res == kValid) {
      assert_int_equal(cx_ecdh(&priv, CX_ECDH_X, public_key, public_key_len,
                               computed_share, shared_len),
                       shared_len);
      assert_memory_equal(computed_share, shared, shared_len);
    } else if (res == kInvalid) {
      assert_int_equal(cx_ecdh(&priv, CX_ECDH_X, public_key, public_key_len,
                               computed_share, shared_len),
                       0);
    }

    free(computed_share);
    free(shared);
    free(private_key);
    free(public_key);
  }
  fclose(f);
  free(line);
}

void test_ecdh_overflow(void **state)
{
  cx_ecfp_640_private_key_t private_key;
  (void)state;
  uint8_t raw_private_key[66] = {
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11
  };

  /* clang-format off */
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
  /* clang-format on */

  size_t key_len = sizeof(raw_private_key);
  uint8_t secret[1 + 66 + 66];

  assert_int_equal(
      cx_ecfp_init_private_key(CX_CURVE_SECP521R1, raw_private_key, key_len,
                               (cx_ecfp_private_key_t *)&private_key),
      key_len);
  cx_ecdh((cx_ecfp_private_key_t *)&private_key, CX_ECDH_POINT, p, sizeof(p),
          secret, sizeof(secret));
}

void test_wycheproof_ecdh_secp256k1(void **state)
{
  (void)state;
  test_wycheproof_vectors(TESTS_PATH "wycheproof/ecdh_secp256k1.data");
}

static void test_ecdh_secp256k1(void **state)
{
  (void)state;
  uint8_t raw_public_key[65];
  uint8_t raw_private_key[32];
  uint8_t secret[32];
  cx_ecfp_private_key_t private_key;

  assert_int_equal(
      hexstr2bin(
          "04d8096af8a11e0b80037e1ee68246b5dcbb0aeb1cf1244fd767db80f3fa27da2b39"
          "6812ea1686e7472e9692eaf3e958e50e9500d3b4c77243db1f2acd67ba9cc4",
          raw_public_key, 65),
      65);
  assert_int_equal(
      hexstr2bin(
          "f4b7ff7cccc98813a69fae3df222bfe3f4e28f764bf91b4a10d8096ce446b254",
          raw_private_key, 32),
      32);
  assert_int_equal(cx_ecfp_init_private_key(CX_CURVE_SECP256K1, raw_private_key,
                                            32, &private_key),
                   32);
  assert_int_equal(cx_ecdh(&private_key, CX_ECDH_X, raw_public_key,
                           sizeof(raw_public_key), secret, sizeof(secret)),
                   32);

  hexstr2bin(
      "0432BDD978EB62B1F369A56D0949AB8551A7AD527D9602E891CE457586C2A8569E981E67"
      "FAE053B03FC33E1A291F0A3BEB58FCEB2E85BB1205DACEE1232DFD316B",
      raw_public_key, 64 + 1);
  hexstr2bin("fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd03640c3",
             raw_private_key, 32);
  assert_int_equal(cx_ecfp_init_private_key(CX_CURVE_SECP256K1, raw_private_key,
                                            32, &private_key),
                   32);
  assert_int_equal(cx_ecdh(&private_key, CX_ECDH_X, raw_public_key, 64 + 1,
                           secret, sizeof(secret)),
                   32);
}

static void verify_point_equals(const uint8_t *point, const uint8_t *x,
                                const uint8_t *y, size_t len)
{
  assert_int_equal(point[0], 0x04);
  assert_memory_equal(point + 1, x, len);
  assert_memory_equal(point + len + 1, y, len);
}

typedef struct {
  cx_curve_t curve;
  const char *dA;
  const char *x_qA;
  const char *y_qA;
  const char *dB;
  const char *x_qB;
  const char *y_qB;
  const char *x_Z;
  const char *y_Z;
} rfc7027_test_vectors;

#define MAX_DOMAIN_LEN (512 / 8)

static void test_ecdh_brainpool(void **state)
{
  (void)state;

  const rfc7027_test_vectors tvs[] = {
    {
        CX_CURVE_BrainPoolP256R1,
        "81DB1EE100150FF2EA338D708271BE38300CB54241D79950F77B063039804F1D",
        "44106E913F92BC02A1705D9953A8414DB95E1AAA49E81D9E85F929A8E3100BE5",
        "8AB4846F11CACCB73CE49CBDD120F5A900A69FD32C272223F789EF10EB089BDC",
        "55E40BC41E37E3E2AD25C3C6654511FFA8474A91A0032087593852D3E7D76BD3",
        "8D2D688C6CF93E1160AD04CC4429117DC2C41825E1E9FCA0ADDD34E6F1B39F7B",
        "990C57520812BE512641E47034832106BC7D3E8DD0E4C7F1136D7006547CEC6A",
        "89AFC39D41D3B327814B80940B042590F96556EC91E6AE7939BCE31F3A18BF2B",
        "49C27868F4ECA2179BFD7D59B1E3BF34C1DBDE61AE12931648F43E59632504DE",
    },
    {
        CX_CURVE_BrainPoolP384R1,
        "1E20F5E048A5886F1F157C74E91BDE2B98C8B52D58E5003D57053FC4B0BD65D6F15EB5"
        "D1EE1610DF870795143627D042",
        "68B665DD91C195800650CDD363C625F4E742E8134667B767B1B476793588F885AB698C"
        "852D4A6E77A252D6380FCAF068",
        "55BC91A39C9EC01DEE36017B7D673A931236D2F1F5C83942D049E3FA20607493E0D038"
        "FF2FD30C2AB67D15C85F7FAA59",
        "032640BC6003C59260F7250C3DB58CE647F98E1260ACCE4ACDA3DD869F74E01F8BA5E0"
        "324309DB6A9831497ABAC96670",
        "4D44326F269A597A5B58BBA565DA5556ED7FD9A8A9EB76C25F46DB69D19DC8CE6AD18E"
        "404B15738B2086DF37E71D1EB4",
        "62D692136DE56CBE93BF5FA3188EF58BC8A3A0EC6C1E151A21038A42E9185329B5B275"
        "903D192F8D4E1F32FE9CC78C48",
        "0BD9D3A7EA0B3D519D09D8E48D0785FB744A6B355E6304BC51C229FBBCE239BBADF640"
        "3715C35D4FB2A5444F575D4F42",
        "0DF213417EBE4D8E40A5F76F66C56470C489A3478D146DECF6DF0D94BAE9E598157290"
        "F8756066975F1DB34B2324B7BD",
    },
    {
        CX_CURVE_BrainPoolP512R1,
        "16302FF0DBBB5A8D733DAB7141C1B45ACBC8715939677F6A56850A38BD87BD59B09E80"
        "279609FF333EB9D4C061231FB26F92EEB04982A5F1D1764CAD57665422",
        "0A420517E406AAC0ACDCE90FCD71487718D3B953EFD7FBEC5F7F27E28C6149999397E9"
        "1E029E06457DB2D3E640668B392C2A7E737A7F0BF04436D11640FD09FD",
        "72E6882E8DB28AAD36237CD25D580DB23783961C8DC52DFA2EC138AD472A0FCEF3887C"
        "F62B623B2A87DE5C588301EA3E5FC269B373B60724F5E82A6AD147FDE7",
        "230E18E1BCC88A362FA54E4EA3902009292F7F8033624FD471B5D8ACE49D12CFABBC19"
        "963DAB8E2F1EBA00BFFB29E4D72D13F2224562F405CB80503666B25429",
        "9D45F66DE5D67E2E6DB6E93A59CE0BB48106097FF78A081DE781CDB31FCE8CCBAAEA8D"
        "D4320C4119F1E9CD437A2EAB3731FA9668AB268D871DEDA55A5473199F",
        "2FDC313095BCDD5FB3A91636F07A959C8E86B5636A1E930E8396049CB481961D365CC1"
        "1453A06C719835475B12CB52FC3C383BCE35E27EF194512B71876285FA",
        "A7927098655F1F9976FA50A9D566865DC530331846381C87256BAF3226244B76D36403"
        "C024D7BBF0AA0803EAFF405D3D24F11A9B5C0BEF679FE1454B21C4CD1F",
        "7DB71C3DEF63212841C463E881BDCF055523BD368240E6C3143BD8DEF8B3B3223B95E0"
        "F53082FF5E412F4222537A43DF1C6D25729DDB51620A832BE6A26680A2",
    }
  };

  for (size_t i = 0; i < sizeof(tvs) / sizeof(tvs[0]); i++) {
    const rfc7027_test_vectors *tv = &tvs[i];

    uint8_t dA[MAX_DOMAIN_LEN], dB[MAX_DOMAIN_LEN];
    uint8_t x_qA[MAX_DOMAIN_LEN], y_qA[MAX_DOMAIN_LEN], x_qB[MAX_DOMAIN_LEN];
    uint8_t y_qB[MAX_DOMAIN_LEN], x_Z[MAX_DOMAIN_LEN], y_Z[MAX_DOMAIN_LEN];
    uint8_t secret1[2 * MAX_DOMAIN_LEN + 1], secret2[2 * MAX_DOMAIN_LEN + 1];

    cx_ecfp_512_private_key_t priv1, priv2;
    cx_ecfp_512_public_key_t pub1, pub2;
    const cx_curve_domain_t *domain = cx_ecfp_get_domain(tv->curve);
    const size_t len = domain->length;

    assert_int_equal(hexstr2bin(tv->dA, dA, len), len);
    assert_int_equal(hexstr2bin(tv->x_qA, x_qA, len), len);
    assert_int_equal(hexstr2bin(tv->y_qA, y_qA, len), len);
    assert_int_equal(hexstr2bin(tv->dB, dB, len), len);
    assert_int_equal(hexstr2bin(tv->x_qB, x_qB, len), len);
    assert_int_equal(hexstr2bin(tv->y_qB, y_qB, len), len);
    assert_int_equal(hexstr2bin(tv->x_Z, x_Z, len), len);
    assert_int_equal(hexstr2bin(tv->y_Z, y_Z, len), len);

    assert_int_equal(cx_ecfp_init_private_key(tv->curve, dA, len,
                                              (cx_ecfp_private_key_t *)&priv1),
                     len);
    assert_int_equal(
        sys_cx_ecfp_generate_pair(tv->curve, (cx_ecfp_public_key_t *)&pub1,
                                  (cx_ecfp_private_key_t *)&priv1, 1),
        0);
    verify_point_equals(pub1.W, x_qA, y_qA, len);

    assert_int_equal(cx_ecfp_init_private_key(tv->curve, dB, len,
                                              (cx_ecfp_private_key_t *)&priv2),
                     len);
    assert_int_equal(
        sys_cx_ecfp_generate_pair(tv->curve, (cx_ecfp_public_key_t *)&pub2,
                                  (cx_ecfp_private_key_t *)&priv2, 1),
        0);
    verify_point_equals(pub2.W, x_qB, y_qB, len);

    assert_int_equal(cx_ecdh((cx_ecfp_private_key_t *)&priv1, CX_ECDH_POINT,
                             pub2.W, pub2.W_len, secret1, 2 * len + 1),
                     2 * len + 1);
    verify_point_equals(secret1, x_Z, y_Z, len);

    assert_int_equal(cx_ecdh((cx_ecfp_private_key_t *)&priv2, CX_ECDH_POINT,
                             pub1.W, pub1.W_len, secret2, 2 * len + 1),
                     2 * len + 1);
    verify_point_equals(secret2, x_Z, y_Z, len);
  }
}

int main(void)
{
  // cx_init();

  const struct CMUnitTest tests[] = {
    // cmocka_unit_test(test_ecdh_overflow),
    cmocka_unit_test(test_ecdh_brainpool),
    cmocka_unit_test(test_ecdh_secp256k1),
    cmocka_unit_test(test_wycheproof_ecdh_secp256k1)
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
