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

  // Check that cx_mpi_rng returns CX_OK and r=1 if n=2:
  ret = BN_set_word(n, 2);
  assert_int_equal(ret, 1);
  for (i = 0; i < 256; i++) {
    error = cx_mpi_rng(r, n);
    assert_int_equal(error, CX_OK);
    assert_int_equal(BN_is_one(r), 1);
  }

  // Pick some random values and check that result is fine:
  srand(time(NULL));
  for (i = 0; i < 1024; i++) {
    rnd = 2 + rand();
    ret = BN_set_word(n, rnd);
    assert_int_equal(ret, 1);
    error = cx_mpi_rng(r, n);
    assert_int_equal(error, CX_OK);
    value = BN_get_word(r);
    assert_true(value >= 1 && value < rnd);
  }

  // Pick some big numbers random values and check that result is fine:
  for (i = 4; i < sizeof(buffer); i++) {

    // Fill a buffer with i random bytes:
    sys_cx_rng(buffer, i);
    // Create a bignumber from this buffer:
    n = BN_bin2bn(buffer, i, n);
    assert_non_null(n);

    for (j = 0; j < 1024; j++) {
      error = cx_mpi_rng(r, n);
      assert_int_equal(error, CX_OK);

      assert_true(BN_cmp(r, BN_value_one()) >= 0);
      assert_true(BN_cmp(r, n) == -1);
    }
  }
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
