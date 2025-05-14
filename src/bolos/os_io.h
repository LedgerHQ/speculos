#ifndef OS_IO_H_
#define OS_IO_H_

#include <stdbool.h>

typedef struct {
  int init;
} os_io_init_t;

int sys_os_io_init(os_io_init_t *init);

int sys_os_io_start(void);

int sys_os_io_stop(void);

int sys_os_io_tx_cmd(unsigned char type, const unsigned char *buffer,
                     unsigned short length, unsigned int *timeout_ms);

int sys_os_io_rx_evt(unsigned char *buffer, unsigned short buffer_max_length,
                     unsigned int *timeout_ms);

int sys_os_io_seph_tx(const unsigned char *buffer, unsigned short length,
                      unsigned int *timeout_ms);

int sys_os_io_seph_se_rx_event(unsigned char *buffer,
                               unsigned short max_length,
                               unsigned int  *timeout_ms,
                               bool           check_se_event,
                               unsigned int   flags);
#endif
