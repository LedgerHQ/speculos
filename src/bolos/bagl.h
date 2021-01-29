#pragma once

#include <stddef.h>
#include <stdint.h>

unsigned long sys_bagl_hal_draw_rect(unsigned int color, int x, int y,
                                     unsigned int width, unsigned int height);
unsigned long sys_bagl_hal_draw_bitmap_within_rect(
    int x, int y, unsigned int width, unsigned int height,
    unsigned int color_count, const unsigned int *colors,
    unsigned int bit_per_pixel, const uint8_t *bitmap,
    unsigned int bitmap_length_bits);
unsigned long sys_screen_update(void);
