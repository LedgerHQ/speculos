#define _SDK_2_0_
#include <err.h>
#include <stdbool.h>
#include <stdio.h>

#include "bolos/bagl.h"
#include "bolos/cx_aes.h"
#include "bolos/cxlib.h"
#include "bolos/endorsement.h"
#include "bolos/nbgl.h"
#include "bolos/touch.h"
#include "emulate.h"

#include "bolos_syscalls_unified_sdk.h"

#define SYSCALL_HANDLED     1
#define SYSCALL_NOT_HANDLED 0

/* Handle bagl related syscalls which behavior are defined in src/bolos/bagl.c
 */
int emulate_syscall_bagl(unsigned long syscall, unsigned long *parameters,
                         unsigned long *ret, bool verbose,
                         sdk_version_t version, hw_model_t model)
{
  (void)version;

  if (model == MODEL_STAX) {
    return SYSCALL_NOT_HANDLED;
  }

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

    SYSCALL0(screen_clear);

    SYSCALL0(screen_update);

  /* clang-format on */
  default:
    return SYSCALL_NOT_HANDLED;
  }

  return SYSCALL_HANDLED;
}

/* Handle nbgl related syscalls which behavior are defined in src/bolos/nbgl.c
 */
int emulate_syscall_nbgl(unsigned long syscall, unsigned long *parameters,
                         unsigned long *ret, bool verbose,
                         sdk_version_t version, hw_model_t model)
{
  (void)version;

  if (model != MODEL_STAX) {
    return SYSCALL_NOT_HANDLED;
  }

  switch (syscall) {
    /* clang-format off */

    SYSCALL1(nbgl_front_draw_rect, "%p",
             nbgl_area_t *, area);

    SYSCALL1(nbgl_front_refresh_area_legacy, "%p",
             nbgl_area_t *, area);

    SYSCALL2(nbgl_front_refresh_area, "%p, %u",
             nbgl_area_t *, area,
             nbgl_post_refresh_t, post_refresh);

    SYSCALL3(nbgl_front_draw_horizontal_line, "%p, %d, %d",
             nbgl_area_t *, area,
             uint8_t,       mask,
             uint8_t,       lineColor);

    SYSCALL4(nbgl_front_draw_img, "%p, %p, %d, %d",
             nbgl_area_t *,    area,
             uint8_t *,        buffer,
             uint8_t,          transformation,
             nbgl_color_map_t, colorMap);

    SYSCALL4(nbgl_front_draw_img_file, "%p, %p, %d, %p",
             nbgl_area_t *,    area,
             uint8_t *,        buffer,
             nbgl_color_map_t, colorMap,
             uint8_t *,        uzlib_buffer);

    SYSCALL1(nbgl_get_font, "%u",
             unsigned int, fontId);

    SYSCALL0(nbgl_screen_reinit);

    SYSCALL4(nbgl_front_draw_img_rle_legacy, "%p, %p, %u, %u",
             nbgl_area_t *,    area,
             uint8_t *,        buffer,
             unsigned int,     buffer_len,
             color_t,          fore_color);

    SYSCALL4(nbgl_front_draw_img_rle_10, "%p, %p, %u, %u",
             nbgl_area_t *,    area,
             uint8_t *,        buffer,
             unsigned int,     buffer_len,
             color_t,          fore_color);

    SYSCALL5(nbgl_front_draw_img_rle, "%p, %p, %u, %u, %u",
             nbgl_area_t *,    area,
             uint8_t *,        buffer,
             unsigned int,     buffer_len,
             color_t,          fore_color,
             uint8_t,          nb_skipped_bytes);

  /* clang-format on */
  default:
    return SYSCALL_NOT_HANDLED;
  }

  return SYSCALL_HANDLED;
}

/* Handle touch related syscalls which behavior are defined in src/bolos/touch.c
 */
int emulate_syscall_touch(unsigned long syscall, unsigned long *parameters,
                          unsigned long *ret, bool verbose,
                          sdk_version_t version, hw_model_t model)
{
  (void)version;

  if (model != MODEL_STAX) {
    return SYSCALL_NOT_HANDLED;
  }

  switch (syscall) {
    /* clang-format off */

    SYSCALL1(touch_get_last_info, "%p",
             io_touch_info_t *, info);

  /* clang-format on */
  default:
    return SYSCALL_NOT_HANDLED;
  }

  return SYSCALL_HANDLED;
}

/* Handle cx related syscalls which behavior are defined in src/bolos/cx*.c */
int emulate_syscall_cx(unsigned long syscall, unsigned long *parameters,
                       unsigned long *ret, bool verbose, sdk_version_t version,
                       hw_model_t model)
{
  (void)version;
  (void)model;

  switch (syscall) {
    /* clang-format off */
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

    SYSCALL2(cx_ecdomain_size, "(%u %p)",
             unsigned int, cv,
             size_t *,     length);

    SYSCALL2(cx_ecdomain_parameters_length, "(%u %p)",
             unsigned int, cv,
             size_t *,     length);

    SYSCALL4(cx_ecdomain_generator, "(%u %p %p %u)",
             unsigned int, cv,
             void *,       Gx,
             void *,       Gy,
             size_t,       len);

    SYSCALL2(cx_ecdomain_generator_bn, "(%u %p)",
             unsigned int, cv,
             void *,       P);

    SYSCALL3(cx_ecdomain_parameter_bn, "(%u, %d, %u)",
             unsigned int, cv,
             int,          id,
             uint32_t,     p);

    SYSCALL4(cx_ecdomain_parameter, "(0x%x, %u, %p, %u)",
             cx_curve_t,           curve,
             cx_curve_dom_param_t, id,
             uint8_t *,            p,
             uint32_t,             p_len);

    SYSCALL2(cx_ecpoint_alloc, "(%p %u)",
             void *,     p,
             cx_curve_t, cv);

    SYSCALL1(cx_ecpoint_destroy, "(%p)",
             void *, P);

    SYSCALL5(cx_ecpoint_init, "(%p, %p, %u, %p, %u)", void *, p, uint8_t *, x,
             size_t,    x_len,
             uint8_t *, y,
             size_t,    y_len);

    SYSCALL3(cx_ecpoint_init_bn, "(%p, %u, %u)",
             void *,   p,
             uint32_t, x,
             uint32_t, y);

    SYSCALL3(cx_ecpoint_export_bn, "(%p, %p, %p)",
             void *,     p,
             uint32_t *, x,
             uint32_t *, y);

    SYSCALL5(cx_ecpoint_export, "(%p, %p, %u, %p, %u)",
             void *,    p,
             uint8_t *, x,
             size_t,    x_len,
             uint8_t *, y,
             size_t,    y_len);

    SYSCALL4(cx_ecpoint_compress, "(%p, %p, %u, %p)",
             void *,     p,
             uint8_t *,  xy_compressed,
             size_t,     xy_compressed_len,
             uint32_t *, sign);

    SYSCALL4(cx_ecpoint_decompress, "(%p, %p, %u, %u)",
             void *,    p,
             uint8_t *, xy_compressed,
             size_t,    xy_compressed_len,
             uint32_t,  sign);

    SYSCALL3(cx_ecpoint_add, "(%p, %p, %p)",
             void *, eR,
             void *, eP,
             void *, eQ);

    SYSCALL1(cx_ecpoint_neg, "(%p)",
             void *, eP);

    SYSCALL3(cx_ecpoint_cmp, "(%p, %p, %p)",
             void *, eP,
             void *, eQ,
             bool *, is_equal);

    SYSCALL3(cx_ecpoint_scalarmul, "(%p, %p, %u)",
             void *,    p,
             uint8_t *, k,
             size_t,    k_len);

    SYSCALL2(cx_ecpoint_scalarmul_bn, "(%p, %u)",
             void *,   ec_P,
             uint32_t, bn_k);

    SYSCALL3(cx_ecpoint_rnd_scalarmul, "(%p, %p, %u)",
             void *,    p,
             uint8_t *, k,
             size_t,    k_len);

    SYSCALL2(cx_ecpoint_rnd_scalarmul_bn, "(%p, %u)",
             void *,   ec_P,
             uint32_t, bn_k);

    SYSCALL3(cx_ecpoint_rnd_fixed_scalarmul, "(%p, %p, %u)",
             void *,    p,
             uint8_t *, k,
             size_t,    k_len);

    SYSCALL7(cx_ecpoint_double_scalarmul, "(%p, %p, %p, %p, %u, %p, %u)",
             void *,    eR,
             void *,    eP,
             void *,    eQ,
             uint8_t *, k,
             size_t,    k_len,
             uint8_t *, r,
             size_t,    r_len);

    SYSCALL5(cx_ecpoint_double_scalarmul_bn, "(%p, %p, %p, %u, %u)",
             void *,   eR,
             void *,   eP,
             void *,   eQ,
             uint32_t, k,
             uint32_t, r);

    SYSCALL2(cx_ecpoint_is_at_infinity, "(%p, %p)",
             void *, ec_P,
             bool *, is_infinite);

    SYSCALL2(cx_ecpoint_is_on_curve, "(%p, %p)",
             void *, ec_P,
             bool *, is_on_curve);

    SYSCALL3(cx_ecpoint_x25519, "(%u, %p, %u)",
             uint32_t,  bn_u,
             uint8_t *, k,
             size_t,    k_len);

    SYSCALL3(cx_ecpoint_x448, "(%u, %p, %u)",
             uint32_t,  bn_u,
             uint8_t *, k,
             size_t,    k_len);

    SYSCALL0(cx_bn_is_locked);

    SYSCALL2(cx_bn_lock, "(%u %u)",
             size_t,   word_nbytes,
             uint32_t, flags);

    SYSCALL0(cx_bn_unlock);

    SYSCALL2(cx_bn_alloc, "(%p %u)",
             void *, x,
             size_t, nbytes);

    SYSCALL2(cx_bn_copy, "(%u %u)",
             uint32_t, a,
             uint32_t, b);

    SYSCALL4(cx_bn_alloc_init, "(%p, %u, %p, %u)",
             void *,    x,
             size_t,    nbytes,
             uint8_t *, value,
             size_t,    value_nbytes);

    SYSCALL1(cx_bn_destroy, "(%p)",
             void *, x);

    SYSCALL2(cx_bn_nbytes, "(%u, %p)",
             uint32_t, x,
             size_t *, nbytes);

    SYSCALL3(cx_bn_init, "(%u, %p, %u)",
             uint32_t,  x,
             uint8_t *, value,
             size_t,    value_nbytes);

    SYSCALL1(cx_bn_rand, "(%u)",
             uint32_t, x);

    SYSCALL2(cx_bn_rng, "(%u, %u)",
             uint32_t, r,
             uint32_t, n);

    SYSCALL3(cx_bn_tst_bit, "(%u, %u, %p)",
             uint32_t, a,
             uint32_t, b,
             bool *,   set);

    SYSCALL2(cx_bn_set_bit, "(%u, %u)",
             uint32_t, r,
             uint32_t, n);

    SYSCALL2(cx_bn_clr_bit, "(%u, %u)",
             uint32_t, r,
             uint32_t, n);

    SYSCALL2(cx_bn_shr, "(%u, %u)",
             uint32_t, r,
             uint32_t, n);

    SYSCALL2(cx_bn_shl, "(%u, %u)",
             uint32_t, r,
             uint32_t, n);

    SYSCALL3(cx_bn_mod_invert_nprime, "(%u, %u, %u)",
             uint32_t, r,
             uint32_t, a,
             uint32_t, n);

    SYSCALL3(cx_bn_mod_u32_invert, "(%u, %u, %u)",
             uint32_t, r,
             uint32_t, a,
             uint32_t, n);

    SYSCALL3(cx_bn_export, "(%u, %p, %u)",
             uint32_t,  x,
             uint8_t *, bytes,
             size_t,    nbytes);

    SYSCALL2(cx_bn_set_u32, "(%u %u)",
             uint32_t, x,
             uint32_t, n);

    SYSCALL2(cx_bn_get_u32, "(%u %p)",
             uint32_t,   x,
             uint32_t *, n);

    SYSCALL2(cx_bn_cnt_bits, "(%u %p)",
             uint32_t,   x,
             uint32_t *, nbits);

    SYSCALL2(cx_bn_is_odd, "(%u, %p)",
             uint32_t, a,
             bool *,   ptr);

    SYSCALL3(cx_bn_cmp, "(%u, %u, %p)",
             uint32_t, a,
             uint32_t, b,
             int *,    ptr);

    SYSCALL3(cx_bn_cmp_u32, "(%u, %u, %p)",
             uint32_t, a,
             uint32_t, b,
             int *,    ptr);

    SYSCALL3(cx_bn_xor, "(%u, %u, %u)",
             uint32_t, r,
             uint32_t, a,
             uint32_t, b);

    SYSCALL3(cx_bn_or, "(%u, %u, %u)",
             uint32_t, r,
             uint32_t, a,
             uint32_t, b);

    SYSCALL3(cx_bn_and, "(%u, %u, %u)",
             uint32_t, r,
             uint32_t, a,
             uint32_t, b);

    SYSCALL3(cx_bn_add, "(%u, %u, %u)",
             uint32_t, r,
             uint32_t, a,
             uint32_t, b);

    SYSCALL3(cx_bn_sub, "(%u, %u, %u)",
             uint32_t, r,
             uint32_t, a,
             uint32_t, b);

    SYSCALL3(cx_bn_mul, "(%u, %u, %u)",
             uint32_t, r,
             uint32_t, a,
             uint32_t, b);

    SYSCALL4(cx_bn_mod_add, "(%u, %u, %u, %u)",
             uint32_t, r,
             uint32_t, a,
             uint32_t, b,
             uint32_t, n);

    SYSCALL4(cx_bn_mod_sub, "(%u, %u, %u, %u)",
             uint32_t, r,
             uint32_t, a,
             uint32_t, b,
             uint32_t, n);

    SYSCALL4(cx_bn_mod_mul, "(%u, %u, %u, %u)",
             uint32_t, r,
             uint32_t, a,
             uint32_t, b,
             uint32_t, n);

    SYSCALL4(cx_bn_mod_sqrt, "(%u, %u, %u, %u)",
             uint32_t, r,
             uint32_t, a,
             uint32_t, b,
             uint32_t, n);

    SYSCALL3(cx_bn_reduce, "(%u, %u, %u)",
             uint32_t, r,
             uint32_t, d,
             uint32_t, n);

    SYSCALL5(cx_bn_mod_pow, "(%u, %u, %p, %u, %u)",
             uint32_t,  r,
             uint32_t,  a,
             uint8_t *, e,
             size_t,    len_e,
             uint32_t,  n);

    SYSCALL5(cx_bn_mod_pow2, "(%u, %u, %p, %u, %u)",
             uint32_t,  r,
             uint32_t,  a,
             uint8_t *, e,
             size_t,    len_e,
             uint32_t,  n);

    SYSCALL4(cx_bn_mod_pow_bn, "(%u, %u, %u, %u)",
             uint32_t, r,
             uint32_t, a,
             uint32_t, e,
             uint32_t, n);

    SYSCALL2(cx_bn_is_prime, "(%u, %p)",
             uint32_t, a,
             bool *,   ptr);

    SYSCALL1(cx_bn_next_prime, "(%u)",
             uint32_t, a);

    SYSCALL10(cx_bls12381_key_gen, "(%u, %p, %u, %p, %u, %p, %u, %p, %p, %u)",
              uint8_t, mode,
              uint8_t *, secret,
              size_t, secret_len,
              uint8_t *, salt,
              size_t, salt_len,
              uint8_t *, key_info,
              size_t, key_info_len,
              void *, private_key,
              uint8_t *, public_key,
              size_t, public_key_len);

    SYSCALL6(cx_hash_to_field, "(%p, %u, %p, %u, %p, %u)",
             uint8_t *, msg,
             size_t, msg_len,
             uint8_t *, dst,
             size_t, dst_len,
             uint8_t *, hash,
             size_t, hash_len);

    SYSCALL5(ox_bls12381_sign, "(%p, %p, %u, %p, %u)",
             cx_ecfp_384_private_key_t *, key,
             uint8_t *, msg,
             size_t, msg_len,
             uint8_t *, sign,
             size_t, sign_len);

    SYSCALL5(cx_bls12381_aggregate, "(%p, %u, %d, %p, %u)",
             uint8_t *, in,
             size_t, in_len,
             bool, first,
             uint8_t *, agg_sign,
             size_t, sign_len);

  /* clang-format on */
  default:
    return SYSCALL_NOT_HANDLED;
  }

  return SYSCALL_HANDLED;
}

/* Handle os related syscalls which behavior are defined in src/bolos/os*.c */
int emulate_syscall_os(unsigned long syscall, unsigned long *parameters,
                       unsigned long *ret, bool verbose, sdk_version_t version,
                       hw_model_t model)
{
  (void)version;
  (void)model;

  switch (syscall) {
    /* clang-format off */

    /* Syscall from os.c */
    SYSCALL0(os_flags);

    SYSCALL3(os_setting_get, "(0x%x, %p, %u)",
             unsigned int, setting_id,
             uint8_t *,    value,
             size_t,       maxlen);

    SYSCALL3(os_registry_get_current_app_tag, "(0x%x, %p, %u)",
             unsigned int, tag,
             uint8_t *,    buffer,
             size_t,       length);

    SYSCALL1(os_lib_call, "(%p)",
             unsigned long *, call_parameters);

    SYSCALL2(os_version, "(%p, %u)",
             uint8_t *, buffer,
             size_t,    length);

    SYSCALL2(os_seph_version, "(%p %u)",
             uint8_t *, buffer,
             size_t,    length);

    SYSCALL0(os_lib_end);

    SYSCALL1(try_context_set, "(%p)",
             try_context_t *, context);

    SYSCALL0(try_context_get);

    SYSCALL1(os_sched_exit, "(%u)",
             unsigned int, code);

    /* Syscall from os_2.0.c */
    SYSCALL1i(os_ux, "(%p)",
              bolos_ux_params_t *, params,
              os_ux_2_0);

    SYSCALL0i(os_global_pin_is_validated,
              os_global_pin_is_validated_2_0);

    SYSCALL0i(os_perso_isonboarded,
              os_perso_isonboarded_2_0);

    SYSCALL1i(os_sched_last_status, "(%u)",
              unsigned int, task_idx,
              os_sched_last_status_2_0);

    SYSCALL2(os_serial, "(%p, %u)",
             unsigned char *, serial,
             unsigned int, maxlength);

  /* clang-format on */
  default:
    return SYSCALL_NOT_HANDLED;
  }

  return SYSCALL_HANDLED;
}

/* Handle default related syscalls which behavior are defined in
 * src/bolos/default.c */
int emulate_syscall_default(unsigned long syscall, unsigned long *parameters,
                            unsigned long *ret, bool verbose,
                            sdk_version_t version, hw_model_t model)
{
  (void)version;
  (void)model;

  switch (syscall) {
    /* clang-format off */

    SYSCALL3(nvm_write, "(%p, %p, %u)",
             void *, dst_addr,
             void *, src_addr,
             size_t, src_len);

    SYSCALL2(nvm_erase, "(%p, %u)",
             void *, dst_addr,
             size_t, src_len);

    SYSCALL1(nvm_erase_page, "(%u)",
             unsigned int, page_addr);

  /* clang-format on */
  default:
    return SYSCALL_NOT_HANDLED;
  }

  return SYSCALL_HANDLED;
}

/* Handle seph related syscalls which behavior are defined in
 * src/bolos/seproxyhal.c */
int emulate_syscall_seph(unsigned long syscall, unsigned long *parameters,
                         unsigned long *ret, bool verbose,
                         sdk_version_t version, hw_model_t model)
{
  (void)version;
  (void)model;

  switch (syscall) {
    /* clang-format off */

    SYSCALL0(io_seph_is_status_sent);

    SYSCALL2(io_seph_send, "(%p, %u)",
             uint8_t *, buffer,
             uint16_t,  length);

    SYSCALL3(io_seph_recv, "(%p, %u, 0x%x)",
             uint8_t *,    buffer,
             uint16_t,     maxlength,
             unsigned int, flags);

  /* clang-format on */
  default:
    return SYSCALL_NOT_HANDLED;
  }

  return SYSCALL_HANDLED;
}

/* Handle os_perso related syscalls which behavior are defined in
 * src/bolos/os_{bip32, eip2333}.c a */
int emulate_syscall_os_perso(unsigned long syscall, unsigned long *parameters,
                             unsigned long *ret, bool verbose,
                             sdk_version_t version, hw_model_t model)
{
  (void)version;
  (void)model;

  switch (syscall) {
    /* clang-format off */

    /* Syscall from os_bip32.c */
    SYSCALL8(os_perso_derive_node_with_seed_key, "(0x%x, 0x%x, %p, %u, %p, %p, %p, %u)",
             unsigned int,         mode,
             cx_curve_t,           curve,
             const unsigned int *, path,
             unsigned int,         pathLength,
             unsigned char *,      privateKey,
             unsigned char *,      chain,
             unsigned char *,      seed_key,
             unsigned int,         seed_key_length);

    SYSCALL5(os_perso_derive_node_bip32, "(0x%x, %p, %u, %p, %p)",
             cx_curve_t,       curve,
             const uint32_t *, path,
             size_t,           length,
             uint8_t *,        private_key,
             uint8_t *,        chain);

    /* Syscall from os_eip2333.c */
    SYSCALL4(os_perso_derive_eip2333, "(0x%x, %p, %u, %p)",
             cx_curve_t,           curve,
             const unsigned int *, path,
             unsigned int,         pathLength,
             unsigned char *,      privateKey);

  /* clang-format on */
  default:
    return SYSCALL_NOT_HANDLED;
  }

  return SYSCALL_HANDLED;
}

/* Handle endorsement related syscalls which behavior are defined in
 * src/bolos/endorsement.c a */
int emulate_syscall_endorsement(unsigned long syscall,
                                unsigned long *parameters, unsigned long *ret,
                                bool verbose, sdk_version_t version,
                                hw_model_t model)
{
  (void)version;
  (void)model;

  switch (syscall) {
    /* clang-format off */

  SYSCALL1(os_endorsement_get_code_hash, "(%p)",
           uint8_t *, buffer);

  SYSCALL3(os_endorsement_key1_sign_data, "(%p, %u, %p)",
           uint8_t *, data,
           size_t,    dataLength,
           uint8_t *, signature);

  SYSCALL3i(os_endorsement_get_public_key, "(%d, %p, %p)",
           uint8_t,   index,
           uint8_t *, buffer,
           uint8_t *, length,
           os_endorsement_get_public_key_new);

  SYSCALL3i(os_endorsement_get_public_key_certificate, "(%d, %p, %p)",
           unsigned char,   index,
           unsigned char *, buffer,
           unsigned char *, length,
           os_endorsement_get_public_key_certificate_new);

  /* clang-format on */
  default:
    return SYSCALL_NOT_HANDLED;
  }

  return SYSCALL_HANDLED;
}

int emulate_unified_sdk(unsigned long syscall, unsigned long *parameters,
                        unsigned long *ret, bool verbose, sdk_version_t version,
                        hw_model_t model)
{
  if (emulate_syscall_bagl(syscall, parameters, ret, verbose, version, model) ==
      SYSCALL_HANDLED) {
    return 0;
  }

  if (emulate_syscall_nbgl(syscall, parameters, ret, verbose, version, model) ==
      SYSCALL_HANDLED) {
    return 0;
  }

  if (emulate_syscall_touch(syscall, parameters, ret, verbose, version,
                            model) == SYSCALL_HANDLED) {
    return 0;
  }

  if (emulate_syscall_cx(syscall, parameters, ret, verbose, version, model) ==
      SYSCALL_HANDLED) {
    return 0;
  }

  if (emulate_syscall_os(syscall, parameters, ret, verbose, version, model) ==
      SYSCALL_HANDLED) {
    return 0;
  }

  if (emulate_syscall_default(syscall, parameters, ret, verbose, version,
                              model) == SYSCALL_HANDLED) {
    return 0;
  }

  if (emulate_syscall_seph(syscall, parameters, ret, verbose, version, model) ==
      SYSCALL_HANDLED) {
    return 0;
  }

  if (emulate_syscall_os_perso(syscall, parameters, ret, verbose, version,
                               model) == SYSCALL_HANDLED) {
    return 0;
  }

  if (emulate_syscall_endorsement(syscall, parameters, ret, verbose, version,
                                  model) == SYSCALL_HANDLED) {
    return 0;
  }

  fprintf(stderr, "syscall 0x%08lx not handled\n", syscall);
  return 0;
}
