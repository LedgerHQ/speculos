#pragma once

#include <stddef.h>
#include <stdint.h>

typedef uint8_t nbgl_transformation_t;
typedef uint8_t nbgl_color_map_t;

typedef struct __attribute__((__packed__)) nbgl_area_s {
  uint16_t x0;     ///< horizontal position of the upper left point of the area
  uint16_t y0;     ///< vertical position of the upper left point of the area
  uint16_t width;  ///< width of the area, in pixels
  uint16_t height; ///< height of the area, in pixels
  uint8_t backgroundColor; ///< color (usually background) to be applied
  uint8_t bpp;             ///< bits per pixel for this area
} nbgl_area_t;

typedef enum { BLACK = 0, DARK_GRAY, LIGHT_GRAY, WHITE } color_t;

unsigned long sys_nbgl_front_draw_rect(nbgl_area_t *area);

unsigned long sys_nbgl_front_draw_horizontal_line(nbgl_area_t *area,
                                                  uint8_t mask,
                                                  color_t lineColor);

unsigned long sys_nbgl_front_draw_img(nbgl_area_t *area, uint8_t *buffer,
                                      nbgl_transformation_t transformation,
                                      nbgl_color_map_t colorMap);

unsigned long sys_nbgl_front_refresh_area(nbgl_area_t *area);

unsigned long sys_nbgl_front_draw_img_file(nbgl_area_t *area, uint8_t *buffer,
                                           nbgl_color_map_t colorMap,
                                           uint8_t *optional_uzlib_work_buffer);

unsigned long sys_nbgl_get_font(unsigned int fontId);
