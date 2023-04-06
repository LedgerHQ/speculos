#define _SDK_2_0_
#include <stdio.h>
#include <stdlib.h>

#include "bolos/cxlib.h"
#include "emulate.h"

//-----------------------------------------------------------------------------

unsigned int sys_get_api_level(void)
{
  return CX_APILEVEL;
}

cx_err_t sys_cx_get_random_bytes(void *buffer, size_t len)
{
  sys_cx_rng(buffer, len);

  return CX_OK;
}

cx_err_t sys_cx_trng_get_random_data(void *buffer, size_t len)
{
  sys_cx_rng(buffer, len);

  return CX_OK;
}

uint32_t sys_cx_crc32_hw(const void *buf, size_t len)
{
  uint32_t crc;
  crc = sys_cx_crc32_update(0xFFFFFFFF, buf, len);

  return crc;
}
