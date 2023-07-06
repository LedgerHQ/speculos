#pragma once

#include <stddef.h>
#include <stdint.h>

/**
 * Width of the front screen in pixels
 */
#define SCREEN_WIDTH 400

/**
 * Height of the front screen in pixels
 */
#define SCREEN_HEIGHT 672

/**
 * Value on 4BPP of WHITE
 *
 */
#define WHITE_4BPP 0xF

/**
 * Value on 4BPP of LIGHT_GRAY
 *
 */
#define LIGHT_GRAY_4BPP 0xA

/**
 * Value on 4BPP of DARK_GRAY
 *
 */
#define DARK_GRAY_4BPP 0x5

/**
 * Value on 4BPP of BLACK
 *
 */
#define BLACK_4BPP 0x0

/**
 * No transformation
 *
 */
#define NO_TRANSFORMATION 0
/**
 * Horizontal mirroring when rendering bitmap
 *
 */
#define HORIZONTAL_MIRROR 0x1
/**
 * Vertical mirroring when rendering bitmap
 *
 */
#define VERTICAL_MIRROR 0x2

/**
 * Both directions mirroring when rendering bitmap
 *
 */
#define BOTH_MIRRORS (HORIZONTAL_MIRROR | VERTICAL_MIRROR)

/**
 * Code to be used for color map when not used
 *
 */
#define INVALID_COLOR_MAP 0x0

/**
 * @brief Enum to represent the number of bits per pixel (BPP)
 *
 */
typedef enum {
  NBGL_BPP_1 = 0, ///< 1 bit per pixel
  NBGL_BPP_2,     ///< 2 bits per pixel
  NBGL_BPP_4,     ///< 4 bits per pixel
  NB_NBGL_BPP,    ///< Number of NBGL_BPP enums
} nbgl_bpp_t;

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

/**
 * @brief Available post-refresh power modes
 *
 * - Power off after a refresh allows to save power
 * - Keep the screen powered on after a refresh allows to
 *   achieve a faster following refresh.
 */
typedef enum nbgl_post_refresh_t {
  POST_REFRESH_FORCE_POWER_OFF,  ///< Force screen power off after refresh
  POST_REFRESH_FORCE_POWER_ON,   ///< Force screen power on after refresh
  POST_REFRESH_KEEP_POWER_STATE, ///< Keep screen power state after refresh
} nbgl_post_refresh_t;

typedef enum { BLACK = 0, DARK_GRAY, LIGHT_GRAY, WHITE } color_t;

unsigned long sys_nbgl_front_draw_rect(nbgl_area_t *area);

unsigned long sys_nbgl_front_draw_horizontal_line(nbgl_area_t *area,
                                                  uint8_t mask,
                                                  color_t lineColor);

unsigned long sys_nbgl_front_draw_img(nbgl_area_t *area, uint8_t *buffer,
                                      nbgl_transformation_t transformation,
                                      nbgl_color_map_t colorMap);

unsigned long sys_nbgl_front_refresh_area_legacy(nbgl_area_t *area);

unsigned long sys_nbgl_front_refresh_area(nbgl_area_t *area,
                                          nbgl_post_refresh_t post_refresh);

unsigned long sys_nbgl_front_draw_img_file(nbgl_area_t *area, uint8_t *buffer,
                                           nbgl_color_map_t colorMap,
                                           uint8_t *optional_uzlib_work_buffer);

unsigned long sys_nbgl_get_font(unsigned int fontId);

unsigned long sys_nbgl_screen_reinit(void);

unsigned long sys_nbgl_front_draw_img_rle_10(nbgl_area_t *area, uint8_t *buffer,
                                             uint32_t buffer_len,
                                             color_t fore_color);

unsigned long sys_nbgl_front_draw_img_rle_legacy(nbgl_area_t *area,
                                                 uint8_t *buffer,
                                                 uint32_t buffer_len,
                                                 color_t fore_color);

unsigned long sys_nbgl_front_draw_img_rle(nbgl_area_t *area, uint8_t *buffer,
                                          uint32_t buffer_len,
                                          color_t fore_color,
                                          uint8_t nb_skipped_bytes);
