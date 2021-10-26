#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#include "emulate.h"
#include "sdk.h"
#include "utils.h"

#define os_os_global_pin_is_validated sys_os_global_pin_is_validated

/* BOLOS_UX_OK varies across SDK versions */
#define BOLOS_UX_OK_1_2 0xB0105011
#define BOLOS_UX_OK_1_5 0xB0105011
#define BOLOS_UX_OK_1_6 0xAA

#define SYSCALL_PIN_1_2 0x60005b89
#define SYSCALL_PIN_1_5 0x60005b89
#define SYSCALL_PIN_1_6 0x6000a03c

sdk_version_t sdk_version = SDK_COUNT;

struct app_s *get_current_app(void)
{
  return NULL;
}

void unload_running_app(bool UNUSED(unload_data))
{
}

int replace_current_code(struct app_s *UNUSED(app))
{
  return 0;
}

int run_lib(char *UNUSED(name), unsigned long *UNUSED(parameters))
{
  return 0;
}

struct testcase_s {
  sdk_version_t sdk_version;
  unsigned long syscall;
  long unsigned int expected;
};

void test_os_global_pin_is_validated(void **UNUSED(state))
{
  assert_int_equal(sys_os_global_pin_is_validated_1_2(), BOLOS_UX_OK_1_2);
  assert_int_equal(sys_os_global_pin_is_validated_1_5(), BOLOS_UX_OK_1_5);
  assert_int_equal(sys_os_global_pin_is_validated_1_6(), BOLOS_UX_OK_1_6);

  struct testcase_s testcases[] = {
    { SDK_NANO_X_1_2, SYSCALL_PIN_1_2, BOLOS_UX_OK_1_2 },
    { SDK_NANO_S_1_5, SYSCALL_PIN_1_5, BOLOS_UX_OK_1_5 },
    { SDK_BLUE_1_5, SYSCALL_PIN_1_5, BOLOS_UX_OK_1_5 },
    { SDK_NANO_S_1_6, SYSCALL_PIN_1_6, BOLOS_UX_OK_1_6 },
  };

  for (size_t i = 0; i < ARRAY_SIZE(testcases); i++) {
    struct testcase_s *test = &testcases[i];
    unsigned long parameters[] = {};
    long unsigned int ret;

    emulate(test->syscall, parameters, &ret, false, test->sdk_version);
    assert_int_equal(ret, test->expected);
  }
}

int main(void)
{
  const struct CMUnitTest tests[] = { cmocka_unit_test(
      test_os_global_pin_is_validated) };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
