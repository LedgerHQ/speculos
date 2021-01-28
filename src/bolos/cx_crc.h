#ifndef CX_CRC_H
#define CX_CRC_H

#include <stddef.h>

unsigned short sys_cx_crc16_update(unsigned short crc, const void *buf,
                                   size_t len);

#endif
