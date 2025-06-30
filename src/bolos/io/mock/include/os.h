#pragma once

#include <stdlib.h>
#include <string.h>

#include "bolos/io/io.h"
#include "os_helpers.h"
#include "os_math.h"
#include "os_pic.h"
#include "os_types.h"

#include "errors.h"

enum task_unsecure_id_e {
  TASK_BOLOS = 0, // can call os
  TASK_SYSCALL,   // can call os
  TASK_USERTASKS_START,
  // disabled for now // TASK_USER_UX, // must call syscalls to reach os, locked
  // in ux ram
  TASK_USER =
      TASK_USERTASKS_START, // must call syscalls to reach os, locked in app ram
  TASK_SUBTASKS_START,
  TASK_SUBTASK_0 = TASK_SUBTASKS_START,
#ifdef TARGET_NANOX
  TASK_SUBTASK_1,
  TASK_SUBTASK_2,
  TASK_SUBTASK_3,
#endif // TARGET_NANOX
  TASK_BOLOS_UX,
  TASK_MAXCOUNT, // must be last in the structure
};

#define os_io_seph_se_rx_event sys_os_io_seph_se_rx_event
#define os_io_seph_tx          sys_os_io_seph_tx

#define os_sched_last_status sys_os_sched_last_status_2_0
