#include <err.h>
#include <stdio.h>

#include "bolos/bagl.h"
#include "bolos/cx_aes.h"
#include "bolos_syscalls_1.2.h"
#include "emulate.h"

unsigned long sys_os_perso_derive_node_bip32_seed_key(
    unsigned int mode, cx_curve_t curve, const unsigned int *path,
    unsigned int pathLength, unsigned char *privateKey, unsigned char *chain,
    unsigned char *seed_key, unsigned int seed_key_length);

int emulate_1_2(unsigned long syscall, unsigned long *parameters,
                unsigned long *ret, bool verbose)
{
  int retid;

  /* clang-format off */
  switch(syscall) {
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

  SYSCALL3(cx_crc16_update, "(%u, %p, %u)",
           unsigned short, crc, const void *, b, size_t, len);

  SYSCALL0(cx_rng_u32);

  SYSCALL1(os_lib_call, "(%p)", unsigned long *, call_parameters);

  case SYSCALL_os_lib_end_ID_IN: {
    *ret = sys_os_lib_end();
    print_syscall("os_lib_end(%s)", "");
    /* replace retid with os_lib_call's because os_lib_end's returns in the end
     * of the os_lib_call stub */
    retid = SYSCALL_os_lib_call_ID_OUT;
    print_ret(*ret);
    break;
  }

  SYSCALL3(io_seph_recv, "(%p, %u, 0x%x)",
           uint8_t *,    buffer,
           uint16_t,     maxlength,
           unsigned int, flags);

  SYSCALL0(io_seph_is_status_sent);

  SYSCALL2(io_seph_send, "(%p, %u)",
           uint8_t *, buffer,
           uint16_t,  length);

  SYSCALL0(os_flags);

  SYSCALL0i(os_perso_isonboarded, os_perso_isonboarded_1_2);

  SYSCALL0i(os_global_pin_is_validated, os_global_pin_is_validated_1_2);

  SYSCALL3(os_registry_get_current_app_tag, "(0x%x, %p, %u)",
           unsigned int, tag,
           uint8_t *,    buffer,
           size_t,       length);

  SYSCALL1(os_sched_exit, "(%u)", unsigned int, code);

  SYSCALL1i(os_sched_last_status, "(%u)", unsigned int, task_idx, os_sched_last_status_1_2);

  SYSCALL2(os_version, "(%p, %u)",
           uint8_t *,    buffer,
           size_t,       length);

  SYSCALL2(os_seph_version, "(%p %u)",
           uint8_t *, buffer,
           size_t,    length);

  SYSCALL8(os_perso_derive_node_bip32_seed_key, "(0x%x, 0x%x, %p, %u, %p, %p, %p, %u)",
           unsigned int,         mode,
           cx_curve_t,           curve,
           const unsigned int *, path,
           unsigned int,         pathLength,
           unsigned char *,      privateKey,
           unsigned char *,      chain,
           unsigned char *,      seed_key,
           unsigned int,         seed_key_length);

  SYSCALL3(os_setting_get, "(0x%x, %p, %u)",
           unsigned int, setting_id,
           uint8_t *,    value,
           size_t,       maxlen);

  SYSCALL1i(os_ux, "(%p)", bolos_ux_params_t *, params, os_ux_1_2);

  SYSCALL0(screen_update);

  SYSCALL8(cx_aes_iv, "(%p, 0x%x, %p, %u, %p, %u, %p, %u)",
           const cx_aes_key_t *, key,
           unsigned int,         mode,
           const uint8_t *,      iv,
           unsigned int,         iv_len,
           const uint8_t *,      in,
           unsigned int,         len,
           uint8_t *,            out,
           unsigned int,         out_len);

  SYSCALL4(os_perso_derive_eip2333, "(0x%x, %p, %u, %p)",
            cx_curve_t,          curve,
            const unsigned int *,path,
            unsigned int,        pathLength,
            unsigned char *,     privateKey);
    /* clang-format on */

  default:
    retid = emulate_common(syscall, parameters, ret, verbose);
    break;
  }
  return retid;
}
