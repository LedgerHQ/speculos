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

int main(void)
{
  const struct CMUnitTest tests[] = {
    /* clang-format off */
    cmocka_unit_test(test_is_on_curve),
    cmocka_unit_test(test_x25519),
    cmocka_unit_test(test_x25519_wycheproof),
    cmocka_unit_test(test_x448),
    cmocka_unit_test(test_x448_wycheproof),
    /* clang-format on */
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
