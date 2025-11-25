#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#include "../utils.h"
#include "bolos_syscalls.h"
#include "emulate.h"
#include "sdk.h"

#define os_os_global_pin_is_validated sys_os_global_pin_is_validated

struct testcase_s {
  hw_model_t hw_model;
  int api_level;
  unsigned long syscall;
  long unsigned int expected;
};

void test_os_global_pin_is_validated(void **UNUSED(state))
{
  struct testcase_s testcases[] = {
    { MODEL_NANO_X, 22, SYSCALL_os_global_pin_is_validated_ID_IN, 0xAA },
  };

  for (size_t i = 0; i < ARRAY_SIZE(testcases); i++) {
    struct testcase_s *test = &testcases[i];
    unsigned long parameters[] = {};
    long unsigned int ret;

    emulate(test->syscall, parameters, &ret, false, test->api_level,
            test->hw_model);
    assert_int_equal(ret, test->expected);
  }
}

int main(void)
{
  const struct CMUnitTest tests[] = { cmocka_unit_test(
      test_os_global_pin_is_validated) };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
