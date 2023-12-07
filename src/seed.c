#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "seed.h"

/* glory promote mansion idle axis finger extra february uncover one trip
 * resource lawn turtle enact monster seven myth punch hobby comfort wild raise
 * skin */
static uint8_t default_seed[MAX_SEED_SIZE] =
    "\xb1\x19\x97\xfa\xff\x42\x0a\x33\x1b\xb4\xa4\xff\xdc\x8b\xdc\x8b\xa7\xc0"
    "\x17\x32\xa9\x9a\x30\xd8\x3d\xbb\xeb\xd4\x69\x66\x6c\x84\xb4\x7d\x09\xd3"
    "\xf5\xf4\x72\xb3\xb9\x38\x4a\xc6\x34\xbe\xba\x2a\x44\x0b\xa3\x6e\xc7\x66"
    "\x11\x44\x13\x2f\x35\xe2\x06\x87\x35\x64";

const char *SEED_ENV_NAME = "SPECULOS_SEED";
typedef struct {
  size_t size;
  uint8_t seed[MAX_SEED_SIZE];
} seed_t;
static seed_t actual_seed = { 0 };

static int unhex(uint8_t *dst, size_t dst_size, const char *src,
                 size_t src_size)
{
  unsigned int i;
  uint8_t acc;
  int8_t c;

  acc = 0;
  for (i = 0; i < src_size && (i >> 1) < dst_size; i++) {
    c = src[i];
    switch (c) {
    case '0' ... '9':
      acc = (acc << 4) + c - '0';
      break;
    case 'a' ... 'f':
      acc = (acc << 4) + c - 'a' + 10;
      break;
    case 'A' ... 'F':
      acc = (acc << 4) + c - 'A' + 10;
      break;
    default:
      return -1;
    }

    if (i % 2 != 0) {
      dst[i >> 1] = acc;
      acc = 0;
    }
  }

  if (i != src_size) {
    return -1;
  }

  return src_size / 2;
}

void init_seed()
{
  ssize_t size;
  char *p;

  p = getenv(SEED_ENV_NAME);
  if (p != NULL) {
    size = unhex(actual_seed.seed, sizeof(actual_seed.seed), p, strlen(p));
    if (size < 0) {
      warnx("invalid seed passed through %s environment variable",
            SEED_ENV_NAME);
      p = NULL;
    } else {
      fprintf(stderr, "[*] Seed initialized from environment: '%s'\n", p);
    }
  }

  if (p == NULL) {
    warnx("using default seed");
    memcpy(actual_seed.seed, default_seed, sizeof(default_seed));
    size = sizeof(default_seed);
    fprintf(stderr, "[*] Seed initialized from default value: '%s'\n",
            default_seed);
  }
  actual_seed.size = size;
}

size_t get_seed(uint8_t *seed, size_t max_size)
{
  memcpy(seed, actual_seed.seed, max_size);
  return (actual_seed.size < max_size) ? actual_seed.size : max_size;
}
