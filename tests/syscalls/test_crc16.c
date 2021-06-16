#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

#include "bolos/cx_crc.h"

void test_crc16(void **state __attribute__((unused)))
{
  assert_int_equal(sys_cx_crc16_update(0xffff, "123456789", 9), 0x29b1);
}

int main(void)
{
  const struct CMUnitTest tests[] = { cmocka_unit_test(test_crc16) };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
