from collections import namedtuple

'''
 {BAGL_GLYPH_ICON_DOWN, 7, 4, 1, C_icon_down_colors, C_icon_down_bitmap},
unsigned int const C_icon_down_colors[] = {
  0x00000000,
  0x00ffffff,

};

unsigned char const C_icon_down_bitmap[] = {
  0x41, 0x11, 0x05, 0x01,
};

typedef struct {
  unsigned int         icon_id;
  unsigned int         width;
  unsigned int         height;
  unsigned int         bits_per_pixel;
  const unsigned int*  default_colors; // color array entry count is (1<<bits_per_pixel)
  const unsigned char* bitmap;
} bagl_glyph_array_entry_t;
'''

Glyph = namedtuple("Glyph", "icon_id width height bpp colors bitmap")

BAGL_GLYPH_ICON_CHECK       =  6
BAGL_GLYPH_ICON_CROSS       =  7
BAGL_GLYPH_ICON_LEFT        =  9
BAGL_GLYPH_ICON_RIGHT       = 10
BAGL_GLYPH_ICON_UP          = 11
BAGL_GLYPH_ICON_DOWN        = 12

GLYPHS = [
    Glyph(BAGL_GLYPH_ICON_CHECK, 8, 6, 1, [0x00000000, 0x00ffffff], [0x80, 0x40, 0x20, 0x11, 0x0a, 0x04]),
    Glyph(BAGL_GLYPH_ICON_CROSS, 7, 7, 1, [0x00000000, 0x00ffffff], [0x41, 0x11, 0x05, 0x41, 0x11, 0x05, 0x01]),
    Glyph(BAGL_GLYPH_ICON_LEFT, 4, 7, 1, [0x00000000, 0x00ffffff], [0x48, 0x12, 0x42, 0x08]),
    Glyph(BAGL_GLYPH_ICON_RIGHT, 4, 7, 1, [0x00000000, 0x00ffffff], [0x21, 0x84, 0x24, 0x01]),
    Glyph(BAGL_GLYPH_ICON_UP, 7, 4, 1, [0x00000000, 0x00ffffff], [0x08, 0x8a, 0x28, 0x08]),
    Glyph(BAGL_GLYPH_ICON_DOWN, 7, 4, 1, [0x00000000, 0x00ffffff], [0x41, 0x11, 0x05, 0x01]),
]

def get(icon_id):
    for glyph in GLYPHS:
        if glyph.icon_id == icon_id:
            return glyph
    return None
