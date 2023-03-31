#include "nbgl_rle.h"
#include <string.h>

// 4BPP Color maps

// Extract a color from color_map, if color_map is not NULL
#define GET_MAPPED_COLOR(color_map, color_index)                               \
  ((color_map == NULL) ? (color_index) : color_map[(color_index)])

// clang-format off
static const uint8_t WHITE_ON_BLACK[] = {
    WHITE_4BPP, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, BLACK_4BPP
};

static const uint8_t DARK_GRAY_ON_WHITE[] = {
    DARK_GRAY_4BPP, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 11, 12, 13, 14, WHITE_4BPP
};

static const uint8_t LIGHT_GRAY_ON_WHITE[] = {
    LIGHT_GRAY_4BPP, 10, 11, 11, 12, 12, 13, 13, 13, 14, 14, 14, 15, 15, 15, WHITE_4BPP
};

static const uint8_t LIGHT_GRAY_ON_BLACK[] = {
    LIGHT_GRAY_4BPP, 9, 8, 7, 6, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, BLACK_4BPP
};
// clang-format on

static const uint8_t *get_color_map_array(nbgl_area_t *area, color_t color)
{
  if ((color == BLACK) && (area->backgroundColor == WHITE)) {
    return NULL;
  } else if ((color == WHITE) && (area->backgroundColor == BLACK)) {
    return WHITE_ON_BLACK;
  } else if ((color == DARK_GRAY) && (area->backgroundColor == WHITE)) {
    return DARK_GRAY_ON_WHITE;
  } else if ((color == LIGHT_GRAY) && (area->backgroundColor == WHITE)) {
    return LIGHT_GRAY_ON_WHITE;
  } else if ((color == LIGHT_GRAY) && (area->backgroundColor == BLACK)) {
    return LIGHT_GRAY_ON_BLACK;
  } else if ((color == DARK_GRAY) && (area->backgroundColor == BLACK)) {
    return LIGHT_GRAY_ON_BLACK;
  }
  return NULL;
}

// Write nb_pix 4BPP pixels of the same color to the display
static inline void fill_4bpp_pixels_color(uint8_t color, uint8_t nb_pix,
                                          uint32_t *pix_cnt, uint8_t *remaining,
                                          uint32_t max_pix_cnt,
                                          uint8_t *out_buffer)
{
  // Check max
  if ((*pix_cnt + nb_pix) > max_pix_cnt) {
    return;
  }

  uint8_t double_pix = (color << 4) | color;
  bool first_non_aligned = (*pix_cnt % 2);
  bool last_non_aligned = (*pix_cnt + nb_pix) % 2;

  // If first pixel to write is non aligned
  if (first_non_aligned) {
    *remaining |= (0x0F & color);
    // Remaining byte is now full: send it
    // spi_queue_byte(*remaining);
    out_buffer[(*pix_cnt) / 2] = *remaining;
    *remaining = 0;
    (*pix_cnt)++;
    nb_pix--;
  }

  // Write pixels 2 by 2
  for (uint32_t i = 0; i < nb_pix / 2; i++) {
    out_buffer[(*pix_cnt) / 2] = double_pix;
    (*pix_cnt) += 2;
  }

  // If last pixel to write is non aligned
  if (last_non_aligned) {
    // Save remaining pixel
    *remaining = color << 4;
    (*pix_cnt)++;
  }
}

// Handle 'Copy white' RLE instruction
static uint32_t handle_4bpp_repeat_white(uint8_t byte_in, uint32_t *pix_cnt,
                                         const uint8_t *color_map,
                                         uint8_t *remaining_byte,
                                         uint32_t max_pix_cnt,
                                         uint8_t *out_buffer)
{
  uint8_t nb_pix = (byte_in & 0x3F) + 1;
  uint8_t color = GET_MAPPED_COLOR(color_map, 0x0F);
  fill_4bpp_pixels_color(color, nb_pix, pix_cnt, remaining_byte, max_pix_cnt,
                         out_buffer);
  return 1;
}

// Handle 'Repeat color' RLE instruction
static uint32_t handle_4bpp_repeat_color(uint8_t byte_in, uint32_t *pix_cnt,
                                         const uint8_t *color_map,
                                         uint8_t *remaining_byte,
                                         uint32_t max_pix_cnt,
                                         uint8_t *out_buffer)
{
  uint8_t nb_pix = ((byte_in & 0x70) >> 4) + 1;
  uint8_t color = GET_MAPPED_COLOR(color_map, byte_in & 0x0F);
  fill_4bpp_pixels_color(color, nb_pix, pix_cnt, remaining_byte, max_pix_cnt,
                         out_buffer);
  return 1;
}

// Handle 'Copy' RLE instruction
static uint32_t handle_4bpp_copy(uint8_t *bytes_in, uint32_t bytes_in_len,
                                 uint32_t *pix_cnt, const uint8_t *color_map,
                                 uint8_t *remaining_byte, uint32_t max_pix_cnt,
                                 uint8_t *out_buffer)
{
  uint8_t nb_pix = ((bytes_in[0] & 0x30) >> 4) + 3;
  uint8_t nb_bytes_read = (nb_pix / 2) + 1;

  // Do not read outside of bytes_in
  if (nb_bytes_read > bytes_in_len) {
    return nb_bytes_read;
  }

  for (uint8_t i = 0; i < nb_pix; i++) {
    // Write pix by pix
    uint8_t index = (i + 1) / 2;
    uint8_t color_index;
    if ((i % 2) == 0) {
      color_index = bytes_in[index] & 0x0F;
    } else {
      color_index = bytes_in[index] >> 4;
    }
    uint8_t color = GET_MAPPED_COLOR(color_map, color_index);
    fill_4bpp_pixels_color(color, 1, pix_cnt, remaining_byte, max_pix_cnt,
                           out_buffer);
  }

  return nb_bytes_read;
}

/**
 * @brief Uncompress a 4BPP RLE buffer
 *
 * 4BPP RLE Encoder:
 *
 * 'Repeat white' byte
 * - [11][number of whites to write - 1 (6 bits)]
 *
 * 'Copy' byte
 * - [10][number of nibbles to copy - 3 (2)][nib0 (4 bits)][nib1 (4 bits)]...
 *
 * 'Repeat color' byte
 * - [0][number of bytes to write - 1 (3 bits)][color index (4 bits)]
 *
 * @param area
 * @param buffer buffer of RLE-encoded data
 * @param buffer_len length of buffer
 * @param fore_color
 * @param out_buffer output buffer
 * @param out_buffer_len length of output buffer
 * @return
 */

void nbgl_uncompress_rle_4bpp(nbgl_area_t *area, uint8_t *buffer,
                              uint32_t buffer_len, color_t fore_color,
                              uint8_t *out_buffer, uint32_t out_buffer_len)
{
  if (area->bpp != NBGL_BPP_4) {
    return;
  }

  uint32_t max_pix_cnt = (area->width * area->height);

  if (max_pix_cnt * 2 > out_buffer_len) {
    return;
  }

  memset(out_buffer, 0, out_buffer_len);

  const uint8_t *color_map = get_color_map_array(area, fore_color);
  uint32_t pix_cnt = 0;
  uint32_t read_cnt = 0;
  uint8_t remaining = 0;

  while (read_cnt < buffer_len) {
    uint8_t byte = buffer[read_cnt];
    if (byte & 0x80) {
      if (byte & 0x40) {
        read_cnt += handle_4bpp_repeat_white(
            byte, &pix_cnt, color_map, &remaining, max_pix_cnt, out_buffer);
      } else {
        uint8_t *bytes_in = buffer + read_cnt;
        uint32_t max_len = buffer_len - read_cnt;
        read_cnt += handle_4bpp_copy(bytes_in, max_len, &pix_cnt, color_map,
                                     &remaining, max_pix_cnt, out_buffer);
      }
    } else {
      read_cnt += handle_4bpp_repeat_color(byte, &pix_cnt, color_map,
                                           &remaining, max_pix_cnt, out_buffer);
    }
  }

  if ((pix_cnt % 2) != 0) {
    out_buffer[(pix_cnt) / 2] = remaining;
  }
}
