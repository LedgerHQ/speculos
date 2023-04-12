#include "nbgl.h"
#include <stdbool.h>

// Functions dedicated to RLE uncompression

void nbgl_uncompress_rle(nbgl_area_t *area, uint8_t *buffer,
                         uint32_t buffer_len, uint8_t *out_buffer,
                         uint32_t out_buffer_len);
