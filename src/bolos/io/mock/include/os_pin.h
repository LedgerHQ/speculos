#pragma once

#include "os_types.h"

static inline bolos_bool_t os_global_pin_is_validated(void)
{
  return BOLOS_TRUE;
}
static inline bolos_bool_t os_perso_is_pin_set(void)
{
  return BOLOS_TRUE;
}