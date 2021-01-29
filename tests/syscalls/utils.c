#include "utils.h"

int hex2num(char c)
{
  if (c >= '0' && c <= '9') {
    return c - '0';
  }
  if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}

int hex2byte(const char *hex)
{
  int a, b;
  a = hex2num(*hex++);
  if (a < 0) {
    return -1;
  }
  b = hex2num(*hex);
  if (b < 0) {
    return -1;
  }
  return (a << 4) | b;
}

size_t hexstr2bin(const char *hex, uint8_t *buf, size_t max_len)
{
  if (hex == NULL) {
    return 0;
  }

  size_t i;
  int a;
  const char *ipos = hex;
  uint8_t *opos = buf;

  i = 0;
  while (i < max_len) {
    if (*ipos == 0) {
      break;
    }
    a = hex2byte(ipos);
    if (a < 0) {
      return 0;
    }
    *opos++ = (uint8_t)a;
    ipos += 2;
    i += 1;
  }
  return i;
}
