#include "nbgl_rle.h"
#include <stdio.h>
#include <string.h>

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
                                         uint8_t *remaining_byte,
                                         uint32_t max_pix_cnt,
                                         uint8_t *out_buffer)
{
  uint8_t nb_pix = (byte_in & 0x3F) + 1;
  uint8_t color = WHITE_4BPP;
  fill_4bpp_pixels_color(color, nb_pix, pix_cnt, remaining_byte, max_pix_cnt,
                         out_buffer);
  return 1;
}

// Handle 'Repeat color' RLE instruction
static uint32_t handle_4bpp_repeat_color(uint8_t byte_in, uint32_t *pix_cnt,
                                         uint8_t *remaining_byte,
                                         uint32_t max_pix_cnt,
                                         uint8_t *out_buffer)
{
  uint8_t nb_pix = ((byte_in & 0x70) >> 4) + 1;
  uint8_t color = byte_in & 0x0F;
  fill_4bpp_pixels_color(color, nb_pix, pix_cnt, remaining_byte, max_pix_cnt,
                         out_buffer);
  return 1;
}

// Handle 'Copy' RLE instruction
static uint32_t handle_4bpp_copy(uint8_t *bytes_in, uint32_t bytes_in_len,
                                 uint32_t *pix_cnt, uint8_t *remaining_byte,
                                 uint32_t max_pix_cnt, uint8_t *out_buffer)
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

    fill_4bpp_pixels_color(color_index, 1, pix_cnt, remaining_byte, max_pix_cnt,
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
 * @param out_buffer output buffer
 * @param out_buffer_len length of output buffer
 * @return
 */

static void nbgl_uncompress_rle_4bpp(nbgl_area_t *area, uint8_t *buffer,
                                     uint32_t buffer_len, uint8_t *out_buffer,
                                     uint32_t out_buffer_len)
{
  uint32_t max_pix_cnt = (area->width * area->height);

  if (max_pix_cnt * 2 > out_buffer_len) {
    return;
  }

  memset(out_buffer, 0xFF, out_buffer_len);
  if (!buffer_len) {
    return;
  }

  uint32_t pix_cnt = 0;
  uint32_t read_cnt = 0;
  uint8_t remaining = 0;

  while (read_cnt < buffer_len) {
    uint8_t byte = buffer[read_cnt];
    if (byte & 0x80) {
      if (byte & 0x40) {
        read_cnt += handle_4bpp_repeat_white(byte, &pix_cnt, &remaining,
                                             max_pix_cnt, out_buffer);
      } else {
        uint8_t *bytes_in = buffer + read_cnt;
        uint32_t max_len = buffer_len - read_cnt;
        read_cnt += handle_4bpp_copy(bytes_in, max_len, &pix_cnt, &remaining,
                                     max_pix_cnt, out_buffer);
      }
    } else {
      read_cnt += handle_4bpp_repeat_color(byte, &pix_cnt, &remaining,
                                           max_pix_cnt, out_buffer);
    }
  }

  if ((pix_cnt % 2) != 0) {
    out_buffer[(pix_cnt) / 2] = remaining;
  }
}

/**
 * @brief Uncompress a 1BPP RLE buffer
 *
 * 1BPP RLE Encoder:
 *
 * compressed bytes contains ZZZZOOOO nibbles, with
 * - ZZZZ: number of consecutives zeros (from 0 to 15)
 * - OOOO: number of consecutives ones (from 0 to 15)
 *
 * @param area
 * @param buffer buffer of RLE-encoded data
 * @param buffer_len length of buffer
 * @param out_buffer output buffer
 * @param out_buffer_len length of output buffer
 * @return
 */

static void nbgl_uncompress_rle_1bpp(nbgl_area_t *area, uint8_t *buffer,
                                     uint32_t buffer_len, uint8_t *out_buffer,
                                     uint32_t out_buffer_len)
{
  size_t index = 0;
  size_t nb_zeros = 0;
  size_t nb_ones = 0;
  uint8_t pixels = 0;
  size_t nb_pixels = 0;
  size_t remaining_pixels = area->width * area->height;

  memset(out_buffer, 0, out_buffer_len);
  if (!buffer_len) {
    return;
  }

  while (remaining_pixels > 0 && (index < buffer_len || nb_zeros || nb_ones)) {

    // Reload nb_zeros & nb_ones if needed
    while (!nb_zeros && !nb_ones && index < buffer_len) {
      uint8_t byte = buffer[index++];
      nb_ones = byte & 0x0F;
      nb_zeros = byte >> 4;
    }
    // Get next pixel
    pixels <<= 1;
    if (nb_zeros) {
      --nb_zeros;
      // Next line do nothing, just kept for clarity
      pixels |= 0; // Add a 0 in bit 0
    } else if (nb_ones) {
      --nb_ones;
      pixels |= 1; // Add a 1 in bit 0
    }
    ++nb_pixels;
    if (nb_pixels == 8) {
      *out_buffer++ = pixels;
      remaining_pixels -= 8;
      nb_pixels -= 8;
    }
  }

  // Is there some remaining pixels to store?
  if (nb_pixels) {
    // Put the remaining pixels on MSB
    pixels <<= 8 - nb_pixels;
    *out_buffer++ = pixels;
  }
}

void nbgl_uncompress_rle(nbgl_area_t *area, uint8_t *buffer,
                         uint32_t buffer_len, uint8_t *out_buffer,
                         uint32_t out_buffer_len)
{
  switch (area->bpp) {
  case NBGL_BPP_4:
    nbgl_uncompress_rle_4bpp(area, buffer, buffer_len, out_buffer,
                             out_buffer_len);
    break;
  case NBGL_BPP_1:
    nbgl_uncompress_rle_1bpp(area, buffer, buffer_len, out_buffer,
                             out_buffer_len);
    break;
  }
}
