#pragma once

#include <stdbool.h>
#include <stdint.h>

// ============================================================================

#define STAX_FONTS_ARRAY_ADDR 0x00805000
// Latest (starting from SDK_API_LEVEL_15)
#define STAX_NB_FONTS 6
// SDK_API_LEVEL_12 to SDK_API_LEVEL_14
#define STAX_NB_FONTS_12 7

#define FLEX_FONTS_ARRAY_ADDR 0x00805000
#define FLEX_NB_FONTS         6

#define NANOSP_FONTS_ARRAY_ADDR 0x00805000
#define NANOX_FONTS_ARRAY_ADDR  0x00205000
#define NANO_NB_FONTS           3

#define MAX_NB_FONTS    STAX_NB_FONTS
#define MAX_NB_FONTS_12 STAX_NB_FONTS_12

// ============================================================================
// BAGL font related structures
// (They are defined in the SDK, in lib_bagl/include/bagl.h
// ============================================================================

// Latest (starting from SDK_API_LEVEL_12)
typedef struct {
  uint32_t encoding : 2;
  uint32_t bitmap_offset : 12;
  uint32_t width : 5;
  uint32_t x_min_offset : 4;
  uint32_t y_min_offset : 5;
  uint32_t x_max_offset : 4;
} bagl_font_character_t;

typedef struct {
  uint16_t bitmap_len;
  uint8_t font_id;
  uint8_t bpp;
  uint8_t height;
  uint8_t baseline;
  uint8_t first_char;
  uint8_t last_char;
  bagl_font_character_t *characters;
  uint8_t *bitmap;
} bagl_font_t;

// SDK_API_LEVEL_5
typedef struct {
  uint32_t char_width : 4;
  uint32_t y_offset : 4;
  uint32_t x_offset : 3;
  uint32_t bitmap_byte_count : 5;
  uint32_t bitmap_offset : 16;
} bagl_font_character_t_5;

typedef struct {
  unsigned int font_id;
  unsigned char bpp;
  unsigned char char_height;
  unsigned char baseline_height;
  signed char char_leftmost_x;
  unsigned short first_char;
  unsigned short last_char;
  bagl_font_character_t_5 *characters;
  unsigned char *bitmap;
} bagl_font_t_5;

// SDK_API_LEVEL_1
typedef struct {
  unsigned char char_width;
  unsigned char bitmap_byte_count;
  unsigned short bitmap_offset;
} bagl_font_character_t_1;

typedef struct {
  unsigned int font_id;
  unsigned char bpp;
  unsigned char char_height;
  unsigned char baseline_height;
  unsigned char char_kerning;
  unsigned short first_char;
  unsigned short last_char;
  bagl_font_character_t_1 *const characters;
  unsigned char *bitmap;
} bagl_font_t_1;

// ============================================================================
// NBGL font related structures
// (They are defined in the SDK, in lib_nbgl/include/nbgl_fonts.h
// ============================================================================

// Current API_LEVEL (15)
typedef struct {
  uint32_t bitmap_offset;
  uint32_t encoding : 1;
  uint32_t width : 6;
  uint32_t x_min_offset : 4;
  uint32_t y_min_offset : 6;
  uint32_t x_max_offset : 4;
  uint32_t y_max_offset : 6;
} nbgl_font_character_t;

typedef struct {
  uint32_t bitmap_len;
  uint8_t font_id;
  uint8_t bpp;
  uint8_t height;
  uint8_t line_height;
  uint8_t char_kerning;
  uint8_t crop;
  uint8_t y_min;
  uint8_t first_char;
  uint8_t last_char;
  nbgl_font_character_t *characters;
  uint8_t *bitmap;
} nbgl_font_t;

// SDK_API_LEVEL_14
typedef struct {
  uint32_t encoding : 1;
  uint32_t bitmap_offset : 14;
  uint32_t width : 6;
  uint32_t x_min_offset : 3;
  uint32_t y_min_offset : 3;
  uint32_t x_max_offset : 2;
  uint32_t y_max_offset : 3;
} nbgl_font_character_t_14;

typedef struct {
  uint32_t bitmap_len;
  uint8_t font_id;
  uint8_t bpp;
  uint8_t height;
  uint8_t line_height;
  uint8_t char_kerning;
  uint8_t crop;
  uint8_t y_min;
  uint8_t first_char;
  uint8_t last_char;
  nbgl_font_character_t_14 *characters;
  uint8_t *bitmap;
} nbgl_font_t_14;

// SDK_API_LEVEL_12 and SDK_API_LEVEL_13
typedef struct {
  uint32_t encoding : 1;
  uint32_t bitmap_offset : 14;
  uint32_t width : 6;
  uint32_t x_min_offset : 3;
  uint32_t y_min_offset : 3;
  uint32_t x_max_offset : 2;
  uint32_t y_max_offset : 3;
} nbgl_font_character_t_12;

typedef struct {
  uint32_t bitmap_len;
  uint8_t font_id;
  uint8_t bpp;
  uint8_t height;
  uint8_t line_height;
  uint8_t crop;
  uint8_t y_min;
  uint8_t first_char;
  uint8_t last_char;
  nbgl_font_character_t_12 *characters;
  uint8_t *bitmap;
} nbgl_font_t_12;

// ============================================================================

void parse_fonts(void *code, unsigned long text_load_addr,
                 unsigned long fonts_addr, unsigned long fonts_size,
                 bool use_nbgl);

uint32_t get_character_from_bitmap(const uint8_t *bitmap);
