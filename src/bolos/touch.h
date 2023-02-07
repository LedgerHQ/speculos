#pragma once

#include <stddef.h>
#include <stdint.h>

typedef struct io_touch_info_s {
  uint16_t x;
  uint16_t y;
  uint8_t state;
} io_touch_info_t;

void catch_touch_info_from_seph(uint8_t *buffer, uint16_t size);
unsigned long sys_touch_get_last_info(io_touch_info_t *info);
