#include <err.h>
#include <stdio.h>
#include <string.h>

#include "emulate.h"
#include "nbgl.h"

#define SEPROXYHAL_TAG_NBGL_DRAW_RECT       0x6A
#define SEPROXYHAL_TAG_NBGL_REFRESH         0x6B
#define SEPROXYHAL_TAG_NBGL_DRAW_LINE       0x6C
#define SEPROXYHAL_TAG_NBGL_DRAW_IMAGE      0x6D
#define SEPROXYHAL_TAG_NBGL_DRAW_IMAGE_FILE 0x6E

unsigned long sys_nbgl_front_draw_rect(nbgl_area_t *area)
{
  uint8_t header[3];
  size_t len = sizeof(nbgl_area_t);

  header[0] = SEPROXYHAL_TAG_NBGL_DRAW_RECT;
  header[1] = (len >> 8) & 0xff;
  header[2] = len & 0xff;

  sys_io_seph_send(header, sizeof(header));
  sys_io_seph_send((const uint8_t *)area, len);

  return 0;
}

unsigned long sys_nbgl_front_draw_horizontal_line(nbgl_area_t *area,
                                                  uint8_t mask,
                                                  color_t lineColor)
{
  uint8_t header[3];
  size_t len = sizeof(nbgl_area_t) + 2;

  header[0] = SEPROXYHAL_TAG_NBGL_DRAW_LINE;
  header[1] = (len >> 8) & 0xff;
  header[2] = len & 0xff;

  sys_io_seph_send(header, sizeof(header));
  sys_io_seph_send((const uint8_t *)area, sizeof(nbgl_area_t));
  sys_io_seph_send((const uint8_t *)&mask, 1);
  sys_io_seph_send((const uint8_t *)&lineColor, 1);

  return 0;
}

unsigned long sys_nbgl_front_draw_img(nbgl_area_t *area, uint8_t *buffer,
                                      nbgl_transformation_t transformation,
                                      nbgl_color_map_t colorMap)
{
  uint8_t header[3];
  uint32_t buffer_len = (area->width * area->height * (area->bpp + 1)) / 8;
  size_t len = sizeof(nbgl_area_t) + buffer_len + 2;

  header[0] = SEPROXYHAL_TAG_NBGL_DRAW_IMAGE;
  header[1] = (len >> 8) & 0xff;
  header[2] = len & 0xff;

  sys_io_seph_send(header, sizeof(header));
  sys_io_seph_send((const uint8_t *)area, sizeof(nbgl_area_t));
  sys_io_seph_send(buffer, buffer_len);
  sys_io_seph_send((const uint8_t *)&transformation, 1);
  sys_io_seph_send((const uint8_t *)&colorMap, 1);

  return 0;
}

unsigned long sys_nbgl_front_refresh_area(nbgl_area_t *area)
{
  uint8_t header[3];
  size_t len = sizeof(nbgl_area_t);

  header[0] = SEPROXYHAL_TAG_NBGL_REFRESH;
  header[1] = (len >> 8) & 0xff;
  header[2] = len & 0xff;

  sys_io_seph_send(header, sizeof(header));
  sys_io_seph_send((const uint8_t *)area, sizeof(nbgl_area_t));

  return 0;
}

unsigned long sys_nbgl_front_draw_img_file(nbgl_area_t *area, uint8_t *buffer,
                                           nbgl_color_map_t colorMap,
                                           uint8_t *optional_uzlib_work_buffer)
{
  uint8_t header[3];

  uint8_t compressed = buffer[4] & 0xF;
  if (compressed && optional_uzlib_work_buffer == NULL) {
    fprintf(stderr, "no uzlib work buffer provided, failing");
    return 0;
  }

  size_t len = sizeof(nbgl_area_t) + 1;
  size_t buffer_len;
  if (compressed) {
    buffer_len = (buffer[5] | (buffer[5 + 1] << 8) | (buffer[5 + 2] << 16)) + 8;
  } else {
    buffer_len = (area->width * area->height * (area->bpp + 1)) / 8;
  }

  len += buffer_len;

  header[0] = SEPROXYHAL_TAG_NBGL_DRAW_IMAGE_FILE;
  header[1] = (len >> 8) & 0xff;
  header[2] = len & 0xff;

  sys_io_seph_send(header, sizeof(header));
  sys_io_seph_send((const uint8_t *)area, sizeof(nbgl_area_t));
  sys_io_seph_send(buffer, buffer_len);
  sys_io_seph_send(&colorMap, 1);

  return 0;
}

#define FONTS_ARRAY_ADDR 0x00805000
unsigned long sys_nbgl_get_font(unsigned int fontId)
{
  if (fontId >= 4) {
    return 0;
  } else {
    return *((unsigned int *)(FONTS_ARRAY_ADDR + (4 * fontId)));
  }
}
