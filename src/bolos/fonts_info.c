#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "fonts.h"
#include "sdk.h"

#define MAX_BITMAP_CHAR (MAX_NB_FONTS * 128)

typedef struct {
  const uint8_t *bitmap;
  uint32_t character;
} BITMAP_CHAR;

BITMAP_CHAR bitmap_char[MAX_BITMAP_CHAR];
uint32_t nb_bitmap_char;

// Return the real addr depending on where the app was loaded
static void *remap_addr(void *code, uint32_t addr, uint32_t text_load_addr)
{
  // No remap on Stax, fonts are loaded at a fixed location
  if (hw_model == MODEL_STAX) {
    return (void *)addr;
  }
  uint8_t *ptr = code;
  ptr += addr - text_load_addr;

  return ptr;
}

// Store bitmap/character pair
static void add_bitmap_character(uint8_t *bitmap, uint32_t character)
{
  if (nb_bitmap_char >= MAX_BITMAP_CHAR) {
    fprintf(stdout, "ERROR: we reached MAX_BITMAP_CHAR!\n");
    return;
  }
  // Space character is empty and have no bitmap -> erase it with next character
  if (nb_bitmap_char && bitmap_char[nb_bitmap_char - 1].bitmap == bitmap) {
    bitmap_char[nb_bitmap_char - 1].character = character;
    return;
  }
  bitmap_char[nb_bitmap_char].bitmap = bitmap;
  bitmap_char[nb_bitmap_char].character = character;
  ++nb_bitmap_char;
}

// Function used by qsort/bsearch to quickly sort/find bitmap_char pairs
int compare_bitmap_char(const void *ptr1, const void *ptr2)
{
  return ((const BITMAP_CHAR *)ptr1)->bitmap -
         ((const BITMAP_CHAR *)ptr2)->bitmap;
}

// Return the character corresponding to provided bitmap, or 0 if not found
uint32_t get_character_from_bitmap(const uint8_t *bitmap)
{
  BITMAP_CHAR value;
  BITMAP_CHAR *result;

  if (!nb_bitmap_char) {
    return 0;
  }

  value.bitmap = bitmap;
  value.character = 0;

  // Use bsearch to speed lookup
  result = bsearch(&value, bitmap_char, nb_bitmap_char, sizeof(bitmap_char[0]),
                   compare_bitmap_char);

  if (result != NULL) {
    return result->character;
  }
  return 0;
}

// Parse provided NBGL font and add bitmap/character pairs
static void parse_nbgl_font(nbgl_font_t *nbgl_font)
{
  uint8_t *bitmap = nbgl_font->bitmap;
  nbgl_font_character_t *characters = nbgl_font->characters;

  for (uint32_t c = nbgl_font->first_char; c <= nbgl_font->last_char;
       c++, characters++) {
    // Be sure data is coherent
    if (characters->bitmap_offset >= nbgl_font->bitmap_len) {
      fprintf(stdout, "bitmap_offset (%d) is >= bitmap_len (%u)!\n",
              characters->bitmap_offset, nbgl_font->bitmap_len);
      return;
    }
    uint8_t *ptr = bitmap + characters->bitmap_offset;
    add_bitmap_character(ptr, c);
  }
}

// Parse provided BAGL font and add bitmap/character pairs
static void parse_bagl_font(bagl_font_t *bagl_font, void *code,
                            unsigned long text_load_addr)
{
  uint8_t *bitmap =
      remap_addr(code, (uint32_t)bagl_font->bitmap, text_load_addr);
  bagl_font_character_t *characters =
      remap_addr(code, (uint32_t)bagl_font->characters, text_load_addr);

  for (uint32_t c = bagl_font->first_char; c <= bagl_font->last_char;
       c++, characters++) {
    // Be sure data is coherent
    if (characters->bitmap_offset >= bagl_font->bitmap_len) {
      fprintf(stdout, "bitmap_offset (%d) is >= bitmap_len (%u)!\n",
              characters->bitmap_offset, bagl_font->bitmap_len);
      return;
    }
    uint8_t *ptr = bitmap + characters->bitmap_offset;
    add_bitmap_character(ptr, c);
  }
}

// Parse provided SDK_API_LEVEL_5 BAGL font and add bitmap/character pairs
static void parse_bagl_font_5(bagl_font_t_5 *bagl_font, void *code,
                              unsigned long text_load_addr)
{
  uint8_t *bitmap =
      remap_addr(code, (uint32_t)bagl_font->bitmap, text_load_addr);
  bagl_font_character_t_5 *characters =
      remap_addr(code, (uint32_t)bagl_font->characters, text_load_addr);
  uint32_t last_character = bagl_font->last_char - bagl_font->first_char;
  uint32_t bitmap_len = characters[last_character].bitmap_offset +
                        characters[last_character].bitmap_byte_count;

  for (uint32_t c = bagl_font->first_char; c <= bagl_font->last_char;
       c++, characters++) {
    // Be sure data is coherent
    if (characters->bitmap_offset >= bitmap_len) {
      fprintf(stdout, "bitmap_offset (%d) is >= bitmap_len (%u)!\n",
              characters->bitmap_offset, bitmap_len);
      return;
    }
    uint8_t *ptr = bitmap + characters->bitmap_offset;
    add_bitmap_character(ptr, c);
  }
}

// Parse provided SDK_API_LEVEL_1 BAGL font and add bitmap/character pairs
static void parse_bagl_font_1(bagl_font_t_1 *bagl_font, void *code,
                              unsigned long text_load_addr)
{
  uint8_t *bitmap =
      remap_addr(code, (uint32_t)bagl_font->bitmap, text_load_addr);
  bagl_font_character_t_1 *characters =
      remap_addr(code, (uint32_t)bagl_font->characters, text_load_addr);
  uint32_t last_character = bagl_font->last_char - bagl_font->first_char;
  uint32_t bitmap_len = characters[last_character].bitmap_offset +
                        characters[last_character].bitmap_byte_count;

  for (uint32_t c = bagl_font->first_char; c <= bagl_font->last_char;
       c++, characters++) {
    // Be sure data is coherent
    if (characters->bitmap_offset >= bitmap_len) {
      fprintf(stdout, "bitmap_offset (%d) is >= bitmap_len (%u)!\n",
              characters->bitmap_offset, bitmap_len);
      return;
    }
    uint8_t *ptr = bitmap + characters->bitmap_offset;
    add_bitmap_character(ptr, c);
  }
}

// Parse all fonts located at provided addr
void parse_fonts(void *code, unsigned long text_load_addr,
                 unsigned long fonts_addr, unsigned long fonts_size)
{
  // Number of fonts stored at fonts_addr
  uint32_t nb_fonts;
  uint32_t *fonts;

  nb_bitmap_char = 0;

  // Be sure API_LEVEL is supported
  switch (sdk_version) {
  // Supported API_LEVELs
  case SDK_API_LEVEL_1:
  case SDK_API_LEVEL_5:
  case SDK_API_LEVEL_12:
  case SDK_API_LEVEL_13:
    break;
  default:
    // Unsupported API_LEVEL, will not parse fonts!
    return;
  }
  // On Stax, fonts are loaded at a known location
  if (hw_model == MODEL_STAX) {
    fonts = (void *)STAX_FONTS_ARRAY_ADDR;
    nb_fonts = STAX_NB_FONTS;
  } else {
    fonts = remap_addr(code, fonts_addr, text_load_addr);
    nb_fonts = fonts_size / 4;
  }

  // There is no font or we don't know its format
  if (!nb_fonts) {
    return;
  }

  // Checks that fonts & nb_fonts are coherent
  if (fonts[nb_fonts] != nb_fonts) {
    fprintf(stdout, "ERROR: Expecting nb_fonts=%u and found %u instead!\n",
            nb_fonts, fonts[nb_fonts]);
    return;
  }
  if (nb_fonts > MAX_NB_FONTS) {
    fprintf(stdout, "ERROR: nb_fonts (%u) is bigger than MAX_NB_FONTS (%d)!\n",
            nb_fonts, MAX_NB_FONTS);
    return;
  }

  // Parse all those fonts and add bitmap/character pairs
  for (uint32_t i = 0; i < nb_fonts; i++) {
    if (hw_model == MODEL_STAX) {
      parse_nbgl_font((void *)fonts[i]);
    } else {
      void *font = remap_addr(code, fonts[i], text_load_addr);

      switch (sdk_version) {
      case SDK_API_LEVEL_1:
        parse_bagl_font_1(font, code, text_load_addr);
        break;
      case SDK_API_LEVEL_5:
        parse_bagl_font_5(font, code, text_load_addr);
        break;
      default:
        parse_bagl_font(font, code, text_load_addr);
        break;
      }
    }
  }

  // Sort all those bitmap/character pairs (should be quick, they are almost
  // sorted)
  qsort(bitmap_char, nb_bitmap_char, sizeof(bitmap_char[0]),
        compare_bitmap_char);
}
