#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
// must come after setjmp.h
#include <cmocka.h>

#include "bolos/cxlib.h"

#define GF2_8_MPI_BYTES 1

static void test_cx_bn_gf2_n_mul(void **state __attribute__((unused)))
{
  cx_err_t error = CX_OK;

  cx_bn_t a, // multiplicand
      b,     // multiplier
      m,     // modulus
      r,     // result
      r2;    // Montgomery constant

  // A(x)
  const uint8_t multiplicand[1] = { 0x1B }; // 27
  // B(x)
  const uint8_t multiplier[1] = { 0x3D }; // 61

  // The irreducible polynomial N(x) = x^8 + x^4 + x^3 + x + 1
  const uint8_t N[2] = { 0x01, 0x1B }; // 283

  // 2nd Montgomery constant: R2 = x^(2*t*8) mod N(x)
  // t = 1 since the number of bytes of R is 1.
  const uint8_t R2[1] = { 0x56 };

  // Expected result of N(x) = A(x )* B(x)
  const uint32_t re = 0x49; // result expected = 71
  int diff;

  CX_CHECK(sys_cx_bn_lock(GF2_8_MPI_BYTES, 0));
  CX_CHECK(sys_cx_bn_alloc(&r, GF2_8_MPI_BYTES));
  CX_CHECK(sys_cx_bn_alloc_init(&a, GF2_8_MPI_BYTES, multiplicand,
                                sizeof(multiplicand)));
  CX_CHECK(sys_cx_bn_alloc_init(&b, GF2_8_MPI_BYTES, multiplier,
                                sizeof(multiplier)));
  CX_CHECK(sys_cx_bn_alloc_init(&m, GF2_8_MPI_BYTES, N, sizeof(N)));
  CX_CHECK(sys_cx_bn_alloc_init(&r2, GF2_8_MPI_BYTES, R2, sizeof(R2)));

  // Perform the Galois Field GF(2m) multiplication operation
  CX_CHECK(sys_cx_bn_gf2_n_mul(r, a, b, m, r2));

  // Compare result to expected result
  CX_CHECK(sys_cx_bn_cmp_u32(r, re, &diff));

  CX_CHECK(sys_cx_bn_destroy(&r));
  CX_CHECK(sys_cx_bn_destroy(&a));
  CX_CHECK(sys_cx_bn_destroy(&b));
  CX_CHECK(sys_cx_bn_destroy(&m));
  CX_CHECK(sys_cx_bn_destroy(&r2));

end:
  if (sys_cx_bn_is_locked()) {
    sys_cx_bn_unlock();
  }

  // Assert that there are no errors
  assert_int_equal(error, CX_OK);

  // Assert that the result is correct
  assert_int_equal(diff, 0);
}

int main(void)
{
  const struct CMUnitTest tests[] = { cmocka_unit_test(test_cx_bn_gf2_n_mul) };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
