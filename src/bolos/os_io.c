#include <assert.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#include "bolos/os_io.h"
#include "emulate.h"

int sys_os_io_init(os_io_init_t *init)
{
  init->init = 1;
  return 0;
}

int sys_os_io_start(void)
{
  return 0;
}

int sys_os_io_stop(void)
{
  return 0;
}

int sys_os_io_rx_evt(unsigned char *buffer, unsigned short buffer_max_length, unsigned int *timeout_ms)
{
  assert(false);
  (void)buffer;
  (void)buffer_max_length;
  (void)timeout_ms;
  return 0;
}

int sys_os_io_tx_cmd(unsigned char        type,
                 const unsigned char *buffer,
                 unsigned short       length,
                 unsigned int        *timeout_ms)
{
  assert(false);
  (void)type;
  (void)buffer;
  (void)length;
  (void)timeout_ms;
  return 0;
}

int sys_os_io_seph_tx(const unsigned char *buffer, unsigned short length,
                      unsigned int *timeout_ms)
{
  (void)buffer;
  (void)length;
  (void)timeout_ms;
  return 0;
}

int sys_os_io_seph_se_rx_event(unsigned char *buffer,
                               unsigned short max_length,
                               unsigned int  *timeout_ms,
                               bool           check_se_event,
                               unsigned int   flags)
{
  (void)buffer;
  (void)max_length;
  (void)timeout_ms;
  (void)check_se_event;
  (void)flags;
  return 0;
}
