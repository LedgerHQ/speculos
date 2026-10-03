#include <err.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "appflags.h"
#include "emulate.h"
#include "environment.h"

#define BOLOS_TRUE  0xAA
#define BOLOS_FALSE 0x55

/* Tries of the device PIN, as the OS resets them after a correct entry. */
#define DEFAULT_PIN_RETRIES 3

static unsigned int pin_retries = DEFAULT_PIN_RETRIES;

/* The OS refuses these syscalls to an application without the flag. */
static void require_global_pin_flag(const char *name)
{
  if (!(app_flags & APPLICATION_FLAG_GLOBAL_PIN)) {
    errx(1, "%s requires APPLICATION_FLAG_GLOBAL_PIN (0x%x) in the app flags",
         name, (unsigned int)APPLICATION_FLAG_GLOBAL_PIN);
  }
}

unsigned long sys_os_global_pin_check(unsigned char *pin_buffer,
                                      unsigned char pin_length)
{
  const uint8_t *pin;
  size_t length;
  uint8_t difference;

  require_global_pin_flag("os_global_pin_check");

  length = env_get_pin(&pin);
  difference = (pin_length == length) ? 0 : 1;
  for (size_t i = 0; i < pin_length && i < length; i++) {
    difference |= pin_buffer[i] ^ pin[i];
  }

  if (difference == 0) {
    pin_retries = DEFAULT_PIN_RETRIES;
    return BOLOS_TRUE;
  }

  /* A device erases itself when its tries run out. That state is a fresh
   * Speculos started with the same seed, which a test gets by restarting it,
   * so the count stops at zero here instead of emulating the wipe. */
  if (pin_retries > 0) {
    pin_retries--;
  }
  return BOLOS_FALSE;
}

unsigned long sys_os_global_pin_retries(void)
{
  require_global_pin_flag("os_global_pin_retries");
  return pin_retries;
}

void reset_global_pin_retries(void)
{
  pin_retries = DEFAULT_PIN_RETRIES;
}
