#define _SDK_2_0_
#include <err.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <stdbool.h>
#include <stdio.h>

#include "bolos/bagl.h"
#include "bolos/cx_aes.h"
#include "bolos/cxlib.h"
#include "bolos/endorsement.h"
#include "emulate.h"

#include "bolos_syscalls_api_level_1.h"

typedef uint8_t nbgl_transformation_t;
typedef uint8_t nbgl_color_map_t;

typedef enum { BLACK = 0, DARK_GRAY, LIGHT_GRAY, WHITE } color_t;

typedef enum {
  NBGL_BPP_1 = 0, ///< 1 bit per pixel
  NBGL_BPP_2,     ///< 2 bits per pixel
  NBGL_BPP_4,     ///< 4 bits per pixel
} nbgl_bpp_t;

typedef struct __attribute__((__packed__)) nbgl_area_s {
  uint16_t x0;     ///< horizontal position of the upper left point of the area
  uint16_t y0;     ///< vertical position of the upper left point of the area
  uint16_t width;  ///< width of the area, in pixels
  uint16_t height; ///< height of the area, in pixels
  uint8_t backgroundColor; ///< color (usually background) to be applied
  uint8_t bpp;             ///< bits per pixel for this area
} nbgl_area_t;

#define SEPROXYHAL_TAG_FINGER_EVENT         0x0C
#define SEPROXYHAL_TAG_FINGER_EVENT_TOUCH   0x01
#define SEPROXYHAL_TAG_FINGER_EVENT_RELEASE 0x02
#define SEPROXYHAL_TAG_NBGL_DRAW_RECT       0x6A
#define SEPROXYHAL_TAG_NBGL_REFRESH         0x6B
#define SEPROXYHAL_TAG_NBGL_DRAW_LINE       0x6C
#define SEPROXYHAL_TAG_NBGL_DRAW_IMAGE      0x6D
#define SEPROXYHAL_TAG_NBGL_DRAW_IMAGE_FILE 0x6E

typedef struct io_touch_info_s {
  uint16_t x;
  uint16_t y;
  uint8_t state;
} io_touch_info_t;

// Last touch infos sent by seph.
// These infos can be returned by touch_get_last_infos syscall.
static io_touch_info_t last_touch_infos;

void catch_touch_info_from_seph(uint8_t *buffer, uint16_t size)
{
  if (size < 5) {
    return;
  }

  if (buffer[0] != SEPROXYHAL_TAG_FINGER_EVENT) {
    return;
  }

  last_touch_infos.state = buffer[3];
  last_touch_infos.x = (buffer[4] << 8) + buffer[5];
  last_touch_infos.y = (buffer[6] << 8) + buffer[7];
}

unsigned long sys_touch_get_last_info(io_touch_info_t *info)
{
  memcpy(info, &last_touch_infos, sizeof(io_touch_info_t));
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

int emulate_sdk_api_level_1(unsigned long syscall, unsigned long *parameters,
                            unsigned long *ret, bool verbose, hw_model_t model)
{
  switch (syscall) {
    /* clang-format off */
  SYSCALL9(bagl_hal_draw_bitmap_within_rect, "(%d, %d, %u, %u, %u, %p, %u, %p, %u)",
           int,                  x,
           int,                  y,
           unsigned int,         width,
           unsigned int,         height,
           unsigned int,         color_count,
           const unsigned int *, colors,
           unsigned int,         bit_per_pixel,
           const uint8_t *,      bitmap,
           unsigned int,         bitmap_length_bits);

  SYSCALL5(bagl_hal_draw_rect, "(0x%08x, %d, %d, %u, %u)",
           unsigned int, color,
           int,          x,
           int,          y,
           unsigned int, width,
           unsigned int, height);

  SYSCALL0(screen_update);

  SYSCALL0(get_api_level);

  SYSCALL2(cx_get_random_bytes, "(%p %u)",
           uint8_t *, buffer,
           size_t,    len);

  SYSCALL2(cx_trng_get_random_data, "(%p %u)",
           uint8_t *, buffer,
           size_t,    len);

  SYSCALL2(cx_crc32_hw, "(%p %u)",
           uint8_t *, buffer,
           size_t,    len);

  SYSCALL2(cx_aes_set_key_hw, "(%p %u)",
           void *,    key,
           uint32_t,  mode);

  SYSCALL2(cx_aes_block_hw, "(%p %p)",
           uint8_t *, in,
           uint8_t *, out);

  SYSCALL0v(cx_aes_reset_hw);

  SYSCALL0(try_context_get);

  SYSCALL3(io_seph_recv, "(%p, %u, 0x%x)",
           uint8_t *,    buffer,
           uint16_t,     maxlength,
           unsigned int, flags);

  SYSCALL0(io_seph_is_status_sent);

  SYSCALL2(io_seph_send, "(%p, %u)",
           uint8_t *, buffer,
           uint16_t,  length);

  SYSCALL8(os_perso_derive_node_with_seed_key, "(0x%x, 0x%x, %p, %u, %p, %p, %p, %u)",
           unsigned int,         mode,
           cx_curve_t,           curve,
           const unsigned int *, path,
           unsigned int,         pathLength,
           unsigned char *,      privateKey,
           unsigned char *,      chain,
           unsigned char *,      seed_key,
           unsigned int,         seed_key_length);

  SYSCALL1i(os_ux, "(%p)", bolos_ux_params_t *, params, os_ux_2_0);
    /* clang-format on */

    SYSCALL2(cx_ecdomain_size, "(%u %p)", unsigned int, cv, size_t *, length);

    SYSCALL2(cx_ecdomain_parameters_length, "(%u %p)", unsigned int, cv,
             size_t *, length);

    SYSCALL4(cx_ecdomain_generator, "(%u %p %p %u)", unsigned int, cv, void *,
             Gx, void *, Gy, size_t, len);

    SYSCALL2(cx_ecdomain_generator_bn, "(%u %p)", unsigned int, cv, void *, P);

    SYSCALL3(cx_ecdomain_parameter_bn, "(%u, %d, %u)", unsigned int, cv, int,
             id, uint32_t, p);

    SYSCALL4(cx_ecdomain_parameter, "(0x%x, %u, %p, %u)", cx_curve_t, curve,
             cx_curve_dom_param_t, id, uint8_t *, p, uint32_t, p_len);

    SYSCALL2(cx_ecpoint_alloc, "(%p %u)", void *, p, cx_curve_t, cv);

    SYSCALL1(cx_ecpoint_destroy, "(%p)", void *, P);

    SYSCALL5(cx_ecpoint_init, "(%p, %p, %u, %p, %u)", void *, p, uint8_t *, x,
             size_t, x_len, uint8_t *, y, size_t, y_len);

    SYSCALL3(cx_ecpoint_init_bn, "(%p, %u, %u)", void *, p, uint32_t, x,
             uint32_t, y);

    SYSCALL3(cx_ecpoint_export_bn, "(%p, %p, %p)", void *, p, uint32_t *, x,
             uint32_t *, y);

    SYSCALL5(cx_ecpoint_export, "(%p, %p, %u, %p, %u)", void *, p, uint8_t *, x,
             size_t, x_len, uint8_t *, y, size_t, y_len);

    SYSCALL4(cx_ecpoint_compress, "(%p, %p, %u, %p)", void *, p, uint8_t *,
             xy_compressed, size_t, xy_compressed_len, uint32_t *, sign);

    SYSCALL4(cx_ecpoint_decompress, "(%p, %p, %u, %u)", void *, p, uint8_t *,
             xy_compressed, size_t, xy_compressed_len, uint32_t, sign);

    SYSCALL3(cx_ecpoint_add, "(%p, %p, %p)", void *, eR, void *, eP, void *,
             eQ);

    SYSCALL1(cx_ecpoint_neg, "(%p)", void *, eP);

    SYSCALL3(cx_ecpoint_cmp, "(%p, %p, %p)", void *, eP, void *, eQ, bool *,
             is_equal);

    SYSCALL3(cx_ecpoint_scalarmul, "(%p, %p, %u)", void *, p, uint8_t *, k,
             size_t, k_len);

    SYSCALL2(cx_ecpoint_scalarmul_bn, "(%p, %u)", void *, ec_P, uint32_t, bn_k);

    SYSCALL3(cx_ecpoint_rnd_scalarmul, "(%p, %p, %u)", void *, p, uint8_t *, k,
             size_t, k_len);

    SYSCALL2(cx_ecpoint_rnd_scalarmul_bn, "(%p, %u)", void *, ec_P, uint32_t,
             bn_k);

    SYSCALL3(cx_ecpoint_rnd_fixed_scalarmul, "(%p, %p, %u)", void *, p,
             uint8_t *, k, size_t, k_len);

    SYSCALL7(cx_ecpoint_double_scalarmul, "(%p, %p, %p, %p, %u, %p, %u)",
             void *, eR, void *, eP, void *, eQ, uint8_t *, k, size_t, k_len,
             uint8_t *, r, size_t, r_len);

    SYSCALL5(cx_ecpoint_double_scalarmul_bn, "(%p, %p, %p, %u, %u)", void *, eR,
             void *, eP, void *, eQ, uint32_t, k, uint32_t, r);

    SYSCALL2(cx_ecpoint_is_at_infinity, "(%p, %p)", void *, ec_P, bool *,
             is_infinite);

    SYSCALL2(cx_ecpoint_is_on_curve, "(%p, %p)", void *, ec_P, bool *,
             is_on_curve);

    SYSCALL0(cx_bn_is_locked);

    SYSCALL2(cx_bn_lock, "(%u %u)", size_t, word_nbytes, uint32_t, flags);

    SYSCALL0(cx_bn_unlock);

    SYSCALL2(cx_bn_alloc, "(%p %u)", void *, x, size_t, nbytes);

    SYSCALL2(cx_bn_copy, "(%u %u)", uint32_t, a, uint32_t, b);

    SYSCALL4(cx_bn_alloc_init, "(%p, %u, %p, %u)", void *, x, size_t, nbytes,
             uint8_t *, value, size_t, value_nbytes);

    SYSCALL1(cx_bn_destroy, "(%p)", void *, x);

    SYSCALL2(cx_bn_nbytes, "(%u, %p)", uint32_t, x, size_t *, nbytes);

    SYSCALL3(cx_bn_init, "(%u, %p, %u)", uint32_t, x, uint8_t *, value, size_t,
             value_nbytes);

    SYSCALL1(cx_bn_rand, "(%u)", uint32_t, x);

    SYSCALL2(cx_bn_rng, "(%u, %u)", uint32_t, r, uint32_t, n);

    SYSCALL3(cx_bn_tst_bit, "(%u, %u, %p)", uint32_t, a, uint32_t, b, bool *,
             set);

    SYSCALL2(cx_bn_set_bit, "(%u, %u)", uint32_t, r, uint32_t, n);

    SYSCALL2(cx_bn_clr_bit, "(%u, %u)", uint32_t, r, uint32_t, n);

    SYSCALL2(cx_bn_shr, "(%u, %u)", uint32_t, r, uint32_t, n);

    SYSCALL2(cx_bn_shl, "(%u, %u)", uint32_t, r, uint32_t, n);

    SYSCALL3(cx_bn_mod_invert_nprime, "(%u, %u, %u)", uint32_t, r, uint32_t, a,
             uint32_t, n);

    SYSCALL3(cx_bn_mod_u32_invert, "(%u, %u, %u)", uint32_t, r, uint32_t, a,
             uint32_t, n);

    SYSCALL3(cx_bn_export, "(%u, %p, %u)", uint32_t, x, uint8_t *, bytes,
             size_t, nbytes);

    SYSCALL2(cx_bn_set_u32, "(%u %u)", uint32_t, x, uint32_t, n);

    SYSCALL2(cx_bn_get_u32, "(%u %p)", uint32_t, x, uint32_t *, n);

    SYSCALL2(cx_bn_cnt_bits, "(%u %p)", uint32_t, x, uint32_t *, nbits);

    SYSCALL2(cx_bn_is_odd, "(%u, %p)", uint32_t, a, bool *, ptr);

    SYSCALL3(cx_bn_cmp, "(%u, %u, %p)", uint32_t, a, uint32_t, b, int *, ptr);

    SYSCALL3(cx_bn_cmp_u32, "(%u, %u, %p)", uint32_t, a, uint32_t, b, int *,
             ptr);

    SYSCALL3(cx_bn_xor, "(%u, %u, %u)", uint32_t, r, uint32_t, a, uint32_t, b);

    SYSCALL3(cx_bn_or, "(%u, %u, %u)", uint32_t, r, uint32_t, a, uint32_t, b);

    SYSCALL3(cx_bn_and, "(%u, %u, %u)", uint32_t, r, uint32_t, a, uint32_t, b);

    SYSCALL3(cx_bn_add, "(%u, %u, %u)", uint32_t, r, uint32_t, a, uint32_t, b);

    SYSCALL3(cx_bn_sub, "(%u, %u, %u)", uint32_t, r, uint32_t, a, uint32_t, b);

    SYSCALL3(cx_bn_mul, "(%u, %u, %u)", uint32_t, r, uint32_t, a, uint32_t, b);

    SYSCALL4(cx_bn_mod_add, "(%u, %u, %u, %u)", uint32_t, r, uint32_t, a,
             uint32_t, b, uint32_t, n);

    SYSCALL4(cx_bn_mod_sub, "(%u, %u, %u, %u)", uint32_t, r, uint32_t, a,
             uint32_t, b, uint32_t, n);

    SYSCALL4(cx_bn_mod_mul, "(%u, %u, %u, %u)", uint32_t, r, uint32_t, a,
             uint32_t, b, uint32_t, n);

    SYSCALL4(cx_bn_mod_sqrt, "(%u, %u, %u, %u)", uint32_t, r, uint32_t, a,
             uint32_t, b, uint32_t, n);

    SYSCALL3(cx_bn_reduce, "(%u, %u, %u)", uint32_t, r, uint32_t, d, uint32_t,
             n);

    SYSCALL5(cx_bn_mod_pow, "(%u, %u, %p, %u, %u)", uint32_t, r, uint32_t, a,
             uint8_t *, e, size_t, len_e, uint32_t, n);

    SYSCALL5(cx_bn_mod_pow2, "(%u, %u, %p, %u, %u)", uint32_t, r, uint32_t, a,
             uint8_t *, e, size_t, len_e, uint32_t, n);

    SYSCALL4(cx_bn_mod_pow_bn, "(%u, %u, %u, %u)", uint32_t, r, uint32_t, a,
             uint32_t, e, uint32_t, n);

    SYSCALL2(cx_bn_is_prime, "(%u, %p)", uint32_t, a, bool *, ptr);

    SYSCALL1(cx_bn_next_prime, "(%u)", uint32_t, a);

    // SYSCALLs that may exists on other SDK versions, but with a different ID:

    SYSCALL0i(os_perso_isonboarded, os_perso_isonboarded_2_0);

    SYSCALL0i(os_global_pin_is_validated, os_global_pin_is_validated_2_0);

    SYSCALL1(os_lib_call, "(%p)", unsigned long *, call_parameters);

    SYSCALL0(os_lib_end);

    SYSCALL0(os_flags);

    SYSCALL2(os_version, "(%p, %u)", uint8_t *, buffer, size_t, length);
    SYSCALL2(os_serial, "(%p, %u)", unsigned char *, serial, unsigned int,
             maxlength);

    SYSCALL3(os_registry_get_current_app_tag, "(0x%x, %p, %u)", unsigned int,
             tag, uint8_t *, buffer, size_t, length);

    SYSCALL2(os_seph_version, "(%p %u)", uint8_t *, buffer, size_t, length);

    SYSCALL3(os_setting_get, "(0x%x, %p, %u)", unsigned int, setting_id,
             uint8_t *, value, size_t, maxlen);

    SYSCALL1(os_sched_exit, "(%u)", unsigned int, code);

    SYSCALL1(try_context_set, "(%p)", try_context_t *, context);

    SYSCALL1i(os_sched_last_status, "(%u)", unsigned int, task_idx,
              os_sched_last_status_2_0);

    SYSCALL2(nvm_erase, "(%p, %u)", void *, dst_addr, size_t, src_len);

    SYSCALL1(nvm_erase_page, "(%u)", unsigned int, page_addr);

    SYSCALL4(os_perso_derive_eip2333, "(0x%x, %p, %u, %p)", cx_curve_t, curve,
             const unsigned int *, path, unsigned int, pathLength,
             unsigned char *, privateKey);
    SYSCALL3(nvm_write, "(%p, %p, %u)", void *, dst_addr, void *, src_addr,
             size_t, src_len);

    SYSCALL2(os_endorsement_get_public_key, "(%d, %p)", uint8_t, index,
             uint8_t *, buffer);

    SYSCALL3(os_endorsement_key1_sign_data, "(%p, %u, %p)", uint8_t *, data,
             size_t, dataLength, uint8_t *, signature);

    SYSCALL5(os_perso_derive_node_bip32, "(0x%x, %p, %u, %p, %p)", cx_curve_t,
             curve, const uint32_t *, path, size_t, length, uint8_t *,
             private_key, uint8_t *, chain);

    SYSCALL1(os_endorsement_get_code_hash, "(%p)", uint8_t *, buffer);

    SYSCALL2(os_endorsement_get_public_key_certificate, "(%d, %p)",
             unsigned char, index, unsigned char *, buffer);

  default:
    if (model == MODEL_STAX) {
      switch (syscall) {
        SYSCALL1(nbgl_front_draw_rect, "%p", nbgl_area_t *, area);
        SYSCALL1(nbgl_front_refresh_area, "%p", nbgl_area_t *, area);
        SYSCALL3(nbgl_front_draw_horizontal_line, "%p, %d, %d", nbgl_area_t *,
                 area, uint8_t, mask, uint8_t, lineColor);
        SYSCALL4(nbgl_front_draw_img, "%p, %p, %d, %d", nbgl_area_t *, area,
                 uint8_t *, buffer, uint8_t, transformation, nbgl_color_map_t,
                 colorMap);
        SYSCALL4(nbgl_front_draw_img_file, "%p, %p, %d, %p", nbgl_area_t *,
                 area, uint8_t *, buffer, nbgl_color_map_t, colorMap, uint8_t *,
                 uzlib_buffer);

        SYSCALL1(touch_get_last_info, "%p", io_touch_info_t *, info);
        SYSCALL1(nbgl_get_font, "%u", unsigned int, fontId);

      default:
        fprintf(stderr, "syscall 0x%08lx not handled\n", syscall);
        break;
      }
      break;
    } else {
      fprintf(stderr, "syscall 0x%08lx not handled\n", syscall);
      break;
    }
  }
  return 0;
}
