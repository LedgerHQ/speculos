#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <setjmp.h>
// must come after setjmp.h
#include <cmocka.h>

#define _SDK_2_0_

#include "bolos/cx.h"
#include "bolos/cx_ec.h"
#include "bolos/cxlib.h"
#include "utils.h"

typedef struct {
  const char *x;
  const char *y;
  cx_curve_t curve;
  int size;
  bool is_on_curve;

} point_on_curve_test_vector;

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

static void test_is_on_curve(void **state __attribute__((unused)))
{
  unsigned int i;

  for (i = 0; i < ARRAY_SIZE(bolos_test_vectors); i++) {
    test_vector(&bolos_test_vectors[i]);
  }
}

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_is_on_curve),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
