#pragma once

#include <stdint.h>
#include <string.h>

static inline uint16_t U2BE(const uint8_t *buf, size_t off)
{
  return (buf[off] << 8) | buf[off + 1];
}

static inline void U2BE_ENCODE(uint8_t *buf, size_t off, uint32_t value)
{
  buf[off + 0] = (value >> 8) & 0xFF;
  buf[off + 1] = value & 0xFF;
}

static inline size_t strlcpy(char *dst, const char *src, size_t size)
{
  size_t srclen;
  size--;
  srclen = strlen(src);

  if (srclen > size)
    srclen = size;

  memcpy(dst, src, srclen);
  dst[srclen] = '\0';

  return (srclen);
}