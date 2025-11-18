#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <setjmp.h>
// must come after setjmp.h
#include <cmocka.h>

#define _SDK_2_0_

#include "../utils.h"

#include "bolos/cx.h"
#include "bolos/cxlib.h"

#define WYCH_MAX_LINE_LENGTH  (360)
#define MONT_CURVE_MAX_LENGTH (56)

typedef struct {
  const char *x;
  const char *y;
  cx_curve_t curve;
  int size;
  bool is_on_curve;

} point_on_curve_test_vector;

typedef struct {
  const char *u;
  const char *scalar;
  cx_curve_t curve;
  size_t size;
  const char *ru;

} x_function_test_vector_t;

static const point_on_curve_test_vector bolos_test_vectors[] = {
  { .x = "0c91822b3af46baff67f105222cfc339e56b1ecedd621ce4a2fad1a370e68049",
    .y = "bda2986e682d0f9bbd03d88a71ef409416336fb66095d9b2d3f5251a99fd777c",
    .curve = CX_CURVE_SECP256K1,
    .size = 32,
    .is_on_curve = true },
  { .x = "0c91822b3af46baff67f105222cfc339e56b1ecedd621ce4a2fad1a370e68049",
    .y = "425d679197d2f06442fc27758e10bf6be9cc90499f6a264d2c0adae4660284b3",
    .curve = CX_CURVE_SECP256K1,
    .size = 32,
    .is_on_curve = true },
  { .x = "973a48effe43e680ecc935ac1ac333e272db313867e3577476625124cc8d990f",
    .y = "f200a8831fcdebf8fc32b4374bc56c48d816f221368c4b26050524b63cc325e9",
    .curve = CX_CURVE_SECP256R1,
    .size = 32,
    .is_on_curve = true },
  { .x = "973a48effe43e680ecc935ac1ac333e272db313867e3577476625124cc8d990f",
    .y = "0dff577be032140803cd4bc8b43a93b727e90ddfc973b4d9fafadb49c33cda16",
    .curve = CX_CURVE_SECP256R1,
    .size = 32,
    .is_on_curve = true },
  { .x = "0033de692003bee7509041a9e0e39d8f3de9c9514b5b1c594af956f92e7e90a89c250"
         "f7f88216be23853a192728250c1",
    .y = "02f2455940a282914d044dffe2628e304523ca2d82a504888b72fc817a2f682522677"
         "77c2395c360928d7db668da6599",
    .curve = CX_CURVE_SECP384R1,
    .size = 48,
    .is_on_curve = true },
  { .x = "0033de692003bee7509041a9e0e39d8f3de9c9514b5b1c594af956f92e7e90a89c250"
         "f7f88216be23853a192728250c1",
    .y = "fd0dbaa6bf5d7d6eb2fbb2001d9d71cfbadc35d27d5afb77748d037e85d097d9dd988"
         "882dc6a3c9f6d72824a97259a66",
    .curve = CX_CURVE_SECP384R1,
    .size = 48,
    .is_on_curve = true }
};

/* https://www.rfc-editor.org/rfc/rfc7748 test vector */

/* clang-format off */
static const x_function_test_vector_t x25519_test_vectors[] = {
  { .u = "e6db6867583030db3594c1a424b15f7c726624ec26b3353b10a903a6d0ab1c4c",
    .scalar = "a546e36bf0527c9d3b16154b82465edd62144c0ac1fc5a18506a2244ba449ac4",
    .curve = CX_CURVE_Curve25519,
    .size = 32,
    .ru = "c3da55379de9c6908e94ea4df28d084f32eccf03491c71f754b4075577a28552" }
};
/* clang-format on */

/* clang-format off */
static const x_function_test_vector_t x448_test_vectors[] = {
  { .u = "06fce640fa3487bfda5f6cf2d5263f8aad88334cbd07437f020f08f9"
         "814dc031ddbdc38c19c6da2583fa5429db94ada18aa7a7fb4ef8a086",
    .scalar = "3d262fddf9ec8e88495266fea19a34d28882acef045104d0d1aae121"
              "700a779c984c24f8cdd78fbff44943eba368f54b29259a4f1c600ad3",
    .curve = CX_CURVE_Curve448,
    .size = 56,
    .ru = "ce3e4ff95a60dc6697da1db1d85e6afbdf79b50a2412d7546d5f239f"
          "e14fbaadeb445fc66a01b0779d98223961111e21766282f73dd96b6f" }
};
/* clang-format on */

static void test_vector(const point_on_curve_test_vector *v)
{
  cx_ecpoint_t P;
  uint8_t x[64], y[64];
  bool res = false;

  assert_int_equal(hexstr2bin(v->x, x, v->size), v->size);
  assert_int_equal(hexstr2bin(v->y, y, v->size), v->size);
  assert_int_equal(sys_cx_bn_lock(v->size, 0), CX_OK);
  assert_int_equal(sys_cx_ecpoint_alloc(&P, v->curve), CX_OK);
  assert_int_equal(sys_cx_ecpoint_init(&P, x, v->size, y, v->size), CX_OK);
  assert_int_equal(sys_cx_ecpoint_is_on_curve(&P, &res), CX_OK);
  assert_int_equal(sys_cx_bn_unlock(), CX_OK);
  assert_int_equal(v->is_on_curve, res);
}

static void reverse_buffer(uint8_t *rev, uint8_t *buf, size_t buf_len)
{
  for (size_t i = 0; i < buf_len; i++) {
    rev[buf_len - 1 - i] = buf[i];
  }
}

static void test_x_function_vector(const x_function_test_vector_t *v)
{
  uint8_t u[MONT_CURVE_MAX_LENGTH], scalar[MONT_CURVE_MAX_LENGTH];
  uint8_t expected_u[MONT_CURVE_MAX_LENGTH], res_u[MONT_CURVE_MAX_LENGTH];
  cx_bn_t bn_u;
  assert_int_equal(hexstr2bin(v->u, u, v->size), v->size);
  assert_int_equal(hexstr2bin(v->scalar, scalar, v->size), v->size);
  assert_int_equal(hexstr2bin(v->ru, expected_u, v->size), v->size);
  assert_int_equal(sys_cx_bn_lock(v->size, 0), CX_OK);
  assert_int_equal(sys_cx_bn_alloc_init(&bn_u, v->size, u, v->size), CX_OK);
  if (v->curve == CX_CURVE_Curve25519) {
    assert_int_equal(sys_cx_ecpoint_x25519(bn_u, scalar, v->size), CX_OK);
  } else if (v->curve == CX_CURVE_Curve448) {
    assert_int_equal(sys_cx_ecpoint_x448(bn_u, scalar, v->size), CX_OK);
  }
  assert_int_equal(sys_cx_bn_export(bn_u, u, v->size), CX_OK);
  assert_int_equal(sys_cx_bn_unlock(), CX_OK);
  reverse_buffer(res_u, u, v->size);
  assert_memory_equal(expected_u, res_u, v->size);
}

static bool GetWycheproofCurve(const char *curve, cx_curve_t *out)
{
  if (strcmp(curve, "curve25519") == 0) {
    *out = CX_CURVE_Curve25519;
  } else if (strcmp(curve, "curve448") == 0) {
    *out = CX_CURVE_Curve448;
  } else {
    printf("Unknown %s curve\n", curve);
    return false;
  }
  return true;
}

static void test_wycheproof_vectors(const char *filename)
{
  char *line = malloc(WYCH_MAX_LINE_LENGTH);
  assert_non_null(line);

  FILE *f = fopen(filename, "r");
  assert_non_null(f);
  size_t u_len, scalar_len, res_u_len;
  x_function_test_vector_t v;

  // curve : in u : scalar : out u : result
  while (fgets(line, WYCH_MAX_LINE_LENGTH, f) != NULL) {
    char *param1 = strchr(line, ':');
    assert_non_null(param1);
    char *param2 = strchr(param1 + 1, ':');
    assert_non_null(param2);
    char *param3 = strchr(param2 + 1, ':');
    assert_non_null(param3);
    char *param4 = strchr(param3 + 1, ':');
    assert_non_null(param4);
    char *param5 = strchr(param4 + 1, ':');
    assert_non_null(param5);

    *param1 = *param2 = *param3 = *param4 = *param5 = 0;

    u_len = strlen(param1 + 1) / 2;
    scalar_len = strlen(param2 + 1) / 2;
    res_u_len = strlen(param3 + 1) / 2;
    assert_int_equal(u_len, scalar_len);
    assert_int_equal(scalar_len, res_u_len);

    v.u = param1 + 1;
    v.scalar = param2 + 1;
    v.ru = param3 + 1;
    assert_true(GetWycheproofCurve(line, &v.curve));
    v.size = u_len;

    test_x_function_vector(&v);
  }
  fclose(f);
  free(line);
}

static void test_is_on_curve(void **state __attribute__((unused)))
{
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE(bolos_test_vectors); i++) {
    test_vector(&bolos_test_vectors[i]);
  }
}

static void test_x25519(void **state __attribute__((unused)))
{
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE(x25519_test_vectors); i++) {
    test_x_function_vector(&x25519_test_vectors[i]);
  }
}

static void test_x25519_wycheproof(void **state __attribute__((unused)))
{
  test_wycheproof_vectors(TESTS_PATH "wycheproof/X25519.data");
}

static void test_x448(void **state __attribute__((unused)))
{
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE(x448_test_vectors); i++) {
    test_x_function_vector(&x448_test_vectors[i]);
  }
}

static void test_x448_wycheproof(void **state __attribute__((unused)))
{
  test_wycheproof_vectors(TESTS_PATH "wycheproof/X448.data");
}

static void test_ecpoint_scalarmul_bls12377(void **state
                                            __attribute__((unused)))
{
  // 0x0400301f10f206c1dd19c602e3c69e47b580033eed2c8ca091bfb0ec4a849aa27b3dad23c63e32c792d93da90756a6abad
  //     008ca7cd476c934c5d16ea76cf6c50490bf266d82b845a0d52e653b9367e64a068c2f75dd056dd0432d379e74e1a6818
  uint8_t point_in[] = { 0x04, 0x00, 0x30, 0x1f, 0x10, 0xf2, 0x06, 0xc1, 0xdd,
                         0x19, 0xc6, 0x02, 0xe3, 0xc6, 0x9e, 0x47, 0xb5, 0x80,
                         0x03, 0x3e, 0xed, 0x2c, 0x8c, 0xa0, 0x91, 0xbf, 0xb0,
                         0xec, 0x4a, 0x84, 0x9a, 0xa2, 0x7b, 0x3d, 0xad, 0x23,
                         0xc6, 0x3e, 0x32, 0xc7, 0x92, 0xd9, 0x3d, 0xa9, 0x07,
                         0x56, 0xa6, 0xab, 0xad, 0x00, 0x8c, 0xa7, 0xcd, 0x47,
                         0x6c, 0x93, 0x4c, 0x5d, 0x16, 0xea, 0x76, 0xcf, 0x6c,
                         0x50, 0x49, 0x0b, 0xf2, 0x66, 0xd8, 0x2b, 0x84, 0x5a,
                         0x0d, 0x52, 0xe6, 0x53, 0xb9, 0x36, 0x7e, 0x64, 0xa0,
                         0x68, 0xc2, 0xf7, 0x5d, 0xd0, 0x56, 0xdd, 0x04, 0x32,
                         0xd3, 0x79, 0xe7, 0x4e, 0x1a, 0x68, 0x18 };
  uint8_t expected_out[] = {
    0x04, 0x01, 0x1d, 0x5b, 0x16, 0x82, 0xa5, 0x4c, 0x5c, 0x3c, 0x4b,
    0x23, 0x4a, 0x0c, 0x97, 0x62, 0x9e, 0x0c, 0xa8, 0xc8, 0x9e, 0x39,
    0xcd, 0xf6, 0x46, 0x5c, 0xeb, 0x8d, 0x56, 0x21, 0xf8, 0x64, 0x7c,
    0xf8, 0xa9, 0x80, 0x02, 0x47, 0x22, 0xfb, 0x9e, 0xf1, 0x36, 0xb8,
    0x47, 0x4d, 0x59, 0xaa, 0xc5, 0x00, 0xe0, 0x13, 0x81, 0x5a, 0x70,
    0xf2, 0x79, 0x41, 0xb8, 0x24, 0x0f, 0xeb, 0x71, 0x41, 0x69, 0xcf,
    0xd2, 0x8d, 0xaf, 0xe0, 0xde, 0x1e, 0xb4, 0xf2, 0x1e, 0x03, 0x08,
    0x9b, 0x7e, 0xca, 0x79, 0x62, 0x04, 0x0e, 0xba, 0x7c, 0x12, 0xf3,
    0x32, 0x67, 0x69, 0xdf, 0x3f, 0xd8, 0xff, 0xe8, 0xde
  };
  // 0x016f173581473e6058e85e5df2dc806324bb4cb20b99b044ecb66ccbb97391ea2cb7144d60bfd3692e115d524a30a921
  uint8_t scalar[] = { 0x01, 0x6f, 0x17, 0x35, 0x81, 0x47, 0x3e, 0x60,
                       0x58, 0xe8, 0x5e, 0x5d, 0xf2, 0xdc, 0x80, 0x63,
                       0x24, 0xbb, 0x4c, 0xb2, 0x0b, 0x99, 0xb0, 0x44,
                       0xec, 0xb6, 0x6c, 0xcb, 0xb9, 0x73, 0x91, 0xea,
                       0x2c, 0xb7, 0x14, 0x4d, 0x60, 0xbf, 0xd3, 0x69,
                       0x2e, 0x11, 0x5d, 0x52, 0x4a, 0x30, 0xa9, 0x21 };

  uint8_t point_out[97] = { 0 };

  cx_ecpoint_t P;

  assert_int_equal(sys_cx_bn_lock(BLS12_377_SIZE_u8, 0), CX_OK);
  assert_int_equal(sys_cx_ecpoint_alloc(&P, CX_CURVE_BLS12_377_G1), CX_OK);
  assert_int_equal(sys_cx_ecpoint_init(&P, point_in + 1, BLS12_377_SIZE_u8,
                                       point_in + 1 + BLS12_377_SIZE_u8,
                                       BLS12_377_SIZE_u8),
                   CX_OK);
  assert_int_equal(sys_cx_ecpoint_scalarmul(&P, scalar, sizeof(scalar)), CX_OK);
  point_out[0] = 0x04;
  assert_int_equal(sys_cx_ecpoint_export(&P, point_out + 1, BLS12_377_SIZE_u8,
                                         point_out + 1 + BLS12_377_SIZE_u8,
                                         BLS12_377_SIZE_u8),
                   CX_OK);
  assert_int_equal(sys_cx_bn_unlock(), CX_OK);
  assert_memory_equal(expected_out, point_out, sizeof(expected_out));
}

static void test_ecpoint_add_bls12377(void **state __attribute__((unused)))
{
  uint8_t p[] = { 0x00, 0x52, 0x83, 0xfe, 0xfa, 0x59, 0x9b, 0xe7, 0x01, 0x1f,
                  0xc1, 0x3a, 0x00, 0x81, 0xba, 0x69, 0x93, 0xa1, 0x38, 0xbf,
                  0xd7, 0x7d, 0xa0, 0xb4, 0x9e, 0xd6, 0x66, 0xe9, 0xe7, 0x2e,
                  0xf5, 0x42, 0xab, 0x20, 0x92, 0xa7, 0x17, 0x24, 0x0d, 0x63,
                  0x74, 0x98, 0xf1, 0x54, 0x1f, 0xc6, 0xcd, 0x72, 0x01, 0x10,
                  0x25, 0xa4, 0x79, 0xad, 0xc5, 0xa0, 0x4e, 0x76, 0xe2, 0x35,
                  0x84, 0x76, 0x2b, 0x7b, 0x7b, 0x72, 0x50, 0x33, 0x5b, 0xff,
                  0xf9, 0x56, 0x65, 0xa8, 0x8c, 0x9b, 0x18, 0x8a, 0x1b, 0xb7,
                  0x5a, 0x07, 0x01, 0x34, 0x15, 0x0e, 0x80, 0x36, 0x04, 0x15,
                  0x97, 0xc8, 0x4c, 0x33, 0xf0, 0x59 };
  uint8_t q[] = { 0x00, 0x85, 0xb6, 0x97, 0x26, 0x5c, 0x00, 0x49, 0x64, 0xac,
                  0x85, 0x48, 0x00, 0x2b, 0x6e, 0x79, 0x7c, 0xfb, 0x25, 0x1b,
                  0x09, 0x28, 0xf7, 0xf6, 0x11, 0x49, 0x60, 0x05, 0x95, 0xc6,
                  0xfd, 0xe3, 0x87, 0x2f, 0x3c, 0x9b, 0x8c, 0xdf, 0xca, 0x30,
                  0xba, 0x63, 0x25, 0xe4, 0x23, 0x01, 0x4d, 0x7c, 0x00, 0x21,
                  0x3c, 0x26, 0xe2, 0xa1, 0x6e, 0xaa, 0x8b, 0x78, 0x61, 0x06,
                  0x74, 0x6c, 0x01, 0xf5, 0x73, 0x66, 0xa9, 0xe2, 0xe6, 0x22,
                  0x63, 0x2e, 0xb5, 0xe7, 0x3e, 0x31, 0xfc, 0xf9, 0x16, 0x07,
                  0x4a, 0xdf, 0x5a, 0x91, 0x41, 0x72, 0xc7, 0xad, 0xae, 0x0c,
                  0xf3, 0xac, 0xd1, 0x92, 0x04, 0x92 };

  uint8_t expected_r[] = {
    0x01, 0xa2, 0xdf, 0xaf, 0x0d, 0x15, 0xb1, 0x6e, 0xab, 0x46, 0x08, 0x7f,
    0xb1, 0xc8, 0x83, 0xd3, 0x4b, 0xcb, 0x33, 0x8b, 0x68, 0xbd, 0x1c, 0xa4,
    0xe5, 0xeb, 0x1c, 0xb3, 0x0b, 0xb8, 0x24, 0xe7, 0xc1, 0xf9, 0xd1, 0x09,
    0x0d, 0x9a, 0xc7, 0xe3, 0x87, 0xcf, 0x9b, 0xb6, 0x40, 0x82, 0xf6, 0xb1,
    0x01, 0x2d, 0xd6, 0x4a, 0x39, 0x54, 0xaf, 0x4a, 0x74, 0xfc, 0x21, 0x9c,
    0x69, 0x7c, 0xae, 0x2c, 0x8b, 0x42, 0xfc, 0x6d, 0x80, 0x45, 0x4b, 0x14,
    0x7b, 0xd3, 0x35, 0x52, 0x37, 0x77, 0x6b, 0x52, 0x52, 0x5e, 0xc2, 0x29,
    0x5e, 0x9a, 0x36, 0x8b, 0xac, 0x74, 0xd9, 0x6e, 0x91, 0x98, 0x54, 0xdd
  };

  uint8_t r[96] = { 0 };

  cx_ecpoint_t ec_p, ec_q, ec_r;

  assert_int_equal(sys_cx_bn_lock(BLS12_377_SIZE_u8, 0), CX_OK);
  assert_int_equal(sys_cx_ecpoint_alloc(&ec_p, CX_CURVE_BLS12_377_G1), CX_OK);
  assert_int_equal(sys_cx_ecpoint_alloc(&ec_q, CX_CURVE_BLS12_377_G1), CX_OK);
  assert_int_equal(sys_cx_ecpoint_alloc(&ec_r, CX_CURVE_BLS12_377_G1), CX_OK);
  assert_int_equal(sys_cx_ecpoint_init(&ec_p, p, BLS12_377_SIZE_u8,
                                       p + BLS12_377_SIZE_u8,
                                       BLS12_377_SIZE_u8),
                   CX_OK);
  assert_int_equal(sys_cx_ecpoint_init(&ec_q, q, BLS12_377_SIZE_u8,
                                       q + BLS12_377_SIZE_u8,
                                       BLS12_377_SIZE_u8),
                   CX_OK);
  assert_int_equal(sys_cx_ecpoint_add(&ec_r, &ec_p, &ec_q), CX_OK);
  assert_int_equal(sys_cx_ecpoint_export(&ec_r, r, BLS12_377_SIZE_u8,
                                         r + BLS12_377_SIZE_u8,
                                         BLS12_377_SIZE_u8),
                   CX_OK);
  assert_int_equal(sys_cx_bn_unlock(), CX_OK);
  assert_memory_equal(expected_r, r, sizeof(expected_r));
}

static void test_ecpoint_add_edwards_bls12377(void **state
                                              __attribute__((unused)))
{
  uint8_t p[] = { 0x12, 0x74, 0x4a, 0x54, 0xf4, 0xa0, 0x16, 0x9e, 0xa0, 0xb6,
                  0xc7, 0xf6, 0xe0, 0xa1, 0xf5, 0xa1, 0xcc, 0x53, 0x3b, 0x8e,
                  0x46, 0xf3, 0x37, 0x9e, 0x44, 0xa9, 0x4f, 0x7f, 0xc3, 0x52,
                  0xa1, 0x70, 0x10, 0x8c, 0x03, 0x0f, 0xb4, 0x98, 0x01, 0xbd,
                  0xe0, 0xdc, 0xca, 0xdb, 0xac, 0xb2, 0xb0, 0xf7, 0x65, 0x55,
                  0x72, 0x49, 0x83, 0x15, 0x44, 0x8f, 0x4e, 0x88, 0x43, 0x9c,
                  0xe6, 0x08, 0x21, 0xb7 };
  uint8_t q[] = { 0x0a, 0xa9, 0x3f, 0x2d, 0xb3, 0xc8, 0x34, 0xf1, 0x73, 0x2b,
                  0x9c, 0xfe, 0x56, 0xc2, 0x96, 0x22, 0x6d, 0x98, 0x4f, 0x7b,
                  0x49, 0x77, 0xf3, 0xa4, 0xcd, 0x92, 0x14, 0x0c, 0xc5, 0xae,
                  0xa0, 0x35, 0x09, 0x7d, 0x30, 0x9e, 0x26, 0xda, 0x2e, 0x79,
                  0x96, 0x98, 0xa2, 0x97, 0xf8, 0x96, 0x32, 0x77, 0x01, 0x8a,
                  0x60, 0xbb, 0x2a, 0xbc, 0x22, 0x13, 0x94, 0x3b, 0xee, 0x9f,
                  0xb7, 0x31, 0x0b, 0x95 };

  uint8_t expected_r[] = { 0x0e, 0xa9, 0xc0, 0xcf, 0xbd, 0x81, 0x12, 0x7b,
                           0xd6, 0x33, 0x34, 0x39, 0xbe, 0xb3, 0x66, 0xfe,
                           0xda, 0xdc, 0xbb, 0x37, 0x90, 0xe8, 0x50, 0xaf,
                           0xfb, 0x89, 0xb7, 0x2e, 0x41, 0x98, 0xa2, 0x5c,
                           0x0e, 0x1a, 0xe4, 0xba, 0x1a, 0xfb, 0xe6, 0xbf,
                           0x97, 0xdb, 0x14, 0x36, 0xc0, 0x14, 0xb7, 0xd7,
                           0x7b, 0xc4, 0xc0, 0x9e, 0xa4, 0x86, 0x4b, 0xef,
                           0x16, 0xc9, 0x0d, 0x39, 0xd3, 0x28, 0x7a, 0xc3 };

  uint8_t r[64] = { 0 };

  cx_ecpoint_t ec_p, ec_q, ec_r;

  assert_int_equal(sys_cx_bn_lock(EDBLS12_377_SIZE_u8, 0), CX_OK);
  assert_int_equal(sys_cx_ecpoint_alloc(&ec_p, CX_CURVE_EdBLS12), CX_OK);
  assert_int_equal(sys_cx_ecpoint_alloc(&ec_q, CX_CURVE_EdBLS12), CX_OK);
  assert_int_equal(sys_cx_ecpoint_alloc(&ec_r, CX_CURVE_EdBLS12), CX_OK);
  assert_int_equal(sys_cx_ecpoint_init(&ec_p, p, EDBLS12_377_SIZE_u8,
                                       p + EDBLS12_377_SIZE_u8,
                                       EDBLS12_377_SIZE_u8),
                   CX_OK);
  assert_int_equal(sys_cx_ecpoint_init(&ec_q, q, EDBLS12_377_SIZE_u8,
                                       q + EDBLS12_377_SIZE_u8,
                                       EDBLS12_377_SIZE_u8),
                   CX_OK);
  assert_int_equal(sys_cx_ecpoint_add(&ec_r, &ec_p, &ec_q), CX_OK);
  assert_int_equal(sys_cx_ecpoint_export(&ec_r, r, EDBLS12_377_SIZE_u8,
                                         r + EDBLS12_377_SIZE_u8,
                                         EDBLS12_377_SIZE_u8),
                   CX_OK);
  assert_int_equal(sys_cx_bn_unlock(), CX_OK);
  assert_memory_equal(expected_r, r, sizeof(expected_r));
}

static void test_ecpoint_scalarmul_edwards_bls12377(void **state
                                                    __attribute__((unused)))
{
  uint8_t point_in[] = { 0x04, 0x09, 0xf1, 0xb5, 0xa5, 0xba, 0xf6, 0xac, 0xf0,
                         0x6f, 0xed, 0x91, 0xc9, 0xae, 0x9e, 0xbf, 0xa0, 0x60,
                         0x68, 0xdd, 0x28, 0x35, 0x79, 0x09, 0x80, 0x89, 0x4e,
                         0x23, 0x28, 0xf3, 0xeb, 0xca, 0x05, 0x09, 0xa2, 0x0d,
                         0xf3, 0x65, 0x71, 0xac, 0x3c, 0xd9, 0x06, 0xb2, 0x56,
                         0x08, 0x0b, 0xa8, 0x45, 0x44, 0x53, 0xc1, 0x77, 0xaa,
                         0xf3, 0x13, 0x1b, 0xb5, 0x0a, 0x67, 0xbf, 0x1a, 0x80,
                         0x67, 0x81 };
  uint8_t expected_out[] = {
    0x04, 0x0c, 0x90, 0x38, 0xd7, 0xdb, 0x48, 0xc1, 0xc6, 0x24, 0x5d,
    0x1f, 0x8d, 0x63, 0x19, 0xda, 0x75, 0x23, 0xd4, 0x10, 0x3e, 0x6c,
    0x8a, 0x31, 0xaa, 0xdc, 0x76, 0xf1, 0x95, 0x60, 0xc3, 0xaa, 0x5c,
    0x05, 0x00, 0x85, 0x31, 0x17, 0xf3, 0x9d, 0xd6, 0x30, 0x95, 0xb5,
    0x8a, 0xc8, 0xb2, 0x63, 0x9d, 0xdf, 0xd7, 0x51, 0x26, 0x59, 0x7b,
    0x98, 0xda, 0x6f, 0xcd, 0x14, 0x47, 0x57, 0x29, 0xf5, 0xa3
  };

  uint8_t scalar[] = { 0x00, 0xde, 0x95, 0x56, 0x32, 0x1d, 0xf4, 0x9e,
                       0x4d, 0x91, 0xe0, 0x43, 0x92, 0xcb, 0xa2, 0x55,
                       0xce, 0x4a, 0x7f, 0xa9, 0x3e, 0x05, 0x4a, 0xaa,
                       0xf0, 0x22, 0xe6, 0x37, 0xd4, 0x24, 0xc8, 0x2e

  };

  uint8_t point_out[65] = { 0 };

  cx_ecpoint_t P;

  assert_int_equal(sys_cx_bn_lock(EDBLS12_377_SIZE_u8, 0), CX_OK);
  assert_int_equal(sys_cx_ecpoint_alloc(&P, CX_CURVE_EdBLS12), CX_OK);
  assert_int_equal(sys_cx_ecpoint_init(&P, point_in + 1, EDBLS12_377_SIZE_u8,
                                       point_in + 1 + EDBLS12_377_SIZE_u8,
                                       EDBLS12_377_SIZE_u8),
                   CX_OK);
  assert_int_equal(sys_cx_ecpoint_scalarmul(&P, scalar, sizeof(scalar)), CX_OK);
  point_out[0] = 0x04;
  assert_int_equal(sys_cx_ecpoint_export(&P, point_out + 1, EDBLS12_377_SIZE_u8,
                                         point_out + 1 + EDBLS12_377_SIZE_u8,
                                         EDBLS12_377_SIZE_u8),
                   CX_OK);
  assert_int_equal(sys_cx_bn_unlock(), CX_OK);
  assert_memory_equal(expected_out, point_out, sizeof(expected_out));
}

int main(void)
{
  const struct CMUnitTest tests[] = {
    /* clang-format off */
    cmocka_unit_test(test_is_on_curve),
    cmocka_unit_test(test_x25519),
    cmocka_unit_test(test_x25519_wycheproof),
    cmocka_unit_test(test_x448),
    cmocka_unit_test(test_x448_wycheproof),
    cmocka_unit_test(test_ecpoint_scalarmul_bls12377),
    cmocka_unit_test(test_ecpoint_add_bls12377),
    cmocka_unit_test(test_ecpoint_add_edwards_bls12377),
    cmocka_unit_test(test_ecpoint_scalarmul_edwards_bls12377),
    /* clang-format on */
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
