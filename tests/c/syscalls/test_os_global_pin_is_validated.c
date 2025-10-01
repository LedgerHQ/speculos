#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#include "../utils.h"
#include "bolos_syscalls.h"
#include "emulate.h"
#include "sdk.h"

#define os_os_global_pin_is_validated sys_os_global_pin_is_validated

hw_model_t hw_model = MODEL_COUNT;
int g_api_level = 0;

void *get_memory_code_address(void)
{
  return NULL;
}

struct app_s *get_current_app(void)
{
  return NULL;
}

char *get_app_nvram_file_name(void)
{
  return NULL;
}

bool get_app_save_nvram(void)
{
  return false;
}

unsigned long get_app_nvram_address(void)
{
  return 0;
}

unsigned long get_app_nvram_size(void)
{
  return 0;
}

unsigned long get_app_text_load_addr(void)
{
  return 0;
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
