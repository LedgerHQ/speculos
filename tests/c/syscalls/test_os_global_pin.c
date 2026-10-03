#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#include "../utils.h"
#include "bolos_syscalls.h"
#include "emulate.h"
#include "sdk.h"

#define BOLOS_TRUE  0xAA
#define BOLOS_FALSE 0x55

static unsigned long pin_check(const char *pin, unsigned char length)
{
  unsigned long parameters[] = { (unsigned long)pin, length };
  unsigned long ret;

  emulate(SYSCALL_os_global_pin_check_ID_IN, parameters, &ret, false, 22,
          MODEL_NANO_X);
  return ret;
}

static unsigned long pin_retries(void)
{
  unsigned long parameters[] = { 0 };
  unsigned long ret;

  emulate(SYSCALL_os_global_pin_retries_ID_IN, parameters, &ret, false, 22,
          MODEL_NANO_X);
  return ret;
}

/* The default PIN 1234 validates and keeps the three tries. */
static void test_right_pin(void **UNUSED(state))
{
  reset_global_pin_retries();
  assert_int_equal(pin_check("1234", 4), BOLOS_TRUE);
  assert_int_equal(pin_retries(), 3);
}

/* A wrong PIN spends a try, and a right one restores them all. */
static void test_wrong_pin_spends_a_try(void **UNUSED(state))
{
  reset_global_pin_retries();
  assert_int_equal(pin_check("0000", 4), BOLOS_FALSE);
  assert_int_equal(pin_retries(), 2);
  assert_int_equal(pin_check("4321", 4), BOLOS_FALSE);
  assert_int_equal(pin_retries(), 1);
  assert_int_equal(pin_check("1234", 4), BOLOS_TRUE);
  assert_int_equal(pin_retries(), 3);
}

/* The tries stop at zero, and a correct PIN still restores them. */
static void test_tries_stop_at_zero(void **UNUSED(state))
{
  reset_global_pin_retries();
  for (int i = 0; i < 4; i++) {
    assert_int_equal(pin_check("0000", 4), BOLOS_FALSE);
  }
  assert_int_equal(pin_retries(), 0);
  assert_int_equal(pin_check("1234", 4), BOLOS_TRUE);
  assert_int_equal(pin_retries(), 3);
}

/* The length counts: neither a prefix nor an extension of the PIN matches. */
static void test_length_must_match(void **UNUSED(state))
{
  reset_global_pin_retries();
  assert_int_equal(pin_check("123", 3), BOLOS_FALSE);
  assert_int_equal(pin_check("12345", 5), BOLOS_FALSE);
  assert_int_equal(pin_retries(), 1);
  assert_int_equal(pin_check("1234", 4), BOLOS_TRUE);
}

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_right_pin),
    cmocka_unit_test(test_wrong_pin_spends_a_try),
    cmocka_unit_test(test_tries_stop_at_zero),
    cmocka_unit_test(test_length_must_match),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
