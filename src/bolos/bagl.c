#include <err.h>
#include <string.h>

#include "bagl.h"
#include "emulate.h"
#include "fonts.h"

#define SEPROXYHAL_TAG_SCREEN_DISPLAY_RAW_STATUS_START 0x00
#define SEPROXYHAL_TAG_SCREEN_DISPLAY_RAW_STATUS_CONT  0x01

#define SEPROXYHAL_TAG_BAGL_DRAW_RECT   0xF1
#define SEPROXYHAL_TAG_BAGL_DRAW_BITMAP 0xF2
#define SEPROXYHAL_TAG_BAGL_DRAW_BITMAP_START                                  \
  SEPROXYHAL_TAG_SCREEN_DISPLAY_RAW_STATUS_START
#define SEPROXYHAL_TAG_BAGL_DRAW_BITMAP_CONT                                   \
  SEPROXYHAL_TAG_SCREEN_DISPLAY_RAW_STATUS_CONT

#define BAGL_FILL      1
#define BAGL_RECTANGLE 3

#define MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct __attribute__((__packed__)) {
  uint8_t type;
  uint8_t userid;
  int16_t x;
  int16_t y;
  uint16_t width;
  uint16_t height;
  uint8_t stroke;
  uint8_t radius;
  uint8_t fill;
  uint8_t padding1[3];
  uint32_t fgcolor;
  uint32_t bgcolor;
  uint16_t font_id;
  uint8_t icon_id;
  uint8_t padding2[1];
} bagl_component_t;

unsigned long sys_bagl_hal_draw_rect(unsigned int color, int x, int y,
                                     unsigned int width, unsigned int height)
{
  bagl_component_t c;
  uint8_t header[3];
  size_t len;

  memset(&c, 0, sizeof(c));

  c.type = BAGL_RECTANGLE;
  c.fgcolor = color;
  c.x = x;
  c.y = y;
  c.width = width;
  c.height = height;
  c.fill = BAGL_FILL;

  len = sizeof(c);

  header[0] = SEPROXYHAL_TAG_BAGL_DRAW_RECT;
  header[1] = (len >> 8) & 0xff;
  header[2] = len & 0xff;

  sys_io_seph_send(header, sizeof(header));
  sys_io_seph_send((const uint8_t *)&c, len);

  return 0;
}

#define HBE(value, dst, offset)                                                \
  do {                                                                         \
    dst[offset++] = (value >> 8) & 0xff;                                       \
    dst[offset++] = value & 0xff;                                              \
  } while (0)

#define IBE(value, dst, offset)                                                \
  do {                                                                         \
    dst[offset++] = (value >> 24) & 0xff;                                      \
    dst[offset++] = (value >> 16) & 0xff;                                      \
    dst[offset++] = (value >> 8) & 0xff;                                       \
    dst[offset++] = value & 0xff;                                              \
  } while (0)

size_t build_chunk(uint8_t *buf, size_t *offset, size_t size,
                   const uint8_t *bitmap, const size_t bitmap_length)
{
  size_t len = MIN(size, bitmap_length - *offset);

  memcpy(buf, bitmap + *offset, len);
  *offset += len;

  return len;
}

unsigned long sys_bagl_hal_draw_bitmap_within_rect(
    int x, int y, unsigned int width, unsigned int height,
    unsigned int color_count, const unsigned int *colors,
    unsigned int bit_per_pixel, const uint8_t *bitmap,
    unsigned int bitmap_length_bits)
{
  size_t i, len, size;
  uint8_t buf[300 - 4]; /* size limit in io_seph_send */
  uint8_t header[4];
  uint32_t character = get_character_from_bitmap(bitmap);
  size = 0;

  HBE(x, buf, size);
  HBE(y, buf, size);
  HBE(width, buf, size);
  HBE(height, buf, size);

  buf[size++] = bit_per_pixel;

  IBE(character, buf, size);

  if (size + color_count * sizeof(unsigned int) > sizeof(buf)) {
    errx(1, "color overflow in sys_bagl_hal_draw_bitmap_within_rect\n");
  }

  for (i = 0; i < color_count; i++) {
    *(unsigned int *)&buf[size] = colors[i];
    size += sizeof(unsigned int);
  }

  size_t bitmap_length = bitmap_length_bits / 8;
  if (bitmap_length_bits % 8 != 0) {
    bitmap_length += 1;
  }
  size_t offset = 0;
  len = build_chunk(buf + size, &offset, sizeof(buf) - size, bitmap,
                    bitmap_length);

  header[0] = SEPROXYHAL_TAG_BAGL_DRAW_BITMAP;
  header[1] = ((size + len + 1) >> 8) & 0xff;
  header[2] = (size + len + 1) & 0xff;
  header[3] = SEPROXYHAL_TAG_BAGL_DRAW_BITMAP_START;

  sys_io_seph_send(header, sizeof(header));
  sys_io_seph_send(buf, size + len);

  while (true) {
    len = build_chunk(buf, &offset, sizeof(buf), bitmap, bitmap_length);
    if (len == 0) {
      break;
    }

    // wait for displayed event
    uint8_t tmp[64];
    sys_io_seproxyhal_spi_recv(tmp, sizeof(tmp), 0);

    header[0] = SEPROXYHAL_TAG_BAGL_DRAW_BITMAP;
    header[1] = ((len + 1) >> 8) & 0xff;
    header[2] = (len + 1) & 0xff;
    header[3] = SEPROXYHAL_TAG_BAGL_DRAW_BITMAP_CONT;

    sys_io_seph_send(header, sizeof(header));
    sys_io_seph_send(buf, len);
  }

  return 0x9000; // SWO_SUCCESS
}

unsigned long sys_screen_update(void)
{
  return 0;
}

unsigned long sys_screen_clear(void)
{
  // assume 64 pixels height even on Nano S (no impact)
  return sys_bagl_hal_draw_rect(0, 0, 0, 128, 64);
}
