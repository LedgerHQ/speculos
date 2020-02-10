#include <err.h>
#include <stdio.h>

#include "emulate.h"
#include "bolos_syscalls_blue_2.2.5.h"

int emulate_blue_2_2_5(unsigned long syscall, unsigned long *parameters, unsigned long *ret, bool verbose)
{
  int retid;

  switch(syscall) {
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

  SYSCALL0(os_global_pin_is_validated);

  default:
    retid = emulate_common(syscall, parameters, ret, verbose);
    break;
  }

  return retid;
}
