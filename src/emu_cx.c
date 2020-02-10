#include <err.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include "emulate.h"

static bool initialized = false;

static unsigned int get_rng_seed_from_env(const char *name)
{
  char *p;

  p = getenv(name);
  if (p != NULL) {
    return atoi(p);
  }
  else {
    return time(NULL);
  }
}

/* not secure, but this is clearly not the goal of this emulator */
unsigned long sys_cx_rng(uint8_t *buffer, unsigned int length)
{
  unsigned int i;

  if (!initialized) {
    srand(get_rng_seed_from_env("RNG_SEED"));
    initialized = true;
  }

  for (i = 0; i < length; i++) {
    buffer[i] = rand() & 0xff;
  }

  return 0;
}

unsigned long sys_cx_rng_u8(void)
{
  uint8_t n;

  sys_cx_rng(&n, sizeof(n));

  return n;
}
