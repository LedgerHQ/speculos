#include <err.h>
#include <stdio.h>

#include "emulate.h"

#include "bolos/cx_aes.h"
#include "bolos_syscalls_1.5.h"

int emulate_1_5(unsigned long syscall, unsigned long *parameters,
                unsigned long *ret, bool verbose)
{
  int retid;

  switch (syscall) {
    /* clang-format off */
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

  SYSCALL0(io_seproxyhal_spi_is_status_sent);

  SYSCALL3(io_seproxyhal_spi_recv, "(%p, %u, 0x%x)",
           uint8_t *,    buffer,
           uint16_t,     maxlength,
           unsigned int, flags);

  SYSCALL2(io_seproxyhal_spi_send, "(%p, %u)",
           uint8_t *, buffer,
           uint16_t,  length);

  SYSCALL0i(os_perso_isonboarded, os_perso_isonboarded_1_5);

  SYSCALL0i(os_sched_last_status, os_sched_last_status_1_5);

  SYSCALL0i(os_global_pin_is_validated, os_global_pin_is_validated_1_5);

  SYSCALL1(os_sched_exit, "(%u)", unsigned int, code);

  SYSCALL2(os_version, "(%p, %u)",
           uint8_t *,    buffer,
           size_t,       length);

  SYSCALL2(os_seph_version, "(%p %u)",
           uint8_t *, buffer,
           size_t,    length);

  SYSCALL1i(os_ux, "(%p)", bolos_ux_params_t *, params, os_ux_1_5);

  SYSCALL8(cx_aes_iv, "(%p, 0x%x, %p, %u, %p, %u, %p, %u)",
           const cx_aes_key_t *, key,
           unsigned int,         mode,
           const uint8_t *,      iv,
           unsigned int,         iv_len,
           const uint8_t *,      in,
           unsigned int,         len,
           uint8_t *,            out,
           unsigned int,         out_len);

  SYSCALL0(reset);
    /* clang-format on */

  default:
    retid = emulate_common(syscall, parameters, ret, verbose);
    break;
  }
  return retid;
}
