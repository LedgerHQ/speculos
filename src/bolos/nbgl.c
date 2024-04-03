#include <err.h>
#include <stdio.h>
#include <string.h>

#include "emulate.h"
#include "fonts.h"
#include "nbgl.h"
#include "nbgl_rle.h"

#define SEPROXYHAL_TAG_NBGL_DRAW_RECT       0xFA
#define SEPROXYHAL_TAG_NBGL_REFRESH         0xFB
#define SEPROXYHAL_TAG_NBGL_DRAW_LINE       0xFC
#define SEPROXYHAL_TAG_NBGL_DRAW_IMAGE      0xFD
#define SEPROXYHAL_TAG_NBGL_DRAW_IMAGE_FILE 0xFE
#define SEPROXYHAL_TAG_NBGL_DRAW_IMAGE_RLE  0xFF

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

static unsigned long
nbgl_front_draw_img_character(nbgl_area_t *area, uint8_t *buffer,
                              nbgl_transformation_t transformation,
                              nbgl_color_map_t colorMap, uint32_t character)
{
  uint8_t header[3];
  uint8_t bpp = 1 << area->bpp;
  uint32_t nb_pixs = (area->width * area->height * bpp);
  uint32_t buffer_len = (nb_pixs / 8) + ((nb_pixs % 8) > 0);
  size_t len = sizeof(nbgl_area_t) + buffer_len + 1 + 1 + 4;

  header[0] = SEPROXYHAL_TAG_NBGL_DRAW_IMAGE;
  header[1] = (len >> 8) & 0xff;
  header[2] = len & 0xff;

  sys_io_seph_send(header, sizeof(header));
  sys_io_seph_send((const uint8_t *)area, sizeof(nbgl_area_t));
  sys_io_seph_send(buffer, buffer_len);
  sys_io_seph_send((const uint8_t *)&transformation, 1);
  sys_io_seph_send((const uint8_t *)&colorMap, 1);
  sys_io_seph_send((const uint8_t *)&character, 4);

  return 0;
}

unsigned long sys_nbgl_front_draw_img(nbgl_area_t *area, uint8_t *buffer,
                                      nbgl_transformation_t transformation,
                                      nbgl_color_map_t colorMap)
{
  // Try to find the character corresponding to provided bitmap
  uint32_t character = get_character_from_bitmap(buffer);
  return nbgl_front_draw_img_character(area, buffer, transformation, colorMap,
                                       character);
}

unsigned long sys_nbgl_front_refresh_area_legacy(nbgl_area_t *area)
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

unsigned long sys_nbgl_front_refresh_area(nbgl_area_t *area,
                                          nbgl_post_refresh_t post_refresh)
{
  (void)post_refresh;
  return sys_nbgl_front_refresh_area_legacy(area);
}

unsigned long sys_nbgl_front_draw_img_file(nbgl_area_t *area, uint8_t *buffer,
                                           nbgl_color_map_t colorMap,
                                           uint8_t *optional_uzlib_work_buffer)
{
  uint8_t header[3];

  uint8_t compressed = buffer[4] & 0xF;
  if (compressed == 1 && optional_uzlib_work_buffer == NULL) {
    fprintf(stderr, "no uzlib work buffer provided, failing");
    return 0;
  }
  size_t len = sizeof(nbgl_area_t) + 1;
  size_t buffer_len = 0;
  switch (compressed) {
  case 0: // no compression
    buffer_len = (area->width * area->height * (area->bpp + 1)) / 8;
    break;
  case 1: // gzlib compression
    buffer_len = (buffer[5] | (buffer[5 + 1] << 8) | (buffer[5 + 2] << 16)) + 8;
    break;
  case 2: // rle compression
    buffer_len = (buffer[5] | (buffer[5 + 1] << 8) | (buffer[5 + 2] << 16));
    buffer += 8;
    return sys_nbgl_front_draw_img_rle(area, buffer, buffer_len,
                                       ((colorMap >> (0 * 2)) & 0x3), 0);
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

unsigned long sys_nbgl_get_font(unsigned int fontId)
{
  switch (hw_model) {
  case MODEL_STAX: {
    unsigned int maxNbFonts;
    if (sdk_version >= SDK_API_LEVEL_15) {
      maxNbFonts = STAX_NB_FONTS;
    } else {
      maxNbFonts = STAX_NB_FONTS_12;
    }
    if (fontId >= maxNbFonts) {
      return 0;
    } else {
      return *((unsigned int *)(STAX_FONTS_ARRAY_ADDR + (4 * fontId)));
    }
  }
  case MODEL_FLEX:
    fontId -= 11; // BAGL_FONT_INTER_REGULAR_28px
    if (fontId >= FLEX_NB_FONTS) {
      return 0;
    } else {
      return *((unsigned int *)(FLEX_FONTS_ARRAY_ADDR + (4 * fontId)));
    }
  case MODEL_NANO_SP:
    fontId -= 8; // BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp
    if (fontId >= NANO_NB_FONTS) {
      return 0;
    } else {
      return *((unsigned int *)(NANOSP_FONTS_ARRAY_ADDR + (4 * fontId)));
    }
  case MODEL_NANO_X:
    fontId -= 8; // BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp
    if (fontId >= NANO_NB_FONTS) {
      return 0;
    } else {
      return *((unsigned int *)(NANOX_FONTS_ARRAY_ADDR + (4 * fontId)));
    }
  default:
    return 0;
  }
}

unsigned long sys_nbgl_screen_reinit(void)
{
  return 0;
}

uint8_t uncompress_rle_buffer[SCREEN_HEIGHT * SCREEN_WIDTH / 2];

unsigned long sys_nbgl_front_draw_img_rle_legacy(nbgl_area_t *area,
                                                 uint8_t *buffer,
                                                 uint32_t buffer_len,
                                                 color_t fore_color)
{
  // Try to find the character corresponding to provided bitmap
  uint32_t character = get_character_from_bitmap(buffer);

  // Uncompress input buffer
  nbgl_uncompress_rle(area, buffer, buffer_len, uncompress_rle_buffer,
                      sizeof(uncompress_rle_buffer));

  // Now send it as if it was an uncompressed image
  nbgl_front_draw_img_character(area, uncompress_rle_buffer, NO_TRANSFORMATION,
                                fore_color, character);

  return 0;
}

unsigned long sys_nbgl_front_draw_img_rle_10(nbgl_area_t *area, uint8_t *buffer,
                                             uint32_t buffer_len,
                                             color_t fore_color)
{
  return sys_nbgl_front_draw_img_rle(area, buffer, buffer_len, fore_color, 0);
}

unsigned long sys_nbgl_front_draw_img_rle(nbgl_area_t *area, uint8_t *buffer,
                                          uint32_t buffer_len,
                                          color_t fore_color,
                                          uint8_t nb_skipped_bytes)
{
  // Try to find the character corresponding to provided bitmap
  uint32_t character = get_character_from_bitmap(buffer);
  // We need to keep data compressed to be able to compare with fonts bitmaps
  uint8_t header[3];
  size_t len = sizeof(nbgl_area_t) + buffer_len + 1 + 1 + 4;

  header[0] = SEPROXYHAL_TAG_NBGL_DRAW_IMAGE_RLE;
  header[1] = (len >> 8) & 0xff;
  header[2] = len & 0xff;

  sys_io_seph_send(header, sizeof(header));
  sys_io_seph_send((const uint8_t *)area, sizeof(nbgl_area_t));
  sys_io_seph_send(buffer, buffer_len);
  sys_io_seph_send((const uint8_t *)&fore_color, 1);
  sys_io_seph_send((const uint8_t *)&nb_skipped_bytes, 1);
  sys_io_seph_send((const uint8_t *)&character, 4);

  return 0;
}
