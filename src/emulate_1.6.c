#include <err.h>
#include <stdio.h>

#include "emulate.h"
#include "bolos_syscalls_1.6.h"

int emulate_1_6(unsigned long syscall, unsigned long *parameters, unsigned long *ret, bool verbose)
{
  int retid;

  switch(syscall) {
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

  SYSCALL0(os_perso_isonboarded);

  SYSCALL0(os_global_pin_is_validated);

  SYSCALL3(os_registry_get_current_app_tag, "(0x%x, %p, %u)",
           unsigned int, tag,
           uint8_t *,    buffer,
           size_t,       length);

  SYSCALL1(os_sched_exit, "(%u)", unsigned int, code);

  SYSCALL1i(os_sched_last_status, "(%u)", unsigned int, task_idx, os_sched_last_status_1_6);

  SYSCALL2(os_version, "(%p, %u)",
           uint8_t *,    buffer,
           size_t,       length);

  SYSCALL2(os_seph_version, "(%p %u)",
           uint8_t *, buffer,
           size_t,    length);

  SYSCALL8(os_perso_derive_node_with_seed_key, "(0x%x, 0x%x, %p, %u, %p, %p, %p, %u)",
           unsigned int,         mode,
           cx_curve_t,           curve,
           const unsigned int *, path,
           unsigned int,         pathLength,
           unsigned char *,      privateKey,
           unsigned char *,      chain,
           unsigned char *,      seed_key,
           unsigned int,         seed_key_length);

  default:
    retid = emulate_common(syscall, parameters, ret, verbose);
    break;
  }
  return retid;
}
