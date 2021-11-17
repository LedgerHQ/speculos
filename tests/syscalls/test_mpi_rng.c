#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// must come after setjmp.h
#include <cmocka.h>

#include <openssl/bn.h>
#include <openssl/rand.h>

#include "bolos/cxlib.h"
#include "utils.h"

unsigned long sys_cx_rng(uint8_t *buffer, unsigned int length);

void test_mpi_rng(void **state __attribute__((unused)))
{
  cx_mpi_t *r, *n;
  int ret;
  cx_err_t error;
  uint32_t i, j, rnd, value;
  uint8_t buffer[64];

  // Those tests will check that cx_mpi_rng is working as expected:
  // cx_mpi_rng(r,n) => Generate a random value r in the range 0 < r < n.

  r = BN_new();
  assert_non_null(r);
  n = BN_new();
  assert_non_null(n);

  // Check that cx_mpi_rng returns CX_INVALID_PARAMETER if n=0:
  BN_zero(n);
  error = cx_mpi_rng(r, n);
  assert_int_equal(error, CX_INVALID_PARAMETER);

  // Check that cx_mpi_rng returns CX_INVALID_PARAMETER if n=1:
  ret = BN_one(n);
  assert_int_equal(ret, 1);
  error = cx_mpi_rng(r, n);
  assert_int_equal(error, CX_INVALID_PARAMETER);

  BN_free(r);
  BN_free(n);
}

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_mpi_rng),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
