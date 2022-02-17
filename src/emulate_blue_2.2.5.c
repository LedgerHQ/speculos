#include <err.h>
#include <stdio.h>

#include "bolos/cx.h"
#include "bolos/cx_aes.h"
#include "bolos_syscalls_blue_2.2.5.h"
#include "emulate.h"

int emulate_blue_2_2_5(unsigned long syscall, unsigned long *parameters,
                       unsigned long *ret, bool verbose)
{
  int retid;

  switch (syscall) {
    /* clang-format off */
  SYSCALL3(cx_crc16_update, "(%u, %p, %u)",
           unsigned short, crc, const void *, b, size_t, len);

  SYSCALL3(io_seph_recv, "(%p, %u, 0x%x)",
           uint8_t *,    buffer,
           uint16_t,     maxlength,
           unsigned int, flags);

  SYSCALL0(io_seph_is_status_sent);

  SYSCALL2(io_seph_send, "(%p, %u)",
           uint8_t *, buffer,
           uint16_t,  length);

  SYSCALL0(os_flags);

  SYSCALL0i(os_perso_isonboarded, os_perso_isonboarded_1_5);

  SYSCALL1i(os_sched_last_status, "(%u)", unsigned int, task_idx, os_sched_last_status_1_6);

  SYSCALL3(os_registry_get_current_app_tag, "(0x%x, %p, %u)",
           unsigned int, tag,
           uint8_t *,    buffer,
           size_t,       length);

  SYSCALL2(os_version, "(%p, %u)",
           uint8_t *,    buffer,
           size_t,       length);

  SYSCALL2(os_seph_version, "(%p %u)",
           uint8_t *, buffer,
           size_t,    length);

  SYSCALL1(os_sched_exit, "(%u)", unsigned int, code);

  SYSCALL0i(os_global_pin_is_validated, os_global_pin_is_validated_1_5);

  SYSCALL1i(os_ux, "(%p)", bolos_ux_params_t *, params, os_ux_1_6);

  SYSCALL8(cx_aes_iv, "(%p, 0x%x, %p, %u, %p, %u, %p, %u)",
           const cx_aes_key_t *, key,
           unsigned int,         mode,
           const uint8_t *,      iv,
           unsigned int,         iv_len,
           const uint8_t *,      in,
           unsigned int,         len,
           uint8_t *,            out,
           unsigned int,         out_len);
    /* clang-format on */

  default:
    retid = emulate_common(syscall, parameters, ret, verbose);
    break;
  }

  return retid;
}
