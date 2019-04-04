#include "emulate.h"

unsigned long sys_os_sched_last_status_1_6(unsigned int UNUSED(task_idx))
{
  // TODO ?
  return 0xAA; // >1.6 : status is just a char
}
