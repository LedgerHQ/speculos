#include <stdio.h>

#include <setjmp.h>
// must come after setjmp.h
#include <cmocka.h>

#include "../utils.h"
#include "bolos/cx.h"
#include "nist_cavp.h"

void test_blake2b_kat(void **state __attribute__((unused)))
{
  test_cavp_short_msg_with_size(TESTS_PATH "cavp/blake2b_kat.data", CX_BLAKE2B,
                                64);
}

int main(void)
{
  const struct CMUnitTest tests[] = { cmocka_unit_test(test_blake2b_kat) };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
