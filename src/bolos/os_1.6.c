#include "emulate.h"

#define BOLOS_UX_OK 0xAA

/* TODO */
unsigned long sys_os_ux_1_6(bolos_ux_params_t *UNUSED(params))
{
  return BOLOS_UX_OK;
}

unsigned long sys_os_global_pin_is_validated_1_6(void)
{
  return BOLOS_UX_OK;
}

unsigned long sys_os_perso_isonboarded_1_6(void)
{
  return BOLOS_UX_OK;
}

unsigned long sys_os_sched_last_status_1_6(unsigned int UNUSED(task_idx))
{
  // TODO ?
  return BOLOS_UX_OK; // >1.6 : status is just a char
}
