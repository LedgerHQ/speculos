#include "emulate.h"

#define BOLOS_UX_OK 0xB0105011

/* TODO */
unsigned long sys_os_ux_1_5(bolos_ux_params_t *UNUSED(params))
{
  return BOLOS_UX_OK;
}

unsigned long sys_os_global_pin_is_validated_1_5(void)
{
  return BOLOS_UX_OK;
}

unsigned long sys_os_perso_isonboarded_1_5(void)
{
  return BOLOS_UX_OK;
}

unsigned long sys_os_sched_last_status_1_5(void)
{
  // TODO ?
  return BOLOS_UX_OK; /* XXX */
}
