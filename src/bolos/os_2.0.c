#include "emulate.h"

#define BOLOS_UX_OK 0xAA

/* TODO */
unsigned long sys_os_ux_2_0(bolos_ux_params_t *UNUSED(params))
{
  return BOLOS_UX_OK;
}

unsigned long sys_os_global_pin_is_validated_2_0(void)
{
  return BOLOS_UX_OK;
}

unsigned long sys_os_perso_isonboarded_2_0(void)
{
  return BOLOS_UX_OK;
}

unsigned long sys_os_sched_last_status_2_0(unsigned int task_idx
                                           __attribute__((unused)))
{
  return BOLOS_UX_OK; // >2.0 : status is just a char
}

//-----------------------------------------------------------------------------
// The functions below are empty, to avoid a crash if an app use them.
//-----------------------------------------------------------------------------

unsigned int sys_os_perso_seed_cookie(unsigned char *seed_cookie
                                      __attribute__((unused)),
                                      unsigned int seed_cookie_length
                                      __attribute__((unused)))
{
  return 0;
}

unsigned int sys_os_serial(unsigned char *serial, unsigned int maxlength)
{
  char *sn = "1.2.3.4";
  size_t size = strlen(sn);
  if (maxlength < size) {
    size = maxlength;
  }
  memcpy(serial, sn, size);
  return size;
}
